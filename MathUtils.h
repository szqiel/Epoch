#ifndef MathUtilsH
#define MathUtilsH

#include "GameTypes.h"

Vector3 MakeVector(float X, float Y, float Z);
Vector3 AddVector(Vector3 A, Vector3 B);
Vector3 SubtractVector(Vector3 A, Vector3 B);
Vector3 ScaleVector(Vector3 A, float S);
float DotVector(Vector3 A, Vector3 B);
Vector3 CrossVector(Vector3 A, Vector3 B);
float LengthVector(Vector3 A);
Vector3 NormalizeVector(Vector3 A);
float ClampValue(float Value, float Minimum, float Maximum);
float RandomUnit();
float HashVector(Vector3 A);
float QuantizeValue(float Value);
Vector3 QuantizeVector(Vector3 A);
float VertexNoise(Vector3 A);

#endif
