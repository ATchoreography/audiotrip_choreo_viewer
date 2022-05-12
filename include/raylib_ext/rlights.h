// This file has been copied from raylib and modified.
// https://github.com/raysan5/raylib/blob/5c66cc1c9bb4a3ae4af7cc6f82a521137ed5484d/examples/shaders/rlights.h

/**********************************************************************************************
 *
 *   raylib.lights - Some useful functions to deal with lights data
 *
 *   CONFIGURATION:
 *
 *   #define RLIGHTS_IMPLEMENTATION
 *       Generates the implementation of the library into the included file.
 *       If not defined, the library is in header only mode and can be included in other headers
 *       or source files without problems. But only ONE file should hold the implementation.
 *
 *   LICENSE: zlib/libpng
 *
 *   Copyright (c) 2017-2020 Victor Fisac (@victorfisac) and Ramon Santamaria (@raysan5)
 *
 *   This software is provided "as-is", without any express or implied warranty. In no event
 *   will the authors be held liable for any damages arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose, including commercial
 *   applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not claim that you
 *     wrote the original software. If you use this software in a product, an acknowledgment
 *     in the product documentation would be appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be misrepresented
 *     as being the original software.
 *
 *     3. This notice may not be removed or altered from any source distribution.
 *
 **********************************************************************************************/

#pragma once

// Stdlib includes

// Library includes
#include "fmt/core.h"
#include "raylib-cpp.hpp"

namespace raylib_ext::rlights {

#define MAX_LIGHTS 4 // Max dynamic lights supported by shader

using namespace raylib;

// Light type
typedef enum {
  LIGHT_DIRECTIONAL,
  LIGHT_POINT
} LightType;

// Light data
class Light {
private:
  static inline int lightsCount = 0;

public:
  bool enabled;
  int type;
  raylib::Vector3 position;
  raylib::Vector3 target;
  raylib::Color color;

  // Shader locations
  int enabledLoc;
  int typeLoc;
  int posLoc;
  int targetLoc;
  int colorLoc;

  Light(int type, raylib::Vector3 position, raylib::Vector3 target, raylib::Color color, raylib::Shader &shader) :
    enabled(true),
    type(type),
    position(position),
    target(target),
    color(color),
    enabledLoc(shader.GetLocation(fmt::format("lights[{}].enabled", lightsCount))),
    typeLoc(shader.GetLocation(fmt::format("lights[{}].type", lightsCount))),
    posLoc(shader.GetLocation(fmt::format("lights[{}].position", lightsCount))),
    targetLoc(shader.GetLocation(fmt::format("lights[{}].target", lightsCount))),
    colorLoc(shader.GetLocation(fmt::format("lights[{}].color", lightsCount))) {

    lightsCount++;
    assert(lightsCount <= MAX_LIGHTS);
  }

  void Update(raylib::Shader &shader) {
    // Send to shader light enabled state and type
    SetShaderValue(shader, enabledLoc, &enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, typeLoc, &type, SHADER_UNIFORM_INT);

    // Send to shader light position values
    float pos[3] = { position.x, position.y, position.z };
    SetShaderValue(shader, posLoc, pos, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float tgt[3] = { target.x, target.y, target.z };
    SetShaderValue(shader, targetLoc, tgt, SHADER_UNIFORM_VEC3);

    // Send to shader light color values
    float clr[4] = { (float) color.r / (float) 255,
                     (float) color.g / (float) 255,
                     (float) color.b / (float) 255,
                     (float) color.a / (float) 255 };
    SetShaderValue(shader, colorLoc, clr, SHADER_UNIFORM_VEC4);
  }
};

} // namespace raylib_ext::rlights