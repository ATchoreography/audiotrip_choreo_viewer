//
// Created by depau on 5/29/22.
//

// STL includes
#include <algorithm>
#include <optional>

// Library includes
#include "raylib-cpp.hpp"

namespace rlgl {
#include "rlgl.h"
}

// Local includes
#include "Application.h"
#include "raylib_ext/text3d.h"
#include "rendering/ribbon_helpers.h"
#include "splines/spline3d.h"

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::vector<raylib::Vector3> RibbonShape{
  { 0.06763590399999997f, -0.03723645799999998f, 0.0f },   { 0.012288303999999983f, 0.05794114199999999f, 0.0f },
  { 0.0076265839999999745f, 0.061651142f, 0.0f },          { 0.0022791439999999825f, 0.063199962f, 0.0f },
  { -0.004558176000000008f, 0.06266969800000001f, 0.0f },  { -0.010277735999999999f, 0.06015566200000001f, 0.0f },
  { -0.014420896000000002f, 0.056452662f, 0.0f },          { -0.017133956f, 0.05142190199999999f, 0.0f },
  { -0.066929156f, -0.036593297999999996f, 0.0f },         { -0.069293896f, -0.04125585799999999f, 0.0f },
  { -0.070000364f, -0.04673069799999998f, 0.0f },          { -0.068853176f, -0.051434137999999976f, 0.0f },
  { -0.06474865600000002f, -0.057691017999999976f, 0.0f }, { -0.05898933600000002f, -0.06178741799999996f, 0.0f },
  { -0.05422261600000002f, -0.06319996199999997f, 0.0f },  { -0.049258216000000014f, -0.06308628199999997f, 0.0f },
  { 0.05459378399999998f, -0.06290008199999995f, 0.0f },   { 0.060133023999999986f, -0.061001205999999975f, 0.0f },
  { 0.064287944f, -0.057901605999999974f, 0.0f },          { 0.06743514399999999f, -0.05396900599999998f, 0.0f },
  { 0.06962773999999998f, -0.048809446f, 0.0f },           { 0.070000364f, -0.043388366000000005f, 0.0f },
  { 0.06763590399999997f, -0.03723645799999998f, 0.0f }
};

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
  raylib::Model tempModel("resources/models/ribbon_fake_model.obj");

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
static void drawChoreoFloor(raylib::Texture2D &texture, const raylib::Camera &camera) {
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

void Application::drawSplash() {
  ClearBackground(WHITE);

  const char *text = "Drag and drop an ATS file on this window";

  Vector2 textSize = MeasureTextEx(GetFontDefault(), text, 20, 1);
  int textWidth = static_cast<int>(textSize.x);
  int textHeight = static_cast<int>(textSize.y);

  int posX = window->GetWidth() / 2 - textWidth / 2;
  int posY = window->GetHeight() / 2 - textHeight / 2;

  DrawText(text, posX, posY, 20, BLACK);
}
void Application::drawChoreo() {
  ClearBackground(GRAY);

  {
    raylib_ext::scoped::Mode3D mode3d(*camera);

    skybox->Draw();

    drawChoreoFloor(*floorTexture, *camera);

    float minDistance = camera->position.z - MAX_RENDER_DISTANCE;
    float maxDistance = camera->position.z + MAX_RENDER_DISTANCE;

    // Draw beat numbers
    for (int beatNum = 1; beatNum <= beats.size(); beatNum++) {
      float beatTime = beats[beatNum - 1].time;
      float beatDistance = choreo().secondsToMeters(beatTime);

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
    for (const audiotrip::ChoreoEvent &event : choreo().events) {
      float beatTime = getBeatTime(static_cast<float>(event.time.beat) + static_cast<float>(event.time.numerator) /
                                                                           static_cast<float>(event.time.denominator));
      float beatDistance = choreo().secondsToMeters(beatTime);

      if (beatDistance > maxDistance || beatDistance < minDistance)
        continue;

      drawChoreoEventElement(event, beatDistance);
    }
  }

  gui.Draw();

  if (mouseCaptured) {
    DrawText("M - Press M to release mouse", 8, window->GetHeight() - 20, 15, WHITE);
  }
}
void Application::drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance) {
  raylib_ext::scoped::Matrix matrix;
  Vector3 v = event.position.vectorWithDistance(distance);

  if (event.type == audiotrip::ChoreoEventTypeBarrier) {
    rlgl::rlTranslatef(0, 1.20, v.z);
    rlgl::rlRotatef(-event.position.z(), 0, 0, 1);
    rlgl::rlTranslatef(0, 0.45f - v.y, 0);
    DrawModel(*barrierModel, { 0, 0, 0 }, 1, gui.barrierColorPickerValue);
    return;
  }

  rlgl::rlTranslatef(v.x, v.y, v.z);
  {
    raylib_ext::scoped::Matrix rotation;
    Color color = event.isRHS() ? gui.rhsColorPickerValue : gui.lhsColorPickerValue;

    switch (event.type) {
    case audiotrip::ChoreoEventTypeGemL:
    case audiotrip::ChoreoEventTypeGemR:
      rlgl::rlRotatef(event.isRHS() ? -30 : 30, 0, 0, 1);
      rlgl::rlRotatef(180, 0, 1, 0);
      DrawModel(*gemModel, { 0, 0, 0 }, 1, color);
      color.a = 0x7f;
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
      Color snakeColor = color;
      snakeColor.a = 0xA0;
      snake.Draw({ 0, 0.006, 0 }, 1, snakeColor);
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

std::pair<raylib::Model &, Vector3> Application::genOrGetRibbon(const audiotrip::ChoreoEvent &event, float distance) {
  RibbonKey key = { event.time.beat, event.time.numerator, event.time.denominator, event.isRHS() };
  auto it = ribbons.find(key);
  if (it != ribbons.end())
    return { it->second.first, it->second.second };

  std::vector<raylib::Vector3> positions;
  float beat = static_cast<float>(event.time.beat) +
               static_cast<float>(event.time.numerator) / static_cast<float>(event.time.denominator);
  float beatIncrement = 1.0f / static_cast<float>(event.beatDivision);

  for (const audiotrip::Position &p : event.subPositions) {
    positions.emplace_back(p.vectorWithDistance(choreo().secondsToMeters(getBeatTime(beat)) - distance));
    beat += beatIncrement;
  }

  using namespace splines;
  std::vector<Spline3D> splines = Spline3D::FromPoints(positions);

  std::vector<raylib::Vector3> sliceShape = ribbons::rotateShapeAroundZAxis(RibbonShape,
                                                                            PI / 6.0 * (event.isRHS() ? -1 : 1));
  raylib::Mesh mesh = ribbons::createRibbonMesh(sliceShape,
                                                splines,
                                                static_cast<size_t>(
                                                  std::max(2.0f, 128.0f / static_cast<float>(event.beatDivision))),
                                                static_cast<float>(splines.size()) *
                                                  (static_cast<float>(choreo().gemSpeed) / 2.5f) /
                                                  static_cast<float>(event.beatDivision));
  ribbons.emplace(key, std::pair<raylib::Model, raylib::Vector3>{ raylib::Model{ mesh }, positions.back() });

  // Prevent the mesh from being unloaded at the end of this scope. It is now owned by the Model.
  mesh.vboId = nullptr;

  raylib::Model &model = ribbons.at(key).first;
  applyMaterialsMtl(model);
  model.meshMaterial[0] = 0;

  unsigned int textureId = model.materials[0].maps[0].texture.id;

  rlgl::rlTextureParameters(textureId, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_ANISOTROPIC);
  rlgl::rlTextureParameters(textureId, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_REPEAT);
  rlgl::rlTextureParameters(textureId, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_REPEAT);

  return { model, positions.back() };
}
