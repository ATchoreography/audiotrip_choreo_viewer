// STL includes
#include <fstream>
#include <iostream>
#include <optional>

// Libraries
#include "raylib-cpp.hpp"

// Local includes
#include "audiotrip/dtos.h"
#include "raylib_ext/rlights.h"
#include "raylib_ext/scoped.h"
#include "raylib_ext/text3d.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

/*
 * Note: Y is UP! The song extends parallel to Z, arms point parallel to X
 */

// Barrier notes:
// - 20cm above the floor
// - 225cm wide
// - XY=0, angle 0 places the barrier facing down, bottom side at 180cm
// - XY=0, angle 90° places the barrier on the side, center tick is at 125cm
// - => reference point is in the middle, 55cm below the bottom side
// - Y position is subtracted, not added

class Application {
private:
  float playerHeight = 1.381876;

  std::unique_ptr<raylib::Window> window;
  std::unique_ptr<raylib::Camera> camera;
  std::unique_ptr<raylib::Shader> shader;

  std::unique_ptr<raylib_ext::rlights::Light> light;

  std::unique_ptr<raylib::Model> barrierModel;

  std::unique_ptr<raylib::Model> drumModelRHS;
  std::unique_ptr<raylib::Model> gemModelRHS;
  std::unique_ptr<raylib::Model> dirgemModelRHS;

  std::unique_ptr<raylib::Model> drumModelLHS;
  std::unique_ptr<raylib::Model> gemModelLHS;
  std::unique_ptr<raylib::Model> dirgemModelLHS;

  Vector3 beatNumbersSize = { -1, -1, -1 };

  std::unique_ptr<audiotrip::AudioTripSong> ats;
  audiotrip::Choreography *choreo = nullptr;
  std::vector<audiotrip::Beat> beats;

public:
  Application() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    window = std::make_unique<raylib::Window>(800, 600, "Audio Trip Choreography Viewer");
    (void) window; // Silence unused variable

    // NOLINTNEXTLINE(modernize-make-unique)
    camera.reset(new raylib::Camera({ 0.0f, playerHeight, 0.0f },
                                    { 0.0f, 0, -20.0f },
                                    { 0.0f, 1.0f, 0.0f },
                                    60.0f,
                                    CAMERA_PERSPECTIVE));
    camera->SetMode(CAMERA_FIRST_PERSON);

    barrierModel = std::make_unique<raylib::Model>("resources/models/barrier.obj");
    drumModelLHS = std::make_unique<raylib::Model>("resources/models/at_drum.obj");
    dirgemModelLHS = std::make_unique<raylib::Model>("resources/models/at_dirgem.obj");
    gemModelLHS = std::make_unique<raylib::Model>("resources/models/at_gem.obj");
    drumModelRHS = std::make_unique<raylib::Model>("resources/models/at_drum.obj");
    dirgemModelRHS = std::make_unique<raylib::Model>("resources/models/at_dirgem.obj");
    gemModelRHS = std::make_unique<raylib::Model>("resources/models/at_gem.obj");

