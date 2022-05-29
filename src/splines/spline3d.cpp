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

static std::vector<V3f> getRotatedShapeForNextPoint(const V3f &position,
                                                    const V3f &normal,
                                                    const std::vector<V3f> &shape,
                                                    float epsilon = 1e-6) {
  // Obtain the rotation matrix that rotates prevNormal into normal
  // Rotation matrix calculation algorithm from https://math.stackexchange.com/a/476311
  V3f a = { 0, 0, 1 };
  V3f b = normal.Normalize();

  // static_cast to const is to silence ISO C++20 warning that considers == commutable
  if (static_cast<const V3f>(a) == b.Scale(-1)) {
    // This causes `c` to be -1, therefore cause a zero division error. Let's tilt this vector slightly and make
    // mathematicians angry
    a = a.Add({ epsilon, epsilon, epsilon }).Normalize();
  }

  V3f v = a.CrossProduct(b);
  float c = a.DotProduct(b);

  Matrix3x3 rotationMatrix = Matrix3x3::identity() + Matrix3x3::skewSymmetricCrossProductMatrix(v) +
                             Matrix3x3::skewSymmetricCrossProductMatrix(v).power(2) * (1.0f / (1.0f + c));

  std::vector<V3f> result;
  result.reserve(shape.size());

  for (const V3f &vector : shape)
    result.push_back((rotationMatrix * vector).Add(position));

  return result;
}

std::vector<V3f> rotateShapeAroundZAxis(const std::vector<V3f> &shape, float angleInRadians) {
  // clang-format off
  Matrix3x3 rotationMatrix(
    cosf(angleInRadians), -sinf(angleInRadians), 1 - cosf(angleInRadians),
    sinf(angleInRadians), cosf(angleInRadians), 1 - cosf(angleInRadians),
    1 - cosf(angleInRadians), 1 - cosf(angleInRadians), 1
  );
  // clang-format on

  std::vector<V3f> result;
  result.reserve(shape.size());

  for (const V3f &vector : shape)
    result.push_back(rotationMatrix * vector);

  return result;
}

static std::vector<V3f> translateShape(const std::vector<V3f> &shape, V3f offset) {
  std::vector<V3f> result;
  result.reserve(shape.size());

  for (const V3f &vector : shape)
    result.push_back(vector + offset);

  return result;
}

