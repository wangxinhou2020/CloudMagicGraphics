#ifndef MESHTRIANGLEH
#define MESHTRIANGLEH
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

class MeshTriangle : public Object
{
public:
    MeshTriangle(
        const std::string name, 
        const MaterialType type,
        const Vec3f *verts,
        const uint32_t *vertsIndex,
        const uint32_t &numTris,
        const Vec2f *st)
    {
        materialType = type;
        switch (materialType) {
            case DIFFUSE_AND_GLOSSY:
                ampRatio = ratio;
                surfaceAngleRatio = 0.0;
                break;
            default:
                ampRatio = 2.*ratio;
                surfaceAngleRatio = 1.*ratio;
                break;
        }
        uint32_t maxIndex = 0;
        for (uint32_t i = 0; i < numTris * 3; ++i)
            if (vertsIndex[i] > maxIndex) maxIndex = vertsIndex[i];
        maxIndex += 1;
        vertices = std::unique_ptr<Vec3f[]>(new Vec3f[maxIndex]);
        memcpy(vertices.get(), verts, sizeof(Vec3f) * maxIndex);
        vertexIndex = std::unique_ptr<uint32_t[]>(new uint32_t[numTris * 3]);
        memcpy(vertexIndex.get(), vertsIndex, sizeof(uint32_t) * numTris * 3);
        numTriangles = numTris;
        stCoordinates = std::unique_ptr<Vec2f[]>(new Vec2f[maxIndex]);
        memcpy(stCoordinates.get(), st, sizeof(Vec2f) * maxIndex);
        
        const Vec3f &v0 = vertices[vertexIndex[0]];
        const Vec3f &v1 = vertices[vertexIndex[1]];
        const Vec3f &v2 = vertices[vertexIndex[2]];
        Vec3f e0 = (v1 - v0);
        Vec3f e1 = (v2 - v0);

        uint32_t vRes = (uint32_t)(ampRatio * dotProduct(e0, e0));
        uint32_t hRes = (uint32_t)(ampRatio * dotProduct(e1, e1));
        setType(OBJECT_TYPE_MESH);
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
        reset();

        uint64_t raysNum = vRes*hRes + vRes*hRes*vAngleRes*hAngleRes;
        std::printf("mesh:%s, shadePoint:%d (vRes:%d, hRes:%d), pointAngle:%d (vAngle:%d, hAngle:%d), rays:%lu\n", 
                    name.c_str(), vRes*hRes, vRes, hRes, vAngleRes*hAngleRes, vAngleRes, hAngleRes, raysNum);
    }

    Surface* getSurfaceByVH(const uint32_t &v, const uint32_t &h, Vec3f *worldPoint = nullptr) const
    {
        //assert( v < vRes && h < hRes);
        Surface *surface;
        surface = pSurfaces.at(v%vRes*hRes + h%hRes).get();
        if (surface != nullptr && worldPoint != nullptr) {
            const Vec3f &v0 = vertices[vertexIndex[0]];
            const Vec3f &v1 = vertices[vertexIndex[1]];
            const Vec3f &v2 = vertices[vertexIndex[2]];
            Vec3f e0 = (v1 - v0) * (1.0/hRes);
            Vec3f e1 = (v2 - v0) * (1.0/vRes);
            *worldPoint = v0 + h*e0 + v*e1;
            //std::printf("h/v(%d,%d), P(%f,%f,%f)\n", h, v, worldPoint->x, worldPoint->y, worldPoint->z);
        }
        return surface;
    }

    void reset(void)
    {
        uint32_t size = sizeof(Surface) * vRes * hRes;
        float theta, phi;
        Surface *curr;

        const Vec3f &v0 = vertices[vertexIndex[0]];
        const Vec3f &v1 = vertices[vertexIndex[1]];
        const Vec3f &v2 = vertices[vertexIndex[2]];
        Vec3f e0 = normalize(v1 - v0);
        Vec3f e1 = normalize(v2 - v0);
        Vec3f N = normalize(crossProduct(e0, e1));

        uint32_t idx = 0;
        Vec3f center;
        for (uint32_t v = 0; v < vRes; ++v) {
            for (uint32_t h = 0; h < hRes; ++h) {
                curr = getSurfaceByVH(v, h, &center);
                //TBD, LEO, center of a mesh is not v1 need to be justified, v1-(v2+v1)/2 ?
                curr->reset(idx++, N, center);
            }
        }
    }

    bool intersect(const Vec3f &orig, const Vec3f &dir, float &tnear, Vec3f &point, Vec2f &mapIdx, Surface **surface, SurfaceAngle **angle) const
    {
        bool intersect = false;
        uint32_t index = 0;
        float u = 0, v = 0;
        for (uint32_t k = 0; k < numTriangles; ++k) {
            const Vec3f & v0 = vertices[vertexIndex[k * 3]];
            const Vec3f & v1 = vertices[vertexIndex[k * 3 + 1]];
            const Vec3f & v2 = vertices[vertexIndex[k * 3 + 2]];
            float tK, uK, vK;
            if (rayTriangleIntersect(v0, v1, v2, orig, dir, tK, uK, vK) && tK < tnear) {
                tnear = tK;
                index = k;
                u = uK;
                v = vK;
                intersect |= true;
            }
        }

        if ( intersect ) {
            const Vec2f &st0 = stCoordinates[vertexIndex[index * 3]];
            const Vec2f &st1 = stCoordinates[vertexIndex[index * 3 + 1]];
            const Vec2f &st2 = stCoordinates[vertexIndex[index * 3 + 2]];
            Vec2f st = st0 * (1 - u - v) + st1 * u + st2 * v;
            point = orig + dir * tnear;
            assert ( 0 <= st.x <= 1.0 && 0 <= st.y <= 1.0);
            Surface *pSurface = getSurfaceByVH(floor(st.y*vRes), floor(st.x*hRes));
            assert(surface != nullptr);
            *surface = pSurface;
            /* set the bitmap index */
            mapIdx = st;
            /* caculate the hit angle refer to sphere center on the surface */
            assert(angle != nullptr);
            if (pSurface != nullptr)
                *angle = pSurface->getSurfaceAngleByDir(-dir);
            else
                *angle = nullptr;
        }
        return intersect;
    }

    Vec3f pointRel2Abs(const Vec3f &rel) const
    {
        return rel;
    }

    Vec3f pointAbs2Rel(const Vec3f &abs) const
    {
        return abs;
    }

    Vec3f evalDiffuseColor(const Vec2f &mapIdx) const
    {
        if (localDiffuseColor == -1.) {
            //float pattern = (fmodf(st.x * mapRatio, 1) > 0.5) ^ (fmodf(st.y * mapRatio, 1) > 0.5);
            float pattern = (fmodf(mapIdx.x * mapRatio, 1) > 0.5) ^ (fmodf(mapIdx.y * mapRatio, 1) > 0.5);
            return mix(Vec3f(0.815, 0.235, 0.031), Vec3f(0.937, 0.937, 0.231), pattern);
        }
        return localDiffuseColor;
    }

    std::unique_ptr<Vec3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vec2f[]> stCoordinates;
    /* there will be mapRatio*mapRatio*4 blocks of different color */
    uint32_t mapRatio = 5;
};
#endif
