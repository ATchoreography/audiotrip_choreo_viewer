//
// Created by depau on 5/29/22.
//

// STL includes
#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_map>

// Libraries
#include "raylib-cpp.hpp"

namespace rlgl {
#include "rlgl.h"
}

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// raylib config
#include "config.h"

// Local includes
#include "Application.h"
#include "audiotrip/dtos.h"
#include "common_defs.h"
#include "raylib_ext/scoped.h"
#include "rendering/SkyBox.h"

Application::Application(bool debug) : debug(debug) {
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

void Application::drawFrame() {
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
