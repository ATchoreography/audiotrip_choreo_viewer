//
// Created by depau on 5/17/22.
//

#include <cassert>
#include <optional>

#include "fmt/format.h"
#include "splines/spline3d.h"

namespace splines {

static std::vector<V3f> getRotatedShapeForNextPoint(const V3f &position,
                                                    const V3f &normal,
                                                    const std::vector<V3f> &prevShape,
                                                    V3f prevPosition,
                                                    V3f prevNormal,
                                                    float epsilon = 1e-6) {
  // Obtain the rotation matrix that rotates prevNormal into normal
  // Rotation matrix calculation algorithm from https://math.stackexchange.com/a/476311
  V3f a = prevNormal.Normalize();
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
  result.reserve(prevShape.size());

  for (const V3f &vector : prevShape)
    result.push_back((rotationMatrix * (vector - prevPosition)).Add(position));

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
  V3f lastTangent = { 0, 0, 1 };

  // First point should always be the origin
  float epsilon = 1e-6;
  assert(splines.front().Position(0).Length() < epsilon);

  std::vector<std::vector<V3f>> slices;
  std::vector<V3f> slicePositions;
  std::vector<float> sliceLengthWiseTCoords;

  slices.push_back(sliceShape);
  slicePositions.push_back(firstSpline.Position(0));
  sliceLengthWiseTCoords.push_back(0);

  unsigned int sliceNum = 0;
  for (const Spline3D &spline : splines) {
    bool isLast = &spline == &lastSpline;

    for (size_t i = 1; i <= splineDivisions; i++) {
      sliceNum++;
      float t = 1.0f / static_cast<float>(splineDivisions) * static_cast<float>(i);
      V3f tangent = isLast ? V3f(0, 0, 1)
                           : spline.Velocity(t);

      // Avoid adding a slice if the last two tangents, normalized (=> 1m long) are less than 1cm apart
      if (!isLast && lastTangent.Normalize().Subtract(tangent.Normalize()).Length() < 0.00001)
        continue;

      slices.push_back(
        getRotatedShapeForNextPoint(spline.Position(t), tangent, slices.back(), slicePositions.back(), lastTangent));
      slicePositions.push_back(spline.Position(t));
      sliceLengthWiseTCoords.push_back((static_cast<float>(sliceNum - 1) + t) / static_cast<float>(maxNumberOfSlices));
      lastTangent = tangent;
    }
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
} // namespace splines