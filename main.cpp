#include "GameConfig.h"
#include "GameTypes.h"
#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

std::vector<Face> PlanetFaces;
std::vector<Particle> Particles;
std::vector<Meteor> Meteors;

int WindowWidth = 1280;
int WindowHeight = 720;
int SelectedTool = ToolTree;
int DisasterKind = DisasterNone;
int DisasterTargetFace = -1;

float CameraYaw = 35.0f;
float CameraPitch = 18.0f;
float CameraDistance = 7.2f;
float ElapsedSeconds = 0.0f;
float LastTickSeconds = 0.0f;
float NextDisasterSeconds = FirstDisasterTime;
float DisasterCountdown = 0.0f;
float AlertSeconds = 0.0f;
float TemperatureAxis = 0.46f;
float AtmosphereAxis = 0.42f;
float HeatPressure = 0.0f;
float AtmospherePressure = 0.0f;
float CloudRotation = 0.0f;
float TreePulse = 1.0f;

bool MouseIsDown = false;
bool MouseIsDragging = false;
bool GameOver = false;
bool GameWon = false;
bool GamePaused = false;

int LastMouseX = 0;
int LastMouseY = 0;
int StartMouseX = 0;
int StartMouseY = 0;

GLuint TreeList = 0;
GLuint CloudList = 0;
GLuint MeteorList = 0;
GLuint UiCircleList = 0;
GLuint VolcanoList = 0;
GLuint MountainList = 0;
GLuint IceClusterList = 0;

GLdouble PickModelMatrix[16];
GLdouble PickProjectionMatrix[16];
GLint PickViewport[4];

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

float BiomeHeight(int Biome) {
  if (Biome == BiomeOcean) {
    return -0.15f;
  }
  if (Biome == BiomeMountain) {
    return 0.19f;
  }
  if (Biome == BiomeIce) {
    return 0.08f;
  }
  if (Biome == BiomeForest) {
    return 0.05f;
  }
  if (Biome == BiomeMagma) {
    return 0.02f;
  }
  if (Biome == BiomeScorched) {
    return 0.01f;
  }
  return 0.04f;
}

int MatureBiomeForFace(Face &F, float Progress) {
  Vector3 Center = NormalizeVector(ScaleVector(
      AddVector(AddVector(F.BaseA, F.BaseB), F.BaseC), 1.0f / 3.0f));
  float OceanField = ((float)sin(Center.X * 1.20f + Center.Z * 0.85f + 0.60f) +
                      (float)cos(Center.Y * 1.15f - Center.X * 1.05f) +
                      (float)sin((Center.X + Center.Y - Center.Z) * 0.95f)) /
                     3.0f;
  float RidgeField = (float)fabs(
      (float)sin(Center.X * 2.20f + Center.Y * 1.10f - Center.Z * 1.80f));
  float Polar = (float)fabs(Center.Y);
  if (Progress > 0.76f && Polar > 0.78f) {
    return BiomeIce;
  }
  if (OceanField < -0.10f) {
    return BiomeOcean;
  }
  if (OceanField > 0.30f && RidgeField > 0.76f) {
    return BiomeMountain;
  }
  if (Progress > 0.52f && OceanField > 0.06f && F.Seed > 0.35f &&
      F.Seed < 0.82f) {
    return BiomeForest;
  }
  return BiomeLand;
}

void ApplyMatureBiome(Face &F, int Target) {
  F.Biome = Target;
  if (Target == BiomeOcean) {
    F.Height = -0.11f + F.Seed * 0.02f;
    F.TreeCount = 0;
  } else if (Target == BiomeLand) {
    F.Height = 0.025f + F.Seed * 0.022f;
    F.TreeCount = 0;
  } else if (Target == BiomeForest) {
    F.Height = 0.035f + F.Seed * 0.018f;
    F.TreeCount = 0;
  } else if (Target == BiomeMountain) {
    F.Height = 0.070f + F.Seed * 0.038f;
    F.TreeCount = 0;
  } else if (Target == BiomeIce) {
    F.Height = 0.050f + F.Seed * 0.020f;
    F.TreeCount = 0;
  }
}

void SetMaterial(float R, float G, float B, bool Shiny, bool Glow) {
  GLfloat Ambient[4];
  GLfloat Diffuse[4];
  GLfloat Specular[4];
  GLfloat Emission[4];
  Ambient[0] = Glow ? R * 0.85f : R * 0.64f;
  Ambient[1] = Glow ? G * 0.60f : G * 0.64f;
  Ambient[2] = Glow ? B * 0.42f : B * 0.64f;
  Ambient[3] = 1.0f;
  Diffuse[0] = R;
  Diffuse[1] = G;
  Diffuse[2] = B;
  Diffuse[3] = 1.0f;
  Specular[0] = Shiny ? 1.0f : 0.0f;
  Specular[1] = Shiny ? 1.0f : 0.0f;
  Specular[2] = Shiny ? 1.0f : 0.0f;
  Specular[3] = 1.0f;
  Emission[0] = Glow ? R * 0.28f : 0.0f;
  Emission[1] = Glow ? G * 0.10f : 0.0f;
  Emission[2] = Glow ? B * 0.05f : 0.0f;
  Emission[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Emission);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Shiny ? 85.0f : 5.0f);
}

void SetBiomeMaterial(int Biome) {
  if (Biome == BiomeOcean) {
    SetMaterial(0.11f, 0.20f, 0.34f, true, false);
  } else if (Biome == BiomeLand) {
    SetMaterial(0.61f, 0.40f, 0.26f, false, false);
  } else if (Biome == BiomeMountain) {
    SetMaterial(0.38f, 0.39f, 0.45f, false, false);
  } else if (Biome == BiomeIce) {
    SetMaterial(0.94f, 0.98f, 0.93f, true, false);
  } else if (Biome == BiomeForest) {
    SetMaterial(0.15f, 0.21f, 0.09f, false, false);
  } else if (Biome == BiomeMagma) {
    SetMaterial(0.88f, 0.34f, 0.13f, false, true);
  } else {
    SetMaterial(0.24f, 0.24f, 0.22f, false, false);
  }
}

