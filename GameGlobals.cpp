#include "GameGlobals.h"
#include "GameConfig.h"

std::vector<Face> PlanetFaces;
std::vector<Particle> Particles;
std::vector<Meteor> Meteors;

std::vector<Vector3> UniqueVertices;
std::vector<float> VertexHeights;
int HoveredFace = -1;

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
GLuint MountainSnowList = 0;
GLuint IceClusterList = 0;

GLdouble PickModelMatrix[16];
GLdouble PickProjectionMatrix[16];
GLint PickViewport[4];
