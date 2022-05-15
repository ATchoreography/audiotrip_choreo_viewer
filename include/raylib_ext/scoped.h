//
// Created by depau on 4/26/22.
//

#pragma once

#include "raylib-cpp.hpp"

namespace rlgl {
extern "C" {
#include "rlgl.h"
}
} // namespace

namespace raylib_ext::scoped {

class Drawing {
public:
  Drawing() { BeginDrawing(); }

  ~Drawing() { EndDrawing(); }
};

class Mode3D {
public:
  explicit Mode3D(raylib::Camera &camera) { BeginMode3D(camera); }

  ~Mode3D() { EndMode3D(); }
};

class Matrix {
public:
  Matrix() { rlgl::rlPushMatrix(); }

  ~Matrix() { rlgl::rlPopMatrix(); }
};
} // namespace raylib_ext::scoped