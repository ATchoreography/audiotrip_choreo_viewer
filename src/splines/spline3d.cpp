//
// Created by depau on 5/17/22.
//

#include <cassert>
#include <optional>

#include "fmt/format.h"
#include "splines/spline3d.h"

namespace splines {

// Algorithm from: https://stackoverflow.com/a/52711312/1124621
static std::optional<V3f> linePlaneIntersection(V3f planePoint, V3f planeNormal, V3f linePoint, V3f lineDirection) {
  lineDirection = lineDirection.Normalize();
  if (planeNormal.DotProduct(lineDirection) == 0) {
    return std::nullopt;
  }

  float t = (planeNormal.DotProduct(planePoint) - planeNormal.DotProduct(linePoint)) /
            planeNormal.DotProduct(lineDirection);
  return linePoint.Add(lineDirection.Scale(t));
}

// Credits for the math: https://math.stackexchange.com/a/3130840
static std::vector<V3f> circleNormalToVector(const V3f &position,
                                             const V3f &normal,
                                             float radius,
                                             size_t radialPoints,
                                             std::optional<V3f> lastCirclePoint = std::nullopt,
                                             std::optional<V3f> lastTangent = std::nullopt) {

  std::optional<V3f> perpendicular;

  if (lastCirclePoint.has_value() && lastTangent.has_value()) {
    V3f linePoint = lastCirclePoint->Subtract(position);
    V3f lineDirection = normal.Normalize().Add(lastTangent->Normalize());
    perpendicular = linePlaneIntersection({ 0, 0, 0 }, normal, linePoint, lineDirection);
    if (perpendicular.has_value()) {
      float epsilon = 1e-6;
      perpendicular->Scale(-1);
      assert(perpendicular->DotProduct(normal) < epsilon);
    }
  }

  if (!perpendicular.has_value())
    perpendicular = normal.Perpendicular();

  V3f x = perpendicular->Normalize().Scale(radius);
  V3f z = (x.CrossProduct(normal) / normal.Length()).Normalize().Scale(radius);

  std::vector<V3f> result;
  result.reserve(radialPoints + 1);

  assert(radialPoints % 2 == 0);

  // TODO: this can probably be optimized further
  // Generate half of the points
  for (float i = 0; i < PI; i += (2 * PI) / static_cast<float>(radialPoints)) // NOLINT(cert-flp30-c)
    result.push_back((x * cosf(i)).Add(z * sinf(i)));

  assert(result.size() == radialPoints / 2);

  // Add the rest of the points by negating the first half
  for (size_t i = 0; i < radialPoints / 2; i++)
    result.push_back(result[i].Scale(-1));

  // Move the circle at the specified position
  if (position != V3f(0, 0, 0)) {
    for (V3f &v : result)
      v += position;
  }

  // Put back the initial vector at the end
  result.push_back(result.front());

  assert(result.size() == (radialPoints + 1));

  return result;
}

static std::vector<V3f> getRotatedCircleForNextPoint(const V3f &position,
                                                     const V3f &normal,
                                                     const std::vector<V3f> &prevCircle,
                                                     V3f prevPosition,
                                                     V3f prevNormal,
                                                     float radius,
                                                     float epsilon = 1e-6) {
  // Obtain the rotation matrix that rotates prevNormal into normal
  // Rotation matrix calculation algorithm from https://math.stackexchange.com/a/476311
  V3f a = prevNormal.Normalize();
  V3f b = normal.Normalize();

  // static_cast to const is to silence ISO C++20 warning that considers == commutable
  if (static_cast<const V3f>(a) == b.Scale(-1)) {
    // This causes `c` to be -1, therefore cause a zero division error. Let's tilt this vector slightly and make
    // mathematicians angry
    prevNormal = prevNormal.Add({ epsilon, epsilon, epsilon }).Normalize();
  }

  V3f v = a.CrossProduct(b);
  float s = v.Length(); // sine of angle
  float c = a.DotProduct(b);

  Matrix3x3 rotationMatrix = Matrix3x3::identity() + Matrix3x3::skewSymmetricCrossProductMatrix(v) +
                             Matrix3x3::skewSymmetricCrossProductMatrix(v).power(2) * (1.0f / (1.0f + c));

  std::vector<V3f> result;
  result.reserve(prevCircle.size());

  for (const V3f &vector : prevCircle) {
    result.push_back((rotationMatrix * (vector - prevPosition).Normalize()).Scale(radius).Add(position));
  }

  return result;
}

raylib::Mesh
createSnakeMesh(const std::vector<Spline3D> &splines, float radius, size_t radialPoints, size_t splineDivisions) {
  size_t numberOfCircles = splines.size() * splineDivisions + 1;
  size_t numberOfVertices = 2 + (radialPoints + 1) * (splineDivisions * splines.size() + 1);
  size_t numberOfTriangles = 2 * radialPoints // ends
                             + (numberOfCircles - 1) * 2 * radialPoints;

  // TODO: reduce number of copies

  // Generate vertices and texture coordinates
  std::vector<std::vector<V3f>> circles;
  std::vector<V3f> circlePositions;

  const Spline3D &firstSpline = splines.front();
  const Spline3D &lastSpline = splines.back();

  // First and last circles always face the player
  V3f lastTangent = { 0, 0, 1 };

  circles.push_back(circleNormalToVector(firstSpline.Position(0), lastTangent, radius, radialPoints));
  circlePositions.push_back(firstSpline.Position(0));

  for (const Spline3D &spline : splines) {
    for (size_t i = 1; i <= splineDivisions; i++) {
      float t = 1.0f / static_cast<float>(splineDivisions) * static_cast<float>(i);
      V3f tangent = &spline == &lastSpline ? V3f(0, 0, 1) : spline.PoorMansTangent(t);

      circles.push_back(getRotatedCircleForNextPoint(spline.Position(t),
                                                     tangent,
                                                     circles.back(),
                                                     circlePositions.back(),
                                                     lastTangent,
                                                     radius));
      circlePositions.push_back(spline.Position(t));
      lastTangent = tangent;
    }
  }

  assert(circles.size() == numberOfCircles);

  float verticesArr[numberOfVertices * 3];
  float normalsArr[numberOfVertices * 3];
  float tcoordsArr[numberOfVertices * 2];
  uint16_t trianglesArr[numberOfTriangles * 3];

  float *points = verticesArr;
  float *normals = normalsArr;
  float *tcoords = tcoordsArr;

  float circleNum = 0;
  for (const std::vector<V3f> &circle : circles) {
    float vertexNum = 0;

    for (const V3f &vertex : circle) {
      V3f normal = (vertex - circlePositions.at(static_cast<int>(circleNum))).Normalize();

      *points++ = vertex.x;
      *points++ = vertex.y;
      *points++ = vertex.z;

      *normals++ = normal.x;
      *normals++ = normal.y;
      *normals++ = normal.z;

      *tcoords++ = circleNum / static_cast<float>(circles.size() - 1);
      *tcoords++ = vertexNum / static_cast<float>(radialPoints);

      vertexNum++;
    }
    circleNum++;
  }

  // Start/end circle center points, to close off the face
  const V3f &start = splines.front().Position(0);
  *points++ = start.x;
  *points++ = start.y;
  *points++ = start.z;

  const V3f startNormal = splines.front().PoorMansTangent(0).Scale(-1).Normalize();
  *normals++ = startNormal.x;
  *normals++ = startNormal.y;
  *normals++ = startNormal.z;

  *tcoords++ = 0;
  *tcoords++ = 0.5;

  const V3f &end = splines.back().Position(1);
  *points++ = end.x;
  *points++ = end.y;
  *points++ = end.z;

  const V3f endNormal = splines.back().PoorMansTangent(1).Normalize();
  *normals++ = endNormal.x;
  *normals++ = endNormal.y;
  *normals++ = endNormal.z;

  *tcoords++ = 1;
  *tcoords++ = 0.5;

  // Generate faces
  uint16_t *triangle = trianglesArr;
  size_t circleStride = radialPoints + 1;

  // Connect each circle with the following
  for (size_t circle = 0; circle < (numberOfCircles - 1) * circleStride; circle += circleStride) {
    for (size_t vertex = 0; vertex < radialPoints; vertex++) {
      *triangle++ = circle + vertex;
      *triangle++ = circle + vertex + 1;
      *triangle++ = circle + vertex + circleStride + 1;

      *triangle++ = circle + vertex;
      *triangle++ = circle + vertex + circleStride + 1;
      *triangle++ = circle + vertex + circleStride;
    }
  }

  // Connect start/end points to start/end circles
  size_t startPoint = numberOfVertices - 2;
  size_t endPoint = numberOfVertices - 1;

  for (size_t vertex = 0; vertex < radialPoints; vertex++) {
    *triangle++ = vertex;
    *triangle++ = startPoint;
    *triangle++ = vertex + 1;
  }

  size_t lastCircle = (numberOfCircles - 1) * circleStride;
  for (size_t vertex = 0; vertex < radialPoints; vertex++) {
    *triangle++ = lastCircle + vertex + 1;
    *triangle++ = endPoint;
    *triangle++ = lastCircle + vertex;
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