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

#include <iomanip>
#include <iostream>
#include <optional>
#include <tuple>

#include "raylib-cpp.hpp"

namespace splines {
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

class Matrix3x3 {
public:
  float m00, m01, m02;
  float m10, m11, m12;
  float m20, m21, m22;

  Matrix3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22) :
    m00(m00), m01(m01), m02(m02), m10(m10), m11(m11), m12(m12), m20(m20), m21(m21), m22(m22) {}

  static Matrix3x3 fromRowVectors(const V3f &row0, const V3f &row1, const V3f &row2) {
    // clang-format off
    return {
      row0.x, row0.y, row0.z,
      row1.x, row1.y, row1.z,
      row2.x, row2.y, row2.z
    };
    // clang-format on
  }

  static Matrix3x3 fromColVectors(const V3f &col0, const V3f &col1, const V3f &col2) {
    // clang-format off
    return {
      col0.x, col1.x, col2.x,
      col0.y, col1.y, col2.y,
      col0.z, col1.z, col2.z,
    };
    // clang-format on
  }

  static Matrix3x3 skewSymmetricCrossProductMatrix(const V3f &v) {
    // clang-format off
    return {
       0,   -v.z,   v.y,
       v.z,  0,    -v.x,
      -v.y,  v.x,   0
    };
    // clang-format on
  }

  static inline Matrix3x3 identity() { return { 1, 0, 0, 0, 1, 0, 0, 0, 1 }; }

  [[nodiscard]] inline V3f row(unsigned int index) const {
    assert(index < 3);
    switch (index) {
    case 0:
      return { m00, m01, m02 };
    case 1:
      return { m10, m11, m12 };
    case 2:
      return { m20, m21, m22 };
    default:
      __builtin_unreachable();
    }
  }

  [[nodiscard]] inline V3f col(unsigned int index) const {
    assert(index < 3);
    switch (index) {
    case 0:
      return { m00, m10, m20 };
    case 1:
      return { m01, m11, m21 };
    case 2:
      return { m02, m12, m22 };
    default:
      __builtin_unreachable();
    }
  }

  Matrix3x3 operator+(const Matrix3x3 &other) const {
    return { this->m00 + other.m00, this->m01 + other.m01, this->m02 + other.m02,
             this->m10 + other.m10, this->m11 + other.m11, this->m12 + other.m12,
             this->m20 + other.m20, this->m21 + other.m21, this->m22 + other.m22 };
  }

  Matrix3x3 operator*(float val) const {
    return { this->m00 * val, this->m01 * val, this->m02 * val, this->m10 * val, this->m11 * val,
             this->m12 * val, this->m20 * val, this->m21 * val, this->m22 * val };
  }

  //  Matrix3x3 operator*(const Matrix3x3 &other) const {
  //    return {
  //      this->m00 * other.m00 + this->m01 * other.m10 + this->m02 * other.m20,
  //      this->m00 * other.m01 + this->m01 * other.m11 + this->m02 * other.m21,
  //      this->m00 * other.m02 + this->m01 * other.m12 + this->m02 * other.m22,
  //
  //      this->m10 * other.m00 + this->m11 * other.m10 + this->m12 * other.m20,
  //      this->m10 * other.m01 + this->m11 * other.m11 + this->m12 * other.m21,
  //      this->m10 * other.m02 + this->m11 * other.m12 + this->m12 * other.m22,
  //
  //      this->m20 * other.m00 + this->m21 * other.m10 + this->m22 * other.m20,
  //      this->m20 * other.m01 + this->m21 * other.m11 + this->m22 * other.m21,
  //      this->m20 * other.m02 + this->m21 * other.m12 + this->m22 * other.m22,
  //    };
  //  }

