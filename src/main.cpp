// STL includes
#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

// Libraries
#include "raylib-cpp.hpp"

// Local includes
#include "audiotrip/dtos.h"
#include "raylib_ext/scoped.h"
#include "raylib_ext/text3d.h"
#include "splines/spline3d.h"

// raylib config
#include "config.h"

namespace rlgl {
#include "rlgl.h"
}

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// The idea was to use the high-definition models on desktop but in practice the difference is barely noticeable, so
// I'll use them everywhere.
#define MODELS_SUFFIX "_lowlod"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
//#define MODELS_SUFFIX ""
#else // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

#define INITIAL_DISTANCE (-2.5f)
#define PLAYER_HEIGHT (1.381876)
#define MAX_RENDER_DISTANCE (300.0f)

/*
 * Note: Y is UP! The song extends parallel to Z, arms point parallel to X
 */

// Barrier notes:
// - 20cm above the floor
// - 200cm wide
// - XY=0, angle 0 places the barrier facing down, bottom side at 180cm
// - XY=0, angle 90° places the barrier on the side, center tick is at 125cm
// - => reference point is in the middle, 55cm below the bottom side
// - Y position is subtracted, not added

/**
 * Nasty trick to load a brand new color material from materials.mtl since raylib is partially broken.
 * Load a fake model and then steal the material from it.
 * @return
 */
static void applyMaterialsMtl(raylib::Model &model) {
  // Unload old model materials
  for (int i = 0; i < model.materialCount; i++) {
    auto &material = reinterpret_cast<raylib::Material &>(model.materials[i]);
    material.Unload();
  }
  RL_FREE(model.materials);

  // Load fake model
  raylib::Model tempModel("resources/models/materials_fake_model.obj");

  // Transfer materials
  model.materialCount = tempModel.materialCount;
  model.materials = tempModel.materials;

  // Prevent materials from being unloaded
  tempModel.materialCount = 0;
  tempModel.materials = nullptr;

  // Destructor will unload the temp model
}

/**
 * Draws the choreography floor around the camera. The floor is actually centered around the camera and it follows it.
 * It is made to appear static and infinite with texture trickery.
 * @param texture
 * @param camera
 */
static void DrawChoreoFloor(raylib::Texture2D &texture, const raylib::Camera &camera) {
  rlgl::rlCheckRenderBatchLimit(4);

  // NOTE: Plane is always created on XZ ground
  raylib_ext::scoped::Matrix matrix;

  rlgl::rlSetTexture(texture.id);

  constexpr float length = MAX_RENDER_DISTANCE * 2;
  constexpr int divider = 3;
  constexpr float textureLength = length / divider;

  static_assert(length - static_cast<int>(length) == 0.0f);
  static_assert(textureLength - static_cast<int>(textureLength) == 0.0f);

  // Block the floor to a position that matches the texture start
  int pos = static_cast<int>(camera.position.z);
  pos -= pos % divider;

  rlgl::rlTranslatef(0, 0, static_cast<float>(pos));
  rlgl::rlScalef(PLAYER_HEIGHT, 1.0f, length);

  // clang-format off
  rlgl::rlBegin(RL_QUADS);

    rlgl::rlNormal3f(0.0f, 1.0f, 0.0f);

    rlgl::rlTexCoord2f(0, textureLength);
    rlgl::rlVertex3f(-0.5f, 0.0f, -0.5f);

    rlgl::rlTexCoord2f(0, 0);
    rlgl::rlVertex3f(-0.5f, 0.0f, 0.5f);

    rlgl::rlTexCoord2f(1,  0);
    rlgl::rlVertex3f(0.5f, 0.0f, 0.5f);

    rlgl::rlTexCoord2f(1, textureLength);
    rlgl::rlVertex3f(0.5f, 0.0f, -0.5f);

  rlgl::rlEnd();
  // clang-format on

  rlgl::rlSetTexture(0);
}

class SkyBox {
public:
  raylib::Shader shader{ TextFormat("resources/shaders/glsl%i/skybox.vs", GLSL_VERSION),
                         TextFormat("resources/shaders/glsl%i/skybox.fs", GLSL_VERSION) };
  raylib::Model skybox{ raylib::Mesh::Cube(100, 100, 100) };

  std::unique_ptr<raylib::TextureCubemap> texture;

