#ifndef RendererH
#define RendererH

#include "GameTypes.h"

void SetMaterial(float R, float G, float B, bool Shiny, bool Glow);
void SetBiomeMaterial(int Biome, int FaceIndex, float Altitude, float Height);
void DrawLowPolyCone(float Radius, float Height, int Sides);
void DrawBlock(float Width, float Height, float Depth);
void BuildDisplayLists();
void DrawPlanet();
bool ShouldPlaceSurfaceAsset(Face &F, float Threshold, float Scale);
void DrawSurfaceObjects();
void DrawClouds();
void DrawMeteors();
void DrawParticles();
void DrawAtmosphereRing();
void DrawHolographicRing();
void DrawOverlay();
extern GLuint LowPolyTextureId;
void GenerateLowPolyTexture();

#endif
