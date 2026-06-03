#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
#include <string>

#include "MathUtils.h"
#include "GameGlobals.h"
#include "GameConfig.h"
#include "Planet.h"
#include "Renderer.h"
#include "SoundSystem.h"

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
  
  float SunAngle = ElapsedSeconds * (Pi * 2.0f / 120.0f);
  GLfloat LightPosition[4] = {7.0f * (float)cos(SunAngle), 3.5f, 7.0f * (float)sin(SunAngle), 0.0f};
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
  DrawHolographicRing();
  DrawAtmosphereRing();
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

void MeteorImpact(int FaceIndex, bool PlayerMade) {
  if (FaceIndex < 0 || FaceIndex >= (int)PlanetFaces.size()) {
    return;
  }
  AffectNeighbors(FaceIndex, PlayerMade ? 0.34f : 0.50f, BiomeScorched, -1,
                  -0.03f, true);
  PlanetFaces[FaceIndex].Biome = BiomeMagma;
  PlanetFaces[FaceIndex].Height -= 0.05f;
  SpawnParticlesOnFace(FaceIndex, 2, 38);
  PlaySfx(2);
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
    PlaySfx(1);
  } else if (SelectedTool == ToolRain) {
    bool WasHot = (F.Biome == BiomeMagma || F.Biome == BiomeScorched);
    AffectNeighbors(FaceIndex, 0.23f, BiomeLand, 0, 0.005f, true);
    SpawnParticlesOnFace(FaceIndex, WasHot ? 3 : 0, 48);
    HeatPressure -= 0.10f;
    PlaySfx(3);
  } else if (SelectedTool == ToolIce) {
    bool WasHot = (F.Biome == BiomeMagma || F.Biome == BiomeScorched);
    AffectNeighbors(FaceIndex, 0.23f, BiomeIce, 0, 0.01f, true);
    SpawnParticlesOnFace(FaceIndex, WasHot ? 3 : 1, 50);
    HeatPressure -= 0.20f;
    PlaySfx(1);
  } else if (SelectedTool == ToolFire) {
    AffectNeighbors(FaceIndex, 0.24f, BiomeScorched, -1, -0.005f, true);
    SpawnParticlesOnFace(FaceIndex, 2, 28);
    HeatPressure += 0.10f;
    PlaySfx(2);
  } else if (SelectedTool == ToolMountain) {
    AffectNeighbors(FaceIndex, 0.19f, BiomeMountain, 0, 0.035f, true);
    PlaySfx(2);
  } else if (SelectedTool == ToolWater) {
    AffectNeighbors(FaceIndex, 0.21f, BiomeOcean, -1, -0.025f, true);
    SpawnParticlesOnFace(FaceIndex, 0, 28);
    PlaySfx(3);
  } else if (SelectedTool == ToolMeteor) {
    SpawnMeteorToFace(FaceIndex, true);
    PlaySfx(1);
  } else if (SelectedTool == ToolRemove) {
    F.TreeCount = 0;
    F.EcosystemLevel = 0;
    if (F.Biome == BiomeForest) {
      F.Biome = BiomeLand;
    }
    F.Locked = true;
    AtmospherePressure -= 0.12f;
    PlaySfx(1);
  }
  HeatPressure = ClampValue(HeatPressure, -1.1f, 1.1f);
  AtmospherePressure = ClampValue(AtmospherePressure, -1.1f, 1.1f);
}

