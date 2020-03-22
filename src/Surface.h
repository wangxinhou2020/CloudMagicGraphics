#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

// shade point on each object
class Surface {
public:
    Surface(const float surfaceAngleRatio) : angleRatio(surfaceAngleRatio) {
        /* caculate the hit angle refer to sphere Normal on the surface */
        /* each surface will cast rays into a half sphere space which express as theta[0,90),phi[0,360) */
        vAngleRes = (90.+1.)*angleRatio;
        hAngleRes = (360.)*angleRatio;
        if (angleRatio > 0.) {
            MY_UINT64_T size = (MY_UINT64_T)sizeof(SurfaceAngle)*vAngleRes*hAngleRes;
            angles = (SurfaceAngle *)malloc(size);
            std::memset(angles, 0, size);
        }
    }
    void reset(uint32_t index, Vec3f &normal, Vec3f center) {
        idx = index;
        N = normal;
        if (angleRatio > 0.0) {
            local2World = Matrix44f(center, center+N);
            world2Local = local2World.inverse();
        }
        MY_UINT64_T size = (MY_UINT64_T)sizeof(SurfaceAngle)*vAngleRes*hAngleRes;
        std::memset(angles, 0, size);
    }

    SurfaceAngle* getSurfaceAngleByVH(const uint32_t v, const uint32_t h, Vec3f * relPoint=nullptr) const
    {
        SurfaceAngle *angle = nullptr;
        if(angles == nullptr) return angle;
        angle = angles + v%vAngleRes*hAngleRes + h%hAngleRes;
        if (relPoint != nullptr) {
            float theta = deg2rad(v*90.0/vAngleRes);
            float phi = deg2rad(h*360.0/hAngleRes);
            assert (theta>=0. && theta<=90.);
            assert (phi>=0. && phi<360.);
            Vec3f relPointN = Vec3f(cos(phi)*sin(theta), cos(theta), sin(phi)*sin(theta));
            local2World.multDirMatrix(relPointN, *relPoint);
        }
        return angle;
    }    

    SurfaceAngle* getSurfaceAngleByPolar(const float theta, const float phi) const
    {
        if(angles == nullptr) return nullptr;
        /* caculate the hit angle refer to sphere Normal on the surface */
        /* each surface will cast rays into a half sphere space which express as theta[0,90),phi[0,360) */
        uint32_t v,h;
        v = floor(theta/90.0*vAngleRes);
        h = floor(phi/360.0*hAngleRes);
        return angles + v*hAngleRes + h;
    }

    SurfaceAngle* getSurfaceAngleByDir(const Vec3f &dir, uint32_t *angleV = nullptr, uint32_t *angleH = nullptr) const
    {
        Vec3f dirWorld;
        if(angles == nullptr) return nullptr;
        world2Local.multDirMatrix(dir, dirWorld);
        float theta = rad2deg(acos(dirWorld.y));
        float phi = rad2deg(atan2(dirWorld.z, dirWorld.x));
        /* caculate the hit angle refer to sphere Normal on the surface */
        /* each surface will cast rays into a half sphere space which express as theta[0,90),phi[0,360) */
        uint32_t v,h;
        v = floor(theta/90.0*vAngleRes);
        h = floor(phi/360.0*hAngleRes);
        if (angleV != nullptr) *angleV = v;
        if (angleH != nullptr) *angleH = h;
        return angles + v*hAngleRes + h;
    }

    // there will be 90*angleRatio*360*angleRatio angles to cast rays
    float angleRatio = 0.0;
    uint32_t vAngleRes, hAngleRes;

    // hitColor = diffuseColor*diffuseAmt + specularColor * specularAmt;
    // diffuseAmt = SUM(diffuseAmt from each light)
    Vec3f diffuseAmt;
    // diffuseColor = hitObject->evalDiffuseColor(st)
    //Vec3f diffuseColor;
    // specularAmt = SUM(specularAmt from each light)
    // each light = powf(std::max(0.f, -dotProduct(reflectionDirection, dir)), hitObject->specularExponent) * lights[i]->intensity;
    //Vec3f specularAmt;
    // now set it to hitObject->Ks;
    //Vec3f specularColor;
    Vec3f N; // normal
    // Change local matrix system to world matrix system
    Matrix44f local2World;
    Matrix44f world2Local;
    /* TBD: index of current surface inside object, it can be caculated instead of using memory */
    uint32_t   idx;
    // store relfect and refract color to each angles
    struct SurfaceAngle *angles = nullptr;
};

