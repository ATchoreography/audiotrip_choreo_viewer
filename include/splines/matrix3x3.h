//
// Created by depau on 5/28/22.
//

#pragma once

#include "raylib-cpp.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>

namespace matrix3x3 {
using V3f = raylib::Vector3;

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
}