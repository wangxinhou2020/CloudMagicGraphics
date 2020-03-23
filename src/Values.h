#ifndef VALUESH
#define VALUESH

#define MY_UINT64_T     uint64_t
#define VIEW_WIDTH      640
#define VIEW_HEIGHT     480
#define RAY_CAST_DESITY 0.25
static const float kEpsilon = 1e-8; 

/*
#define LIGHT_NUM_MAX 3
#define OBJ_NUM_MAX   5
#define V_RES_MAX     3000
#define H_RES_MAX     3000
*/

#define OVERSTACK_PROTECT_DEPTH 9 
//#define INTENSITY_TOO_WEAK   0.01*0.01
#define INTENSITY_TOO_WEAK   0.001*0.001
const float kInfinity = std::numeric_limits<float>::max();

enum ObjectType { OBJECT_TYPE_NONE, OBJECT_TYPE_MESH, OBJECT_TYPE_SPHERE };
enum MaterialType { DIFFUSE_AND_GLOSSY, REFLECTION_AND_REFRACTION, REFLECTION };
enum RayStatus { VALID_RAY, NOHIT_RAY, INVISIBLE_RAY, OVERFLOW_RAY };
enum RayType { RAY_TYPE_ORIG, RAY_TYPE_REFLECTION, RAY_TYPE_REFRACTION, RAY_TYPE_DIFFUSE };
char RayTypeString[10][20] = {"orig", "reflect", "refract", "diffuse"};
#endif
