#ifndef PlanetH
#define PlanetH

#include "GameTypes.h"
#include <vector>

float BiomeHeight(int Biome);
int MatureBiomeForFace(Face &F, float Progress);
void ApplyMatureBiome(Face &F, int Target);
float GetGeographyNoise(Vector3 Center);
int GetOrCreateUniqueVertex(Vector3 V);
void AddSubdividedFace(std::vector<Face> &Faces, Vector3 A, Vector3 B, Vector3 C, int Depth);
void GeneratePlanet();
Vector3 WarpedVertex(Vector3 Base, int VertIdx);
void UpdateFaceGeometry(Face &F);
void UpdateAllGeometry();
void AffectNeighbors(int FaceIndex, float Radius, int Biome, int TreeChange, float HeightChange, bool LockFaces);

#endif
