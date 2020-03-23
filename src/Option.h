#ifndef OPTIONH
#define OPTIONH
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

struct Options
{
    // samples per pixel
    uint32_t spp;
    uint32_t diffuseSpliter;
    uint32_t width;
    uint32_t height;
    float fov;
    float imageAspectRatio;
    uint8_t maxDepth;
    Vec3f backgroundColor;
    float bias;
    bool  doTraditionalRender;
    bool  doRenderAfterDiffusePreprocess;
    bool  doRenderAfterDiffuseAndReflectPreprocess;
    // a list of viewpoint to cast the original rays
    Vec3f viewpoints[100];
};
#endif
