// STL includes
#include <fstream>
#include <iostream>

// Libraries
#include "raylib-cpp.hpp"

// Local includes
#include "audiotrip/dtos.h"
#include "raylib_ext/scoped.h"
#include "raylib_ext/text3d.h"

/*
 * Note: Y is UP! The song extends parallel to Z, arms point parallel to X
 */

Model barrierModel;

static unsigned char GetRandomValueU8(unsigned char min, unsigned char max) {
  return GetRandomValue(min, max);
}

static void drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance) {
  raylib_ext::scoped::Matrix matrix;
  Vector3 v = event.position.vectorWithDistance(distance);

  if (event.type == audiotrip::ChoreoEventTypeBarrier) {
    rlTranslatef(v.x, 1.55 / 2 + 0.01, v.z);
    {
      raylib_ext::scoped::Matrix rotation;
      rlRotatef(-event.position.z(), 0, 0, 1);
      rlTranslatef(0, v.y, 0);
      DrawModel(barrierModel, { 0, 0, 0 }, 0.013, RED);
    }
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [ats file]" << std::endl;
    return 1;
  }

  audiotrip::AudioTripSong ats = audiotrip::AudioTripSong::fromFile(argv[1]);
  audiotrip::Choreography &choreo = ats.choreographies.front(); // TODO: let the user pick
  std::vector<audiotrip::Beat> beats = ats.computeBeats();

  raylib::Window window(1600, 1200, "Audio Trip Choreography Viewer");

  barrierModel = LoadModel("barrier.obj");

  float playerHeight = 1.55;
  float songWidth = playerHeight;
  float songLength = choreo.secondsToMeters(ats.songEndTimeInSeconds);

  // Define the camera to look into our 3d world (position, target, up vector)
  raylib::Camera camera((Vector3){ 0, playerHeight, 0 },
                        (Vector3){ 0.0f, 0, -20.0f },
                        (Vector3){ 0.0f, 1.0f, 0.0f },
                        60.0f,
                        CAMERA_PERSPECTIVE);

  camera.SetMode(CAMERA_FIRST_PERSON);

  SetTargetFPS(60);

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
      //      std::cout << NewPos.x << " " << NewPos.y << " " << NewPos.z << std::endl;
      camera.SetPosition(NewPos);
    }

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

  window.Close();

  return 0;
}