void AddSubdividedFace(std::vector<Face> &Faces, Vector3 A, Vector3 B,
                       Vector3 C, int Depth) {
  if (Depth <= 0) {
    Face NewFace;
    Vector3 Center = NormalizeVector(
        ScaleVector(AddVector(AddVector(A, B), C), 1.0f / 3.0f));
    float Noise = HashVector(Center);
    NewFace.BaseA = NormalizeVector(A);
    NewFace.BaseB = NormalizeVector(B);
    NewFace.BaseC = NormalizeVector(C);
    NewFace.Height = (Noise - 0.5f) * 0.03f;
    NewFace.Phase = Noise * Pi * 12.0f;
    NewFace.Seed = Noise;
    NewFace.TreeCount = 0;
    NewFace.Locked = false;
    float PlateField =
        ((float)sin(Center.X * 1.35f + Center.Z * 1.05f + 0.7f) +
         (float)cos(Center.Y * 1.55f - Center.X * 1.20f) +
         (float)sin((Center.X - Center.Y + Center.Z) * 1.10f - 0.4f)) /
        3.0f;
    float RidgeField = (float)fabs(
        (float)sin(Center.X * 2.10f + Center.Y * 1.25f - Center.Z * 1.70f));
    float Score = PlateField + (Noise - 0.5f) * 0.10f;
    if (Score > 0.30f && RidgeField > 0.80f) {
      NewFace.Biome = BiomeMountain;
      NewFace.Height = 0.06f + Noise * 0.035f;
    } else if (Score > -0.08f) {
      NewFace.Biome = BiomeScorched;
      NewFace.Height = 0.015f + Noise * 0.02f;
    } else {
      NewFace.Biome = BiomeMagma;
      NewFace.Height = -0.06f + Noise * 0.018f;
    }
    Faces.push_back(NewFace);
    return;
  }
  Vector3 AB = NormalizeVector(ScaleVector(AddVector(A, B), 0.5f));
  Vector3 BC = NormalizeVector(ScaleVector(AddVector(B, C), 0.5f));
  Vector3 CA = NormalizeVector(ScaleVector(AddVector(C, A), 0.5f));
  AddSubdividedFace(Faces, A, AB, CA, Depth - 1);
  AddSubdividedFace(Faces, B, BC, AB, Depth - 1);
  AddSubdividedFace(Faces, C, CA, BC, Depth - 1);
  AddSubdividedFace(Faces, AB, BC, CA, Depth - 1);
}

