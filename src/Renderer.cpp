#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <sstream>
#include "Renderer.h"
#include "MathUtils.h"
#include "GameGlobals.h"
#include "GameConfig.h"
#include "Planet.h"

GLuint LowPolyTextureId = 0;

void GenerateLowPolyTexture() {
  const int Width = 128;
  const int Height = 128;
  unsigned char Data[Width * Height * 4];
  
  for (int Y = 0; Y < Height; Y++) {
    for (int X = 0; X < Width; X++) {
      float U = (float)X / (float)(Width - 1);
      float V = (float)Y / (float)(Height - 1);
      
      float DistBottom = (float)fabs(V - 0.1f);
      float DistLeft = (float)fabs(0.7f * (U - 0.1f) - 0.4f * (V - 0.1f)) / 0.806f;
      float DistRight = (float)fabs(-0.7f * (U - 0.9f) - 0.4f * (V - 0.1f)) / 0.806f;
      
      float MinDist = DistBottom;
      if (DistLeft < MinDist) MinDist = DistLeft;
      if (DistRight < MinDist) MinDist = DistRight;
      
      float W_A = ((0.1f - 0.8f)*(U - 0.5f) + (0.5f - 0.9f)*(V - 0.8f)) / 0.56f;
      float W_B = ((0.8f - 0.1f)*(U - 0.5f) + (0.1f - 0.5f)*(V - 0.8f)) / 0.56f;
      float W_C = 1.0f - W_A - W_B;
      
      bool Inside = (W_A >= -0.01f && W_B >= -0.01f && W_C >= -0.01f);
      
      float BaseShade = 1.0f;
      if (Inside) {
        float EdgeShadow = 1.0f;
        if (MinDist < 0.05f) {
          EdgeShadow = 0.62f + 0.38f * (MinDist / 0.05f);
        }
        float Noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.06f;
        float Gradient = 1.0f - (V - 0.1f) * 0.12f; 
        BaseShade = EdgeShadow * Gradient + Noise;
        BaseShade = ClampValue(BaseShade, 0.0f, 1.0f);
      } else {
        BaseShade = 0.45f;
      }
      
      int Index = (Y * Width + X) * 4;
      unsigned char ByteVal = (unsigned char)(BaseShade * 255.0f);
      Data[Index] = ByteVal;
      Data[Index + 1] = ByteVal;
      Data[Index + 2] = ByteVal;
      Data[Index + 3] = 255;
    }
  }
  
  glGenTextures(1, &LowPolyTextureId);
  glBindTexture(GL_TEXTURE_2D, LowPolyTextureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
}

// Internal rendering helper
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

// Text rendering helpers
void DrawText(float X, float Y, std::string Text, void *Font) {
  glRasterPos2f(X, Y);
  for (size_t I = 0; I < Text.length(); I++) {
    glutBitmapCharacter(Font, Text[I]);
  }
}

void DrawCenteredText(float CenterX, float Y, std::string Text, void *Font) {
  int Width = glutBitmapLength(Font, (const unsigned char *)Text.c_str());
  glRasterPos2f(CenterX - (float)Width * 0.5f, Y);
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

void SetBiomeMaterial(int Biome, int FaceIndex, float Altitude, float Height) {
  float Var = (float)((FaceIndex * 73 + 123) % 100) / 100.0f * 0.08f - 0.04f;

  float HotFactor = ClampValue(VisualTemperatureAxis, 0.0f, 1.0f);
  float ColdFactor = ClampValue(-VisualTemperatureAxis, 0.0f, 1.0f);
  float NeutralFactor = 1.0f - HotFactor - ColdFactor;

  if (Biome == BiomeOcean) {
    // Default: (0.11f, 0.20f, 0.34f)
    // Hot target: (0.62f, 0.24f, 0.08f) (boiling toxic orange)
    // Cold target: (0.65f, 0.80f, 0.95f) (frozen ice-blue)
    float R = 0.11f * NeutralFactor + 0.62f * HotFactor + 0.65f * ColdFactor;
    float G = 0.20f * NeutralFactor + 0.24f * HotFactor + 0.80f * ColdFactor;
    float B = 0.34f * NeutralFactor + 0.08f * HotFactor + 0.95f * ColdFactor;
    SetMaterial(R + Var * 0.3f, G + Var * 0.4f, B + Var * 0.5f, true, false);
  } else if (Biome == BiomeLand) {
    // Default: (0.61f, 0.40f, 0.26f)
    // Hot target: (0.16f, 0.14f, 0.12f) (scorched charcoal black)
    // Cold target: (0.65f, 0.70f, 0.78f) (frozen tundra light grey)
    float R = 0.61f * NeutralFactor + 0.16f * HotFactor + 0.65f * ColdFactor;
    float G = 0.40f * NeutralFactor + 0.14f * HotFactor + 0.70f * ColdFactor;
    float B = 0.26f * NeutralFactor + 0.12f * HotFactor + 0.78f * ColdFactor;
    SetMaterial(R + Var * 0.5f, G + Var * 0.5f, B + Var * 0.4f, false, false);
  } else if (Biome == BiomeMountain) {
    float Progress = ElapsedSeconds / GameDuration;
    bool HasSnow = (Progress > 0.35f && (Height > 0.062f || (float)fabs(Altitude) > 1.15f));
    if (HasSnow && VisualTemperatureAxis <= 0.25f) {
      // Snowy Peak - melts when too hot
      float MeltFactor = ClampValue((VisualTemperatureAxis - 0.0f) / 0.25f, 0.0f, 1.0f);
      float R = 0.92f * (1.0f - MeltFactor) + 0.35f * MeltFactor;
      float G = 0.94f * (1.0f - MeltFactor) + 0.36f * MeltFactor;
      float B = 0.96f * (1.0f - MeltFactor) + 0.42f * MeltFactor;
      SetMaterial(R + Var * 0.08f, G + Var * 0.08f, B + Var * 0.08f, true, false);
    } else {
      // Base mountain: default (0.35f, 0.36f, 0.42f)
      // Hot target: (0.20f, 0.18f, 0.18f) (volcanic dark grey)
      // Cold target: (0.80f, 0.85f, 0.90f) (snowy white mountain)
      float R = 0.35f * NeutralFactor + 0.20f * HotFactor + 0.80f * ColdFactor;
      float G = 0.36f * NeutralFactor + 0.18f * HotFactor + 0.85f * ColdFactor;
      float B = 0.42f * NeutralFactor + 0.18f * HotFactor + 0.90f * ColdFactor;
      SetMaterial(R + Var * 0.3f, G + Var * 0.3f, B + Var * 0.3f, false, false);
    }
  } else if (Biome == BiomeIce) {
    // Default: (0.94f, 0.98f, 0.93f)
    // Hot target: (0.55f, 0.50f, 0.45f) (dirty melting ice sludge)
    float R = 0.94f * (1.0f - HotFactor) + 0.55f * HotFactor;
    float G = 0.98f * (1.0f - HotFactor) + 0.50f * HotFactor;
    float B = 0.93f * (1.0f - HotFactor) + 0.45f * HotFactor;
    SetMaterial(R + Var * 0.1f, G + Var * 0.1f, B + Var * 0.1f, true, false);
  } else if (Biome == BiomeForest) {
    // Default: (0.15f, 0.21f, 0.09f)
    // Hot target: (0.45f, 0.32f, 0.12f) (wilted autumn yellow-brown)
    // Cold target: (0.30f, 0.45f, 0.48f) (frosty teal pine)
    float R = 0.15f * NeutralFactor + 0.45f * HotFactor + 0.30f * ColdFactor;
    float G = 0.21f * NeutralFactor + 0.32f * HotFactor + 0.45f * ColdFactor;
    float B = 0.09f * NeutralFactor + 0.12f * HotFactor + 0.48f * ColdFactor;
    SetMaterial(R + Var * 0.3f, G + Var * 0.4f, B + Var * 0.2f, false, false);
  } else if (Biome == BiomeMagma) {
    // Default: (0.88f, 0.34f, 0.13f), Glow = true
    // Cold target: (0.18f, 0.18f, 0.19f), Glow = false (magma cools to solid black obsidian)
    float R = 0.88f * (1.0f - ColdFactor) + 0.18f * ColdFactor;
    float G = 0.34f * (1.0f - ColdFactor) + 0.18f * ColdFactor;
    float B = 0.13f * (1.0f - ColdFactor) + 0.19f * ColdFactor;
    bool ActiveGlow = (ColdFactor < 0.70f);
    SetMaterial(R + Var * 0.2f, G + Var * 0.3f, B + Var * 0.1f, false, ActiveGlow);
  } else {
    SetMaterial(0.24f + Var * 0.3f, 0.24f + Var * 0.3f, 0.22f + Var * 0.3f, false, false);
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

  glBegin(GL_TRIANGLES);
  glNormal3f(0.0f, -1.0f, 0.0f);
  for (int I = 1; I < Sides - 1; I++) {
    float A0 = 0.0f;
    float A1 = (float)I / (float)Sides * Pi * 2.0f;
    float A2 = (float)(I + 1) / (float)Sides * Pi * 2.0f;
    glVertex3f((float)cos(A0) * Radius, 0.0f, (float)sin(A0) * Radius);
    glVertex3f((float)cos(A2) * Radius, 0.0f, (float)sin(A2) * Radius);
    glVertex3f((float)cos(A1) * Radius, 0.0f, (float)sin(A1) * Radius);
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
  SetMaterial(0.22f, 0.11f, 0.05f, false, false);
  DrawBlock(0.075f, 0.14f, 0.075f);
  glTranslatef(0.0f, 0.12f, 0.0f);
  SetMaterial(0.28f, 0.15f, 0.08f, false, false);
  DrawBlock(0.060f, 0.16f, 0.060f);
  glTranslatef(0.0f, 0.12f, 0.0f);
  SetMaterial(0.11f, 0.18f, 0.07f, false, false);
  DrawLowPolyCone(0.23f, 0.34f, 5);
  glTranslatef(0.0f, 0.18f, 0.0f);
  SetMaterial(0.16f, 0.26f, 0.09f, false, false);
  DrawLowPolyCone(0.18f, 0.28f, 5);
  glTranslatef(0.0f, 0.16f, 0.0f);
  SetMaterial(0.22f, 0.38f, 0.12f, false, false);
  DrawLowPolyCone(0.12f, 0.22f, 5);
  glPopMatrix();
  glEndList();

  CloudList = glGenLists(1);
  glNewList(CloudList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.92f, 0.92f, 0.94f, true, false);
  glPushMatrix();
  glScalef(0.34f, 0.16f, 0.22f);
  glutSolidIcosahedron();
  glPopMatrix();
  SetMaterial(0.86f, 0.86f, 0.89f, true, false);
  glPushMatrix();
  glTranslatef(0.24f, 0.03f, 0.05f);
  glScalef(0.24f, 0.13f, 0.18f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.21f, -0.02f, -0.06f);
  glScalef(0.22f, 0.12f, 0.16f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPopMatrix();
  glEndList();

  MeteorList = glGenLists(1);
  glNewList(MeteorList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.26f, 0.24f, 0.23f, false, false);
  glScalef(0.16f, 0.14f, 0.15f);
  glutSolidIcosahedron();
  glPopMatrix();
  glEndList();

  MountainList = glGenLists(1);
  glNewList(MountainList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.32f, 0.33f, 0.38f, false, false);
  DrawLowPolyCone(0.22f, 0.48f, 5);
  glPushMatrix();
  glTranslatef(0.18f, 0.0f, 0.08f);
  glScalef(0.72f, 0.78f, 0.72f);
  SetMaterial(0.32f, 0.33f, 0.38f, false, false);
  DrawLowPolyCone(0.18f, 0.40f, 5);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.18f, 0.0f, -0.06f);
  glScalef(0.62f, 0.70f, 0.62f);
  SetMaterial(0.32f, 0.33f, 0.38f, false, false);
  DrawLowPolyCone(0.16f, 0.34f, 5);
  glPopMatrix();
  glPopMatrix();
  glEndList();

  MountainSnowList = glGenLists(1);
  glNewList(MountainSnowList, GL_COMPILE);
  glPushMatrix();
  glPushMatrix();
  glTranslatef(0.0f, 0.318f, 0.0f);
  glScalef(1.03f, 1.02f, 1.03f);
  SetMaterial(0.92f, 0.94f, 0.96f, true, false);
  DrawLowPolyCone(0.074f, 0.162f, 5);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(0.18f, 0.0f, 0.08f);
  glScalef(0.72f, 0.78f, 0.72f);
  glPushMatrix();
  glTranslatef(0.0f, 0.258f, 0.0f);
  glScalef(1.03f, 1.02f, 1.03f);
  SetMaterial(0.92f, 0.94f, 0.96f, true, false);
  DrawLowPolyCone(0.063f, 0.142f, 5);
  glPopMatrix();
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.18f, 0.0f, -0.06f);
  glScalef(0.62f, 0.70f, 0.62f);
  glPushMatrix();
  glTranslatef(0.0f, 0.218f, 0.0f);
  glScalef(1.03f, 1.02f, 1.03f);
  SetMaterial(0.92f, 0.94f, 0.96f, true, false);
  DrawLowPolyCone(0.056f, 0.122f, 5);
  glPopMatrix();
  glPopMatrix();
  glPopMatrix();
  glEndList();

  IceClusterList = glGenLists(1);
  glNewList(IceClusterList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.88f, 0.95f, 1.00f, true, false);
  DrawLowPolyCone(0.13f, 0.38f, 5);
  glPushMatrix();
  glTranslatef(0.13f, 0.0f, 0.08f);
  glScalef(0.70f, 0.82f, 0.70f);
  SetMaterial(0.70f, 0.88f, 0.98f, true, false);
  DrawLowPolyCone(0.12f, 0.32f, 5);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(-0.11f, 0.0f, -0.07f);
  glScalef(0.58f, 0.66f, 0.58f);
  SetMaterial(0.60f, 0.82f, 0.96f, true, false);
  DrawLowPolyCone(0.11f, 0.29f, 5);
  glPopMatrix();
  glPopMatrix();
  glEndList();

  VolcanoList = glGenLists(1);
  glNewList(VolcanoList, GL_COMPILE);
  glPushMatrix();
  SetMaterial(0.18f, 0.18f, 0.19f, false, false);
  DrawLowPolyCone(0.20f, 0.34f, 6);
  SetMaterial(0.95f, 0.35f, 0.05f, false, true);
  glPushMatrix();
  glRotatef(30.0f, 0.0f, 1.0f, 0.0f);
  glTranslatef(0.08f, 0.05f, 0.08f);
  glScalef(0.02f, 0.20f, 0.02f);
  glutSolidCube(1.0);
  glPopMatrix();
  glPushMatrix();
  glRotatef(150.0f, 0.0f, 1.0f, 0.0f);
  glTranslatef(0.08f, 0.08f, -0.08f);
  glScalef(0.02f, 0.16f, 0.02f);
  glutSolidCube(1.0);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(0.0f, 0.22f, 0.0f);
  SetMaterial(1.00f, 0.45f, 0.05f, false, true);
  DrawLowPolyCone(0.08f, 0.13f, 5);
  glPopMatrix();
  glPopMatrix();
  glEndList();

  UiCircleList = glGenLists(1);
  glNewList(UiCircleList, GL_COMPILE);
  glBegin(GL_LINE_LOOP);
  for (int I = 0; I < 32; I++) {
    float A = (float)I / 32.0f * Pi * 2.0f;
    glVertex3f((float)cos(A), 0.0f, (float)sin(A));
  }
  glEnd();
  glEndList();
}

void DrawPlanet() {
  UpdateAllGeometry();
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    SetBiomeMaterial(F.Biome, (int)I, F.Center.Y, F.Height);
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
                              ScaleVector(F.Normal, 0.001f));
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
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.001f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);
      glRotatef(F.Seed * 360.0f, 0.0f, 1.0f, 0.0f);
      float S = 1.10f + F.Seed * 0.30f;
      glScalef(S, S, S);
      glCallList(MountainList);
      float Progress = ElapsedSeconds / GameDuration;
      if (Progress > 0.35f) {
        glCallList(MountainSnowList);
      }
      glPopMatrix();
    }
    if (F.Biome == BiomeIce && ShouldPlaceSurfaceAsset(F, 0.90f, 3.5f)) {
      glPushMatrix();
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.001f));
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
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.001f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);
      float S = F.Biome == BiomeMagma ? 0.84f : 0.62f;
      glScalef(S, S, S);
      glCallList(VolcanoList);
      glPopMatrix();
    }

    // Ecosystem rendering based on EcosystemLevel
    if (F.EcosystemLevel >= 1) {
      // Level 1: Microbial green algae patches (flat hex cells)
      for (int PIdx = 0; PIdx < 3; PIdx++) {
        float A = F.Seed * Pi * 10.0f + (float)PIdx * 2.09f;
        float R = 0.04f + 0.02f * (float)PIdx;
        Vector3 Offset = AddVector(ScaleVector(Tangent, (float)cos(A) * R),
                                   ScaleVector(Bitangent, (float)sin(A) * R));
        glPushMatrix();
        Vector3 P = AddVector(AddVector(F.Center, Offset), ScaleVector(F.Normal, 0.003f));
        glTranslatef(P.X, P.Y, P.Z);
        AlignToNormal(F.Normal);
        SetMaterial(0.20f + (float)PIdx * 0.05f, 0.68f - (float)PIdx * 0.05f, 0.38f, false, false);
        glBegin(GL_POLYGON);
        for (int S = 0; S < 6; S++) {
          float Angle = (float)S / 6.0f * Pi * 2.0f;
          glVertex3f((float)cos(Angle) * 0.022f, 0.0f, (float)sin(Angle) * 0.022f);
        }
        glEnd();
        glPopMatrix();
      }
    }

    if (F.EcosystemLevel >= 2) {
      // Level 2: Wildlife/plants (tiny low-poly bushes/plants)
      for (int PIdx = 0; PIdx < 2; PIdx++) {
        float A = F.Seed * Pi * 12.0f + (float)PIdx * 3.14f + 1.0f;
        float R = 0.05f + 0.01f * (float)PIdx;
        Vector3 Offset = AddVector(ScaleVector(Tangent, (float)cos(A) * R),
                                   ScaleVector(Bitangent, (float)sin(A) * R));
        glPushMatrix();
        Vector3 P = AddVector(AddVector(F.Center, Offset), ScaleVector(F.Normal, 0.001f));
        glTranslatef(P.X, P.Y, P.Z);
        AlignToNormal(F.Normal);
        glRotatef(F.Seed * 180.0f + (float)PIdx * 45.0f, 0.0f, 1.0f, 0.0f);

        // Brown stem
        SetMaterial(0.32f, 0.22f, 0.12f, false, false);
        DrawLowPolyCone(0.012f, 0.038f, 4);

        // Green leaves cone
        glTranslatef(0.0f, 0.03f, 0.0f);
        SetMaterial(0.14f, 0.54f, 0.26f, false, false);
        DrawLowPolyCone(0.032f, 0.048f, 4);
        glPopMatrix();
      }
    }

    if (F.EcosystemLevel >= 3) {
      // Level 3: Primitive tribal camp huts and a glowing central campfire
      for (int HIdx = 0; HIdx < 2; HIdx++) {
        float A = F.Seed * Pi * 15.0f + (float)HIdx * 3.14f + 2.0f;
        float R = 0.055f;
        Vector3 Offset = AddVector(ScaleVector(Tangent, (float)cos(A) * R),
                                   ScaleVector(Bitangent, (float)sin(A) * R));
        glPushMatrix();
        Vector3 P = AddVector(AddVector(F.Center, Offset), ScaleVector(F.Normal, 0.001f));
        glTranslatef(P.X, P.Y, P.Z);
        AlignToNormal(F.Normal);

        // Hut base body (cylinder/cone)
        SetMaterial(0.48f, 0.34f, 0.22f, false, false);
        DrawLowPolyCone(0.026f, 0.032f, 5);

        // Hut straw roof (yellow cone)
        glTranslatef(0.0f, 0.026f, 0.0f);
        SetMaterial(0.90f, 0.74f, 0.28f, false, false);
        DrawLowPolyCone(0.030f, 0.022f, 5);
        glPopMatrix();
      }

      // Campfire
      glPushMatrix();
      Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.001f));
      glTranslatef(P.X, P.Y, P.Z);
      AlignToNormal(F.Normal);

      // Stone ring
      SetMaterial(0.28f, 0.28f, 0.32f, false, false);
      for (int S = 0; S < 5; S++) {
        float Angle = (float)S / 5.0f * Pi * 2.0f;
        glPushMatrix();
        glTranslatef((float)cos(Angle) * 0.016f, 0.0f, (float)sin(Angle) * 0.016f);
        DrawLowPolyCone(0.006f, 0.006f, 3);
        glPopMatrix();
      }

      // Glowing fire flame (pulsing)
      float FlamePulse = 1.0f + (float)sin(ElapsedSeconds * 12.0f) * 0.16f;
      SetMaterial(1.0f, 0.54f, 0.08f, false, true); // Glow = true
      DrawLowPolyCone(0.009f, 0.022f * FlamePulse, 4);
      glPopMatrix();
    }
  }
}

