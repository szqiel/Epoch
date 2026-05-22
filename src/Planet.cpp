#include "Planet.h"
#include "MathUtils.h"
#include "GameGlobals.h"
#include "GameConfig.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

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
  float OceanField = GetGeographyNoise(Center);
  float RidgeField = (float)fabs(
      (float)sin(Center.X * 2.20f + Center.Y * 1.10f - Center.Z * 1.80f));
  float Polar = (float)fabs(Center.Y);
  if (Progress > 0.76f && Polar > 0.78f) {
    return BiomeIce;
  }
  if (OceanField < -0.12f) {
    return BiomeOcean;
  }
  if (OceanField > 0.22f && RidgeField > 0.72f) {
    return BiomeMountain;
  }
  if (Progress > 0.52f && OceanField > 0.05f && RidgeField < 0.65f) {
    return BiomeForest;
  }
  return BiomeLand;
}

void ApplyMatureBiome(Face &F, int Target) {
  F.Biome = Target;
  if (Target == BiomeOcean) {
    F.Height = -0.12f + F.Seed * 0.01f;
    F.TreeCount = 0;
  } else if (Target == BiomeLand) {
    F.Height = 0.02f + F.Seed * 0.015f;
    F.TreeCount = 0;
  } else if (Target == BiomeForest) {
    F.Height = 0.03f + F.Seed * 0.01f;
    F.TreeCount = 0;
  } else if (Target == BiomeMountain) {
    F.Height = 0.10f + F.Seed * 0.04f;
    F.TreeCount = 0;
  } else if (Target == BiomeIce) {
    F.Height = 0.04f + F.Seed * 0.015f;
    F.TreeCount = 0;
  }
}

float GetGeographyNoise(Vector3 Center) {
  float LowFreq = ((float)sin(Center.X * 1.8f + Center.Z * 1.4f + 0.6f) +
                   (float)cos(Center.Y * 1.6f - Center.X * 1.5f) +
                   (float)sin((Center.X + Center.Y - Center.Z) * 1.4f)) / 3.0f;
  float HighFreq = ((float)sin(Center.X * 5.2f + Center.Y * 4.2f) +
                    (float)cos(Center.Z * 4.8f - Center.X * 3.6f)) / 2.0f;
  return LowFreq * 0.70f + HighFreq * 0.30f;
}

int GetOrCreateUniqueVertex(Vector3 V) {
  for (size_t I = 0; I < UniqueVertices.size(); I++) {
    if (sqrt((UniqueVertices[I].X - V.X)*(UniqueVertices[I].X - V.X) +
             (UniqueVertices[I].Y - V.Y)*(UniqueVertices[I].Y - V.Y) +
             (UniqueVertices[I].Z - V.Z)*(UniqueVertices[I].Z - V.Z)) < 0.001f) {
      return (int)I;
    }
  }
  UniqueVertices.push_back(V);
  VertexHeights.push_back(0.0f);
  return (int)(UniqueVertices.size() - 1);
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
    NewFace.VertIdxA = GetOrCreateUniqueVertex(NewFace.BaseA);
    NewFace.VertIdxB = GetOrCreateUniqueVertex(NewFace.BaseB);
    NewFace.VertIdxC = GetOrCreateUniqueVertex(NewFace.BaseC);
    NewFace.Phase = Noise * Pi * 12.0f;
    NewFace.Seed = Noise;
    NewFace.TreeCount = 0;
    NewFace.EcosystemLevel = 0;
    NewFace.Locked = false;
    float PlateField = GetGeographyNoise(Center);
    float RidgeField = (float)fabs(
        (float)sin(Center.X * 2.10f + Center.Y * 1.25f - Center.Z * 1.70f));
    float Score = PlateField + (Noise - 0.5f) * 0.10f;
    if (Score > 0.22f && RidgeField > 0.72f) {
      NewFace.Biome = BiomeMountain;
      NewFace.Height = 0.10f + Noise * 0.03f;
    } else if (Score > -0.06f) {
      NewFace.Biome = BiomeScorched;
      NewFace.Height = 0.02f + Noise * 0.01f;
    } else {
      NewFace.Biome = BiomeMagma;
      NewFace.Height = -0.12f + Noise * 0.01f;
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
  UniqueVertices.clear();
  VertexHeights.clear();
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

Vector3 WarpedVertex(Vector3 Base, int VertIdx) {
  float Time = ElapsedSeconds;
  float Noise = VertexNoise(Base);
  float Phase = Noise * Pi * 12.0f;
  float Drift = (float)sin(Time * 0.050f + Phase) * 0.005f;
  Vector3 Shift = MakeVector((float)sin(Phase * 1.37f + Base.Z) * Drift,
                             (float)cos(Phase * 0.91f + Base.X) * Drift,
                             (float)sin(Phase * 2.11f + Base.Y) * Drift);
  Vector3 Direction = NormalizeVector(AddVector(Base, Shift));
  float Ridge =
      (float)sin(Base.X * 9.0f + Base.Y * 4.5f - Base.Z * 6.5f) * 0.01f;
  float Pulse = (float)sin(Time * 0.12f + Phase * 0.7f) * 0.004f;
  float HeightOffset = (VertIdx >= 0 && VertIdx < (int)VertexHeights.size()) ? VertexHeights[VertIdx] : 0.0f;
  float Radius = PlanetRadius + (Noise - 0.5f) * 0.04f + Ridge + Pulse + HeightOffset;
  return ScaleVector(Direction, Radius);
}

void UpdateFaceGeometry(Face &F) {
  F.CurrentA = WarpedVertex(F.BaseA, F.VertIdxA);
  F.CurrentB = WarpedVertex(F.BaseB, F.VertIdxB);
  F.CurrentC = WarpedVertex(F.BaseC, F.VertIdxC);
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
  if (UniqueVertices.empty()) return;
  std::vector<float> AccumHeights(UniqueVertices.size(), 0.0f);
  std::vector<int> ShareCount(UniqueVertices.size(), 0);
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];
    AccumHeights[F.VertIdxA] += F.Height;
    ShareCount[F.VertIdxA]++;
    AccumHeights[F.VertIdxB] += F.Height;
    ShareCount[F.VertIdxB]++;
    AccumHeights[F.VertIdxC] += F.Height;
    ShareCount[F.VertIdxC]++;
  }
  for (size_t I = 0; I < UniqueVertices.size(); I++) {
    if (ShareCount[I] > 0) {
      VertexHeights[I] = AccumHeights[I] / (float)ShareCount[I];
    }
  }
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    UpdateFaceGeometry(PlanetFaces[I]);
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
      PlanetFaces[I].Height = ClampValue(PlanetFaces[I].Height, -0.15f, 0.20f);
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