  SkyBox(const std::string &imagePath) {
    skybox.materials[0].shader = shader;

    int environmentMapVal[] = { MATERIAL_MAP_CUBEMAP };
    int doGammaVal[] = { 0 };
    int vflippedVal[] = { 0 };
    shader.SetValue(shader.GetLocation("environmentMap"), environmentMapVal, SHADER_UNIFORM_INT);
    shader.SetValue(shader.GetLocation("doGamma"), doGammaVal, SHADER_UNIFORM_INT);
    shader.SetValue(shader.GetLocation("vflipped"), vflippedVal, SHADER_UNIFORM_INT);

    LoadTexture(imagePath);
  }

  void LoadTexture(const std::string &imagePath) {
    raylib::Image image(imagePath);
    texture = std::make_unique<raylib::TextureCubemap>(image, CUBEMAP_LAYOUT_AUTO_DETECT);
    skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = *texture;
  }

  void Draw() {
    // We are inside the cube, we need to disable backface culling!
    rlgl::rlDisableBackfaceCulling();
    rlgl::rlDisableDepthMask();

    skybox.Draw((Vector3){ 0, 0, 0 }, 1.0f, WHITE);

    rlgl::rlEnableBackfaceCulling();
    rlgl::rlEnableDepthMask();
  }
};

struct hash_tuple {
  size_t operator()(const std::tuple<int, int, int, int> &x) const {
    return std::get<0>(x) ^ std::get<1>(x) ^ std::get<2>(x) ^ std::get<3>(x);
  }
};

class Application {
private:
  using RibbonKey = std::tuple<int, int, int, int>; // beat, numerator, denominator, RHS (bool as int)

  std::unique_ptr<raylib::Window> window;
  std::unique_ptr<raylib::Camera> camera;
  std::unique_ptr<raylib::Shader> shader;

  std::unique_ptr<raylib::Texture2D> floorTexture;

  std::unique_ptr<raylib::Model> barrierModel;

  std::unique_ptr<raylib::Model> gemModel;
  std::unique_ptr<raylib::Model> gemTrailModel;
  std::unique_ptr<raylib::Model> drumModel;
  std::unique_ptr<raylib::Model> dirgemModel;

  std::unique_ptr<SkyBox> skybox;

  Vector3 beatNumbersSize = { -1, -1, -1 };

  std::unique_ptr<audiotrip::AudioTripSong> ats;
  audiotrip::Choreography *choreo = nullptr;
  std::vector<audiotrip::Beat> beats;
  std::unordered_map<RibbonKey, std::pair<raylib::Model, raylib::Vector3>, hash_tuple> ribbons;

  bool mouseCaptured = true;

public:
  Application() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    window = std::make_unique<raylib::Window>(800, 600, "Audio Trip Choreography Viewer");
    (void) window; // Silence unused variable

    // NOLINTNEXTLINE(modernize-make-unique)
    camera.reset(new raylib::Camera({ 0.0f, PLAYER_HEIGHT, INITIAL_DISTANCE },
                                    { 0.0f, 0, -20 },
                                    { 0.0f, 1.0f, 0.0f },
                                    60.0f,
                                    CAMERA_PERSPECTIVE));
    camera->SetMode(CAMERA_FIRST_PERSON);
    mouseCapture(false);

    floorTexture = std::make_unique<raylib::Texture2D>(raylib::Image("resources/floor_texture.png").Mipmaps());
    rlgl::rlTextureParameters(floorTexture->id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_ANISOTROPIC);
    rlgl::rlTextureParameters(floorTexture->id, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlgl::rlTextureParameters(floorTexture->id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_REPEAT);

    barrierModel = std::make_unique<raylib::Model>("resources/models/barrier.obj");
    gemTrailModel = std::make_unique<raylib::Model>("resources/models/gem_trail.obj");
    gemModel = std::make_unique<raylib::Model>("resources/models/gem" MODELS_SUFFIX ".obj");
    drumModel = std::make_unique<raylib::Model>("resources/models/drum" MODELS_SUFFIX ".obj");
    dirgemModel = std::make_unique<raylib::Model>("resources/models/dirgem" MODELS_SUFFIX ".obj");

    shader = std::make_unique<raylib::Shader>(TextFormat("resources/shaders/glsl%i/base_lighting.vs", GLSL_VERSION),
                                              TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));

