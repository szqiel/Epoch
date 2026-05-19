#ifndef GameTypesH
#define GameTypesH

struct Vector3
{
    float X;
    float Y;
    float Z;
};

struct Face
{
    Vector3 BaseA;
    Vector3 BaseB;
    Vector3 BaseC;
    Vector3 CurrentA;
    Vector3 CurrentB;
    Vector3 CurrentC;
    Vector3 Normal;
    Vector3 Center;
    float Height;
    float Phase;
    float Seed;
    int Biome;
    int TreeCount;
    bool Locked;
};

struct Particle
{
    Vector3 Position;
    Vector3 Velocity;
    float Life;
    int Kind;
};

struct Meteor
{
    Vector3 Position;
    Vector3 Velocity;
    int TargetFace;
    bool Active;
    bool PlayerMade;
};

enum BiomeType
{
    BiomeOcean = 0,
    BiomeLand = 1,
    BiomeMountain = 2,
    BiomeIce = 3,
    BiomeForest = 4,
    BiomeMagma = 5,
    BiomeScorched = 6
};

enum ToolType
{
    ToolTree = 0,
    ToolRain = 1,
    ToolIce = 2,
    ToolFire = 3,
    ToolMountain = 4,
    ToolWater = 5,
    ToolMeteor = 6,
    ToolRemove = 7
};

enum DisasterType
{
    DisasterNone = 0,
    DisasterMeteor = 1,
    DisasterIceAge = 2
};

#endif