void GeneratePlanet() {
  PlanetFaces.clear();
  float T = (1.0f + (float)sqrt(5.0f)) * 0.5f;
  Vector3 V[12];
  V[0] = NormalizeVector(MakeVector(-1.0f, T, 0.0f));
  V[1] = NormalizeVector(MakeVector(1.0f, T, 0.0f));
  V[2] = NormalizeVector(MakeVector(-1.0f, -T, 0.0f));
  V[3] = NormalizeVector(MakeVector(1.0f, -T, 0.0f));
  V[4] = NormalizeVector(MakeVector(0.0f, -1.0f, T));
  V[5] = NormalizeVector(MakeVector(0.0f, 1.0f, T));
  V[6] = NormalizeVector(MakeVector(0.0f, -1.0f, -T));
  V[7] = NormalizeVector(MakeVector(0.0f, 1.0f, -T));
  V[8] = NormalizeVector(MakeVector(T, 0.0f, -1.0f));
  V[9] = NormalizeVector(MakeVector(T, 0.0f, 1.0f));
  V[10] = NormalizeVector(MakeVector(-T, 0.0f, -1.0f));
  V[11] = NormalizeVector(MakeVector(-T, 0.0f, 1.0f));
  int I[20][3] = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                  {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                  {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                  {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
  for (int N = 0; N < 20; N++) {
    AddSubdividedFace(PlanetFaces, V[I[N][0]], V[I[N][1]], V[I[N][2]], 3);
  }
}

Vector3 WarpedVertex(Vector3 Base, Face &F) {
  float Time = ElapsedSeconds;
  float Noise = VertexNoise(Base);
  float Phase = Noise * Pi * 12.0f;
  float Drift = (float)sin(Time * 0.050f + Phase) * 0.018f;
  Vector3 Shift = MakeVector((float)sin(Phase * 1.37f + Base.Z) * Drift,
                             (float)cos(Phase * 0.91f + Base.X) * Drift,
                             (float)sin(Phase * 2.11f + Base.Y) * Drift);
  Vector3 Direction = NormalizeVector(AddVector(Base, Shift));
  float Ridge =
      (float)sin(Base.X * 9.0f + Base.Y * 4.5f - Base.Z * 6.5f) * 0.026f;
  float Pulse = (float)sin(Time * 0.12f + Phase * 0.7f) * 0.008f;
  float Radius = PlanetRadius + (Noise - 0.5f) * 0.15f + Ridge + Pulse;
  return ScaleVector(Direction, Radius);
}

void UpdateFaceGeometry(Face &F) {
  F.CurrentA = WarpedVertex(F.BaseA, F);
  F.CurrentB = WarpedVertex(F.BaseB, F);
  F.CurrentC = WarpedVertex(F.BaseC, F);
  F.Center = ScaleVector(
      AddVector(AddVector(F.CurrentA, F.CurrentB), F.CurrentC), 1.0f / 3.0f);
  F.Normal =
      NormalizeVector(CrossVector(SubtractVector(F.CurrentB, F.CurrentA),
                                  SubtractVector(F.CurrentC, F.CurrentA)));
  if (DotVector(F.Normal, F.Center) < 0.0f) {
    F.Normal = ScaleVector(F.Normal, -1.0f);
  }
}

void UpdateAllGeometry() {
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    UpdateFaceGeometry(PlanetFaces[I]);
  }
}

void DrawLowPolyCone(float Radius, float Height, int Sides) {
  glBegin(GL_TRIANGLES);
  for (int I = 0; I < Sides; I++) {
    float A0 = (float)I / (float)Sides * Pi * 2.0f;
    float A1 = (float)(I + 1) / (float)Sides * Pi * 2.0f;
    Vector3 P0 =
        MakeVector((float)cos(A0) * Radius, 0.0f, (float)sin(A0) * Radius);
    Vector3 P1 =
        MakeVector((float)cos(A1) * Radius, 0.0f, (float)sin(A1) * Radius);
    Vector3 Tip = MakeVector(0.0f, Height, 0.0f);
    Vector3 N = NormalizeVector(
        CrossVector(SubtractVector(P1, P0), SubtractVector(Tip, P0)));
    glNormal3f(N.X, N.Y, N.Z);
    glVertex3f(P0.X, P0.Y, P0.Z);
    glVertex3f(P1.X, P1.Y, P1.Z);
    glVertex3f(Tip.X, Tip.Y, Tip.Z);
  }
  glEnd();
}

void DrawBlock(float Width, float Height, float Depth) {
  float X = Width * 0.5f;
  float Y = Height;
  float Z = Depth * 0.5f;
  glBegin(GL_QUADS);
  glNormal3f(0.0f, 0.0f, 1.0f);
  glVertex3f(-X, 0.0f, Z);
  glVertex3f(X, 0.0f, Z);
  glVertex3f(X, Y, Z);
  glVertex3f(-X, Y, Z);
  glNormal3f(0.0f, 0.0f, -1.0f);
  glVertex3f(X, 0.0f, -Z);
  glVertex3f(-X, 0.0f, -Z);
  glVertex3f(-X, Y, -Z);
  glVertex3f(X, Y, -Z);
  glNormal3f(1.0f, 0.0f, 0.0f);
  glVertex3f(X, 0.0f, Z);
  glVertex3f(X, 0.0f, -Z);
  glVertex3f(X, Y, -Z);
  glVertex3f(X, Y, Z);
  glNormal3f(-1.0f, 0.0f, 0.0f);
  glVertex3f(-X, 0.0f, -Z);
  glVertex3f(-X, 0.0f, Z);
  glVertex3f(-X, Y, Z);
  glVertex3f(-X, Y, -Z);
  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(-X, Y, Z);
  glVertex3f(X, Y, Z);
  glVertex3f(X, Y, -Z);
  glVertex3f(-X, Y, -Z);
  glNormal3f(0.0f, -1.0f, 0.0f);
  glVertex3f(-X, 0.0f, -Z);
  glVertex3f(X, 0.0f, -Z);
  glVertex3f(X, 0.0f, Z);
  glVertex3f(-X, 0.0f, Z);
  glEnd();
}

void BuildDisplayLists() {
  TreeList = glGenLists(1);
  glNewList(TreeList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.25f, 0.13f, 0.06f, false, false);
  DrawBlock(0.075f, 0.28f, 0.075f);
  glTranslatef(0.0f, 0.20f, 0.0f);
  SetMaterial(0.15f, 0.21f, 0.09f, false, false);
  DrawLowPolyCone(0.22f, 0.42f, 5);
  glTranslatef(0.0f, 0.23f, 0.0f);
  DrawLowPolyCone(0.17f, 0.35f, 5);
  glPopMatrix();
  glEndList();

  CloudList = glGenLists(1);
  glNewList(CloudList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.87f, 0.88f, 0.86f, true, false);
  glPushMatrix();
  glScalef(0.34f, 0.16f, 0.22f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPushMatrix();
  glTranslatef(0.24f, 0.03f, 0.05f);
  glScalef(0.24f, 0.13f, 0.16f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.23f, 0.02f, -0.02f);
  glScalef(0.22f, 0.12f, 0.17f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPopMatrix();
  glEndList();

  MeteorList = glGenLists(1);
  glNewList(MeteorList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.29f, 0.30f, 0.41f, false, false);
  glScalef(0.18f, 0.18f, 0.18f);
  glutSolidIcosahedron();
  glPopMatrix();
  glEndList();

  MountainList = glGenLists(1);
  glNewList(MountainList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.38f, 0.39f, 0.45f, false, false);
  DrawLowPolyCone(0.22f, 0.48f, 5);
  glPushMatrix();
  glTranslatef(0.18f, 0.0f, 0.08f);
  glScalef(0.72f, 0.78f, 0.72f);
  DrawLowPolyCone(0.18f, 0.40f, 5);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.18f, 0.0f, -0.06f);
  glScalef(0.62f, 0.70f, 0.62f);
  DrawLowPolyCone(0.16f, 0.34f, 5);
  glPopMatrix();
  glPopMatrix();
  glEndList();

  IceClusterList = glGenLists(1);
  glNewList(IceClusterList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.94f, 0.98f, 0.93f, true, false);
  DrawLowPolyCone(0.13f, 0.38f, 5);
  glPushMatrix();
  glTranslatef(0.13f, 0.0f, 0.08f);
  glScalef(0.70f, 0.82f, 0.70f);
  DrawLowPolyCone(0.12f, 0.32f, 5);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.11f, 0.0f, -0.07f);
  glScalef(0.58f, 0.66f, 0.58f);
  DrawLowPolyCone(0.11f, 0.29f, 5);
  glPopMatrix();
  glPopMatrix();
  glEndList();

  VolcanoList = glGenLists(1);
  glNewList(VolcanoList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.22f, 0.22f, 0.23f, false, false);
  DrawLowPolyCone(0.20f, 0.34f, 6);
  glTranslatef(0.0f, 0.08f, 0.0f);
  SetMaterial(0.88f, 0.34f, 0.13f, false, true);
  DrawLowPolyCone(0.07f, 0.09f, 5);
  glPopMatrix();
  glEndList();

  UiCircleList = glGenLists(1);
  glNewList(UiCircleList, GL_COMPILE);
  for (int R = 0; R < 3; R++) {
    float Radius = 25.0f + (float)R * 25.0f;
    glBegin(GL_LINE_LOOP);
    for (int I = 0; I < 96; I++) {
      float A = (float)I / 96.0f * Pi * 2.0f;
      glVertex2f((float)cos(A) * Radius, (float)sin(A) * Radius);
    }
    glEnd();
  }
  glEndList();
}

void AlignToNormal(Vector3 Normal) {
  Vector3 Up = MakeVector(0.0f, 1.0f, 0.0f);
  float D = ClampValue(DotVector(Up, Normal), -1.0f, 1.0f);
  Vector3 Axis = CrossVector(Up, Normal);
  float L = LengthVector(Axis);
  if (L > 0.0001f) {
    Axis = ScaleVector(Axis, 1.0f / L);
    glRotatef((float)acos(D) * 180.0f / Pi, Axis.X, Axis.Y, Axis.Z);
  } else if (D < 0.0f) {
    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
  }
}

void DrawPlanet() {
  UpdateAllGeometry();
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    SetBiomeMaterial(F.Biome);
    glBegin(GL_TRIANGLES);
    glNormal3f(F.Normal.X, F.Normal.Y, F.Normal.Z);
    glVertex3f(F.CurrentA.X, F.CurrentA.Y, F.CurrentA.Z);
    glVertex3f(F.CurrentB.X, F.CurrentB.Y, F.CurrentB.Z);
    glVertex3f(F.CurrentC.X, F.CurrentC.Y, F.CurrentC.Z);
    glEnd();
  }
}

bool ShouldPlaceSurfaceAsset(Face &F, float Threshold, float Scale) {
  Vector3 P = NormalizeVector(ScaleVector(
      AddVector(AddVector(F.BaseA, F.BaseB), F.BaseC), 1.0f / 3.0f));
  float Placement = HashVector(MakeVector(QuantizeValue(P.X * Scale),
                                          QuantizeValue(P.Y * Scale),
                                          QuantizeValue(P.Z * Scale)));
  return Placement > Threshold;
}

void DrawSurfaceObjects() {
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    Vector3 Tangent =
        NormalizeVector(CrossVector(F.Normal, MakeVector(0.0f, 1.0f, 0.0f)));
    if (LengthVector(Tangent) < 0.01f) {
      Tangent =
          NormalizeVector(CrossVector(F.Normal, MakeVector(1.0f, 0.0f, 0.0f)));
    }
    Vector3 Bitangent = NormalizeVector(CrossVector(F.Normal, Tangent));
    int VisibleTrees = F.TreeCount;
    if (F.Biome == BiomeForest && VisibleTrees < 1 &&
        ShouldPlaceSurfaceAsset(F, 0.92f, 2.8f)) {
      VisibleTrees = 1;
    }
    if (VisibleTrees > 0) {
      for (int T = 0; T < VisibleTrees; T++) {
        float A = F.Seed * Pi * 8.0f + (float)T * 2.09f;
        float R = ((float)T - (float)(VisibleTrees - 1) * 0.5f) * 0.085f;
        Vector3 Offset = AddVector(ScaleVector(Tangent, (float)cos(A) * R),
                                   ScaleVector(Bitangent, (float)sin(A) * R));
        glPushMatrix();
        Vector3 P = AddVector(AddVector(F.Center, Offset),
                              ScaleVector(F.Normal, 0.035f));
        glTranslatef(P.X, P.Y, P.Z);
        AlignToNormal(F.Normal);
        glRotatef(F.Seed * 360.0f + (float)T * 47.0f, 0.0f, 1.0f, 0.0f);
        float S = (0.27f + 0.025f * (float)T) * TreePulse;
        glScalef(S, S, S);
        glCallList(TreeList);
        glPopMatrix();
      }
    }
    if (F.Biome == BiomeMountain && ShouldPlaceSurfaceAsset(F, 0.965f, 3.0f)) {
      glPushMatrix();
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.028f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);
      glRotatef(F.Seed * 360.0f, 0.0f, 1.0f, 0.0f);
      float S = 1.10f + F.Seed * 0.30f;
      glScalef(S, S, S);
      glCallList(MountainList);
      glPopMatrix();
    }
    if (F.Biome == BiomeIce && ShouldPlaceSurfaceAsset(F, 0.90f, 3.5f)) {
      glPushMatrix();
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.03f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);
      glRotatef(F.Seed * 300.0f, 0.0f, 1.0f, 0.0f);
      glScalef(0.46f, 0.46f, 0.46f);
      glCallList(IceClusterList);
      glPopMatrix();
    }
    if ((F.Biome == BiomeMagma || F.Biome == BiomeScorched ||
         F.Biome == BiomeMountain) &&
        ShouldPlaceSurfaceAsset(F, 0.975f, 2.6f)) {
      glPushMatrix();
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.035f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);
      float S = F.Biome == BiomeMagma ? 0.84f : 0.62f;
      glScalef(S, S, S);
      glCallList(VolcanoList);
      glPopMatrix();
    }
  }
}

