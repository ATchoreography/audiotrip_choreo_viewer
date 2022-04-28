// STL includes
#include <fstream>
#include <iostream>

// Libraries
#include "raylib-cpp.hpp"

// Local includes
#include "audiotrip/dtos.h"
#include "raylib_ext/rlights.h"
#include "raylib_ext/scoped.h"
#include "raylib_ext/text3d.h"

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

std::unique_ptr<raylib::Model> barrierModel;

static unsigned char GetRandomValueU8(unsigned char min, unsigned char max) {
  return GetRandomValue(min, max);
}

static void drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance) {
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
      DrawCube({ 0, 0, 0 }, 0.2, 0.2, 0.2, color);
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
      DrawCylinder({ 0, 0, 0 }, 0.25, 0.25, 0.1, 6, color);
      break;
    case audiotrip::ChoreoEventTypeDirGemL:
    case audiotrip::ChoreoEventTypeDirGemR:
      rlRotatef(-event.subPositions.front().x(), 1, 0, 0);
      rlRotatef(event.subPositions.front().y(), 0, 0, 1);
      DrawCylinder({ 0, 0, 0 }, 0, 0.2, 0.4, 20, color);
      break;
    default:
      break;
    }
  }
}

int main(int argc, const char *argv[]) {
  float playerHeight = 1.381876;

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [ats file]" << std::endl;
    return 1;
  }

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  raylib::Window window(1600, 1200, "Audio Trip Choreography Viewer");

  barrierModel = std::make_unique<raylib::Model>("resources/models/barrier.obj");

  // Define the camera to look into our 3d world (position, target, up vector)
  raylib::Camera camera({ 0, playerHeight, 0 }, { 0.0f, 0, -20.0f }, { 0.0f, 1.0f, 0.0f }, 60.0f, CAMERA_PERSPECTIVE);
  camera.SetMode(CAMERA_FIRST_PERSON);
  SetTargetFPS(60);

  // Set up lighting
  //  raylib::Shader shader(TextFormat("resources/shaders/glsl%i/base_lighting.vs", GLSL_VERSION),
  //                        TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));
  //
  //  shader.locs[SHADER_LOC_VECTOR_VIEW] = shader.GetLocation("viewPos");
  //  int ambientLoc = shader.GetLocation("ambient");
  //  float ambientLocValue[] = { 0.1f, 0.1f, 0.1f, 0.1f };
  //  SetShaderValue(shader, ambientLoc, ambientLocValue, SHADER_UNIFORM_VEC4);
  //  barrierModel->materials[0].shader = shader;
  //
  //  raylib_ext::rlights::Light light(raylib_ext::rlights::LIGHT_POINT, { 0, 0, 0 }, { 0, 0, 0 }, WHITE, shader);

  audiotrip::AudioTripSong ats = audiotrip::AudioTripSong::fromFile(argv[1]);
  audiotrip::Choreography &choreo = ats.choreographies.front(); // TODO: let the user pick
  std::vector<audiotrip::Beat> beats = ats.computeBeats();

  float songWidth = playerHeight;
  float songLength = choreo.secondsToMeters(ats.songEndTimeInSeconds);

  // 689.221
  Vector3 beatNumbersSize = raylib_ext::text3d::MeasureText3D(GetFontDefault(), "1", 8.0f, 1.0f, 0.0f);

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    camera.Update();

    bool plusPressed = IsKeyPressed(KEY_PAGE_UP);
    bool minusPressed = IsKeyPressed(KEY_PAGE_DOWN);
    if (plusPressed || minusPressed) {
      Vector3 NewPos = camera.GetPosition();
      NewPos.z += choreo.secondsToMeters(beats.at(1).time) * (minusPressed ? -1.0f : 1.0f);
      std::cout << NewPos.x << " " << NewPos.y << " " << NewPos.z << std::endl;
      camera.SetPosition(NewPos);
    }
    //    light.position = camera.position;
    //    light.Update(shader);
    //
    //    float cameraPosValue[] = { camera.position.x, camera.position.y, camera.position.z };
    //    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPosValue, SHADER_UNIFORM_VEC3);

    {
      raylib_ext::scoped::Drawing drawing;
      ClearBackground(DARKGRAY);

      {
        raylib_ext::scoped::Mode3D mode3d(camera);

        DrawPlane({ 0.0f, 0.0f, songLength / 2.0f }, { songWidth, songLength }, BLACK);

        // Draw beat numbers
        for (int beatNum = 1; beatNum <= beats.size(); beatNum++) {
          float beatTime = beats[beatNum - 1].time;
          float beatDistance = choreo.secondsToMeters(beatTime);

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
        for (const audiotrip::ChoreoEvent &event : choreo.events) {
          audiotrip::Beat beat = beats[event.time.beat];
          float beatTime = beat.time;
          float fraction = static_cast<float>(event.time.numerator) / static_cast<float>(event.time.denominator);

          if (fraction != 1 && fraction != 0) {
            float secondsPerBeat = 60.0f / beat.bpm;
            beatTime += secondsPerBeat * fraction;
          }

          float beatDistance = choreo.secondsToMeters(beatTime);
          drawChoreoEventElement(event, beatDistance);
        }
      }

      Vector2 textSize = MeasureTextEx(GetFontDefault(), "Test", 20, 1);
      int margin = 10;
      int padding = 10;
      int lineSpacing = 5;
      int textHeight = static_cast<int>(textSize.y);
      int textWidth = std::max({ MeasureText(ats.title.c_str(), 20),
                                 MeasureText(ats.artist.c_str(), 20),
                                 MeasureText(ats.authorID.displayName.c_str(), 20) });

      int boxHeight = textHeight * 3 + lineSpacing * 2 + padding * 2;
      int boxWidth = textWidth + padding * 2;

      DrawRectangle(margin, margin, boxWidth, boxHeight, Fade(WHITE, 0.5f));
      DrawRectangleLines(margin, margin, boxWidth, boxHeight, BLUE);

      DrawText(ats.title.c_str(), padding + margin, padding + margin, 20, BLACK);
      DrawText(ats.artist.c_str(), padding + margin, padding + margin + textHeight + lineSpacing, 20, DARKGRAY);
      DrawText(ats.authorID.displayName.c_str(),
               padding + margin,
               padding + margin + textHeight * 2 + lineSpacing * 2,
               20,
               DARKGRAY);
    }
  }

  return 0;
}