raylib::Mesh createRibbonMesh(const std::vector<V3f> &sliceShape,
                              const std::vector<Spline3D> &splines,
                              size_t splineDivisions,
                              float textureScale) {

  size_t maxNumberOfSlices = splines.size() * splineDivisions + 1;

  // TODO: reduce number of copies

  // Generate vertices and texture coordinates

  const Spline3D &firstSpline = splines.front();
  const Spline3D &lastSpline = splines.back();

  // First and last slices always face the player
  V3f firstTangent = { 0, 0, 1 };
  V3f lastTangent = firstTangent;

  // First point should always be the origin
  float epsilon = 1e-6;
  assert(splines.front().Position(0).Length() < epsilon);

  float totalRibbonLength = 0;
  for (const Spline3D &spline : splines)
    totalRibbonLength += spline.Length();

  std::vector<std::vector<V3f>> slices;
  std::vector<V3f> slicePositions;
  std::vector<float> sliceLengthWiseTCoords;

  slices.push_back(sliceShape);
  slicePositions.push_back(firstSpline.Position(0));
  sliceLengthWiseTCoords.push_back(0);

  unsigned int sliceNum = 0;
  float ribbonLengthSoFar = 0;

  for (const Spline3D &spline : splines) {
    bool isLast = &spline == &lastSpline;

    for (size_t i = 1; i <= splineDivisions; i++) {
      sliceNum++;
      float t = 1.0f / static_cast<float>(splineDivisions) * static_cast<float>(i);
      V3f tangent = isLast ? V3f(0, 0, 1) : spline.Velocity(t);

      // Avoid adding a slice if the last two tangents, normalized (=> 1m long) are less than 0.5cm apart
      if (!isLast && lastTangent.Normalize().Subtract(tangent.Normalize()).Length() < 0.005)
        continue;

      slices.push_back(getRotatedShapeForNextPoint(spline.Position(t), tangent, sliceShape));
      slicePositions.push_back(spline.Position(t));

      float ribbonLengthAtT = ribbonLengthSoFar + spline.Length(0.0f, t);
      sliceLengthWiseTCoords.push_back(ribbonLengthAtT / totalRibbonLength);
    }

    ribbonLengthSoFar += spline.Length();
  }

  slices.back() = translateShape(sliceShape, slicePositions.back());

  size_t numberOfSlices = slices.size();
  size_t numberOfVertices = 2 + sliceShape.size() * numberOfSlices;
  size_t numberOfTriangles = 2 * (sliceShape.size() - 1) // ends
                             + (numberOfSlices - 1) * 2 * (sliceShape.size() - 1);

  float verticesArr[numberOfVertices * 3];
  float normalsArr[numberOfVertices * 3];
  float tcoordsArr[numberOfVertices * 2];
  uint16_t trianglesArr[numberOfTriangles * 3];

  float *points = verticesArr;
  float *normals = normalsArr;
  float *tcoords = tcoordsArr;

  sliceNum = 0;
  for (const std::vector<V3f> &slice : slices) {
    float vertexNum = 0;

    for (const V3f &vertex : slice) {
      V3f normal = (vertex - slicePositions.at(sliceNum)).Normalize();

      *points++ = vertex.x;
      *points++ = vertex.y;
      *points++ = vertex.z;

      *normals++ = normal.x;
      *normals++ = normal.y;
      *normals++ = normal.z;

      *tcoords++ = textureScale * sliceLengthWiseTCoords.at(sliceNum);
      *tcoords++ = vertexNum / static_cast<float>((sliceShape.size() - 1));

      vertexNum++;
    }
    sliceNum++;
  }

  // Start/end shape center points, to close off the face
  const V3f &start = splines.front().Position(0);
  *points++ = start.x;
  *points++ = start.y;
  *points++ = start.z;

  const V3f startNormal = splines.front().Velocity(0).Scale(-1).Normalize();
  *normals++ = startNormal.x;
  *normals++ = startNormal.y;
  *normals++ = startNormal.z;

  *tcoords++ = 0;
  *tcoords++ = 0.5f;

  const V3f &end = splines.back().Position(1);
  *points++ = end.x;
  *points++ = end.y;
  *points++ = end.z;

  const V3f endNormal = splines.back().Velocity(1).Normalize();
  *normals++ = endNormal.x;
  *normals++ = endNormal.y;
  *normals++ = endNormal.z;

  *tcoords++ = textureScale;
  *tcoords++ = 0.5f;

  // Generate faces
  uint16_t *triangle = trianglesArr;
  size_t sliceStride = sliceShape.size();

  // Connect each slice with the following
  for (size_t slice = 0; slice < (numberOfSlices - 1) * sliceStride; slice += sliceStride) {
    for (size_t vertex = 0; vertex < (sliceShape.size() - 1); vertex++) {
      *triangle++ = slice + vertex;
      *triangle++ = slice + vertex + 1;
      *triangle++ = slice + vertex + sliceStride + 1;

      *triangle++ = slice + vertex;
      *triangle++ = slice + vertex + sliceStride + 1;
      *triangle++ = slice + vertex + sliceStride;
    }
  }

  // Connect start/end points to start/end slices
  size_t startPoint = numberOfVertices - 2;
  size_t endPoint = numberOfVertices - 1;

  for (size_t vertex = 0; vertex < (sliceShape.size() - 1); vertex++) {
    *triangle++ = vertex;
    *triangle++ = startPoint;
    *triangle++ = vertex + 1;
  }

  size_t lastSlice = (numberOfSlices - 1) * sliceStride;
  for (size_t vertex = 0; vertex < (sliceShape.size() - 1); vertex++) {
    *triangle++ = lastSlice + vertex + 1;
    *triangle++ = endPoint;
    *triangle++ = lastSlice + vertex;
  }

  // Ensure we reached the end of the allocated arrays
  assert(points == verticesArr + (sizeof(verticesArr) / sizeof(float)));
  assert(normals == normalsArr + (sizeof(normalsArr) / sizeof(float)));
  assert(tcoords == tcoordsArr + (sizeof(tcoordsArr) / sizeof(float)));
  assert(triangle == trianglesArr + (sizeof(trianglesArr) / sizeof(uint16_t)));

  // Now create a raylib mesh from the points we calculated
  // Raylib annoyingly requires to duplicate vertices for the triangles
  raylib::Mesh mesh(static_cast<int>(numberOfTriangles) * 3, static_cast<int>(numberOfTriangles));

  mesh.vertices = (float *) RL_MALLOC(numberOfTriangles * 3 * 3 * sizeof(float));
  mesh.texcoords = (float *) RL_MALLOC(numberOfTriangles * 3 * 2 * sizeof(float));
  mesh.normals = (float *) RL_MALLOC(numberOfTriangles * 3 * 3 * sizeof(float));

  for (int k = 0; k < mesh.vertexCount; k++) {
    mesh.vertices[k * 3] = verticesArr[trianglesArr[k] * 3];
    mesh.vertices[k * 3 + 1] = verticesArr[trianglesArr[k] * 3 + 1];
    mesh.vertices[k * 3 + 2] = verticesArr[trianglesArr[k] * 3 + 2];

    mesh.normals[k * 3] = normalsArr[trianglesArr[k] * 3];
    mesh.normals[k * 3 + 1] = normalsArr[trianglesArr[k] * 3 + 1];
    mesh.normals[k * 3 + 2] = normalsArr[trianglesArr[k] * 3 + 2];

    mesh.texcoords[k * 2] = tcoordsArr[trianglesArr[k] * 2];
    mesh.texcoords[k * 2 + 1] = tcoordsArr[trianglesArr[k] * 2 + 1];
  }

  mesh.Upload();
  return mesh;
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