void TriggerIceAge() {
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Vector3 NormCenter = NormalizeVector(PlanetFaces[I].Center);
    float IceField = GetGeographyNoise(NormCenter);
    float Polar = (float)fabs(NormCenter.Y);
    if (IceField > -0.05f || Polar > 0.55f) {
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
  int CurrentPhase = 1 + (int)(ElapsedSeconds / 60.0f);
  if (CurrentPhase > 5) CurrentPhase = 5;

  if (CurrentPhase == 1 || CurrentPhase == 2) {
    DisasterKind = DisasterMeteor;
  } else if (CurrentPhase == 3) {
    DisasterKind = DisasterIceAge;
  } else {
    DisasterKind = RandomUnit() < 0.55f ? DisasterMeteor : DisasterIceAge;
  }

  // Warning countdown speed scales up in later phases
  DisasterCountdown = (CurrentPhase >= 4) ? 5.5f : 8.0f;
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

  // Phase-dependent interval scaling
  float MinInt = 95.0f;
  float RangeInt = 45.0f;
  int CurrentPhase = 1 + (int)(ElapsedSeconds / 60.0f);
  if (CurrentPhase > 5) CurrentPhase = 5;

  if (CurrentPhase == 1) {
    MinInt = 12.0f;
    RangeInt = 6.0f;
  } else if (CurrentPhase == 2) {
    MinInt = 20.0f;
    RangeInt = 10.0f;
  } else if (CurrentPhase == 3) {
    MinInt = 24.0f;
    RangeInt = 10.0f;
  } else if (CurrentPhase == 4) {
    MinInt = 22.0f;
    RangeInt = 10.0f;
  } else if (CurrentPhase == 5) {
    MinInt = 18.0f;
    RangeInt = 8.0f;
  }
  NextDisasterSeconds = ElapsedSeconds + MinInt + RandomUnit() * RangeInt;
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

  // Apply Phase Thermodynamic Drift
  int CurrentPhase = 1 + (int)(ElapsedSeconds / 60.0f);
  if (CurrentPhase > 5) CurrentPhase = 5;

  float HeatDrift = 0.0f;
  float AtmosDrift = 0.0f;
  if (CurrentPhase == 1) {
    HeatDrift = 0.12f;
    AtmosDrift = -0.15f;
  } else if (CurrentPhase == 2) {
    HeatDrift = 0.04f;
    AtmosDrift = 0.18f;
  } else if (CurrentPhase == 3) {
    HeatDrift = -0.15f;
    AtmosDrift = 0.05f;
  } else if (CurrentPhase == 5) {
    HeatDrift = 0.10f;
    AtmosDrift = -0.05f;
  }
  TemperatureAxis += HeatDrift * Delta;
  AtmosphereAxis += AtmosDrift * Delta;

  HeatPressure -= HeatPressure * 0.040f * Delta;
  AtmospherePressure -= AtmospherePressure * 0.035f * Delta;
  TemperatureAxis = ClampValue(TemperatureAxis, -1.15f, 1.15f);
  AtmosphereAxis = ClampValue(AtmosphereAxis, -1.15f, 1.15f);
  HeatPressure = ClampValue(HeatPressure, -1.1f, 1.1f);
  AtmospherePressure = ClampValue(AtmospherePressure, -1.1f, 1.1f);

  // Update Visual offsets: slowly heal towards 0 if indicators are inside the safe zone (radius 0.33),
  // otherwise accumulate extreme axis damage as persistent offset (scarring).
  if ((float)fabs(TemperatureAxis) <= 0.33f) {
    VisualTemperatureOffset -= VisualTemperatureOffset * 0.08f * Delta;
  } else {
    VisualTemperatureOffset += (TemperatureAxis - VisualTemperatureOffset) * 0.03f * Delta;
  }

  if ((float)fabs(AtmosphereAxis) <= 0.33f) {
    VisualAtmosphereOffset -= VisualAtmosphereOffset * 0.08f * Delta;
  } else {
    VisualAtmosphereOffset += (AtmosphereAxis - VisualAtmosphereOffset) * 0.03f * Delta;
  }

  VisualTemperatureAxis = ClampValue(TemperatureAxis + VisualTemperatureOffset, -1.15f, 1.15f);
  VisualAtmosphereAxis = ClampValue(AtmosphereAxis + VisualAtmosphereOffset, -1.15f, 1.15f);
}

void UpdateEvolution(float Delta) {
  float Progress = ClampValue(ElapsedSeconds / GameDuration, 0.0f, 1.0f);
  
  // Calculate magma/scorched coverage
  int MagmaCount = 0;
  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    if (PlanetFaces[I].Biome == BiomeMagma || PlanetFaces[I].Biome == BiomeScorched) {
      MagmaCount++;
    }
  }
  float MagmaFraction = PlanetFaces.empty() ? 0.0f : (float)MagmaCount / (float)PlanetFaces.size();
  bool HighMagmaCoverage = (MagmaFraction > 0.25f);
  
  // Global hostility checks (temperature or atmosphere offsets > 0.40, or high magma)
  bool HostileGlobal = (fabs(VisualTemperatureAxis) > 0.40f || 
                        fabs(VisualAtmosphereAxis) > 0.40f || 
                        HighMagmaCoverage);

  for (size_t I = 0; I < PlanetFaces.size(); I++) {
    Face &F = PlanetFaces[I];

    // Ice Melting Cycle
    if (F.Biome == BiomeIce && TemperatureAxis > -0.30f) {
      Vector3 NormCenter = NormalizeVector(F.Center);
      float Polar = (float)fabs(NormCenter.Y);
      if (Polar < 0.78f || TemperatureAxis > 0.25f) {
        F.Locked = false;
        int Target = MatureBiomeForFace(F, Progress);
        if (Target != BiomeIce) {
          ApplyMatureBiome(F, Target);
        } else {
          ApplyMatureBiome(F, BiomeLand);
        }
      }
    }

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

    // Ecosystem Evolution
    if (F.Biome == BiomeMagma || F.Biome == BiomeScorched || F.Biome == BiomeIce) {
      F.EcosystemLevel = 0;
    } else {
      float BalanceDistance = (float)sqrt(TemperatureAxis * TemperatureAxis +
                                          AtmosphereAxis * AtmosphereAxis);
      bool SafeZone = (BalanceDistance <= SafeBalanceRadius);

      if (SafeZone && !HostileGlobal) {
        float GrowChance = 0.005f * Delta;
        if (RandomUnit() < GrowChance) {
          if (F.EcosystemLevel == 0 && Progress >= 0.20f) { // Restrict newborn planet (Hadean, Phase 1) spawning
            if (F.Biome == BiomeOcean || F.Biome == BiomeLand || F.Biome == BiomeForest) {
              F.EcosystemLevel = 1;
            }
          } else if (F.EcosystemLevel == 1 && Progress >= 0.40f) {
            if (F.Biome == BiomeLand || F.Biome == BiomeForest) {
              F.EcosystemLevel = 2;
            }
          } else if (F.EcosystemLevel == 2 && Progress >= 0.70f) {
            if (F.Biome == BiomeForest || (F.Biome == BiomeLand && F.TreeCount > 0)) {
              F.EcosystemLevel = 3;
            }
          }
        }
      } else {
        float DecayChance = 0.12f * Delta;
        if (RandomUnit() < DecayChance) {
          if (F.EcosystemLevel > 0) {
            F.EcosystemLevel--;
          }
        }
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
    Vector3 Vel = Particles[I].Velocity;
    if (Particles[I].Kind == 1) {
      Vel.X += (float)sin(ElapsedSeconds * 4.0f + Particles[I].Life * 8.0f) * 0.12f;
      Vel.Z += (float)cos(ElapsedSeconds * 4.0f + Particles[I].Life * 8.0f) * 0.12f;
    } else if (Particles[I].Kind == 2) {
      Vel.X += (float)sin(Particles[I].Life * 12.0f) * 0.08f;
      Vel.Z += (float)cos(Particles[I].Life * 12.0f) * 0.08f;
    } else if (Particles[I].Kind == 3) {
      Vel = ScaleVector(Vel, 0.40f);
      Vel.Y += 0.28f;
      Vel.X += (float)sin(ElapsedSeconds * 2.0f + Particles[I].Life * 5.0f) * 0.05f;
      Vel.Z += (float)cos(ElapsedSeconds * 2.0f + Particles[I].Life * 5.0f) * 0.05f;
    }
    Particles[I].Position = AddVector(
        Particles[I].Position, ScaleVector(Vel, Delta));
    Particles[I].Life -= Delta;
  }
  
  // Ambient snow flurries when cold or during Ice Age
  if (DisasterKind == DisasterIceAge || TemperatureAxis < -0.30f) {
    if (RandomUnit() < 0.12f && !PlanetFaces.empty()) {
      int FaceIdx = rand() % (int)PlanetFaces.size();
      Face &F = PlanetFaces[FaceIdx];
      Particle P;
      Vector3 N = F.Normal;
      P.Position = AddVector(F.Center, ScaleVector(N, 1.4f + RandomUnit() * 0.6f));
      P.Velocity = ScaleVector(N, -0.45f - RandomUnit() * 0.20f);
      P.Life = 2.0f + RandomUnit() * 1.5f;
      P.Kind = 1;
      Particles.push_back(P);
    }
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
    if (BalanceDistance <= 0.33f) {
      GameWonStatus = 2; // Absolute Win (Green)
      GameWon = true;
    } else if (BalanceDistance <= SafeBalanceRadius) { // 0.66
      GameWonStatus = 1; // Unstable Habitability (Yellow)
      GameWon = true;
    } else {
      GameWonStatus = 0; // Collapsed (Red)
      GameWon = false;
    }
  }
}

void UpdateAudio() {
  if (GamePaused || GameOver) {
    SoundSystem::TargetVolume = 0.0f;
    return;
  }
  float BalanceDistance = (float)sqrt(VisualTemperatureAxis * VisualTemperatureAxis +
                                      VisualAtmosphereAxis * VisualAtmosphereAxis);
  bool SafeZone = (BalanceDistance <= SafeBalanceRadius);
  if (VisualTemperatureAxis > 0.40f) {
    SoundSystem::Waveform = 2;
    SoundSystem::TargetFrequency = 60.0f + VisualTemperatureAxis * 40.0f;
    SoundSystem::TargetVolume = 0.16f;
  } else if (VisualTemperatureAxis < -0.40f) {
    SoundSystem::Waveform = 1;
    SoundSystem::TargetFrequency = 800.0f - VisualTemperatureAxis * 300.0f;
    SoundSystem::TargetVolume = 0.08f;
  } else {
    SoundSystem::Waveform = 0;
    if (SafeZone) {
      SoundSystem::TargetFrequency = 220.0f + (float)sin(ElapsedSeconds * 0.4f) * 20.0f;
      SoundSystem::TargetVolume = 0.12f;
    } else {
      SoundSystem::TargetFrequency = 290.0f + (float)sin(ElapsedSeconds * 2.0f) * 30.0f;
      SoundSystem::TargetVolume = 0.10f;
    }
  }
}

void Timer(int Value) {
  float Now = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
  float Delta = Now - LastTickSeconds;
  LastTickSeconds = Now;
  if (Delta > 0.10f) {
    Delta = 0.10f;
  }
  
  if (InMainMenu) {
    ElapsedSeconds += Delta;
    CameraYaw += 7.0f * Delta;
    if (CameraYaw > 360.0f) CameraYaw -= 360.0f;
    SoundSystem::TargetVolume = 0.0f;
    UpdateObjects(Delta);
    glutPostRedisplay();
    glutTimerFunc(16, Timer, 0);
    return;
  }

  if (!GamePaused && !GameOver) {
    ElapsedSeconds += Delta;
    
    // Phase detection & transition trigger
    int CurrentPhase = 1 + (int)(ElapsedSeconds / 60.0f);
    if (CurrentPhase > 5) CurrentPhase = 5;
    if (CurrentPhase != LastPhase) {
      LastPhase = CurrentPhase;
      PhaseBannerSeconds = 5.0f;
      PlaySfx(1);
    }
    
    if (PhaseBannerSeconds > 0.0f) {
      PhaseBannerSeconds -= Delta;
    }

    UpdateEvolution(Delta);
    UpdatePlanetIndicator(Delta);
    if (ElapsedSeconds >= NextDisasterSeconds && DisasterKind == DisasterNone) {
      if (GameDuration - ElapsedSeconds > 18.0f) {
        BeginDisaster();
      }
    }
    if (DisasterCountdown > 0.0f) {
      DisasterCountdown -= Delta;
      if (DisasterCountdown <= 0.0f) {
        ResolveDisaster();
      }
    }
    CheckWinLose();
    UpdateAudio();
  } else {
    SoundSystem::TargetVolume = 0.0f;
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
  NextDisasterSeconds = 12.0f; // First disaster at 12.0s
  DisasterCountdown = 0.0f;
  AlertSeconds = 0.0f;
  TemperatureAxis = 0.24f;
  AtmosphereAxis = 0.20f;
  HeatPressure = 0.0f;
  AtmospherePressure = 0.0f;
  CloudRotation = 0.0f;
  
  VisualTemperatureAxis = 0.24f;
  VisualAtmosphereAxis = 0.20f;
  VisualTemperatureOffset = 0.0f;
  VisualAtmosphereOffset = 0.0f;
  PhaseBannerSeconds = 5.0f; // Show banner for phase 1 at start
  LastPhase = 0;
  GameWonStatus = 0;
  
  GameOver = false;
  GameWon = false;
  GamePaused = false;
  InMainMenu = false;
  UpdateAllGeometry();
}

void Keyboard(unsigned char Key, int X, int Y) {
  if (InMainMenu) {
    if (Key == 13 || Key == ' ' || Key == 's' || Key == 'S') {
      InMainMenu = false;
      ResetGame();
    }
    return;
  }
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
  if (InMainMenu) {
    if (Button == GLUT_LEFT_BUTTON && State == GLUT_DOWN) {
      InMainMenu = false;
      ResetGame();
    }
    return;
  }
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

void PassiveMotion(int X, int Y) {
  if (X <= 186) {
    HoveredFace = -1;
  } else {
    HoveredFace = PickFace(X, Y);
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
  GenerateLowPolyTexture();
  ResetGame();
  InMainMenu = true;
  SoundSystem::Start();
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
  glutPassiveMotionFunc(PassiveMotion);
  glutTimerFunc(16, Timer, 0);
  glutMainLoop();
  SoundSystem::Stop();
  return 0;
}