  Matrix3x3 operator*(const Matrix3x3 &other) const {
    return {
      row(0).DotProduct(col(0)), row(0).DotProduct(col(1)), row(0).DotProduct(col(2)),

      row(1).DotProduct(col(0)), row(1).DotProduct(col(1)), row(1).DotProduct(col(2)),

      row(2).DotProduct(col(0)), row(2).DotProduct(col(1)), row(2).DotProduct(col(2)),
    };
  }

  V3f operator*(const V3f &v) const { return { row(0).DotProduct(v), row(1).DotProduct(v), row(2).DotProduct(v) }; }

  template<typename T>
  void operator+=(const T &other) {
    *this = (*this + other);
  }

  template<typename T>
  void operator*=(const T &other) {
    *this = (*this * other);
  }

  [[nodiscard]] Matrix3x3 power(unsigned int exponent) const {
    Matrix3x3 accumulator = *this;
    for (unsigned int i = 1; i < exponent; i++) {
      accumulator *= *this;
    }
    return accumulator;
  }

  void dump() const {
    // clang-format off
    std::cout << std::setw(3) << std::setfill(' ') << \
      m00 << " " << m01 << " " << m02 << std::endl << \
      m10 << " " << m11 << " " << m12 << std::endl <<
      m20 << " " << m21 << " " << m22 << std::endl;
    ;
    // clang-format on
  }
};

std::vector<raylib::Vector3> circleNormalToVector(const raylib::Vector3 &v, size_t points);

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

    float s = (1.0f - tension) * (1.0f / 6.0f);

    raylib::Vector3 pb1 = p1 + s * (p2 - p0);
    raylib::Vector3 pb2 = p2 - s * (p3 - p1);

    return Bezier(p1, pb1, pb2, p2);
  }

public:
  ///< Fills 'splines' with splines that interpolate the points in 'p', and returns the number of these splines.
  ///< 'tension' controls the interpolation -- the default value of 0 specifies Catmull-Rom splines that
  ///< guarantee tangent continuity. With +1 you get straight lines, and -1 gives more of a circular appearance.
  // I don't really understand this code tbh, I just copied it and C++-ified it as best as I could
  static std::vector<Spline3D> FromPoints(const std::vector<raylib::Vector3> &points, float tension = 0.0f) {
    switch (points.size()) {
    case 0:
      return {};
    case 1:
      return { splineFromPoints3(points.at(0), points.at(0), points.at(0), points.at(0), tension) };
    case 2:
      return { splineFromPoints3(points.at(0), points.at(0), points.at(1), points.at(1), tension) };
    }

    std::vector<Spline3D> result;
    result.reserve(NumSplinesForPoints(points.size()));

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
    return { Vector4DotProduct(this->xb, w), Vector4DotProduct(this->yb, w), Vector4DotProduct(this->zb, w) };
  }

public:
  ///< Returns interpolated position
  [[nodiscard]] raylib::Vector3 Position(float t) const {
    return Evaluate(BezierWeights(t));
  } ///< Returns interpolated velocity

  raylib::Vector3 PoorMansTangent(float t) const {
    float t1 = t == 1 ? t - 0.1f : t;
    float t2 = t == 1 ? t : t + 0.1f;
    return Position(t2).Subtract(Position(t1));
  }

  [[nodiscard]] raylib::Vector3 Velocity(float t) const {
    raylib::Vector4 dt4(0, 1, 2 * t, 3 * t * t);
    return Evaluate(BezierWeights(dt4));
  }
  ///< Returns interpolated acceleration
  [[nodiscard]] raylib::Vector3 Acceleration(float t) const {
    raylib::Vector4 ddt4(0, 0, 2, 6 * t);
    return Evaluate(BezierWeights(ddt4));
  }
  // Subdivision
};

std::vector<V3f> rotateShapeAroundZAxis(const std::vector<V3f> &shape, float angleInRadians);

raylib::Mesh createRibbonMesh(const std::vector<V3f> &sliceShape,
                              const std::vector<Spline3D> &splines,
                              size_t splineDivisions);
} // namespace splines