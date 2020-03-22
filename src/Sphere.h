#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

class Sphere : public Object
{
public:
    Sphere(const std::string name, const MaterialType type, const Vec3f &c, const float &r) : center(c), radius(r), radius2(r * r) 
    {
        materialType = type;
        switch (materialType) {
            case DIFFUSE_AND_GLOSSY:
                ampRatio = ratio;
                surfaceAngleRatio = 0.0;
                break;
            default:
                ampRatio = 4.*ratio;
                surfaceAngleRatio = ratio;
                break;
        }
        // vertical range is [0,180], horizon range is [0,360)
        uint32_t vRes = (180.+1.)*ampRatio*r, hRes = 360.*ampRatio*r;
        setType(OBJECT_TYPE_SPHERE);
        setName(name);
        setResolution(vRes, hRes);
        //MY_UINT64_T size = (MY_UINT64_T)sizeof(Surface) * vRes * hRes;
        //pSurfaces = (Surface *)malloc(size);
        uint32_t vAngleRes = 0;
        uint32_t hAngleRes = 0;
        for (uint32_t i=0; i<vRes*hRes; i++) {
            Surface *surface = new Surface(surfaceAngleRatio);
            pSurfaces.push_back(std::unique_ptr<Surface>(surface));
            if (vAngleRes < surface->vAngleRes) vAngleRes = surface->vAngleRes;
            if (hAngleRes < surface->hAngleRes) hAngleRes = surface->hAngleRes;
        }
        uint64_t raysNum = vRes*hRes + vRes*hRes*vAngleRes*hAngleRes;
        std::printf("sphere:%s, shadePoint:%d (vRes:%d, hRes:%d), pointAngle:%d (vAngle:%d, hAngle:%d), rays:%lu\n", 
                    name.c_str(), vRes*hRes, vRes, hRes, vAngleRes*hAngleRes, vAngleRes, hAngleRes, raysNum);
        reset();
    }
    // store the pre-caculated shade value of each point
    void reset(void)
    {
        float theta, phi;
        Surface *curr;
        // DEBUG
        uint32_t idx = 0;
        for (uint32_t v = 0; v < vRes; ++v) {
            for (uint32_t h = 0; h < hRes; ++h) {
                curr = getSurfaceByVH(v, h);
                // v(0, 1, 2, ... , 17) ==> theta(0, 10, 20, ..., 180)
                // TBD: bugfix when v is 180, theta is only 180/181*180
                theta = deg2rad(180.f * v/vRes);
                phi = deg2rad(360.f * h/hRes);
                //Vec3f normal = Vec3f(sin(phi)*sin(theta), cos(theta), cos(phi)*sin(theta));
                Vec3f normal = Vec3f(cos(phi)*sin(theta), cos(theta), sin(phi)*sin(theta));
                Vec3f surfaceCenter = center+normal*radius;
                curr->reset(idx++, normal, surfaceCenter);
/*
                if (v >= 360 )
                    std::printf("&&&&v=%d, vRes=%d, theta=%f, N.y=%f\n",v, vRes, theta, curr->N.y);
*/
            }
        }
    }
    bool intersect(const Vec3f &orig, const Vec3f &dir, float &tnear, Vec3f &point, Vec2f &mapIdx, Surface **surface, SurfaceAngle **angle) const
    {
        // analytic solution
        Vec3f L = orig - center;
        float a = dotProduct(dir, dir);
        float b = 2 * dotProduct(dir, L);
        float c = dotProduct(L, L) - radius2;
        float t0, t1;
        if (!solveQuadratic(a, b, c, t0, t1)) return false;
        if (t0 < 0) t0 = t1;
        if (t0 < 0) return false;
        tnear = t0;

        point = orig + dir * tnear;
        Vec3f N = normalize(point - center);
        /* caculate the hit point refer to sphere center on the surface */
        float theta = rad2deg(acos(N.y));
        //float phi = rad2deg(atan2(N.x, N.z));
        float phi = rad2deg(atan2(N.z, N.x));
        /* set the bitmap index */
        mapIdx.x = theta;
        mapIdx.y = phi;
        uint32_t v,h;
        v = floor(theta/181.0*vRes);
        h = floor(phi/360.0*hRes);
        Surface *pSurface = getSurfaceByVH(v, h);
        assert(surface != nullptr);
        *surface = pSurface;
        /* caculate the hit angle refer to sphere center on the surface */
        assert(angle != nullptr);
        if (pSurface != nullptr)
            *angle = pSurface->getSurfaceAngleByDir(-dir);
        else
            *angle = nullptr;

        return true;
    }

    Vec3f pointRel2Abs(const Vec3f &rel) const
    {
        return center + rel*radius;
    }

    Vec3f pointAbs2Rel(const Vec3f &abs) const
    {
        return (abs - center) * (1/radius);
    }

    Surface* getSurfaceByVH(const uint32_t &v, const uint32_t &h, Vec3f *worldPoint = nullptr) const
    {
        //assert( v < vRes && h < hRes);
        Surface *surface;
        surface = pSurfaces.at(v%vRes*hRes + h%hRes).get();
        if (surface != nullptr && worldPoint != nullptr)
            *worldPoint = center + surface->N*radius;
        return surface;
    }

    Vec3f center;
    float radius, radius2;
};

