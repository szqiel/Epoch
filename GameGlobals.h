#ifndef GameGlobalsH
#define GameGlobalsH

#include "GameTypes.h"
#include <vector>
#include <GL/glut.h>

extern std::vector<Face> PlanetFaces;
extern std::vector<Particle> Particles;
extern std::vector<Meteor> Meteors;

extern std::vector<Vector3> UniqueVertices;
extern std::vector<float> VertexHeights;
extern int HoveredFace;

extern int WindowWidth;
extern int WindowHeight;
extern int SelectedTool;
extern int DisasterKind;
extern int DisasterTargetFace;

extern float CameraYaw;
extern float CameraPitch;
extern float CameraDistance;
extern float ElapsedSeconds;
extern float LastTickSeconds;
extern float NextDisasterSeconds;
extern float DisasterCountdown;
extern float AlertSeconds;
extern float TemperatureAxis;
extern float AtmosphereAxis;
extern float HeatPressure;
extern float AtmospherePressure;
extern float CloudRotation;
extern float TreePulse;

extern bool MouseIsDown;
extern bool MouseIsDragging;
extern bool GameOver;
extern bool GameWon;
extern bool GamePaused;

extern int LastMouseX;
extern int LastMouseY;
extern int StartMouseX;
extern int StartMouseY;

extern GLuint TreeList;
extern GLuint CloudList;
extern GLuint MeteorList;
extern GLuint UiCircleList;
extern GLuint VolcanoList;
extern GLuint MountainList;
extern GLuint MountainSnowList;
extern GLuint IceClusterList;

extern GLdouble PickModelMatrix[16];
extern GLdouble PickProjectionMatrix[16];
extern GLint PickViewport[4];

#endif
