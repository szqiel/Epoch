#include "MathUtils.h"
#include "GameConfig.h"
#include <cmath>
#include <cstdlib>

Vector3 MakeVector(float X, float Y, float Z) {
  Vector3 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  return Result;
}

Vector3 AddVector(Vector3 A, Vector3 B) {
  return MakeVector(A.X + B.X, A.Y + B.Y, A.Z + B.Z);
}

Vector3 SubtractVector(Vector3 A, Vector3 B) {
  return MakeVector(A.X - B.X, A.Y - B.Y, A.Z - B.Z);
}

Vector3 ScaleVector(Vector3 A, float S) {
  return MakeVector(A.X * S, A.Y * S, A.Z * S);
}

float DotVector(Vector3 A, Vector3 B) {
  return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

Vector3 CrossVector(Vector3 A, Vector3 B) {
  return MakeVector(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z,
                    A.X * B.Y - A.Y * B.X);
}

float LengthVector(Vector3 A) { return (float)sqrt(DotVector(A, A)); }

Vector3 NormalizeVector(Vector3 A) {
  float L = LengthVector(A);
  if (L < 0.00001f) {
    return MakeVector(0.0f, 1.0f, 0.0f);
  }
  return ScaleVector(A, 1.0f / L);
}

float ClampValue(float Value, float Minimum, float Maximum) {
  if (Value < Minimum) {
    return Minimum;
  }
  if (Value > Maximum) {
    return Maximum;
  }
  return Value;
}

float RandomUnit() { return (float)rand() / (float)RAND_MAX; }

float HashVector(Vector3 A) {
  float S =
      (float)sin(A.X * 12.9898f + A.Y * 78.233f + A.Z * 37.719f) * 43758.5453f;
  return S - (float)floor(S);
}

float QuantizeValue(float Value) {
  return (float)floor(Value * 10000.0f + 0.5f) / 10000.0f;
}

Vector3 QuantizeVector(Vector3 A) {
  return MakeVector(QuantizeValue(A.X), QuantizeValue(A.Y), QuantizeValue(A.Z));
}

float VertexNoise(Vector3 A) {
  A = QuantizeVector(A);
  float N1 = HashVector(A);
  float N2 = HashVector(MakeVector(A.Z + 1.7f, A.X - 2.3f, A.Y + 0.9f));
  float N3 =
      HashVector(MakeVector(A.Y * 2.1f - 0.4f, A.Z * 1.6f + 0.8f, A.X * 1.8f));
  return N1 * 0.52f + N2 * 0.31f + N3 * 0.17f;
}
