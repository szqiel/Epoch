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

  if (Biome == BiomeOcean) {
    SetMaterial(0.11f + Var * 0.3f, 0.20f + Var * 0.4f, 0.34f + Var * 0.5f, true, false);
  } else if (Biome == BiomeLand) {
    SetMaterial(0.61f + Var * 0.5f, 0.40f + Var * 0.5f, 0.26f + Var * 0.4f, false, false);
  } else if (Biome == BiomeMountain) {
    float Progress = ElapsedSeconds / GameDuration;
    if (Progress > 0.35f && (Height > 0.062f || (float)fabs(Altitude) > 1.15f)) {
      SetMaterial(0.92f + Var * 0.08f, 0.94f + Var * 0.08f, 0.96f + Var * 0.08f, true, false);
    } else {
      SetMaterial(0.35f + Var * 0.3f, 0.36f + Var * 0.3f, 0.42f + Var * 0.3f, false, false);
    }
  } else if (Biome == BiomeIce) {
    SetMaterial(0.94f + Var * 0.1f, 0.98f + Var * 0.1f, 0.93f + Var * 0.1f, true, false);
  } else if (Biome == BiomeForest) {
    SetMaterial(0.15f + Var * 0.3f, 0.21f + Var * 0.4f, 0.09f + Var * 0.2f, false, false);
  } else if (Biome == BiomeMagma) {
    SetMaterial(0.88f + Var * 0.2f, 0.34f + Var * 0.3f, 0.13f + Var * 0.1f, false, true);
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

  if (TemperatureAxis > 0.35f) {
    float Factor = ClampValue((TemperatureAxis - 0.35f) / 0.65f, 0.0f, 1.0f);
    Red = 0.15f * (1.0f - Factor) + 0.90f * Factor;
    Green = 0.65f * (1.0f - Factor) + 0.35f * Factor;
    Blue = 0.95f * (1.0f - Factor) + 0.08f * Factor;
  } else if (TemperatureAxis < -0.35f) {
    float Factor = ClampValue((-TemperatureAxis - 0.35f) / 0.65f, 0.0f, 1.0f);
    Red = 0.15f * (1.0f - Factor) + 0.85f * Factor;
    Green = 0.65f * (1.0f - Factor) + 0.92f * Factor;
    Blue = 0.95f * (1.0f - Factor) + 1.00f * Factor;
  }

  float NormAtmos = (AtmosphereAxis + 1.15f) / 2.3f;
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

  glColor3f(0.92f, 0.86f, 0.40f);
  DrawText(18.0f, WindowHeight - 32.0f, "Epoch", GLUT_BITMAP_HELVETICA_18);
  glColor3f(0.60f, 0.65f, 0.72f);
  DrawText(18.0f, WindowHeight - 56.0f, "Selected God Tool:", GLUT_BITMAP_HELVETICA_10);

  for (int I = 0; I < 8; I++) {
    float Y = WindowHeight - 96.0f - (float)I * 48.0f;
    bool Selected = (SelectedTool == I);

    if (Selected) {
      float Pulse = (float)sin(ElapsedSeconds * 6.0f) * 0.15f + 0.75f;
      glColor4f(0.92f, 0.76f, 0.20f, Pulse);
      glBegin(GL_LINE_LOOP);
      glVertex2f(14.0f, Y - 26.0f);
      glVertex2f(172.0f, Y - 26.0f);
      glVertex2f(172.0f, Y + 14.0f);
      glVertex2f(14.0f, Y + 14.0f);
      glEnd();

      glColor4f(0.10f, 0.16f, 0.26f, 0.90f);
    } else {
      glColor4f(0.18f, 0.20f, 0.24f, 0.40f);
      glBegin(GL_LINE_LOOP);
      glVertex2f(16.0f, Y - 24.0f);
      glVertex2f(170.0f, Y - 24.0f);
      glVertex2f(170.0f, Y + 12.0f);
      glVertex2f(16.0f, Y + 12.0f);
      glEnd();

      glColor4f(0.06f, 0.08f, 0.10f, 0.55f);
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
      glColor3f(0.88f, 0.30f, 0.30f);
    }
    glBegin(GL_QUADS);
    glVertex2f(25.0f, Y - 15.0f);
    glVertex2f(47.0f, Y - 15.0f);
    glVertex2f(47.0f, Y + 7.0f);
    glVertex2f(25.0f, Y + 7.0f);
    glEnd();

    glColor3f(0.90f, 0.90f, 0.90f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(25.0f, Y - 15.0f);
    glVertex2f(47.0f, Y - 15.0f);
    glVertex2f(47.0f, Y + 7.0f);
    glVertex2f(25.0f, Y + 7.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(33.0f, Y - 3.0f, FormatFloat((float)(I + 1), 0), GLUT_BITMAP_HELVETICA_10);

    if (Selected) {
      glColor3f(1.0f, 0.92f, 0.50f);
    } else {
      glColor3f(0.80f, 0.82f, 0.86f);
    }
    DrawText(58.0f, Y - 2.0f, ToolName(I), GLUT_BITMAP_HELVETICA_12);
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
  float TempMarkerX = 94.0f + TemperatureAxis * 76.0f;
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
  float AtmosMarkerX = 94.0f + AtmosphereAxis * 76.0f;
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

  // Indicator Dot
  float DotX = ClampValue(TemperatureAxis, -1.0f, 1.0f) * 75.0f;
  float DotY = ClampValue(AtmosphereAxis, -1.0f, 1.0f) * 75.0f;

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
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight * 0.5f + 24.0f,
                     Title, GLUT_BITMAP_HELVETICA_18);
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight * 0.5f - 6.0f,
                     Body, GLUT_BITMAP_HELVETICA_12);
    DrawCenteredText((float)WindowWidth * 0.5f, (float)WindowHeight * 0.5f - 34.0f,
                     "Press R to restart", GLUT_BITMAP_HELVETICA_12);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}