void DrawClouds() {
  glPushMatrix();
  glRotatef(CloudRotation * 0.35f, 0.0f, 1.0f, 0.0f);
  glRotatef(CloudRotation * 0.05f, 1.0f, 0.0f, 0.0f);
  for (int I = 0; I < 26; I++) {
    float Y = -0.86f + 1.72f * ((float)I + 0.5f) / 26.0f;
    float R = (float)sqrt(std::max(0.0f, 1.0f - Y * Y));
    float A = (float)I * 2.399963f;
    Vector3 N =
        NormalizeVector(MakeVector((float)cos(A) * R, Y, (float)sin(A) * R));
    Vector3 P = ScaleVector(N, PlanetRadius + 0.74f +
                                   (float)sin((float)I * 1.41f) * 0.07f);
    glPushMatrix();
    glTranslatef(P.X, P.Y, P.Z);
    AlignToNormal(N);
    glRotatef((float)I * 37.0f, 0.0f, 1.0f, 0.0f);
    float S = 0.86f +
              0.14f * HashVector(MakeVector((float)I * 0.37f, (float)I * 1.13f,
                                            (float)I * 2.71f));
    glScalef(S, S, S);
    glCallList(CloudList);
    glPopMatrix();
  }
  glPopMatrix();
}

void DrawParticles() {
  glDisable(GL_LIGHTING);
  glLineWidth(2.0f);
  glBegin(GL_LINES);
  for (size_t I = 0; I < Particles.size(); I++) {
    Particle &P = Particles[I];
    if (P.Kind == 0) {
      glColor3f(0.25f, 0.50f, 1.0f);
    } else if (P.Kind == 1) {
      glColor3f(0.94f, 0.98f, 0.93f);
    } else {
      glColor3f(0.95f, 0.22f, 0.06f);
    }
    Vector3 Tail = SubtractVector(P.Position, ScaleVector(P.Velocity, 0.08f));
    glVertex3f(P.Position.X, P.Position.Y, P.Position.Z);
    glVertex3f(Tail.X, Tail.Y, Tail.Z);
  }
  glEnd();
  glEnable(GL_LIGHTING);
}

void DrawMeteors() {
  for (size_t I = 0; I < Meteors.size(); I++) {
    Meteor &M = Meteors[I];
    if (M.Active) {
      glPushMatrix();
      glTranslatef(M.Position.X, M.Position.Y, M.Position.Z);
      glCallList(MeteorList);
      glPopMatrix();
    }
  }
}

void DrawText(float X, float Y, std::string Text, void *Font) {
  glRasterPos2f(X, Y);
  for (size_t I = 0; I < Text.length(); I++) {
    glutBitmapCharacter(Font, Text[I]);
  }
}

std::string FormatFloat(float Value, int Precision) {
  std::ostringstream Stream;
  Stream.setf(std::ios::fixed);
  Stream.precision(Precision);
  Stream << Value;
  return Stream.str();
}

std::string ToolName(int Tool) {
  if (Tool == ToolTree) {
    return "Tree";
  }
  if (Tool == ToolRain) {
    return "Rain";
  }
  if (Tool == ToolIce) {
    return "Ice";
  }
  if (Tool == ToolFire) {
    return "Fire";
  }
  if (Tool == ToolMountain) {
    return "Raise";
  }
  if (Tool == ToolWater) {
    return "River";
  }
  if (Tool == ToolMeteor) {
    return "Meteor";
  }
  return "Remove";
}

