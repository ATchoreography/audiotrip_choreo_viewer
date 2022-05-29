//
// Created by depau on 5/17/22.
//

/**
 * Adaptation and port to raylib vectors of some parts of splines-lib
 * https://github.com/andrewwillmott/splines-lib
 *
 * I mostly have no understanding of the math that is involved. The idea is if splines-lib is able to calculate the
 * spline through a number of points, let me subdivide it in equidistant locations and tell me the tangent vector I can
 * then compute circles around each vector and use these point to create the faces of the spline.
 */

#pragma once

#include <optional>
#include <tuple>

#include "matrix3x3.h"
#include "raylib-cpp.hpp"

namespace splines {
using namespace matrix3x3;
using V3f = raylib::Vector3;

inline raylib::Vector3 operator+(const raylib::Vector3 &v1, const raylib::Vector3 &v2) {
  return Vector3Add(v1, v2);
}
inline raylib::Vector3 operator-(const raylib::Vector3 &v1, const raylib::Vector3 &v2) {
  return Vector3Subtract(v1, v2);
}
inline raylib::Vector3 operator*(float f, const raylib::Vector3 &v) {
  return v * raylib::Vector3(f);
}
inline float Vector4DotProduct(const raylib::Vector4 &v1, const raylib::Vector4 &v2) {
  return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) + (v1.w * v2.w);
}

/// Returns Bezier basis weights for 't'
inline raylib::Vector4 BezierWeights(float t) {
  float s = 1.0f - t;

  float t2 = t * t;
  float t3 = t2 * t;

  float s2 = s * s;
  float s3 = s2 * s;

  return { s3, 3.0f * s2 * t, 3.0f * s * t2, t3 };
}

/// Vector version useful for derivatives
inline raylib::Vector4 BezierWeights(const raylib::Vector4 &t) {
  return { t.x - 3.0f * t.y + 3.0f * t.z - t.w, 3.0f * t.y - 6.0f * t.z + 3.0f * t.w, 3.0f * t.z - 3.0f * t.w, t.w };
}

class Spline3D {
  raylib::Vector4 xb;
  raylib::Vector4 yb;
  raylib::Vector4 zb;

public:
  Spline3D(const Vector4 &xb, const Vector4 &yb, const Vector4 &zb) : xb(xb), yb(yb), zb(zb) {}

  // Spline creation
  ///< Return Bezier spline from p0 to p3 with guide points p1, p2
  static Spline3D Bezier(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3) {
    return { raylib::Vector4(p0.x, p1.x, p2.x, p3.x),
             raylib::Vector4(p0.y, p1.y, p2.y, p3.y),
             raylib::Vector4(p0.z, p1.z, p2.z, p3.z) };
  }

  ///< Returns number of splines needed to represent the given number of points; generally n-1 except for n < 2.
  static inline size_t NumSplinesForPoints(int numPoints) {
    if (numPoints < 2)
      return numPoints;
    return numPoints - 1;
  }

private:
  static Spline3D splineFromPoints3(const raylib::Vector3 &p0,
                                    const raylib::Vector3 &p1,
                                    const raylib::Vector3 &p2,
                                    const raylib::Vector3 &p3,
                                    float tension) {

    float s = (1.0f - tension) / 6.0f;

    raylib::Vector3 pb1 = p1 + (p2 - p0).Scale(s);
    raylib::Vector3 pb2 = p2 - (p3 - p1).Scale(s);

    return Bezier(p1, pb1, pb2, p2);
  }

public:
  ///< Fills 'splines' with splines that interpolate the points in 'p', and returns the number of these splines.
  ///< 'tension' controls the interpolation -- the default value of 0 specifies Catmull-Rom splines that
  ///< guarantee tangent continuity. With +1 you get straight lines, and -1 gives more of a circular appearance.
  static std::vector<Spline3D> FromPoints(const std::vector<raylib::Vector3> &points, float tension = -1) {
    switch (points.size()) {
    case 0:
      return {};
    case 1:
      return { splineFromPoints3(points.at(0), points.at(0), points.at(0), points.at(0), tension) };
    case 2:
      return { splineFromPoints3(points.at(0), points.at(0), points.at(1), points.at(1), tension) };
    }

    std::vector<Spline3D> result;
    result.reserve(NumSplinesForPoints(static_cast<int>(points.size())));

    result.push_back(splineFromPoints3(points.at(0), points.at(0), points.at(1), points.at(2), tension));

    for (int i = 0; i < points.size() - 3; i++) {
      result.push_back(
        splineFromPoints3(points.at(i + 0), points.at(i + 1), points.at(i + 2), points.at(i + 3), tension));
    }

    size_t offset = points.size() - 3;
    result.push_back(splineFromPoints3(points.at(offset + 0),
                                       points.at(offset + 1),
                                       points.at(offset + 2),
                                       points.at(offset + 2),
                                       tension));

    return result;
  }

private:
  [[nodiscard]] inline raylib::Vector3 Evaluate(const raylib::Vector4 &w) const {
    return { Vector4DotProduct(xb, w), Vector4DotProduct(yb, w), Vector4DotProduct(zb, w) };
  }

public:
  ///< Returns interpolated position
  [[nodiscard]] raylib::Vector3 Position(float t) const { return Evaluate(BezierWeights(t)); }

  ///< Returns interpolated velocity
  [[nodiscard]] raylib::Vector3 Velocity(float t) const {
    raylib::Vector4 dt4(0, 1, 2 * t, 3 * t * t);
    return Evaluate(BezierWeights(dt4));
  }

  [[nodiscard]] std::pair<Spline3D, Spline3D> Split(float t) const;
  [[nodiscard]] std::pair<Spline3D, Spline3D> Split() const;

  [[nodiscard]] float LengthEstimate(float &error) const;
  [[nodiscard]] float Length(float maxError = 0.01f) const;
  [[nodiscard]] float Length(float t0, float t1, float maxError = 0.01f) const;
};

std::vector<V3f> rotateShapeAroundZAxis(const std::vector<V3f> &shape, float angleInRadians);

raylib::Mesh createRibbonMesh(const std::vector<V3f> &sliceShape,
                              const std::vector<Spline3D> &splines,
                              size_t splineDivisions,
                              float textureScale = 1.0f);
} // namespace splines