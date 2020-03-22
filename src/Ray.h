#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

class Ray
{
public:
    Ray(const RayType rayType, const Vec3f &orig, const Vec3f &dir, const Vec3f &leftIntensity = -1) : orig(orig), dir(dir){
        hitObject = nullptr;
        status = NOHIT_RAY;
        type   = rayType;
        intensity = leftIntensity;
        inside    = false;
        validCount = nohitCount = invisibleCount = overflowCount = weakCount = 0;
    }
    Vec3f orig;
    Vec3f dir;
    // Hitted object of current ray
    void *hitObject;
    Vec3f  hitPoint;
    // The ray is inside or outside the object
    bool   inside;
    Vec3f  intensity;
    // child reflection rays
    std::vector<std::unique_ptr<Ray>> reflectionLink;
    // child refraction rays
    std::vector<std::unique_ptr<Ray>> refractionLink;
    // child diffuse rays
    std::vector<std::unique_ptr<Ray>> diffuseLink;

    // status of current ray
    enum RayStatus status;
    // type of current ray
    enum RayType type;

    // Counter of valid child rays
    uint32_t validCount;
    // Counter of invalid rays as per nohit
    uint32_t nohitCount;
    // Counter of invisible diffuse reflection rays
    uint32_t invisibleCount;
    // Counter of invalid rays as per overflow
    uint32_t overflowCount;
    // Counter of too weak ingnored rays
    uint32_t weakCount;
};