void DrawOverlay() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0.0, WindowWidth, 0.0, WindowHeight);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(0.04f, 0.05f, 0.07f, 0.72f);
  glBegin(GL_QUADS);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(188.0f, 0.0f);
  glVertex2f(188.0f, (float)WindowHeight);
  glVertex2f(0.0f, (float)WindowHeight);
  glEnd();

  glColor3f(1.0f, 1.0f, 1.0f);
  DrawText(18.0f, WindowHeight - 32.0f, "Epoch", GLUT_BITMAP_HELVETICA_18);
  DrawText(18.0f, WindowHeight - 56.0f, "Tool: " + ToolName(SelectedTool),
           GLUT_BITMAP_HELVETICA_12);

  for (int I = 0; I < 8; I++) {
    float Y = WindowHeight - 96.0f - (float)I * 48.0f;
    if (SelectedTool == I) {
      glColor3f(0.95f, 0.86f, 0.32f);
    } else {
      glColor3f(0.30f, 0.33f, 0.39f);
    }
    glBegin(GL_QUADS);
    glVertex2f(16.0f, Y - 24.0f);
    glVertex2f(170.0f, Y - 24.0f);
    glVertex2f(170.0f, Y + 12.0f);
    glVertex2f(16.0f, Y + 12.0f);
    glEnd();
    if (I == ToolTree) {
      glColor3f(0.15f, 0.55f, 0.20f);
    } else if (I == ToolRain) {
      glColor3f(0.25f, 0.50f, 1.0f);
    } else if (I == ToolIce) {
      glColor3f(0.82f, 0.96f, 1.0f);
    } else if (I == ToolFire) {
      glColor3f(1.0f, 0.25f, 0.07f);
    } else if (I == ToolMountain) {
      glColor3f(0.52f, 0.52f, 0.57f);
    } else if (I == ToolWater) {
      glColor3f(0.15f, 0.33f, 0.88f);
    } else if (I == ToolMeteor) {
      glColor3f(0.70f, 0.48f, 0.32f);
    } else {
      glColor3f(0.92f, 0.92f, 0.92f);
    }
    glBegin(GL_QUADS);
    glVertex2f(25.0f, Y - 15.0f);
    glVertex2f(47.0f, Y - 15.0f);
    glVertex2f(47.0f, Y + 7.0f);
    glVertex2f(25.0f, Y + 7.0f);
    glEnd();
    glColor3f(0.02f, 0.03f, 0.04f);
    DrawText(31.0f, Y - 3.0f, FormatFloat((float)(I + 1), 0),
             GLUT_BITMAP_HELVETICA_12);
    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(58.0f, Y - 2.0f, ToolName(I), GLUT_BITMAP_HELVETICA_12);
  }

  float Remaining = std::max(0.0f, GameDuration - ElapsedSeconds);
  float Age = ElapsedSeconds / GameDuration * 5.0f;
  glColor3f(1.0f, 1.0f, 1.0f);
  DrawText(18.0f, 78.0f, "Time: " + FormatFloat(Remaining, 0) + "s",
           GLUT_BITMAP_HELVETICA_12);
  DrawText(18.0f, 56.0f, "Age: " + FormatFloat(Age, 2) + "B years",
           GLUT_BITMAP_HELVETICA_12);
  DrawText(18.0f, 34.0f, "Drag orbit | Wheel zoom", GLUT_BITMAP_HELVETICA_10);
  DrawText(18.0f, 18.0f, "Click planet | P pause | R reset",
           GLUT_BITMAP_HELVETICA_10);

  float Cx = (float)WindowWidth - 118.0f;
  float Cy = 130.0f;
  glPushMatrix();
  glTranslatef(Cx, Cy, 0.0f);
  glColor3f(0.55f, 0.95f, 0.62f);
  glCallList(UiCircleList);
  glColor3f(0.90f, 0.18f, 0.16f);
  float DotX = ClampValue(TemperatureAxis, -1.0f, 1.0f) * 75.0f;
  float DotY = ClampValue(AtmosphereAxis, -1.0f, 1.0f) * 75.0f;
  glBegin(GL_POLYGON);
  for (int I = 0; I < 32; I++) {
    float A = (float)I / 32.0f * Pi * 2.0f;
    glVertex2f(DotX + (float)cos(A) * 7.0f, DotY + (float)sin(A) * 7.0f);
  }
  glEnd();
  glColor3f(1.0f, 1.0f, 1.0f);
  DrawText(-50.0f, 92.0f, "High Atmosphere", GLUT_BITMAP_HELVETICA_10);
  DrawText(-48.0f, -104.0f, "Low Atmosphere", GLUT_BITMAP_HELVETICA_10);
  DrawText(-106.0f, -4.0f, "Cold", GLUT_BITMAP_HELVETICA_10);
  DrawText(84.0f, -4.0f, "Hot", GLUT_BITMAP_HELVETICA_10);
  glPopMatrix();

  if (DisasterKind != DisasterNone && DisasterCountdown > 0.0f) {
    glColor3f(1.0f, 0.22f, 0.10f);
    std::string Name = DisasterKind == DisasterMeteor
                           ? "GIANT METEOR INCOMING"
                           : "GLOBAL ICE AGE FORMING";
    DrawText((float)WindowWidth * 0.5f - 128.0f, (float)WindowHeight - 76.0f,
             "ALERT: " + Name, GLUT_BITMAP_HELVETICA_18);
    DrawText((float)WindowWidth * 0.5f - 88.0f, (float)WindowHeight - 102.0f,
             "Stabilize balance quickly", GLUT_BITMAP_HELVETICA_12);
  } else if (AlertSeconds > 0.0f) {
    glColor3f(1.0f, 0.80f, 0.18f);
    DrawText((float)WindowWidth * 0.5f - 92.0f, (float)WindowHeight - 76.0f,
             "EXTREME EVENT IMPACT", GLUT_BITMAP_HELVETICA_18);
  }

  if (GameOver) {
    glColor4f(0.02f, 0.03f, 0.04f, 0.82f);
    glBegin(GL_QUADS);
    glVertex2f(230.0f, (float)WindowHeight * 0.5f - 75.0f);
    glVertex2f((float)WindowWidth - 230.0f, (float)WindowHeight * 0.5f - 75.0f);
    glVertex2f((float)WindowWidth - 230.0f, (float)WindowHeight * 0.5f + 75.0f);
    glVertex2f(230.0f, (float)WindowHeight * 0.5f + 75.0f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string Title = GameWon ? "PLANET HABITABLE" : "PLANET COLLAPSED";
    std::string Body = GameWon
                           ? "You guided 5 billion years of survival."
                           : "The final balance was outside the middle ring.";
    DrawText((float)WindowWidth * 0.5f - 86.0f,
             (float)WindowHeight * 0.5f + 24.0f, Title,
             GLUT_BITMAP_HELVETICA_18);
    DrawText((float)WindowWidth * 0.5f - 142.0f,
             (float)WindowHeight * 0.5f - 6.0f, Body, GLUT_BITMAP_HELVETICA_12);
    DrawText((float)WindowWidth * 0.5f - 64.0f,
             (float)WindowHeight * 0.5f - 34.0f, "Press R to restart",
             GLUT_BITMAP_HELVETICA_12);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void ApplyCamera() {
  float Yaw = CameraYaw * Pi / 180.0f;
  float Pitch = CameraPitch * Pi / 180.0f;
  float X = CameraDistance * (float)cos(Pitch) * (float)sin(Yaw);
  float Y = CameraDistance * (float)sin(Pitch);
  float Z = CameraDistance * (float)cos(Pitch) * (float)cos(Yaw);
  gluLookAt(X, Y, Z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

void Display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (double)WindowWidth / (double)WindowHeight, 0.1, 80.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  ApplyCamera();
  glGetDoublev(GL_MODELVIEW_MATRIX, PickModelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, PickProjectionMatrix);
  glGetIntegerv(GL_VIEWPORT, PickViewport);
  GLfloat LightPosition[4] = {5.5f, 6.0f, 4.5f, 0.0f};
  GLfloat LightSpecular[4] = {0.50f, 0.50f, 0.46f, 1.0f};
  GLfloat LightDiffuse[4] = {0.72f, 0.70f, 0.66f, 1.0f};
  GLfloat FillPosition[4] = {-4.0f, 3.0f, -5.5f, 0.0f};
  GLfloat FillDiffuse[4] = {0.33f, 0.36f, 0.42f, 1.0f};
  GLfloat FillSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
  glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, FillPosition);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, FillDiffuse);
  glLightfv(GL_LIGHT1, GL_SPECULAR, FillSpecular);
  DrawClouds();
  DrawPlanet();
  DrawSurfaceObjects();
  DrawParticles();
  DrawMeteors();
  DrawOverlay();
  glutSwapBuffers();
}

bool RayTriangle(Vector3 Origin, Vector3 Direction, Vector3 A, Vector3 B,
                 Vector3 C, float &Distance) {
  const float Epsilon = 0.000001f;
  Vector3 Edge1 = SubtractVector(B, A);
  Vector3 Edge2 = SubtractVector(C, A);
  Vector3 H = CrossVector(Direction, Edge2);
  float Det = DotVector(Edge1, H);
  if (Det > -Epsilon && Det < Epsilon) {
    return false;
  }
  float InvDet = 1.0f / Det;
  Vector3 S = SubtractVector(Origin, A);
  float U = InvDet * DotVector(S, H);
  if (U < 0.0f || U > 1.0f) {
    return false;
  }
  Vector3 Q = CrossVector(S, Edge1);
  float V = InvDet * DotVector(Direction, Q);
  if (V < 0.0f || U + V > 1.0f) {
    return false;
  }
  float T = InvDet * DotVector(Edge2, Q);
  if (T > Epsilon) {
    Distance = T;
    return true;
  }
  return false;
}

int PickFace(int X, int Y) {
  GLdouble NearX;
  GLdouble NearY;
  GLdouble NearZ;
  GLdouble FarX;
  GLdouble FarY;
  GLdouble FarZ;
  gluUnProject((GLdouble)X, (GLdouble)(PickViewport[3] - Y), 0.0,
               PickModelMatrix, PickProjectionMatrix, PickViewport, &NearX,
               &NearY, &NearZ);
  gluUnProject((GLdouble)X, (GLdouble)(PickViewport[3] - Y), 1.0,
               PickModelMatrix, PickProjectionMatrix, PickViewport, &FarX,
               &FarY, &FarZ);
  Vector3 Origin = MakeVector((float)NearX, (float)NearY, (float)NearZ);
  Vector3 Far = MakeVector((float)FarX, (float)FarY, (float)FarZ);
  Vector3 Direction = NormalizeVector(SubtractVector(Far, Origin));
  UpdateAllGeometry();
  float Closest = 999999.0f;
  int Hit = -1;
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    float T = 0.0f;
    Face &F = PlanetFaces[I];
    if (RayTriangle(Origin, Direction, F.CurrentA, F.CurrentB, F.CurrentC, T)) {
      if (T < Closest) {
        Closest = T;
        Hit = (int)I;
      }
    }
  }
  return Hit;
}