    shader->locs[SHADER_LOC_VECTOR_VIEW] = shader->GetLocation("viewPos");
    shader->locs[SHADER_LOC_MATRIX_MODEL] = shader->GetLocation("matModel");
    shader->locs[SHADER_LOC_COLOR_AMBIENT] = shader->GetLocation("ambient");
    shader->locs[SHADER_LOC_COLOR_DIFFUSE] = shader->GetLocation("colDiffuse");

    float ambientVal[]{ 1, 1, 1, 1 };
    SetShaderValue(*shader, shader->locs[SHADER_LOC_COLOR_AMBIENT], ambientVal, SHADER_UNIFORM_VEC4);

    //  float fogDensity = 0.15f;
    //  int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    //  SetShaderValue(shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    barrierModel->materials[0].shader = *shader;
    gemModel->materials[0].shader = *shader;
    gemTrailModel->materials[0].shader = *shader;
    drumModel->materials[0].shader = *shader;
    dirgemModel->materials[0].shader = *shader;

    skybox = std::make_unique<SkyBox>("resources/at-cubemap.png");
  }

  ~Application() { ClearDroppedFiles(); }

  void main(std::optional<std::string> atsFile) {
    beatNumbersSize = raylib_ext::text3d::MeasureText3D(GetFontDefault(), "1", 8.0f, 1.0f, 0.0f);

    if (atsFile.has_value()) {
      openAts(*atsFile);
    } else {
      mouseCapture(false);
    }

#ifdef PLATFORM_WEB
    emscripten_set_main_loop_arg(emscriptenMainloop, this, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
      drawFrame();
    }
#endif
  }