void SetCloudMaterial(float R, float G, float B, float Alpha, bool Shiny, bool Glow) {
  GLfloat Ambient[4];
  GLfloat Diffuse[4];
  GLfloat Specular[4];
  GLfloat Emission[4];
  Ambient[0] = Glow ? R * 0.85f : R * 0.64f;
  Ambient[1] = Glow ? G * 0.60f : G * 0.64f;
  Ambient[2] = Glow ? B * 0.42f : B * 0.64f;
  Ambient[3] = Alpha;
  Diffuse[0] = R;
  Diffuse[1] = G;
  Diffuse[2] = B;
  Diffuse[3] = Alpha;
  Specular[0] = Shiny ? 1.0f : 0.0f;
  Specular[1] = Shiny ? 1.0f : 0.0f;
  Specular[2] = Shiny ? 1.0f : 0.0f;
  Specular[3] = Alpha;
  Emission[0] = Glow ? R * 0.28f : 0.0f;
  Emission[1] = Glow ? G * 0.10f : 0.0f;
  Emission[2] = Glow ? B * 0.05f : 0.0f;
  Emission[3] = Alpha;
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Emission);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Shiny ? 85.0f : 5.0f);
}

void DrawSingleCloud(float HotFactor, float ThickFactor, float Alpha) {
  // Default cloud color is white-gray: (0.92f, 0.92f, 0.94f)
  // If hot, blend to yellow-orange sulfur ash: (0.65f, 0.52f, 0.35f)
  // If thick, blend to dark storm grey: (0.30f, 0.30f, 0.34f)
  float R1 = 0.92f * (1.0f - HotFactor) + 0.65f * HotFactor;
  float G1 = 0.92f * (1.0f - HotFactor) + 0.52f * HotFactor;
  float B1 = 0.94f * (1.0f - HotFactor) + 0.35f * HotFactor;
  R1 = R1 * (1.0f - ThickFactor) + 0.30f * ThickFactor;
  G1 = G1 * (1.0f - ThickFactor) + 0.30f * ThickFactor;
  B1 = B1 * (1.0f - ThickFactor) + 0.34f * ThickFactor;

  float R2 = 0.86f * (1.0f - HotFactor) + 0.60f * HotFactor;
  float G2 = 0.86f * (1.0f - HotFactor) + 0.48f * HotFactor;
  float B2 = 0.89f * (1.0f - HotFactor) + 0.32f * HotFactor;
  R2 = R2 * (1.0f - ThickFactor) + 0.26f * ThickFactor;
  G2 = G2 * (1.0f - ThickFactor) + 0.26f * ThickFactor;
  B2 = B2 * (1.0f - ThickFactor) + 0.30f * ThickFactor;

  glPushMatrix();
  SetCloudMaterial(R1, G1, B1, Alpha, true, false);
  glPushMatrix();
  glScalef(0.34f, 0.16f, 0.22f);
  glutSolidIcosahedron();
  glPopMatrix();
  
  SetCloudMaterial(R2, G2, B2, Alpha, true, false);
  glPushMatrix();
  glTranslatef(0.24f, 0.03f, 0.05f);
  glScalef(0.24f, 0.13f, 0.18f);
  glutSolidIcosahedron();
  glPopMatrix();
  
  glPushMatrix();
  glTranslatef(-0.21f, -0.02f, -0.06f);
  glScalef(0.22f, 0.12f, 0.16f);
  glutSolidIcosahedron();
  glPopMatrix();
  glPopMatrix();
}

