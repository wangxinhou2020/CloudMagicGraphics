#ifndef OBJECTH
#define OBJECTH
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "Values.h"
#include "Surface.h"
#include "Option.h"
#include "Ray.h"


class Object
{
 public:
    Object() :
        type(OBJECT_TYPE_NONE),
        name("NO NAME"),
        materialType(DIFFUSE_AND_GLOSSY),
        // LEO: TBD
        ior(1.3), Kd(0.1), Ks(0.2), diffuseColor(0.2), specularExponent(25),
        //ior(1.3), Kd(0.4), Ks(0.2), diffuseColor(0.2), specularExponent(25),
        vRes(0), hRes(0) {}
    virtual ~Object() {}
    virtual bool intersect(const Vec3f &, const Vec3f &, float &, Vec3f &, Vec2f &, Surface **, SurfaceAngle **) const = 0;
    virtual Surface* getSurfaceByVH(const uint32_t &, const uint32_t &, Vec3f * =nullptr) const = 0;
    virtual Vec3f evalDiffuseColor(const Vec2f &) const { return diffuseColor; }
    virtual Vec3f pointRel2Abs(const Vec3f &) const =0;
    virtual Vec3f pointAbs2Rel(const Vec3f &) const =0;
    virtual void reset(void) {};
    void enableRecorder(void)
    {
        if (traceLinks == nullptr) {
            // creating the ray links to record the rays
            traceLinks = (std::vector<std::unique_ptr<Ray>> *)malloc((MY_UINT64_T)sizeof(std::vector<std::unique_ptr<Ray>>)*vRes*hRes);
            assert(traceLinks != nullptr);
            std::memset(traceLinks, 0, sizeof(std::vector<std::unique_ptr<Ray>>)*vRes*hRes);
        }
        if (traceLinks != nullptr)
            recorderEnabled = true;
        else {
            recorderEnabled = false;
            std::printf("failed to enable rays recorder for object:%s\n", name.c_str());
        }
    }
    void disableRecorder(void) { recorderEnabled = false; }

    void setType(ObjectType objType) { type = objType; }
    void setName(std::string objName)
    {
        name = objName;
    }
    std::string getName(void) {return name;}
    void setResolution(uint32_t verticalRes, uint32_t horizonRes)
    {
        vRes  = verticalRes;
        hRes  = horizonRes;
    }
    void dumpSurface(const Options &option) const
    {
        char outfile[256];
        std::sprintf(outfile,
            "obj[%s]_density.%.2f_dep.%d_spp.%d_split.%d.ppm", name.c_str(), RAY_CAST_DESITY, option.maxDepth, option.spp,
            option.diffuseSpliter);
        // save framebuffer to file
        std::ofstream ofs;
        /* text file for compare */
        ofs.open(outfile);
        ofs << "P3\n" << hRes << " " << vRes << "\n255\n";
        Surface *curr;
        for (uint32_t v = 0; v < vRes; ++v) {
            for (uint32_t h = 0; h < hRes; ++h) {
                curr = getSurfaceByVH(v, h);
                int r = (int)(255 * clamp(0, 1, curr->diffuseAmt.x));
                int g = (int)(255 * clamp(0, 1, curr->diffuseAmt.y));
                int b = (int)(255 * clamp(0, 1, curr->diffuseAmt.z));
                ofs << r << " " << g << " " << b << "\n ";
            }
        }
        ofs.close();
    }

    void dumpSurfaceAngles(const Options &option) const
    {
        char outfile[256];
        std::sprintf(outfile,
            "objangle[%s]_density.%.2f_dep.%d_spp.%d_split.%d.ppm", name.c_str(), RAY_CAST_DESITY, option.maxDepth, option.spp,
            option.diffuseSpliter);
        // save framebuffer to file
        std::ofstream ofs;
        /* text file for compare */
        ofs.open(outfile);
        ofs << "P3\n" << hRes << " " << vRes << "\n255\n";
        Surface *curr;
/*
        for (uint32_t v = 0; v < vRes; ++v) {
            for (uint32_t h = 0; h < hRes; ++h) {
                curr = getSurfaceByVH(v, h);
*/
                curr = getSurfaceByVH(0, 0);
                for (uint32_t vAngle=0; vAngle<curr->vAngleRes; vAngle++) {
                    for (uint32_t hAngle=0; hAngle<curr->hAngleRes; hAngle++) {
                        SurfaceAngle *angle = curr->getSurfaceAngleByVH(vAngle, hAngle);
                        int r = (int)(255 * clamp(0, 1, angle->angleColor.x));
                        int g = (int)(255 * clamp(0, 1, angle->angleColor.y));
                        int b = (int)(255 * clamp(0, 1, angle->angleColor.z));
                        ofs << r << " " << g << " " << b << "\n ";
                    }
                }
/*
            }
        }
*/
        ofs.close();
    }


    ObjectType type;
    // material properties
    MaterialType materialType;
    float ior;
    float Kd, Ks;
    Vec3f diffuseColor;
    float specularExponent;
    std::string  name;
    // vertical resolution is the factor to split x from [min, max] or THETA from [0, 180]
    // we set the vertical resolution as r/10 by now.
    uint32_t vRes;
    // horizontal resolution is the factor to split y from [min, max] or PHI from [0, 360)
    // we set the horizontal resolution as r/10 by now.
    uint32_t hRes;
    // ratio determine the object and light field datas
    float ratio = RAY_CAST_DESITY;

    // link stack to record the rays
    std::vector<std::unique_ptr<Ray>> * traceLinks = nullptr;
    bool recorderEnabled = false;
    // there will be vRes*ampRatio*hRes*ampRatio blocks of amp value
    float ampRatio = 1.0;
    // there will be vAngleRes*ampRatio*hAngleRes*ampRatio blocks of amp value
    float surfaceAngleRatio = 0.0;
    // the number point is vRes * hRes
    //struct Surface * pSurfaces;
    std::vector<std::unique_ptr<Surface>> pSurfaces; 
    // the diffuse color the object by itself
    Vec3f  localDiffuseColor = -1.;
};

#endif
