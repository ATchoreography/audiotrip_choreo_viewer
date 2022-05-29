//
// Created by depau on 5/29/22.
//

#pragma once

// STL includes
#include <memory>

// Libraries
#include "raylib-cpp.hpp"

namespace rlgl {
#include "rlgl.h"
}

// Local includes
#include "common_defs.h"

class SkyBox {
public:
  raylib::Shader shader{ TextFormat("resources/shaders/glsl%i/skybox.vs", GLSL_VERSION),
                         TextFormat("resources/shaders/glsl%i/skybox.fs", GLSL_VERSION) };
  raylib::Model skybox{ raylib::Mesh::Cube(100, 100, 100) };

  std::unique_ptr<raylib::TextureCubemap> texture;

  SkyBox(const std::string &imagePath);

  void LoadTexture(const std::string &imagePath);

  void Draw();
};