void DrawClouds() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  float HotFactor = ClampValue(VisualTemperatureAxis, 0.0f, 1.0f);
  float ThickFactor = ClampValue(VisualAtmosphereAxis, 0.0f, 1.0f);
  float NormAtmos = (VisualAtmosphereAxis + 1.15f) / 2.3f;
  float CloudAlpha = ClampValue(NormAtmos * 0.88f, 0.0f, 0.88f);

  if (CloudAlpha > 0.01f) {
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
      float AtmosScale = ClampValue(NormAtmos * 1.30f, 0.0f, 1.0f);
      glScalef(S * AtmosScale, S * AtmosScale, S * AtmosScale);
      DrawSingleCloud(HotFactor, ThickFactor, CloudAlpha);
      glPopMatrix();
    }
    glPopMatrix();
  }

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
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

void DrawAtmosphereRing() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  glDisable(GL_LIGHTING);

  float Red = 0.15f;
  float Green = 0.65f;
  float Blue = 0.95f;

  if (VisualTemperatureAxis > 0.35f) {
    float Factor = ClampValue((VisualTemperatureAxis - 0.35f) / 0.65f, 0.0f, 1.0f);
    Red = 0.15f * (1.0f - Factor) + 0.90f * Factor;
    Green = 0.65f * (1.0f - Factor) + 0.35f * Factor;
    Blue = 0.95f * (1.0f - Factor) + 0.08f * Factor;
  } else if (VisualTemperatureAxis < -0.35f) {
    float Factor = ClampValue((-VisualTemperatureAxis - 0.35f) / 0.65f, 0.0f, 1.0f);
    Red = 0.15f * (1.0f - Factor) + 0.85f * Factor;
    Green = 0.65f * (1.0f - Factor) + 0.92f * Factor;
    Blue = 0.95f * (1.0f - Factor) + 1.00f * Factor;
  }

  float NormAtmos = (VisualAtmosphereAxis + 1.15f) / 2.3f;
  float Alpha = 0.015f + NormAtmos * 0.065f;

  glColor4f(Red, Green, Blue, Alpha);
  glutSolidSphere(PlanetRadius + 0.09f, 32, 32);

  glEnable(GL_LIGHTING);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void DrawHolographicRing() {
  if (HoveredFace < 0 || HoveredFace >= (int)PlanetFaces.size() || GameOver) {
    return;
  }
  Face &F = PlanetFaces[HoveredFace];
  float RVal = 0.06f;
  if (SelectedTool == ToolRain) RVal = 0.23f;
  else if (SelectedTool == ToolIce) RVal = 0.23f;
  else if (SelectedTool == ToolFire) RVal = 0.24f;
  else if (SelectedTool == ToolMountain) RVal = 0.19f;
  else if (SelectedTool == ToolWater) RVal = 0.21f;
  else if (SelectedTool == ToolMeteor) RVal = 0.25f;
  else if (SelectedTool == ToolRemove) RVal = 0.15f;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  glDisable(GL_LIGHTING);
  glLineWidth(2.5f);

  if (SelectedTool == ToolRemove) {
    glColor4f(0.85f, 0.20f, 0.20f, 0.80f);
  } else if (SelectedTool == ToolMeteor) {
    glColor4f(0.92f, 0.45f, 0.15f, 0.80f);
  } else {
    glColor4f(0.35f, 0.75f, 0.90f, 0.80f);
  }

  glPushMatrix();
  Vector3 P = AddVector(F.Center, ScaleVector(F.Normal, 0.012f));
  glTranslatef(P.X, P.Y, P.Z);
  AlignToNormal(F.Normal);
  glScalef(RVal, RVal, RVal);
  glCallList(UiCircleList);
  glPopMatrix();

  glLineWidth(1.0f);
  glEnable(GL_LIGHTING);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
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

  if (InMainMenu) {
    glColor4f(0.03f, 0.04f, 0.06f, 0.70f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)WindowWidth, 0.0f);
    glVertex2f((float)WindowWidth, (float)WindowHeight);
    glVertex2f(0.0f, (float)WindowHeight);
    glEnd();

    float MidX = (float)WindowWidth * 0.5f;
    float MidY = (float)WindowHeight * 0.5f;

    glColor3f(0.92f, 0.86f, 0.40f);
    DrawCenteredText(MidX, MidY + 40.0f, "E P O C H", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.80f, 0.82f, 0.86f);
    DrawCenteredText(MidX, MidY + 5.0f, "3D Planetary & Climate Simulator", GLUT_BITMAP_HELVETICA_12);

    float Pulse = (float)sin(ElapsedSeconds * 4.0f) * 0.25f + 0.75f;
    glColor4f(0.92f, 0.86f, 0.40f, Pulse);
    DrawCenteredText(MidX, MidY - 60.0f, "PRESS ENTER OR CLICK TO START", GLUT_BITMAP_HELVETICA_12);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    return;
  }

  // Left Sidebar background panel
  glColor4f(0.04f, 0.05f, 0.08f, 0.88f);
  glBegin(GL_QUADS);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(188.0f, 0.0f);
  glVertex2f(188.0f, (float)WindowHeight);
  glVertex2f(0.0f, (float)WindowHeight);
  glEnd();

  // Left Sidebar vertical accent line
  glColor4f(0.32f, 0.40f, 0.50f, 0.40f);
  glLineWidth(2.0f);
  glBegin(GL_LINES);
  glVertex2f(188.0f, 0.0f);
  glVertex2f(188.0f, (float)WindowHeight);
  glEnd();
  glLineWidth(1.0f);

  int CurrentPhase = 1 + (int)(ElapsedSeconds / 60.0f);
  if (CurrentPhase > 5) CurrentPhase = 5;

  std::string EraName = "";
  if (CurrentPhase == 1) EraName = "Phase 1: Hadean";
  else if (CurrentPhase == 2) EraName = "Phase 2: Archean";
  else if (CurrentPhase == 3) EraName = "Phase 3: Proterozoic";
  else if (CurrentPhase == 4) EraName = "Phase 4: Phanerozoic";
  else EraName = "Phase 5: Anthropocene";

  glColor3f(0.92f, 0.86f, 0.40f);
  DrawText(18.0f, WindowHeight - 32.0f, "Epoch", GLUT_BITMAP_HELVETICA_18);
  
  glColor3f(0.85f, 0.85f, 0.85f);
  DrawText(18.0f, WindowHeight - 52.0f, EraName, GLUT_BITMAP_HELVETICA_12);
  
  glColor3f(0.60f, 0.65f, 0.72f);
  DrawText(18.0f, WindowHeight - 72.0f, "Selected God Tool:", GLUT_BITMAP_HELVETICA_10);

  for (int I = 0; I < 8; I++) {
    float Y = WindowHeight - 116.0f - (float)I * 45.0f;
    bool Selected = (SelectedTool == I);

    if (Selected) {
      float Pulse = (float)sin(ElapsedSeconds * 6.0f) * 0.15f + 0.75f;
      glColor4f(0.92f, 0.76f, 0.20f, Pulse);
      glBegin(GL_LINE_LOOP);
      glVertex2f(14.0f, Y - 23.0f);
      glVertex2f(172.0f, Y - 23.0f);
      glVertex2f(172.0f, Y + 13.0f);
      glVertex2f(14.0f, Y + 13.0f);
      glEnd();

      glColor4f(0.10f, 0.16f, 0.26f, 0.90f);
    } else {
      glColor4f(0.18f, 0.20f, 0.24f, 0.40f);
      glBegin(GL_LINE_LOOP);
      glVertex2f(16.0f, Y - 21.0f);
      glVertex2f(170.0f, Y - 21.0f);
      glVertex2f(170.0f, Y + 11.0f);
      glVertex2f(16.0f, Y + 11.0f);
      glEnd();

      glColor4f(0.06f, 0.08f, 0.10f, 0.55f);
    }

    glBegin(GL_QUADS);
    glVertex2f(16.0f, Y - 21.0f);
    glVertex2f(170.0f, Y - 21.0f);
    glVertex2f(170.0f, Y + 11.0f);
    glVertex2f(16.0f, Y + 11.0f);
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
      glColor3f(0.88f, 0.30f, 0.30f);
    }
    glBegin(GL_QUADS);
    glVertex2f(25.0f, Y - 13.0f);
    glVertex2f(47.0f, Y - 13.0f);
    glVertex2f(47.0f, Y + 5.0f);
    glVertex2f(25.0f, Y + 5.0f);
    glEnd();

    glColor3f(0.90f, 0.90f, 0.90f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(25.0f, Y - 13.0f);
    glVertex2f(47.0f, Y - 13.0f);
    glVertex2f(47.0f, Y + 5.0f);
    glVertex2f(25.0f, Y + 5.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(33.0f, Y - 5.0f, FormatFloat((float)(I + 1), 0), GLUT_BITMAP_HELVETICA_10);

    if (Selected) {
      glColor3f(1.0f, 0.92f, 0.50f);
    } else {
      glColor3f(0.80f, 0.82f, 0.86f);
    }
    DrawText(58.0f, Y - 4.0f, ToolName(I), GLUT_BITMAP_HELVETICA_12);
  }

  // Temperature Gauge
  glColor3f(0.80f, 0.82f, 0.86f);
  DrawText(18.0f, 235.0f, "Temperature", GLUT_BITMAP_HELVETICA_10);
  glBegin(GL_QUADS);
  glColor3f(0.15f, 0.40f, 0.85f);
  glVertex2f(18.0f, 220.0f);
  glColor3f(0.85f, 0.20f, 0.15f);
  glVertex2f(170.0f, 220.0f);
  glVertex2f(170.0f, 230.0f);
  glColor3f(0.15f, 0.40f, 0.85f);
  glVertex2f(18.0f, 230.0f);
  glEnd();
  glColor4f(0.30f, 0.35f, 0.40f, 0.60f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(18.0f, 220.0f);
  glVertex2f(170.0f, 220.0f);
  glVertex2f(170.0f, 230.0f);
  glVertex2f(18.0f, 230.0f);
  glEnd();
  glColor4f(1.0f, 1.0f, 1.0f, 0.60f);
  glBegin(GL_LINES);
  glVertex2f(94.0f, 218.0f);
  glVertex2f(94.0f, 232.0f);
  glEnd();
  float TempMarkerX = 94.0f + VisualTemperatureAxis * 76.0f;
  TempMarkerX = ClampValue(TempMarkerX, 18.0f, 170.0f);
  glColor3f(1.0f, 0.95f, 0.20f);
  glBegin(GL_QUADS);
  glVertex2f(TempMarkerX - 3.0f, 216.0f);
  glVertex2f(TempMarkerX + 3.0f, 216.0f);
  glVertex2f(TempMarkerX + 3.0f, 234.0f);
  glVertex2f(TempMarkerX - 3.0f, 234.0f);
  glEnd();

  // Atmosphere Gauge
  glColor3f(0.80f, 0.82f, 0.86f);
  DrawText(18.0f, 175.0f, "Atmosphere", GLUT_BITMAP_HELVETICA_10);
  glBegin(GL_QUADS);
  glColor3f(0.20f, 0.22f, 0.25f);
  glVertex2f(18.0f, 160.0f);
  glColor3f(0.50f, 0.80f, 0.95f);
  glVertex2f(170.0f, 160.0f);
  glVertex2f(170.0f, 170.0f);
  glColor3f(0.20f, 0.22f, 0.25f);
  glVertex2f(18.0f, 170.0f);
  glEnd();
  glColor4f(0.30f, 0.35f, 0.40f, 0.60f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(18.0f, 160.0f);
  glVertex2f(170.0f, 160.0f);
  glVertex2f(170.0f, 170.0f);
  glVertex2f(18.0f, 170.0f);
  glEnd();
  glColor4f(1.0f, 1.0f, 1.0f, 0.60f);
  glBegin(GL_LINES);
  glVertex2f(94.0f, 158.0f);
  glVertex2f(94.0f, 172.0f);
  glEnd();
  float AtmosMarkerX = 94.0f + VisualAtmosphereAxis * 76.0f;
  AtmosMarkerX = ClampValue(AtmosMarkerX, 18.0f, 170.0f);
  glColor3f(1.0f, 0.95f, 0.20f);
  glBegin(GL_QUADS);
  glVertex2f(AtmosMarkerX - 3.0f, 158.0f);
  glVertex2f(AtmosMarkerX + 3.0f, 158.0f);
  glVertex2f(AtmosMarkerX + 3.0f, 172.0f);
  glVertex2f(AtmosMarkerX - 3.0f, 172.0f);
  glEnd();

  int TotalCamps = 0;
  int TotalWildlife = 0;
  int TotalMicrobes = 0;
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    if (PlanetFaces[I].EcosystemLevel == 3) {
      TotalCamps++;
    } else if (PlanetFaces[I].EcosystemLevel == 2) {
      TotalWildlife++;
    } else if (PlanetFaces[I].EcosystemLevel == 1) {
      TotalMicrobes++;
    }
  }

  glColor3f(0.60f, 0.65f, 0.72f);
  DrawText(18.0f, 134.0f, "Ecosystem Status:", GLUT_BITMAP_HELVETICA_10);
  if (TotalCamps > 0) {
    glColor3f(0.30f, 0.85f, 0.40f);
    DrawText(18.0f, 116.0f, "Tribal Camps: " + FormatFloat((float)TotalCamps, 0), GLUT_BITMAP_HELVETICA_12);
  } else if (TotalWildlife > 0) {
    glColor3f(0.35f, 0.75f, 0.90f);
    DrawText(18.0f, 116.0f, "Wildlife Roaming", GLUT_BITMAP_HELVETICA_12);
  } else if (TotalMicrobes > 0) {
    glColor3f(0.55f, 0.85f, 0.60f);
    DrawText(18.0f, 116.0f, "Microbial Algae", GLUT_BITMAP_HELVETICA_12);
  } else {
    glColor3f(0.85f, 0.35f, 0.35f);
    DrawText(18.0f, 116.0f, "Sterile / Dead", GLUT_BITMAP_HELVETICA_12);
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

  // Radar Indicator HUD
  float Cx = (float)WindowWidth - 118.0f;
  float Cy = 130.0f;
  glPushMatrix();
  glTranslatef(Cx, Cy, 0.0f);

  // Glass backing circle
  glColor4f(0.04f, 0.05f, 0.08f, 0.76f);
  glBegin(GL_POLYGON);
  for (int I = 0; I < 64; I++) {
    float A = (float)I / 64.0f * Pi * 2.0f;
    glVertex2f((float)cos(A) * 86.0f, (float)sin(A) * 86.0f);
  }
  glEnd();

  // Glass border accent
  glColor4f(0.32f, 0.40f, 0.50f, 0.40f);
  glBegin(GL_LINE_LOOP);
  for (int I = 0; I < 64; I++) {
    float A = (float)I / 64.0f * Pi * 2.0f;
    glVertex2f((float)cos(A) * 86.0f, (float)sin(A) * 86.0f);
  }
  glEnd();

  // Concentric color loops
  for (int R = 0; R < 3; R++) {
    float Radius = 25.0f + (float)R * 25.0f;
    if (R == 0) glColor4f(0.20f, 0.85f, 0.35f, 0.65f);      // Inner/Safe (Green)
    else if (R == 1) glColor4f(0.92f, 0.64f, 0.18f, 0.65f); // Mid/Warning (Yellow)
    else glColor4f(0.90f, 0.22f, 0.16f, 0.65f);             // Outer/Danger (Red)

    glBegin(GL_LINE_LOOP);
    for (int I = 0; I < 96; I++) {
      float A = (float)I / 96.0f * Pi * 2.0f;
      glVertex2f((float)cos(A) * Radius, (float)sin(A) * Radius);
    }
    glEnd();
  }

  // Crosshairs
  glColor4f(0.32f, 0.40f, 0.50f, 0.25f);
  glBegin(GL_LINES);
  glVertex2f(-82.0f, 0.0f);
  glVertex2f(82.0f, 0.0f);
  glVertex2f(0.0f, -82.0f);
  glVertex2f(0.0f, 82.0f);
  glEnd();

  // Indicator Dot using Visual axes
  float DotX = ClampValue(VisualTemperatureAxis, -1.0f, 1.0f) * 75.0f;
  float DotY = ClampValue(VisualAtmosphereAxis, -1.0f, 1.0f) * 75.0f;

  // Pulsing halo glow
  float PulsingHaloRadius = 12.0f + (float)sin(ElapsedSeconds * 8.0f) * 3.0f;
  glColor4f(0.90f, 0.18f, 0.16f, 0.24f);
  glBegin(GL_POLYGON);
  for (int I = 0; I < 32; I++) {
    float A = (float)I / 32.0f * Pi * 2.0f;
    glVertex2f(DotX + (float)cos(A) * PulsingHaloRadius, DotY + (float)sin(A) * PulsingHaloRadius);
  }
  glEnd();

  // Solid dot core
  glColor3f(1.0f, 0.25f, 0.20f);
  glBegin(GL_POLYGON);
  for (int I = 0; I < 32; I++) {
    float A = (float)I / 32.0f * Pi * 2.0f;
    glVertex2f(DotX + (float)cos(A) * 6.0f, DotY + (float)sin(A) * 6.0f);
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
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight - 76.0f,
                     "ALERT: " + Name, GLUT_BITMAP_HELVETICA_18);
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight - 102.0f,
                     "Stabilize balance quickly", GLUT_BITMAP_HELVETICA_12);
  } else if (AlertSeconds > 0.0f) {
    glColor3f(1.0f, 0.80f, 0.18f);
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight - 76.0f,
                     "EXTREME EVENT IMPACT", GLUT_BITMAP_HELVETICA_18);
  }

  // Phase Banner HUD
  if (PhaseBannerSeconds > 0.0f && !GameOver) {
    float BannerAlpha = ClampValue(PhaseBannerSeconds, 0.0f, 1.0f);
    float HalfWidth = 320.0f;
    float HalfHeight = 60.0f;
    float MidX = (float)WindowWidth * 0.5f;
    float MidY = (float)WindowHeight * 0.5f;

    // Glass panel background
    glColor4f(0.04f, 0.05f, 0.08f, 0.85f * BannerAlpha);
    glBegin(GL_QUADS);
    glVertex2f(MidX - HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY + HalfHeight);
    glVertex2f(MidX - HalfWidth, MidY + HalfHeight);
    glEnd();

    // Golden border
    glColor4f(1.0f, 0.85f, 0.20f, 0.70f * BannerAlpha);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(MidX - HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY + HalfHeight);
    glVertex2f(MidX - HalfWidth, MidY + HalfHeight);
    glEnd();
    glLineWidth(1.0f);

    std::string BannerTitle = "";
    std::string BannerLine1 = "";
    std::string BannerLine2 = "";

    if (CurrentPhase == 1) {
      BannerTitle = "PHASE 1: HADEAN ERA (0.0B - 1.0B Years)";
      BannerLine1 = "Challenge: Runaway heat drift and thinning atmosphere.";
      BannerLine2 = "Heal the planet from high-frequency meteor bombardments!";
    } else if (CurrentPhase == 2) {
      BannerTitle = "PHASE 2: ARCHEAN ERA (1.0B - 2.0B Years)";
      BannerLine1 = "Challenge: Aggressive atmospheric buildup from volcanic outgassing.";
      BannerLine2 = "Cool and reduce pressure using Rain and Ice tools!";
    } else if (CurrentPhase == 3) {
      BannerTitle = "PHASE 3: PROTEROZOIC ERA (2.0B - 3.0B Years)";
      BannerLine1 = "Challenge: Snowball Earth. Aggressive temperature collapse.";
      BannerLine2 = "Prevent deep freeze and high frequency Ice Ages!";
    } else if (CurrentPhase == 4) {
      BannerTitle = "PHASE 4: PHANEROZOIC ERA (3.0B - 4.0B Years)";
      BannerLine1 = "Challenge: Biological bloom with high climate volatility.";
      BannerLine2 = "Balance temperature and atmosphere for complex life.";
    } else {
      BannerTitle = "PHASE 5: ANTHROPOCENE ERA (4.0B - 5.0B Years)";
      BannerLine1 = "Challenge: Final Criticality. Industrial depletion & solar heating.";
      BannerLine2 = "Can you survive the ultimate phase and establish a Golden Era?";
    }

    glColor4f(1.0f, 0.85f, 0.20f, BannerAlpha);
    DrawCenteredText(MidX, MidY + 24.0f, BannerTitle, GLUT_BITMAP_HELVETICA_18);

    glColor4f(0.90f, 0.90f, 0.90f, BannerAlpha);
    DrawCenteredText(MidX, MidY - 4.0f, BannerLine1, GLUT_BITMAP_HELVETICA_12);
    DrawCenteredText(MidX, MidY - 26.0f, BannerLine2, GLUT_BITMAP_HELVETICA_12);
  }

  // Game Over overlay screen
  if (GameOver) {
    float HalfWidth = 280.0f;
    float HalfHeight = 75.0f;
    float MidX = (float)WindowWidth * 0.5f;
    float MidY = (float)WindowHeight * 0.5f;

    std::string Title = "";
    std::string Line1 = "";
    std::string Line2 = "";
    float TitleR = 1.0f, TitleG = 1.0f, TitleB = 1.0f;

    if (GameWonStatus == 2) {
      Title = "PLANET HABITABLE: GOLDEN ERA";
      Line1 = "5 billion years of survival. Life flourishes in";
      Line2 = "perfect, glorious, and permanent equilibrium!";
      TitleR = 0.2f; TitleG = 0.85f; TitleB = 0.35f;
    } else if (GameWonStatus == 1) {
      Title = "PLANET STABILIZED: HARSH LIFE";
      Line1 = "The biosphere survived, but conditions are extremely volatile.";
      Line2 = "Survival remains a difficult, day-to-day struggle.";
      TitleR = 0.92f; TitleG = 0.64f; TitleB = 0.18f;
    } else { // GameWonStatus == 0
      TitleR = 0.90f; TitleG = 0.22f; TitleB = 0.16f;
      if (VisualTemperatureAxis > 0.5f) {
        Title = "PLANET COLLAPSED: LIVING HELL";
        Line1 = "Runaway greenhouse effect boiled the oceans.";
        Line2 = "The planet is a scorched, boiling magma wasteland.";
      } else if (VisualTemperatureAxis < -0.5f) {
        Title = "PLANET COLLAPSED: FROZEN WASTELAND";
        Line1 = "Extreme glaciation froze the entire crust.";
        Line2 = "The planet is a completely dead, frozen snowball.";
      } else if (VisualAtmosphereAxis < -0.4f) {
        Title = "PLANET COLLAPSED: BARREN VACUUM";
        Line1 = "The atmosphere was completely stripped away.";
        Line2 = "Solar winds have left the planetary surface sterile.";
      } else if (VisualAtmosphereAxis > 0.4f) {
        Title = "PLANET COLLAPSED: TOXIC SMOG";
        Line1 = "Extreme atmospheric pressure crushed all life";
        Line2 = "under a dense, choking blanket of poisonous gas.";
      } else {
        Title = "PLANET COLLAPSED: DEAD ZONE";
        Line1 = "The climate balance was completely lost.";
        Line2 = "The ecosystem collapsed into an uninhabitable void.";
      }
    }

    // Panel Background
    glColor4f(0.04f, 0.05f, 0.08f, 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(MidX - HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY + HalfHeight);
    glVertex2f(MidX - HalfWidth, MidY + HalfHeight);
    glEnd();

    // Border Accent
    glColor4f(TitleR, TitleG, TitleB, 0.60f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(MidX - HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY - HalfHeight);
    glVertex2f(MidX + HalfWidth, MidY + HalfHeight);
    glVertex2f(MidX - HalfWidth, MidY + HalfHeight);
    glEnd();
    glLineWidth(1.0f);

    glColor3f(TitleR, TitleG, TitleB);
    DrawCenteredText(MidX, MidY + 34.0f, Title, GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.90f, 0.90f, 0.90f);
    DrawCenteredText(MidX, MidY + 4.0f, Line1, GLUT_BITMAP_HELVETICA_12);
    DrawCenteredText(MidX, MidY - 18.0f, Line2, GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.60f, 0.65f, 0.72f);
    DrawCenteredText(MidX, MidY - 48.0f, "Press R to restart", GLUT_BITMAP_HELVETICA_10);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}