void PushParticle(Vector3 Position, Vector3 Velocity, float Life, int Kind) {
  Particle P;
  P.Position = Position;
  P.Velocity = Velocity;
  P.Life = Life;
  P.Kind = Kind;
  Particles.push_back(P);
}

void SpawnParticlesOnFace(int FaceIndex, int Kind, int Count) {
  if (FaceIndex < 0 || FaceIndex >= (int)PlanetFaces.size()) {
    return;
  }
  Face &F = PlanetFaces[FaceIndex];
  for (int I = 0; I < Count; I++) {
    float J = (RandomUnit() - 0.5f) * 0.24f;
    Vector3 Tangent =
        NormalizeVector(CrossVector(F.Normal, MakeVector(0.0f, 1.0f, 0.0f)));
    if (LengthVector(Tangent) < 0.01f) {
      Tangent = MakeVector(1.0f, 0.0f, 0.0f);
    }
    Vector3 Position = AddVector(
        F.Center, AddVector(ScaleVector(F.Normal, 0.92f + RandomUnit() * 0.24f),
                            ScaleVector(Tangent, J)));
    Vector3 Velocity = ScaleVector(F.Normal, Kind == 2 ? 0.55f : -1.55f);
    if (Kind != 2) {
      Velocity = ScaleVector(F.Normal, -1.40f - RandomUnit() * 0.65f);
    }
    PushParticle(Position, Velocity, 1.0f + RandomUnit() * 0.8f, Kind);
  }
}

void AffectNeighbors(int FaceIndex, float Radius, int Biome, int TreeChange,
                     float HeightChange, bool LockFaces) {
  if (FaceIndex < 0) {
    return;
  }
  Vector3 Center = NormalizeVector(PlanetFaces[FaceIndex].Center);
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Vector3 Other = NormalizeVector(PlanetFaces[I].Center);
    float D = LengthVector(SubtractVector(Center, Other));
    if (D < Radius) {
      PlanetFaces[I].Biome = Biome;
      PlanetFaces[I].Height += HeightChange * (1.0f - D / Radius);
      PlanetFaces[I].Height = ClampValue(PlanetFaces[I].Height, -0.20f, 0.26f);
      if (TreeChange > 0) {
        PlanetFaces[I].TreeCount =
            std::min(4, PlanetFaces[I].TreeCount + TreeChange);
      } else if (TreeChange < 0) {
        PlanetFaces[I].TreeCount = 0;
      }
      if (LockFaces) {
        PlanetFaces[I].Locked = true;
      }
    }
  }
}

void MeteorImpact(int FaceIndex, bool PlayerMade) {
  if (FaceIndex < 0 || FaceIndex >= (int)PlanetFaces.size()) {
    return;
  }
  AffectNeighbors(FaceIndex, PlayerMade ? 0.34f : 0.50f, BiomeScorched, -1,
                  -0.06f, true);
  PlanetFaces[FaceIndex].Biome = BiomeMagma;
  PlanetFaces[FaceIndex].Height -= 0.09f;
  SpawnParticlesOnFace(FaceIndex, 2, 38);
  HeatPressure += PlayerMade ? 0.50f : 0.38f;
  AlertSeconds = 5.5f;
}

void SpawnMeteorToFace(int FaceIndex, bool PlayerMade) {
  if (FaceIndex < 0 || FaceIndex >= (int)PlanetFaces.size()) {
    return;
  }
  Face &F = PlanetFaces[FaceIndex];
  Meteor M;
  Vector3 Start = ScaleVector(NormalizeVector(F.Center), PlanetRadius + 4.5f);
  Start = AddVector(Start, MakeVector((RandomUnit() - 0.5f) * 1.5f,
                                      1.25f + RandomUnit(),
                                      (RandomUnit() - 0.5f) * 1.5f));
  Vector3 Target = AddVector(F.Center, ScaleVector(F.Normal, 0.03f));
  M.Position = Start;
  M.Velocity = ScaleVector(NormalizeVector(SubtractVector(Target, Start)),
                           PlayerMade ? 3.5f : 2.6f);
  M.TargetFace = FaceIndex;
  M.Active = true;
  M.PlayerMade = PlayerMade;
  Meteors.push_back(M);
}

