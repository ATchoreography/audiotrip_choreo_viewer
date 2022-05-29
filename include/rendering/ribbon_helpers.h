//
// Created by depau on 5/29/22.
//

#pragma once

#include <vector>

#include "fmt/format.h"
#include "raylib-cpp.hpp"
#include "splines/spline3d.h"

namespace ribbons {

using namespace splines;
using V3f = raylib::Vector3;

std::vector<V3f> rotateShapeAroundZAxis(const std::vector<V3f> &shape, float angleInRadians);

raylib::Mesh createRibbonMesh(const std::vector<V3f> &sliceShape,
                              const std::vector<Spline3D> &splines,
                              size_t splineDivisions,
                              float textureScale = 1.0f);

} // namespace ribbons