#ifndef RAYSTOREH
#define RAYSTOREH
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "Values.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Light.h"
#include "Matrix44.h"
#include "Object.h"
#include "Sphere.h"
#include "MeshTriangle.h"
#include "Utils.h"
#include "Option.h"
#include "SurfaceAngle.h"


class RayStore
{
public:
    RayStore(const Options &currOption) : option(currOption) 
    {
        currPixel = 0;
        currRay = nullptr;

//#define RAY_TRACE_LINK_RECORDER
#ifdef RAY_TRACE_LINK_RECORDER
        // creating the rays from eye tracker
        eyeTraceLinks = (std::vector<std::unique_ptr<Ray>> *)malloc((MY_UINT64_T)sizeof(std::vector<std::unique_ptr<Ray>>)*VIEW_HEIGHT*VIEW_WIDTH);
        assert(eyeTraceLinks != nullptr);
        std::memset(eyeTraceLinks, 0, sizeof(std::vector<std::unique_ptr<Ray>>)*VIEW_HEIGHT*VIEW_WIDTH);
#endif
        totalMem = 0;
        totalRays = 0;
        originRays = 0;
        reflectionRays = 0;
        refractionRays = 0;
        diffuseRays = 0;
        invisibleRays = 0;
        weakRays = 0;
        overflowRays = 0;
        loopInternalRays = 0;
        validRays = 0;
        invalidRays = 0;
        nohitRays = 0;
    }
    Ray * record(const RayType type, std::vector<std::unique_ptr<Ray>> *links, const uint32_t index, 
                    const Vec3f &orig, const Vec3f &dir, const Vec3f &intensity = -1)
    {
        if (links == nullptr || links+index == nullptr)
            return nullptr;
        // tracker the ray
        Ray * currRay = new Ray(type, orig, dir);
        totalMem += sizeof(Ray);
        currRay = currRay;
        links[index].push_back(std::unique_ptr<Ray>(currRay));
        return currRay;
    }
    void dumpRay(const std::vector<std::unique_ptr<Ray>> &curr, uint32_t idx, uint32_t depth)
    {
        #define MAX_DUMP_DEPTH 256 
        char prefix[MAX_DUMP_DEPTH*2] = "";
        if(depth >= MAX_DUMP_DEPTH) {
            std::printf("dumpRay out of stack\n");
            return;
        }

        std::printf("%*s%d from(%f,%f,%f)-%d->to(%f,%f,%f), intensity(%f,%f,%f)*\n", depth, "#", depth,
                    curr[idx]->orig.x, curr[idx]->orig.y, curr[idx]->orig.z, 
                    curr[idx]->inside,
                    curr[idx]->dir.x, curr[idx]->dir.y, curr[idx]->dir.z,
                    curr[idx]->intensity.x, curr[idx]->intensity.y, curr[idx]->intensity.z);
        if(curr[idx]->hitObject != nullptr) {
            std::printf("%*s%d hit object: %p, point(%f, %f, %f)\n", depth, "#", depth, 
                    ((Object *)(curr[idx]->hitObject))->name.c_str(), 
                    curr[idx]->hitPoint.x, curr[idx]->hitPoint.y, curr[idx]->hitPoint.z);

            for(uint32_t i=0; i<curr[idx]->reflectionLink.size(); i++) {
                std::printf("%*s%d reflect[%d]:\n", depth+1, "#", depth+1, i);
                dumpRay(curr[idx]->reflectionLink, i, depth+1);
            }

            for(uint32_t i=0; i<curr[idx]->refractionLink.size(); i++) {
                std::printf("%*s%d refract[%d]:\n", depth+1, "#", depth+1, i);
                dumpRay(curr[idx]->refractionLink, i, depth+1);
            }

            for(uint32_t i=0; i<curr[idx]->diffuseLink.size(); i++) {
                std::printf("%*s%d diffuse[%d]:\n", depth+1, "#", depth+1, i);
                dumpRay(curr[idx]->diffuseLink, i, depth+1);
            }
        }
        else
            std::printf("%*s%d nohit\n", depth, "#", depth);
    }

    void dumpObjectTraceLink(
        const std::vector<std::unique_ptr<Object>> &objects,
        uint32_t objIdx,
        uint32_t vertical,
        uint32_t horizon)
    {
        if (objects[objIdx]->traceLinks == nullptr) return;
        std::printf("***dump light trace rays of object-vertical-horizon(%s,%d,%d)***\n", objects[objIdx]->name.c_str(), vertical, horizon);
        uint32_t index = vertical*objects[objIdx]->hRes + horizon;
        dumpRay(objects[objIdx]->traceLinks[index], 0, 1);
    }

    void dumpEyeTraceLink(
        uint32_t vertical,
        uint32_t horizon) 
    {
        if (eyeTraceLinks == nullptr) return;
        std::printf("***********dump eye trace rays of pixel(%d,%d)**************\n", vertical, horizon);
        assert( vertical < VIEW_HEIGHT && horizon < VIEW_WIDTH );
        uint32_t index = vertical*VIEW_WIDTH + horizon;
        dumpRay(eyeTraceLinks[index], 0, 1);
    }

    Options option;
    // Pixel point of current processing original ray
    Vec3f currPixel;
    Ray *currRay;

    std::vector<std::unique_ptr<Ray>> *eyeTraceLinks = nullptr;

    // Counter of total memory to record rays
    MY_UINT64_T totalMem;
    // Counter of total rays, DO NOT include overflow rays
    uint32_t totalRays;
    // Counter of origin rays from eyes
    uint32_t originRays;
    // Counter of reflectoin rays
    uint32_t reflectionRays;
    // Counter of reflectoin rays
    uint32_t refractionRays;
    // Counter of diffuse reflectoin rays
    uint32_t diffuseRays;
    // Counter of invisible diffuse reflectoin rays
    uint32_t invisibleRays;
    // Counter of valid rays as per hitted
    //uint32_t hittedRays;
    // Counter of ignored weak rays
    uint32_t weakRays;
    // Counter of invalid rays as per overflow
    uint32_t overflowRays;
    // Rays loop inside object
    uint32_t loopInternalRays;
    // Counter of valid rays
    uint32_t validRays;
    // Counter of invalid rays
    uint32_t invalidRays;
    // Counter of invalid rays as per nohit
    uint32_t nohitRays;
};
#endif