void ApplyTool(int FaceIndex) {
  if (FaceIndex < 0 || FaceIndex >= (int)PlanetFaces.size() || GameOver) {
    return;
  }
  Face &F = PlanetFaces[FaceIndex];
  if (SelectedTool == ToolTree) {
    F.Biome = BiomeForest;
    F.TreeCount = std::min(3, F.TreeCount + 1);
    F.Locked = true;
    AtmospherePressure += 0.12f;
  } else if (SelectedTool == ToolRain) {
    AffectNeighbors(FaceIndex, 0.23f, BiomeLand, 0, 0.01f, true);
    SpawnParticlesOnFace(FaceIndex, 0, 48);
    HeatPressure -= 0.10f;
  } else if (SelectedTool == ToolIce) {
    AffectNeighbors(FaceIndex, 0.23f, BiomeIce, 0, 0.02f, true);
    SpawnParticlesOnFace(FaceIndex, 1, 50);
    HeatPressure -= 0.20f;
  } else if (SelectedTool == ToolFire) {
    AffectNeighbors(FaceIndex, 0.24f, BiomeScorched, -1, -0.01f, true);
    SpawnParticlesOnFace(FaceIndex, 2, 28);
    HeatPressure += 0.10f;
  } else if (SelectedTool == ToolMountain) {
    AffectNeighbors(FaceIndex, 0.19f, BiomeMountain, 0, 0.075f, true);
  } else if (SelectedTool == ToolWater) {
    AffectNeighbors(FaceIndex, 0.21f, BiomeOcean, -1, -0.055f, true);
    SpawnParticlesOnFace(FaceIndex, 0, 28);
  } else if (SelectedTool == ToolMeteor) {
    SpawnMeteorToFace(FaceIndex, true);
  } else if (SelectedTool == ToolRemove) {
    F.TreeCount = 0;
    if (F.Biome == BiomeForest) {
      F.Biome = BiomeLand;
    }
    F.Locked = true;
    AtmospherePressure -= 0.12f;
  }
  HeatPressure = ClampValue(HeatPressure, -1.1f, 1.1f);
  AtmospherePressure = ClampValue(AtmospherePressure, -1.1f, 1.1f);
}

void TriggerIceAge() {
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    if (PlanetFaces[I].Seed > 0.62f || PlanetFaces[I].Center.Y > 1.1f ||
        PlanetFaces[I].Center.Y < -1.1f) {
      PlanetFaces[I].Biome = BiomeIce;
      PlanetFaces[I].TreeCount = 0;
      PlanetFaces[I].Locked = true;
    }
  }
  HeatPressure -= 0.34f;
  AtmospherePressure -= 0.08f;
  AlertSeconds = 6.0f;
}

void BeginDisaster() {
  DisasterKind = RandomUnit() < 0.55f ? DisasterMeteor : DisasterIceAge;
  DisasterCountdown = 8.0f;
  DisasterTargetFace = rand() % (int)PlanetFaces.size();
}

void ResolveDisaster() {
  if (DisasterKind == DisasterMeteor) {
    SpawnMeteorToFace(DisasterTargetFace, false);
  } else if (DisasterKind == DisasterIceAge) {
    TriggerIceAge();
  }
  DisasterKind = DisasterNone;
  DisasterCountdown = 0.0f;
  DisasterTargetFace = -1;
  NextDisasterSeconds = ElapsedSeconds + DisasterIntervalMinimum +
                        RandomUnit() * DisasterIntervalRange;
}

void UpdatePlanetIndicator(float Delta) {
  float HeatTotal = 0.0f;
  float AtmosphereTotal = 0.0f;
  if (PlanetFaces.empty()) {
    return;
  }
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    if (F.Biome == BiomeMagma) {
      HeatTotal += 0.82f;
      AtmosphereTotal -= 0.18f;
    } else if (F.Biome == BiomeScorched) {
      HeatTotal += 0.38f;
      AtmosphereTotal -= 0.18f;
    } else if (F.Biome == BiomeOcean) {
      HeatTotal -= 0.10f;
      AtmosphereTotal += 0.06f;
    } else if (F.Biome == BiomeLand) {
      AtmosphereTotal += 0.04f;
    } else if (F.Biome == BiomeMountain) {
      HeatTotal -= 0.03f;
      AtmosphereTotal += 0.02f;
    } else if (F.Biome == BiomeIce) {
      HeatTotal -= 0.58f;
      AtmosphereTotal -= 0.08f;
    } else if (F.Biome == BiomeForest) {
      HeatTotal -= 0.06f;
      AtmosphereTotal += 0.34f;
    }
    AtmosphereTotal += (float)F.TreeCount * 0.018f;
  }
  float FaceCount = (float)PlanetFaces.size();
  float TargetHeat = HeatTotal / FaceCount + HeatPressure;
  float TargetAtmosphere = AtmosphereTotal / FaceCount + AtmospherePressure;
  TargetHeat = ClampValue(TargetHeat, -1.0f, 1.0f);
  TargetAtmosphere = ClampValue(TargetAtmosphere, -1.0f, 1.0f);
  float Response = 0.95f;
  TemperatureAxis += (TargetHeat - TemperatureAxis) * Response * Delta;
  AtmosphereAxis += (TargetAtmosphere - AtmosphereAxis) * Response * Delta;
  HeatPressure -= HeatPressure * 0.040f * Delta;
  AtmospherePressure -= AtmospherePressure * 0.035f * Delta;
  TemperatureAxis = ClampValue(TemperatureAxis, -1.15f, 1.15f);
  AtmosphereAxis = ClampValue(AtmosphereAxis, -1.15f, 1.15f);
  HeatPressure = ClampValue(HeatPressure, -1.1f, 1.1f);
  AtmospherePressure = ClampValue(AtmospherePressure, -1.1f, 1.1f);
}

void UpdateEvolution(float Delta) {
  float Progress = ClampValue(ElapsedSeconds / GameDuration, 0.0f, 1.0f);
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    bool CanEvolve = !F.Locked || Progress > 0.46f;
    if (CanEvolve) {
      int Target = MatureBiomeForFace(F, Progress);
      float BirthGate = 0.14f + F.Seed * 0.18f;
      float WaterGate = 0.22f + F.Seed * 0.18f;
      float GreenGate = 0.46f + F.Seed * 0.16f;
      float IceGate = 0.72f + F.Seed * 0.10f;
      if (Progress > BirthGate &&
          (F.Biome == BiomeMagma || F.Biome == BiomeScorched) &&
          (Target == BiomeLand || Target == BiomeMountain)) {
        ApplyMatureBiome(F, Target);
      }
      if (Progress > WaterGate && Target == BiomeOcean &&
          F.Biome != BiomeOcean) {
        ApplyMatureBiome(F, Target);
      }
      if (Progress > GreenGate &&
          (Target == BiomeForest || Target == BiomeMountain) &&
          F.Biome != Target) {
        ApplyMatureBiome(F, Target);
      }
      if (Progress > IceGate && Target == BiomeIce) {
        ApplyMatureBiome(F, Target);
      }
    }
  }
}

