/**
 * Adaptation and port to raylib vectors of some parts of splines-lib
 * https://github.com/andrewwillmott/splines-lib
 */

#include <cassert>
#include <optional>
#include <utility>

#include "fmt/format.h"
#include "splines/spline3d.h"

namespace splines {

static float sqr(float x) {
  return x * x;
}

static float lerp(float a, float b, float t) {
  return (1.0f - t) * a + t * b;
}


// Optimised for t=0.5
static void SplitImpl(const raylib::Vector4 &spline, raylib::Vector4 &spline0, raylib::Vector4 &spline1) {
  float q0 = (spline.x + spline.y) * 0.5f; // x + y / 2
  float q1 = (spline.y + spline.z) * 0.5f; // y + z / 2
  float q2 = (spline.z + spline.w) * 0.5f; // z + w / 2

  float r0 = (q0 + q1) * 0.5f; // x + 2y + z / 4
  float r1 = (q1 + q2) * 0.5f; // y + 2z + w / 4

  float s0 = (r0 + r1) * 0.5f; // q0 + 2q1 + q2 / 4 = x+y + 2(y+z) + z+w / 8 = x + 3y + 3z + w

  float sx = spline.x; // support aliasing
  float sw = spline.w;

  spline0 = raylib::Vector4{ sx, q0, r0, s0 };
  spline1 = raylib::Vector4{ s0, r1, q2, sw };
}

static void SplitImpl(float t, const raylib::Vector4 &spline, raylib::Vector4 &spline0, raylib::Vector4 &spline1) {
  // assumption: seg = (P0, P1, P2, P3)
  float q0 = lerp(spline.x, spline.y, t);
  float q1 = lerp(spline.y, spline.z, t);
  float q2 = lerp(spline.z, spline.w, t);

  float r0 = lerp(q0, q1, t);
  float r1 = lerp(q1, q2, t);

  float s0 = lerp(r0, r1, t);

  float sx = spline.x; // support aliasing
  float sw = spline.w;

  spline0 = raylib::Vector4{ sx, q0, r0, s0 };
  spline1 = raylib::Vector4{ s0, r1, q2, sw };
}

std::pair<Spline3D, Spline3D> Spline3D::Split(float t) const {
  std::pair<Spline3D, Spline3D> result({ {}, {}, {} }, { {}, {}, {} });

  SplitImpl(t, xb, result.first.xb, result.second.xb);
  SplitImpl(t, yb, result.first.yb, result.second.yb);
  SplitImpl(t, zb, result.first.zb, result.second.zb);

  return result;
}

std::pair<Spline3D, Spline3D> Spline3D::Split() const {
  std::pair<Spline3D, Spline3D> result({ {}, {}, {} }, { {}, {}, {} });

  SplitImpl(xb, result.first.xb, result.second.xb);
  SplitImpl(yb, result.first.yb, result.second.yb);
  SplitImpl(zb, result.first.zb, result.second.zb);

  return result;
}

float Spline3D::LengthEstimate(float &error) const {
  // Our convex hull is p0, p1, p2, p3, so p0_p3 is our minimum possible length, and p0_p1 + p1_p2 + p2_p3 our maximum.
  float d03 = sqr(xb.x - xb.w) + sqr(yb.x - yb.w) + sqr(zb.x - zb.w);

  float d01 = sqr(xb.x - xb.y) + sqr(yb.x - yb.y) + sqr(zb.x - zb.y);
  float d12 = sqr(xb.y - xb.z) + sqr(yb.y - yb.z) + sqr(zb.y - zb.z);
  float d23 = sqr(xb.z - xb.w) + sqr(yb.z - yb.w) + sqr(zb.z - zb.w);

  float minLength = sqrtf(d03);
  float maxLength = sqrtf(d01) + sqrtf(d12) + sqrtf(d23);

  minLength *= 0.5f;
  maxLength *= 0.5f;

  error = maxLength - minLength;
  return minLength + maxLength;
}

// NOLINTNEXTLINE(misc-no-recursion)
float Spline3D::Length(float maxError) const {
  float error;
  float length = LengthEstimate(error);

  if (error > maxError) {
    std::pair<Spline3D, Spline3D> split = Split();
    return split.first.Length(maxError) + split.second.Length(maxError);
  }

  return length;
}

float Spline3D::Length(float t0, float t1, float maxError) const {
  assert(t0 >= 0.0f && t0 < 1.0f);
  assert(t1 >= 0.0f && t1 <= 1.0f);
  assert(t0 <= t1);

  if (t0 == 0.0f) {
    if (t1 == 1.0f)
      return Length(maxError);

    std::pair<Spline3D, Spline3D> split = Split(t1);
    return split.first.Length(maxError);
  }

  std::pair<Spline3D, Spline3D> split = Split(t0);

  if (t1 == 1.0f)
    return split.second.Length(maxError);

  std::pair<Spline3D, Spline3D> split2 = split.second.Split((t1 - t0) / (1.0f - t0));
  return split2.first.Length(maxError);
}

} // namespace splines