private:
  void mouseCapture(std::optional<bool> val) {
    mouseCaptured = val.has_value() ? *val : !mouseCaptured;
    if (mouseCaptured)
      DisableCursor();
    else
      EnableCursor();
  }

  void openAts(const std::string &path) {
    ats = std::make_unique<audiotrip::AudioTripSong>(std::move(audiotrip::AudioTripSong::fromFile(path)));
    choreo = &ats->choreographies.front(); // TODO: let the user pick
    beats = ats->computeBeats();
    camera->position.z = INITIAL_DISTANCE; // Go back to the start
    mouseCapture(true);
    std::cout << "Opened ATS file: " << path << std::endl;

    ribbons.clear();
  }

  static void emscriptenMainloop(void *obj) {
    static_cast<Application *>(obj)->drawFrame();
  }

  void drawFrame() {
    if (IsFileDropped()) {
      std::vector<std::string> files = raylib::GetDroppedFiles();
      for (const std::string &path : files) {
        if (path.ends_with(".ats")) {
          openAts(path);
          break;
        }
      }
    }

    camera->Update();

    if (IsKeyPressed(KEY_M)) {
      mouseCapture(std::nullopt); // Toggle capture
    }

    if (choreo != nullptr) {
      bool plusPressed = IsKeyPressed(KEY_PAGE_UP);
      bool minusPressed = IsKeyPressed(KEY_PAGE_DOWN);
      if (plusPressed || minusPressed) {
        camera->position.z += choreo->secondsToMeters(beats.at(1).time) * (minusPressed ? -1.0f : 1.0f);
      }
    }

    float cameraPosValue[] = { camera->position.x, camera->position.y, camera->position.z };
    SetShaderValue(*shader, shader->locs[SHADER_LOC_VECTOR_VIEW], cameraPosValue, SHADER_UNIFORM_VEC3);

    Vector3 pos = camera->position;
    pos.z += 0.1;

    {
      raylib_ext::scoped::Drawing drawing;

      if (ats != nullptr)
        drawChoreo();
      else
        drawSplash();
    }
  }

  void drawSplash() {
    ClearBackground(WHITE);

    const char *text = "Drag and drop an ATS file on this window";

    Vector2 textSize = MeasureTextEx(GetFontDefault(), text, 20, 1);
    int textWidth = static_cast<int>(textSize.x);
    int textHeight = static_cast<int>(textSize.y);

    int posX = window->GetWidth() / 2 - textWidth / 2;
    int posY = window->GetHeight() / 2 - textHeight / 2;

    DrawText(text, posX, posY, 20, BLACK);
  }

  float getBeatTime(float beatNum) {
    auto intBeat = static_cast<size_t>(beatNum);
    audiotrip::Beat beat = beats[intBeat];
    float beatTime = beat.time;
    float fraction = beatNum - static_cast<float>(intBeat);

    if (fraction != 1 && fraction != 0) {
      float secondsPerBeat = 60.0f / beat.bpm;
      beatTime += secondsPerBeat * fraction;
    }

    return beatTime;
  }

  void drawChoreo() {
    ClearBackground(GRAY);

    {
      raylib_ext::scoped::Mode3D mode3d(*camera);

      skybox->Draw();

      DrawChoreoFloor(*floorTexture, *camera);

      float minDistance = camera->position.z - MAX_RENDER_DISTANCE;
      float maxDistance = camera->position.z + MAX_RENDER_DISTANCE;

      // Draw beat numbers
      for (int beatNum = 1; beatNum <= beats.size(); beatNum++) {
        float beatTime = beats[beatNum - 1].time;
        float beatDistance = choreo->secondsToMeters(beatTime);

        if (beatDistance > maxDistance || beatDistance < minDistance)
          continue;

        {
          raylib_ext::scoped::Matrix translateM;
          rlgl::rlTranslatef(-PLAYER_HEIGHT / 2 - 0.1f, 0, beatDistance + beatNumbersSize.z / 2.0f);
          {
            raylib_ext::scoped::Matrix rotateM;
            rlgl::rlRotatef(180, 0, 1, 0);

            raylib_ext::text3d::DrawText3D(GetFontDefault(),
                                           std::to_string(beatNum).c_str(),
                                           { 0, 0, 0 },
                                           8,
                                           1,
                                           0,
                                           false,
                                           BLUE);
          }
        }
      }
      for (const audiotrip::ChoreoEvent &event : choreo->events) {
        float beatTime = getBeatTime(static_cast<float>(event.time.beat) +
                                     static_cast<float>(event.time.numerator) /
                                       static_cast<float>(event.time.denominator));
        float beatDistance = choreo->secondsToMeters(beatTime);

        if (beatDistance > maxDistance || beatDistance < minDistance)
          continue;

        drawChoreoEventElement(event, beatDistance);
      }
    }

    Vector2 textSize = MeasureTextEx(GetFontDefault(), "Test", 20, 1);
    int margin = 10;
    int padding = 10;
    int lineSpacing = 5;
    int textHeight = static_cast<int>(textSize.y);
    int textWidth = std::max({ MeasureText(ats->title.c_str(), 20),
                               MeasureText(ats->artist.c_str(), 20),
                               MeasureText(ats->authorID.displayName.c_str(), 20) });

    int boxHeight = textHeight * 3 + lineSpacing * 2 + padding * 2;
    int boxWidth = textWidth + padding * 2;

    DrawRectangle(margin, margin, boxWidth, boxHeight, Fade(WHITE, 0.5f));
    DrawRectangleLines(margin, margin, boxWidth, boxHeight, BLUE);

    DrawText(ats->title.c_str(), padding + margin, padding + margin, 20, BLACK);
    DrawText(ats->artist.c_str(), padding + margin, padding + margin + textHeight + lineSpacing, 20, DARKGRAY);
    DrawText(ats->authorID.displayName.c_str(),
             padding + margin,
             padding + margin + textHeight * 2 + lineSpacing * 2,
             20,
             DARKGRAY);

    //    DrawFPS(padding, window->GetHeight() - MeasureTextEx(GetFontDefault(), "FPS", 20, 1).y - padding);
  }

  void drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance) {
    raylib_ext::scoped::Matrix matrix;
    Vector3 v = event.position.vectorWithDistance(distance);

    if (event.type == audiotrip::ChoreoEventTypeBarrier) {
      rlgl::rlTranslatef(0, 1.20, v.z);
      rlgl::rlRotatef(-event.position.z(), 0, 0, 1);
      rlgl::rlTranslatef(0, 0.45f - v.y, 0);
      DrawModel(*barrierModel, { 0, 0, 0 }, 1, RED);
      return;
    }

    rlgl::rlTranslatef(v.x, v.y, v.z);
    {
      raylib_ext::scoped::Matrix rotation;
      Color color = event.isRHS() ? ORANGE : PURPLE;

      switch (event.type) {
      case audiotrip::ChoreoEventTypeGemL:
      case audiotrip::ChoreoEventTypeGemR:
        rlgl::rlRotatef(event.isRHS() ? -30 : 30, 0, 0, 1);
        rlgl::rlRotatef(180, 0, 1, 0);
        DrawModel(*gemModel, { 0, 0, 0 }, 1, color);
        DrawModel(*gemTrailModel, { 0, 0, 0 }, 1, color);
        break;
      case audiotrip::ChoreoEventTypeDrumL:
      case audiotrip::ChoreoEventTypeDrumR:
        // Somebody smarter than me please fix the angles, thanks!
        rlgl::rlRotatef(-event.subPositions.front().y(), 0, 1, 0);
        rlgl::rlRotatef(event.subPositions.front().x(), 1, 0, 0);
        rlgl::rlRotatef(180, 0, 1, 0);
        DrawModel(*drumModel, { 0, 0, 0 }, 1, color);
        break;
      case audiotrip::ChoreoEventTypeDirGemL:
      case audiotrip::ChoreoEventTypeDirGemR:
        rlgl::rlRotatef(-event.subPositions.front().y(), 0, 1, 0);
        rlgl::rlRotatef(event.subPositions.front().x(), 1, 0, 0);
        rlgl::rlRotatef(180, 0, 1, 0);
        rlgl::rlRotatef(event.isRHS() ? 30 : -30, 0, 0, 1);
        DrawModel(*dirgemModel, { 0, 0, 0 }, 1, color);
        break;
      case audiotrip::ChoreoEventTypeRibbonL:
      case audiotrip::ChoreoEventTypeRibbonR: {
        // Ribbon
        auto [snake, endPosition] = genOrGetRibbon(event, distance);
        snake.Draw({ 0, 0, 0 }, 1, color);
        {
          // Initial gem
          raylib_ext::scoped::Matrix m;
          // Move 5cm back so it doesn't intersect the ribbon
          rlgl::rlTranslatef(0, 0, -0.05);
          rlgl::rlRotatef(event.isRHS() ? -30 : 30, 0, 0, 1);
          rlgl::rlRotatef(180, 0, 1, 0);
          DrawModel(*gemModel, { 0, 0, 0 }, 1, color);
        }
        {
          // Final gem
          raylib_ext::scoped::Matrix m;
          rlgl::rlTranslatef(endPosition.x, endPosition.y, endPosition.z);
          rlgl::rlRotatef(event.isRHS() ? -30 : 30, 0, 0, 1);
          rlgl::rlRotatef(180, 0, 1, 0);
          DrawModel(*gemModel, { 0, 0, 0 }, 1, color);
        }

        break;
      }
      default:
        break;
      }
    }
  }

  std::pair<raylib::Model &, Vector3> genOrGetRibbon(const audiotrip::ChoreoEvent &event, float distance) {
    RibbonKey key = { event.time.beat, event.time.numerator, event.time.denominator, event.isRHS() };
    auto it = ribbons.find(key);
    if (it != ribbons.end())
      return { it->second.first, it->second.second };

    std::vector<raylib::Vector3> positions;
    float beat = static_cast<float>(event.time.beat) +
                 static_cast<float>(event.time.numerator) / static_cast<float>(event.time.denominator);
    float beatIncrement = 1.0f / static_cast<float>(event.beatDivision);

    for (const audiotrip::Position &p : event.subPositions) {
      positions.emplace_back(p.vectorWithDistance(choreo->secondsToMeters(getBeatTime(beat)) - distance));
      beat += beatIncrement;
    }

    using namespace splines;
    std::vector<Spline3D> splines = Spline3D::FromPoints(positions);

    raylib::Mesh mesh = createSnakeMesh(splines, 0.05, 20, 8);
    ribbons.emplace(key, std::pair<raylib::Model, raylib::Vector3>{ raylib::Model{ mesh }, positions.back() });

    // Prevent the mesh from being unloaded at the end of this scope. It is now owned by the Model.
    mesh.vboId = nullptr;

    raylib::Model &model = ribbons.at(key).first;
    applyMaterialsMtl(model);
    model.meshMaterial[0] = 1;

    return { model, positions.back() };
  }
};

int main(int argc, const char *argv[]) {
  std::optional<std::string> filename = std::nullopt;

  //  chdir("/home/depau/CLionProjects/AudioTrip-LevelViewer");

  if (argc > 1) {
    if (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help") {
      std::cout << "Usage: " << argv[0] << " [ats file]" << std::endl;
      return 0;
    } else {
      filename = argv[1];
    }
  }

  Application app;
  app.main(filename);
  return 0;
}