void UpdateObjects(float Delta) {
  CloudRotation += 1.6f * Delta;
  if (CloudRotation > 360.0f) {
    CloudRotation -= 360.0f;
  }
  TreePulse = 1.0f + (float)sin(ElapsedSeconds * 0.12f) * 0.003f;
  for (size_t I = 0; I < Particles.size(); I++) {
    Particles[I].Position = AddVector(
        Particles[I].Position, ScaleVector(Particles[I].Velocity, Delta));
    Particles[I].Life -= Delta;
  }
  std::vector<Particle> AliveParticles;
  for (size_t I = 0; I < Particles.size(); I++) {
    if (Particles[I].Life > 0.0f) {
      AliveParticles.push_back(Particles[I]);
    }
  }
  Particles = AliveParticles;

  for (size_t I = 0; I < Meteors.size(); I++) {
    if (Meteors[I].Active) {
      Meteors[I].Position = AddVector(Meteors[I].Position,
                                      ScaleVector(Meteors[I].Velocity, Delta));
      if (Meteors[I].TargetFace >= 0 &&
          Meteors[I].TargetFace < (int)PlanetFaces.size()) {
        Vector3 Target = PlanetFaces[Meteors[I].TargetFace].Center;
        if (LengthVector(SubtractVector(Meteors[I].Position, Target)) < 0.25f) {
          Meteors[I].Active = false;
          MeteorImpact(Meteors[I].TargetFace, Meteors[I].PlayerMade);
        }
      }
    }
  }
}

void CheckWinLose() {
  float BalanceDistance = (float)sqrt(TemperatureAxis * TemperatureAxis +
                                      AtmosphereAxis * AtmosphereAxis);
  if (ElapsedSeconds >= GameDuration) {
    GameOver = true;
    GameWon = BalanceDistance <= SafeBalanceRadius;
  }
}

void Timer(int Value) {
  float Now = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
  float Delta = Now - LastTickSeconds;
  LastTickSeconds = Now;
  if (Delta > 0.10f) {
    Delta = 0.10f;
  }
  if (!GamePaused && !GameOver) {
    ElapsedSeconds += Delta;
    UpdateEvolution(Delta);
    UpdatePlanetIndicator(Delta);
    if (ElapsedSeconds >= NextDisasterSeconds && DisasterKind == DisasterNone) {
      BeginDisaster();
    }
    if (DisasterCountdown > 0.0f) {
      DisasterCountdown -= Delta;
      if (DisasterCountdown <= 0.0f) {
        ResolveDisaster();
      }
    }
    CheckWinLose();
  }
  if (AlertSeconds > 0.0f) {
    AlertSeconds -= Delta;
  }
  UpdateObjects(Delta);
  glutPostRedisplay();
  glutTimerFunc(16, Timer, 0);
}

void ResetGame() {
  GeneratePlanet();
  Particles.clear();
  Meteors.clear();
  SelectedTool = ToolTree;
  DisasterKind = DisasterNone;
  DisasterTargetFace = -1;
  ElapsedSeconds = 0.0f;
  NextDisasterSeconds = FirstDisasterTime;
  DisasterCountdown = 0.0f;
  AlertSeconds = 0.0f;
  TemperatureAxis = 0.24f;
  AtmosphereAxis = 0.20f;
  HeatPressure = 0.0f;
  AtmospherePressure = 0.0f;
  CloudRotation = 0.0f;
  GameOver = false;
  GameWon = false;
  GamePaused = false;
  UpdateAllGeometry();
}

void Keyboard(unsigned char Key, int X, int Y) {
  if (Key >= '1' && Key <= '8') {
    SelectedTool = Key - '1';
  } else if (Key == 'p' || Key == 'P') {
    GamePaused = !GamePaused;
  } else if (Key == 'r' || Key == 'R') {
    ResetGame();
  } else if (Key == 27 || Key == 'q' || Key == 'Q') {
    exit(0);
  }
}

void Mouse(int Button, int State, int X, int Y) {
  if (Button == 3 && State == GLUT_DOWN) {
    CameraDistance = ClampValue(CameraDistance - 0.42f, 4.0f, 12.0f);
    return;
  }
  if (Button == 4 && State == GLUT_DOWN) {
    CameraDistance = ClampValue(CameraDistance + 0.42f, 4.0f, 12.0f);
    return;
  }
  if (Button == GLUT_LEFT_BUTTON) {
    if (State == GLUT_DOWN) {
      MouseIsDown = true;
      MouseIsDragging = false;
      LastMouseX = X;
      LastMouseY = Y;
      StartMouseX = X;
      StartMouseY = Y;
    } else {
      if (!MouseIsDragging) {
        int Hit = PickFace(X, Y);
        ApplyTool(Hit);
      }
      MouseIsDown = false;
    }
  }
}

void Motion(int X, int Y) {
  if (MouseIsDown) {
    int Dx = X - LastMouseX;
    int Dy = Y - LastMouseY;
    if (abs(X - StartMouseX) + abs(Y - StartMouseY) > 5) {
      MouseIsDragging = true;
    }
    CameraYaw += (float)Dx * 0.45f;
    CameraPitch += (float)Dy * 0.35f;
    CameraPitch = ClampValue(CameraPitch, -82.0f, 82.0f);
    LastMouseX = X;
    LastMouseY = Y;
  }
}

void Reshape(int Width, int Height) {
  WindowWidth = Width > 1 ? Width : 1;
  WindowHeight = Height > 1 ? Height : 1;
  glViewport(0, 0, WindowWidth, WindowHeight);
}

void InitOpenGl() {
  srand((unsigned int)time(0));
  glClearColor(0.43f, 0.42f, 0.47f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_FLAT);
  GLfloat GlobalAmbient[4] = {0.40f, 0.40f, 0.43f, 1.0f};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, GlobalAmbient);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  BuildDisplayLists();
  ResetGame();
}

int main(int ArgCount, char **ArgValues) {
  glutInit(&ArgCount, ArgValues);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(WindowWidth, WindowHeight);
  glutCreateWindow("Epoch");
  InitOpenGl();
  LastTickSeconds = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
  glutDisplayFunc(Display);
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Keyboard);
  glutMouseFunc(Mouse);
  glutMotionFunc(Motion);
  glutTimerFunc(16, Timer, 0);
  glutMainLoop();
  return 0;
}