    shader = std::make_unique<raylib::Shader>(TextFormat("resources/shaders/glsl%i/base_lighting.vs", GLSL_VERSION),
                                              TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));

    shader->locs[SHADER_LOC_VECTOR_VIEW] = shader->GetLocation("viewPos");
    shader->locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(*shader, "matModel");

    int ambientLoc = shader->GetLocation("ambient");
    float ambientVal[]{ 0.1f, 0.1f, 0.1f, 0.1f };
    SetShaderValue(*shader, ambientLoc, ambientVal, SHADER_UNIFORM_VEC4);

    //  float fogDensity = 0.15f;
    //  int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    //  SetShaderValue(shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    barrierModel->materials[0].shader = *shader;
    drumModelRHS->materials[0].shader = *shader;
    gemModelRHS->materials[0].shader = *shader;
    dirgemModelRHS->materials[0].shader = *shader;
    drumModelLHS->materials[0].shader = *shader;
    gemModelLHS->materials[0].shader = *shader;
    dirgemModelLHS->materials[0].shader = *shader;

    light = std::make_unique<raylib_ext::rlights::Light>(raylib_ext::rlights::LIGHT_DIRECTIONAL,
                                                         camera->position,
                                                         camera->target,
                                                         WHITE,
                                                         *shader);
  }

  void main(std::optional<std::string> atsFile) {
    beatNumbersSize = raylib_ext::text3d::MeasureText3D(GetFontDefault(), "1", 8.0f, 1.0f, 0.0f);

    if (atsFile.has_value()) {
      ats = std::make_unique<audiotrip::AudioTripSong>(std::move(audiotrip::AudioTripSong::fromFile(*atsFile)));
      choreo = &ats->choreographies.front(); // TODO: let the user pick
      beats = ats->computeBeats();
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
  static void emscriptenMainloop(void *obj) {
    static_cast<Application *>(obj)->drawFrame();
  }

  void drawFrame() {
    camera->Update();

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
    light->position = pos;
    light->target = camera->target;
    light->Update(*shader);

    {
      //      raylib_ext::scoped::Drawing drawing;
      BeginDrawing();
      drawChoreo();
      EndDrawing();
    }
  }

  void drawChoreo() {
    float songWidth = playerHeight;
    float songLength = choreo->secondsToMeters(ats->songEndTimeInSeconds);

    ClearBackground(GRAY);

    {
      BeginMode3D(*camera);
      //      raylib_ext::scoped::Mode3D mode3d(*camera);

      DrawPlane({ 0.0f, 0.0f, songLength / 2.0f }, { songWidth, songLength }, BLACK);

      // Draw beat numbers
      for (int beatNum = 1; beatNum <= beats.size(); beatNum++) {
        float beatTime = beats[beatNum - 1].time;
        float beatDistance = choreo->secondsToMeters(beatTime);

        {
          raylib_ext::scoped::Matrix translateM;
          rlTranslatef(-songWidth / 2 - 0.1f, 0, beatDistance / 3.0f + beatNumbersSize.z / 2.0f);
          {
            raylib_ext::scoped::Matrix rotateM;
            rlRotatef(180, 0, 1, 0);

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
        audiotrip::Beat beat = beats[event.time.beat];
        float beatTime = beat.time;
        float fraction = static_cast<float>(event.time.numerator) / static_cast<float>(event.time.denominator);

        if (fraction != 1 && fraction != 0) {
          float secondsPerBeat = 60.0f / beat.bpm;
          beatTime += secondsPerBeat * fraction;
        }

        float beatDistance = choreo->secondsToMeters(beatTime);
        drawChoreoEventElement(event, beatDistance);
      }
      EndMode3D();
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
  }

  void drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance) {
    raylib_ext::scoped::Matrix matrix;
    Vector3 v = event.position.vectorWithDistance(distance);

    if (event.type == audiotrip::ChoreoEventTypeBarrier) {
      rlTranslatef(v.x, 1.25, v.z);
      rlRotatef(-event.position.z(), 0, 0, 1);
      rlTranslatef(0, 0.55f - v.y, 0);
      DrawModel(*barrierModel, { 0, 0, 0 }, 0.0145, RED);
      return;
    }

    rlTranslatef(v.x, v.y, v.z);
    {
      raylib_ext::scoped::Matrix rotation;
      Color color = event.isRHS() ? ORANGE : PURPLE;

      switch (event.type) {
      case audiotrip::ChoreoEventTypeGemL:
      case audiotrip::ChoreoEventTypeGemR:
        //      DrawCube({ 0, 0, 0 }, 0.2, 0.2, 0.2, color);
        rlRotatef(event.isRHS() ? -30 : 30, 0, 0, 1);
        rlRotatef(180, 0, 1, 0);
        rlRotatef(90, 1, 0, 0);
        DrawModel(*(event.isRHS() ? gemModelRHS : gemModelLHS), { 0, 0, 0 }, 0.01, color);
        break;
      case audiotrip::ChoreoEventTypeRibbonL:
      case audiotrip::ChoreoEventTypeRibbonR:
        break;
      case audiotrip::ChoreoEventTypeDrumL:
      case audiotrip::ChoreoEventTypeDrumR:
        // Somebody smarter than me please fix the angles, thanks!
        rlRotatef(180, 0, 1, 0);
        rlRotatef(90 - event.subPositions.front().x(), 1, 0, 0);
        rlRotatef(270 - event.subPositions.front().y(), 0, 0, 1);
        //      DrawCylinder({ 0, 0, 0 }, 0.25, 0.25, 0.1, 6, color);
        DrawModel(*(event.isRHS() ? drumModelRHS : drumModelLHS), { 0, 0, 0 }, 0.01, color);
        break;
      case audiotrip::ChoreoEventTypeDirGemL:
      case audiotrip::ChoreoEventTypeDirGemR:
        rlRotatef(-event.subPositions.front().x(), 1, 0, 0);
        rlRotatef(event.subPositions.front().y(), 0, 0, 1);
        DrawModel(*(event.isRHS() ? dirgemModelRHS : dirgemModelLHS), { 0, 0, 0 }, 0.01, color);
        //      DrawCylinder({ 0, 0, 0 }, 0, 0.2, 0.4, 20, color);
        break;
      default:
        break;
      }
    }
  }
};

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [ats file]" << std::endl;
    return 1;
  } else {
    Application app;
    app.main(argv[1]);
  }
  return 0;
}