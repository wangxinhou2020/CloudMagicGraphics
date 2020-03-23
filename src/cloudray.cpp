/*********************************************************
    Leo, lili 
    Prototype to verify cloud ray tracing
    Usage: c++ -O0 -g -std=c++11 -o cloudray.cpp cloudray 
*********************************************************/

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <cstring>
#include <string>
#include <time.h>
#include <assert.h>

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
#include "RayStore.h"


// [comment]
// Returns true if the ray intersects an object, false otherwise.
//
// \param orig is the ray origin
//
// \param dir is the ray direction
//
// \param objects is the list of objects the scene contains
//
// \param[out] tNear contains the distance to the cloesest intersected object.
//
// \param[out] index stores the index of the intersect triangle if the interesected object is a mesh.
//
// \param[out] uv stores the u and v barycentric coordinates of the intersected point
//
// \param[out] *hitObject stores the pointer to the intersected object (used to retrieve material information, etc.)
//
// \param isShadowRay is it a shadow ray. We can return from the function sooner as soon as we have found a hit.
// [/comment]
bool trace(
    const Vec3f &orig, const Vec3f &dir,
    const std::vector<std::unique_ptr<Object>> &objects,
    float &tNear, Vec3f &hitPoint, Vec2f &mapIdx, Surface **hitSurface, SurfaceAngle **hitAngle, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        Vec3f pointK = 0;
        Vec2f mapK = 0;
        Surface *surfaceK = nullptr;
        SurfaceAngle *angleK = nullptr;
        if (objects[k]->intersect(orig, dir, tNearK, pointK, mapK, &surfaceK, &angleK) && tNearK < tNear) {
            *hitObject = objects[k].get();
            tNear = tNearK;
            *hitSurface = surfaceK;
            *hitAngle = angleK;
            hitPoint = pointK;
            mapIdx = mapK;
        }
    }
    
    bool hitted = (*hitObject != nullptr);
    return hitted;

//    return (*hitObject != nullptr);
}

/* precast ray from light to object*/
Vec3f forwordCastRay(
    RayStore &rayStore,
    const Vec3f &orig, const Vec3f &dir,
    const std::vector<std::unique_ptr<Object>> &objects,
    const Vec3f intensity,
    const Options &options,
    uint32_t depth,
    // index of object which is direct casted
    Object *targetObject=nullptr,
    Surface *targetSurface=nullptr,
    Vec3f targetPoint = 0,
    Vec2f targetMapIdx = 0)
{
    Ray * newRay = nullptr;
    Ray * currRay = nullptr;

    if (depth > OVERSTACK_PROTECT_DEPTH) {
        rayStore.overflowRays++;
        if(rayStore.currRay != nullptr) {
            rayStore.currRay->status = OVERFLOW_RAY;
            rayStore.currRay->overflowCount++;
        }
        return options.backgroundColor;
    }

    rayStore.totalRays++;
    
    Vec3f hitColor = options.backgroundColor;
    Vec3f leftIntensity = -1;
    float tnear = kInfinity;
    Object *hitObject = nullptr;
    Surface *hitSurface = nullptr;
    SurfaceAngle *hitAngle = nullptr;
    Vec3f hitPoint = 0;
    Vec2f mapIdx = 0;
    bool hitted = trace(orig, dir, objects, tnear, hitPoint, mapIdx, &hitSurface, &hitAngle, &hitObject);
    bool insideObject = false;
/*
    if(hitted && depth >=1)
        std::printf("this should not happen.depth=%d, orig(%f,%f,%f)-->dir(%f,%f,%f), hitObject(%s)\n", 
                    depth, orig.x, orig.y, orig.z, dir.x, dir.y, dir.z, hitObject->name.c_str());
    if (hitted && depth >= 3)
        std::printf("o(%f,%f), type(%d), depth:%d, tnear: %f, hitObject:%s, intensity:%f\n",
                    rayStore.currPixel.x, rayStore.currPixel.y, rayStore.currType,
                    depth, tnear, hitObject==nullptr?"nothing":hitObject->name.c_str(), intensity.x);
*/
    if (targetObject != nullptr) {
/*
        if (hitted && targetObject->name == "mesh1" && hitObject->name == "mesh1") {
            std::printf("o(%f,%f), point(%f,%f,%f), tnear: %f\n", rayStore.currPixel.x, rayStore.currPixel.y, 
                        hitPoint.x, hitPoint.y, hitPoint.z, tnear);
        }
*/
        if (hitted && tnear <=1) {
      //      std::printf("targetPoint(%f,%f,%f) is in shadow of tnear(%f)\n", dir.x, dir.y, dir.z, tnear);
            rayStore.nohitRays++;
            if(rayStore.currRay != nullptr) {
                rayStore.currRay->status = NOHIT_RAY;
                rayStore.currRay->nohitCount++;
            }
            return hitColor;
        }
        hitObject = targetObject;
        hitSurface = targetSurface;
        hitPoint = targetPoint;
        tnear = 1.0;
    }
    else {
        if (!hitted) {
            rayStore.nohitRays++;
            if(rayStore.currRay != nullptr) {
                rayStore.currRay->status = NOHIT_RAY;
                rayStore.currRay->nohitCount++;
            }
            return hitColor;
        }
    }
    if (hitSurface == nullptr) {
        std::printf("BUG: ####targetSurface should not be NULL here\n");
        return hitColor;
    }
    Vec3f N = hitSurface->N; // normal

/*
        Vec3f relTgt = hitObject->pointAbs2Rel(orig + dir);
        Vec3f relHit = hitObject->pointAbs2Rel(hitPoint);
        std::printf("Hitpoint not match. tgtRel(%f,%f,%f), hitpointRel(%f,%f,%f), v(%d, %d), h(%d, %d), tnear(%f)\n", 
                    relTgt.x, relTgt.y, relTgt.z, relHit.x, relHit.y, relHit.z,
                    v, hitSurface->v, h, hitSurface->h, tnear);
        if(hitSurface->v != v || hitSurface->h != h) {
            std::printf("##########################################\n");
        }
*/


    if(rayStore.currRay != nullptr) {
        rayStore.currRay->status = VALID_RAY;
        rayStore.currRay->validCount++;
        rayStore.currRay->hitObject = hitObject;
        rayStore.currRay->hitPoint = hitPoint;
    }

    switch (hitObject->materialType) {
        case REFLECTION_AND_REFRACTION:
        {
            float kr;
            fresnel(dir, N, hitObject->ior, kr);
            leftIntensity = intensity*kr;
            float leftSqureValue = dotProduct(leftIntensity, leftIntensity);
            if (leftSqureValue < INTENSITY_TOO_WEAK) {
                rayStore.weakRays ++;
                if(rayStore.currRay != nullptr)
                    rayStore.currRay->weakCount ++;
                break;
            }
            Vec3f reflectionDirection = normalize(reflect(dir, N));
            insideObject = (dotProduct(reflectionDirection, N) < 0);
            Vec3f reflectionRayOrig = insideObject ?
                hitPoint - N * options.bias :
                hitPoint + N * options.bias;
            Vec3f reflectionColor = 0;
/*
            Vec3f reflectionRayOrig = (dotProduct(reflectionDirection, N) < 0) ?
                hitPoint - N * options.bias :
                hitPoint + N * options.bias;
            Vec3f refractionRayOrig = (dotProduct(refractionDirection, N) < 0) ?
                hitPoint - N * options.bias :
                hitPoint + N * options.bias;
*/
            /* don't trace reflection ray inside object */
            if(!insideObject) {
                rayStore.reflectionRays++;
                // tracker the ray
                if(rayStore.currRay != nullptr) {
                    newRay = rayStore.record(RAY_TYPE_REFLECTION, &rayStore.currRay->reflectionLink, 0, reflectionRayOrig, reflectionDirection, leftIntensity);
                    newRay->inside = insideObject;
                    currRay = rayStore.currRay;
                    rayStore.currRay = newRay;
                }
                reflectionColor = forwordCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, leftIntensity, options, depth + 1);
                rayStore.currRay = currRay;
                //Vec3f reflectionColor = forwordCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, intensity*kr, options, depth + 1);
            }

            leftIntensity = intensity*(1-kr);
            Vec3f refractionDirection = normalize(refract(dir, N, hitObject->ior));
            insideObject = (dotProduct(refractionDirection, N) < 0);
            Vec3f refractionRayOrig = insideObject ?
                hitPoint - N * options.bias :
                hitPoint + N * options.bias;
            rayStore.refractionRays++;
            // tracker the ray
            if(rayStore.currRay != nullptr) {
                newRay = rayStore.record(RAY_TYPE_REFRACTION, &rayStore.currRay->refractionLink, 0, refractionRayOrig, refractionDirection, leftIntensity);
                newRay->inside = insideObject;
                currRay = rayStore.currRay;
                rayStore.currRay = newRay;
            }
            Vec3f refractionColor = forwordCastRay(rayStore, refractionRayOrig, refractionDirection, objects, leftIntensity, options, depth + 1);
            rayStore.currRay = currRay;
            hitColor = reflectionColor * kr + refractionColor * (1 - kr);
            break;
        }
        case REFLECTION:
        {
            float kr = 0.9;
            //fresnel(dir, N, hitObject->ior, kr);
            leftIntensity = intensity*kr;
            float leftSqureValue = dotProduct(leftIntensity, leftIntensity);
            if (leftSqureValue < INTENSITY_TOO_WEAK) {
                rayStore.weakRays ++;
                if(rayStore.currRay != nullptr)
                    rayStore.currRay->weakCount ++;
                break;
            }
            Vec3f reflectionDirection = reflect(dir, N);
            insideObject = (dotProduct(reflectionDirection, N) < 0);
            Vec3f reflectionRayOrig = insideObject ?
                hitPoint - N * options.bias :
                hitPoint + N * options.bias;
            rayStore.reflectionRays++;
            // tracker the ray
            if(rayStore.currRay != nullptr) {
                newRay = rayStore.record(RAY_TYPE_REFLECTION, &rayStore.currRay->reflectionLink, 0, reflectionRayOrig, reflectionDirection, leftIntensity);
                newRay->inside = insideObject;
                currRay = rayStore.currRay;
                rayStore.currRay = newRay;
            }
           // hitColor = forwordCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, intensity*(1-kr), options, depth + 1) * kr;
            hitColor = forwordCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, leftIntensity, options, depth + 1) * kr;
            rayStore.currRay = currRay;
            break;
        }
        default:
        {
            // TBD , leo
            break;
            // Diffuse relfect
            // each relfect light will share part of the light
            // how many diffuse relfect light will be traced
            uint32_t count = options.diffuseSpliter;
            // Prevent memory waste before overflow
            if (depth+1 > OVERSTACK_PROTECT_DEPTH) {
                rayStore.overflowRays += count;
                if(rayStore.currRay != nullptr)
                    rayStore.currRay->overflowCount += count;
            }
            //leftIntensity = intensity*(1.0-kr)*(1.0/(count+1));
            leftIntensity = intensity*hitObject->Kd*(1.0/(count+1));
            float leftSqureValue = dotProduct(leftIntensity, leftIntensity);
            if (leftSqureValue < INTENSITY_TOO_WEAK) {
                rayStore.weakRays ++;
                if(rayStore.currRay != nullptr)
                    rayStore.currRay->weakCount ++;
                break;
            }
            else {
                for (int32_t i=count*(-1./2); i<=count/2.; i+=1) {
                //for (uint32_t i=1; i<=count; i++) {
                    Vec3f reflectionDirection = diffuse(dir, N, i*4.0/(count+1));
                    //Vec3f reflectionDirection = normalize(reflect(dir, N));
                    //Vec3f reflectionRayOrig = hitPoint + N * options.bias;
                    insideObject = (dotProduct(reflectionDirection, N) < 0);
                    Vec3f reflectionRayOrig = insideObject ?
                        hitPoint - N * options.bias :
                        hitPoint + N * options.bias;
                    rayStore.diffuseRays++;
                    //std::printf("DEBUG--(%d, %d)-->Total rays(%d) = Origin rays(%d) + Reflection rays(%d) + Refraction rays(%d) + Diffuse rays(%d)\n", i, depth, rays.totalRays, rays.originRays, rays.reflectionRays, rays.refractionRays, rays.diffuseRays);
                    // tracker the ray
                    if(rayStore.currRay != nullptr) {
                        newRay = rayStore.record(RAY_TYPE_DIFFUSE, &rayStore.currRay->diffuseLink, 0, reflectionRayOrig, reflectionDirection, leftIntensity);
                        newRay->inside = insideObject;
                        currRay = rayStore.currRay;
                        rayStore.currRay = newRay;
                    }
                    
                    forwordCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, leftIntensity, options, depth + 1);
                    rayStore.currRay = currRay;
                }
            }
            
            break;
        }
    }

    /* pre-caculate diffuse amt */
    float Kd = hitObject->Kd;
    Vec3f lightDir = hitPoint - orig;
    lightDir = normalize(lightDir);
    float LdotN = std::max(0.f, dotProduct(-lightDir, N));
    if (intensity.x < 0 || intensity.y <0 || intensity.z < 0 || Kd <0)
        std::printf("ERROR: intensity=(%f,%f,%f), Kd=%f\n", intensity.x, intensity.y, intensity.z, Kd);
    /* pre-caculate diffuse amt */
    hitSurface->diffuseAmt += intensity * LdotN * Kd;
    /* pre-caculate specular amt */
    /*
    Vec3f reflectionDirection = reflect(lightDir, N);
    hitSurface->specularAmt += powf(std::max(0.f, -dotProduct(reflectionDirection, dir)), hitObject->specularExponent) * intensity;
    */
    return hitColor;
}
// [comment]
// Implementation of the Whitted-syle light transport algorithm (E [S*] (D|G) L)
//
// This function is the function that compute the color at the intersection point
// of a ray defined by a position and a direction. Note that thus function is recursive (it calls itself).
//
// If the material of the intersected object is either reflective or reflective and refractive,
// then we compute the reflection/refracton direction and cast two new rays into the scene
// by calling the backwardCastRay() function recursively. When the surface is transparent, we mix
// the reflection and refraction color using the result of the fresnel equations (it computes
// the amount of reflection and refractin depending on the surface normal, incident view direction
// and surface refractive index).
//
// If the surface is duffuse/glossy we use the Phong illumation model to compute the color
// at the intersection point.
// [/comment]
Vec3f backwardCastRay(
    RayStore &rayStore,
    const Vec3f &orig, const Vec3f &dir,
    const std::vector<std::unique_ptr<Object>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights,
    const Options &options,
    uint32_t depth,
    bool withLightRender = false,
    bool withObjectRender = false,
    Vec3f *pDeltaAmt = nullptr)
{
/*
    uint32_t  xPos = (uint32_t)rayStore.currPixel.x;
    uint32_t  yPos = (uint32_t)rayStore.currPixel.y;
*/
    Ray * newRay = nullptr;
    Ray * currRay = nullptr;

    if (depth > options.maxDepth) {
        rayStore.overflowRays++;
        if(rayStore.currRay != nullptr) {
            rayStore.currRay->status = OVERFLOW_RAY;
            rayStore.currRay->overflowCount++;
        }
        return options.backgroundColor;
    }

    rayStore.totalRays++;
    
    Vec3f hitColor = options.backgroundColor;
    float tnear = kInfinity;
    Object *hitObject = nullptr;
    Surface * hitSurface = nullptr;
    SurfaceAngle *hitAngle = nullptr;
    Vec3f hitPoint = 0;
    Vec2f mapIdx = 0;
    Vec3f globalAmt = 0, localAmt = 0, specularColor = 0;
    bool  insideObject = false;
    if (trace(orig, dir, objects, tnear, hitPoint, mapIdx, &hitSurface, &hitAngle, &hitObject)) {
        Vec3f N = hitSurface->N; // normal
//        std::printf("%*s%d hit[%s]:\n", depth+1, "#", depth+1, hitObject->name.c_str());
//        Vec3f testColor = hitObject->evalDiffuseColor(mapIdx);
//        std::printf("#hitPoint(%f, %f, %f), color(%f, %f, %f) \n", hitPoint.x, hitPoint.y, hitPoint.z, testColor.x, testColor.y, testColor.z);
        if(rayStore.currRay != nullptr) {
            rayStore.currRay->status = VALID_RAY;
            rayStore.currRay->validCount++;
            rayStore.currRay->hitObject = hitObject;
            rayStore.currRay->hitPoint = hitPoint;
        }

        if (withObjectRender) {
            if (hitAngle == nullptr) {
                globalAmt = hitSurface->diffuseAmt;
                localAmt = 0;
                hitColor = (globalAmt + localAmt) * hitObject->evalDiffuseColor(mapIdx) + specularColor * hitObject->Ks;
            }
            else
                hitColor = hitAngle->angleColor;
            return hitColor;
        }

            
/*
        if (rayStore.currPixel.x == 235. && rayStore.currPixel.y == 304.)
            std::printf("stop here.\n");
*/
        switch (hitObject->materialType) {
            case REFLECTION_AND_REFRACTION:
            {
                Vec3f reflectionDirection = normalize(reflect(dir, N));
                insideObject = (dotProduct(reflectionDirection, N) < 0);
                Vec3f reflectionRayOrig = insideObject ?
                    hitPoint - N * options.bias :
                    hitPoint + N * options.bias;
                Vec3f reflectionColor = 0;
                Vec3f diffuseColor = 0;
                float kr;
                fresnel(dir, N, hitObject->ior, kr);
                /* don't trace reflection ray inside object */
                if(!insideObject) {
                    rayStore.reflectionRays++;
                    // tracker the ray
                    if(rayStore.currRay != nullptr) {
                        newRay = rayStore.record(RAY_TYPE_REFLECTION, &rayStore.currRay->reflectionLink, 0, reflectionRayOrig, reflectionDirection);
                        newRay->inside = insideObject;
                        currRay = rayStore.currRay;
                        rayStore.currRay = newRay;
                    }
                    reflectionColor = backwardCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, lights, options, depth + 1, withLightRender);
                    rayStore.currRay = currRay;
                }

                Vec3f refractionDirection = normalize(refract(dir, N, hitObject->ior));
                insideObject = (dotProduct(refractionDirection, N) < 0);
                Vec3f refractionRayOrig = insideObject ?
                    hitPoint - N * options.bias :
                    hitPoint + N * options.bias;

                rayStore.refractionRays++;
                // tracker the ray
                if(rayStore.currRay != nullptr) {
                    newRay = rayStore.record(RAY_TYPE_REFRACTION, &rayStore.currRay->refractionLink, 0, refractionRayOrig, refractionDirection);
                    newRay->inside = insideObject;
                    currRay = rayStore.currRay;
                    rayStore.currRay = newRay;
                }
                Vec3f refractionColor = backwardCastRay(rayStore, refractionRayOrig, refractionDirection, objects, lights, options, depth + 1, withLightRender);
                rayStore.currRay = currRay;
                if (withLightRender)
                    diffuseColor = hitSurface->diffuseAmt * hitObject->evalDiffuseColor(mapIdx);
                hitColor = reflectionColor * kr + refractionColor * (1 - kr) + diffuseColor;
                break;
            }
            case REFLECTION:
            {
                float kr = 0.5;
                Vec3f reflectionColor = 0;
                Vec3f diffuseColor = 0;
                //fresnel(dir, N, hitObject->ior, kr);
                // LEO: do i need normolization here.
                Vec3f reflectionDirection = reflect(dir, N);
                insideObject = (dotProduct(reflectionDirection, N) < 0);
                Vec3f reflectionRayOrig = insideObject ?
                    hitPoint - N * options.bias :
                    hitPoint + N * options.bias;
                rayStore.reflectionRays++;
                // tracker the ray
                if(rayStore.currRay != nullptr) {
                    newRay = rayStore.record(RAY_TYPE_REFLECTION, &rayStore.currRay->reflectionLink, 0, reflectionRayOrig, reflectionDirection);
                    newRay->inside = insideObject;
                    currRay = rayStore.currRay;
                    rayStore.currRay = newRay;
                }
                reflectionColor = backwardCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, lights, options, depth + 1, withLightRender) * kr;
                if (withLightRender)
                    diffuseColor = hitSurface->diffuseAmt * hitObject->evalDiffuseColor(mapIdx);
                hitColor = reflectionColor + diffuseColor;
                rayStore.currRay = currRay;
                break;
            }
            default:
            {
                // [comment]
                // We use the Phong illumation model int the default case. The phong model
                // is composed of a diffuse and a specular reflection component.
                // [/comment]
                Vec3f shadowPointOrig = (dotProduct(dir, N) < 0) ?
                    hitPoint + N * options.bias :
                    hitPoint - N * options.bias;

                if (false) {
                //if (!withLightRender) {
                    // Diffuse relfect
                    // each relfect light will share part of the light
                    // how many diffuse relfect light will be traced
                    uint32_t count = options.diffuseSpliter;
                    float weight = 1./(count+1);
                    //float weight = 0.001;
                    hitColor = 0;
                    // Prevent memory waste before overflow
                    if (depth+1 > options.maxDepth) {
                        rayStore.overflowRays += count;
                        if(rayStore.currRay != nullptr)
                            rayStore.currRay->overflowCount += count;
                    }
                    else {
                        //for (uint32_t i=1; i<=count; i++) {
                        for (int32_t i=count*(-1./2); i<=count/2.; i+=1) {
                            Vec3f reflectionDirection = normalize(diffuse(dir, N, i*4.0/count));
                            insideObject = (dotProduct(reflectionDirection, N) < 0);
                            Vec3f reflectionRayOrig = insideObject ?
                                hitPoint - N * options.bias :
                                hitPoint + N * options.bias;
                            rayStore.diffuseRays++;
                            //std::printf("DEBUG--(%d, %d)-->Total rays(%d) = Origin rays(%d) + Reflection rays(%d) + Refraction rays(%d) + Diffuse rays(%d)\n", i, depth, rays.totalRays, rays.originRays, rays.reflectionRays, rays.refractionRays, rays.diffuseRays);
                            // tracker the ray
                            if(rayStore.currRay != nullptr) {
                                newRay = rayStore.record(RAY_TYPE_DIFFUSE, &rayStore.currRay->diffuseLink, 0, reflectionRayOrig, reflectionDirection);
                                newRay->inside = insideObject;
                                currRay = rayStore.currRay;
                                rayStore.currRay = newRay;
                            }
                            Vec3f deltaAmt = 0;
                            backwardCastRay(rayStore, reflectionRayOrig, reflectionDirection, objects, lights, options, depth + 1, false, false, &deltaAmt);
                            rayStore.currRay = currRay;
                            globalAmt += deltaAmt * powf(weight, depth+1);
                        }
                    }
                }

                // [comment]
                // Loop over all lights in the scene and sum their contribution up
                // We also apply the lambert cosine law here though we haven't explained yet what this means.
                // [/comment]
                Vec3f tmpAmt = 0;
                for (uint32_t i = 0; i < lights.size(); ++i) {
                    Vec3f lightDir = lights[i]->position - hitPoint;
                    // square of the distance between hitPoint and the light
                    float lightDistance2 = dotProduct(lightDir, lightDir);
                    lightDir = normalize(lightDir);
                    if (!withLightRender) {
                        float LdotN = std::max(0.f, dotProduct(lightDir, N));
                        Object *shadowHitObject = nullptr;
                        Surface *shadowHitSurface = nullptr;
                        SurfaceAngle *shadowHitAngle = nullptr;
                        Vec3f shadowHitPoint = 0;
                        Vec2f shadowMapIdx = 0;
                        float tNearShadow = kInfinity;
                        // is the point in shadow, and is the nearest occluding object closer to the object than the light itself?
                        bool inShadow = trace(shadowPointOrig, lightDir, objects, tNearShadow, shadowHitPoint, shadowMapIdx, 
                              &shadowHitSurface, &shadowHitAngle, &shadowHitObject) && tNearShadow * tNearShadow < lightDistance2;

/*
                        if (inShadow)
                            std::printf("inShadow: obj(%s), point(%f)\n", shadowHitObject->name.c_str(), tNearShadow);
*/
                        tmpAmt = (1 - inShadow) * lights[i]->intensity * LdotN * hitObject->Kd;
                        if (pDeltaAmt != nullptr)
                            *pDeltaAmt += tmpAmt;
                        else
                            localAmt += tmpAmt;
                            
                    }
                    Vec3f reflectionDirection = reflect(-lightDir, N);
                    specularColor += powf(std::max(0.f, -dotProduct(reflectionDirection, dir)), hitObject->specularExponent) * lights[i]->intensity;
                }
                if (withLightRender) {
                    globalAmt = hitSurface->diffuseAmt;
                    localAmt = 0;
                }

                if (tmpAmt == 0) rayStore.invisibleRays++;
                else rayStore.validRays++;

                hitColor = (globalAmt + localAmt) * hitObject->evalDiffuseColor(mapIdx) + specularColor * hitObject->Ks;

/*
                if(rayStore.currPixel.x < 240.)
                    std::printf("o(%f,%f), object(%s), point(%f,%f,%f), mapidx(%f,%f)\n", 
                                rayStore.currPixel.x, rayStore.currPixel.y, hitObject->name.c_str(),
                                hitPoint.x, hitPoint.y, hitPoint.z, mapIdx.x, mapIdx.y);
*/
                            
                break;
            }
        }
    }
    else {
        rayStore.nohitRays++;
        //std::printf("%*s%d nohit\n", depth+1, "#", depth+1);
        if(rayStore.currRay != nullptr) {
            rayStore.currRay->status = NOHIT_RAY;
            rayStore.currRay->nohitCount++;
        }
    }

    return hitColor;
}

/* objectRender the object from Surface angles */
void objectRender(
    RayStore &rayStore,
    const Options &options,
    const std::vector<std::unique_ptr<Object>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights)
{
    Object *targetObject;
    Surface *targetSurface;
    Vec3f   target;
    Vec3f   dir = 0;
    Vec3f   orig = 0;
    uint32_t v=0, h=0;

//#define DEBUG_ANGLE_ZERO

    for (uint32_t i=0; i<objects.size(); i++) {
        targetObject = objects[i].get();

        if (targetObject->surfaceAngleRatio <= 0.) continue;
        for (v=0; v<objects[i]->vRes; v++) {
            for (h=0; h<objects[i]->hRes; h++) {
                targetSurface = targetObject->getSurfaceByVH(v, h, &target);
                if (targetSurface == nullptr) continue;

                // LEO: debug a angle color
#ifdef DEBUG_ANGLE_ZERO
                Vec3f debugDir = normalize(Vec3f(0) - target);
                uint32_t vAngleTarget = 0, hAngleTarget = 0;
                targetSurface->getSurfaceAngleByDir(debugDir, &vAngleTarget, &hAngleTarget);
#endif

                for (uint32_t vAngle=0; vAngle<targetSurface->vAngleRes; vAngle++) {
                    for (uint32_t hAngle=0; hAngle<targetSurface->hAngleRes; hAngle++) {
                        SurfaceAngle *angle = targetSurface->getSurfaceAngleByVH(vAngle, hAngle, &dir);
                        if (angle == nullptr) continue;

#ifdef DEBUG_ANGLE_ZERO
                        if (abs(vAngleTarget-vAngle)<5*ceil(targetSurface->angleRatio) && \
                            abs(hAngleTarget-hAngle)<5*ceil(targetSurface->angleRatio)) {
#endif
                            // dir of forwordCastRay is relative to orig
                            rayStore.originRays++;
                            // tracker the ray
                            if (objects[i]->recorderEnabled)
                                rayStore.record(RAY_TYPE_ORIG, objects[i]->traceLinks, v*objects[i]->hRes + h, orig, dir);
                            orig = target + dir;
                            rayStore.currPixel = {(float)v, (float)h, 0};
                            angle->angleColor = backwardCastRay(rayStore, orig, -dir, objects, lights, options, 0);
                            //std::cout << angle->angleColor <<  std::endl;
#ifdef DEBUG_ANGLE_ZERO
                        }
#endif
                    }
                }
                // rayStore.dumpObjectTraceLink(objects, i, 0, 0);
                // dump object shadepoint as ppm file
                //objects[i]->dumpSurfaceAngles(options);
            }
        }
        objects[i]->dumpSurfaceAngles(options);
    }

    // LEO: debug a angle color
    //ofs.close();
}


/* lightRender the object from light */
void lightRender(
    RayStore &rayStore,
    const Options &options,
    const std::vector<std::unique_ptr<Object>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights)
{
    Object *targetObject;
    Surface *targetSurface;
    Vec3f   targetPoint;
    Vec3f   testPoint;
    Vec3f orig = 0;
    uint32_t v=0, h=0;

    for (uint32_t l=0; l<lights.size(); l++) {
        orig = lights[l]->position;
        for (uint32_t i=0; i<objects.size(); i++) {
            targetObject = objects[i].get();
            for (v=0; v<objects[i]->vRes; v++) {
                for (h=0; h<objects[i]->hRes; h++) {
                targetSurface = targetObject->getSurfaceByVH(v, h, &targetPoint);
                if (targetSurface == nullptr)
                    continue;
                // dir of forwordCastRay is relative to orig
                // dir = center + P.rel(theta, phi)*radius - orig
                // set the test point a little bit far away the center of sphere. test point is rel address from orig.
                testPoint = targetPoint + targetSurface->N*options.bias - orig;
                testPoint = normalize(testPoint);
                rayStore.originRays++;
                // tracker the ray
                if (objects[i]->recorderEnabled)
                    rayStore.record(RAY_TYPE_ORIG, objects[i]->traceLinks, v*objects[i]->hRes + h, orig, testPoint);
/*
                assert( l < LIGHT_NUM_MAX && i < OBJ_NUM_MAX && v < V_RES_MAX && h < H_RES_MAX );
                rayStore.record(RAY_TYPE_ORIG, rayStore.lightTraceLinks, 
                                l*OBJ_NUM_MAX*V_RES_MAX*H_RES_MAX + i*V_RES_MAX*H_RES_MAX + v*H_RES_MAX + h,
                                orig, testPoint);
*/
    /*
                std::printf("v/h(%d,%d): targetPoint[%f,%f,%f], testPoint[%f,%f,%f]\n",
                            v, h, targetPoint.x, targetPoint.y, targetPoint.z,
                            testPoint.x,testPoint.y, testPoint.z);
    */
                rayStore.currPixel = {(float)v, (float)h, 0};
                forwordCastRay(rayStore, orig, testPoint, objects, lights[l]->intensity, options, 0, targetObject, targetSurface, targetPoint);
                //std::printf("light[%d]:%.0f%%\r",l, (h*vRes+v)*100.0/(vRes*hRes));
                }
            }
            rayStore.dumpObjectTraceLink(objects, i, 0, 0);
            // dump object shadepoint as ppm file
            objects[i]->dumpSurface(options);
        }
    }
}


// [comment]
// The main eyeRender function. This where we iterate over all pixels in the image, generate
// primary rays and cast these rays into the scene. The content of the framebuffer is
// saved to a file.
// [/comment]
void eyeRender(
    RayStore &rayStore,
    char * outfile,
    const Options &options,
    const Vec3f &viewpoint,
    const std::vector<std::unique_ptr<Object>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights,
    const bool withLightRender = false,
    const bool withObjectRender = false)
{
    Vec3f orig = viewpoint;
    // change the camera to world
#ifdef CAMERATOWORLD
    Vec3f origWorld, dirWorld;
    Matrix44f cameraToWorld(orig, orig+Vec3f{0.,0.,-1.});
    std::cout << cameraToWorld << std::endl;
#endif
    Vec3f *framebuffer = new Vec3f[options.width * options.height];
    Vec3f *pix = framebuffer;
    float scale = tan(deg2rad(options.fov * 0.5));
    float imageAspectRatio = options.width / (float)options.height;
    //Vec3f orig(0);
#if 1
    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)options.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)options.height) * scale;
            Vec3f dir = normalize(Vec3f(x, y, -1));
            rayStore.originRays++;
            rayStore.currPixel = {(float)j, (float)i, -1.0};
            // tracker the ray
            rayStore.record(RAY_TYPE_ORIG, rayStore.eyeTraceLinks, j*VIEW_WIDTH+i, orig, dir);

//DEBUG by LEO to compare backward tracing and forward tracing
#else
    for (uint32_t i = 0; i < options.width; ++i) {
        for (uint32_t j = 0; j < options.height; ++j) {
            Object *obj = objects[1].get();
            uint32_t h = (uint32_t)((float)j * obj->hRes / options.height);
            uint32_t v = (uint32_t)((float)i * obj->vRes / options.width);
            Vec3f  worldTarget;
            obj->getSurfaceByVH(v, h, &worldTarget);
            Vec3f dir = normalize(worldTarget-orig);


#endif

#ifdef CAMERATOWORLD
            cameraToWorld.multVecMatrix(orig, origWorld);
            cameraToWorld.multDirMatrix(dir, dirWorld);
            dirWorld.normalize();
            *(pix++) = backwardCastRay(rayStore, origWorld, dirWorld, objects, lights, options, 0, withLightRender, withObjectRender);
#else
            *(pix++) = backwardCastRay(rayStore, orig, dir, objects, lights, options, 0, withLightRender, withObjectRender);
#endif

#if 0
            std::cout << "oooo===" << v << "," << h << "," << 0 << "," << 0 << *(pix-1) << "===" << std::endl;
            std::cout << dir << std::endl;
//            std::printf("%.0f%%\r",(j*options.width+i)*100.0/(options.width * options.height));
#endif
        }
        //std::printf("%f\r",(j*1.0/options.height));
    }

    // save framebuffer to file
    std::ofstream ofs;
    /* text file for compare */
    ofs.open(outfile);
    ofs << "P3\n" << options.width << " " << options.height << "\n255\n";
    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            int r = (int)(255 * clamp(0, 1, framebuffer[j*options.width + i].x));
            int g = (int)(255 * clamp(0, 1, framebuffer[j*options.width + i].y));
            int b = (int)(255 * clamp(0, 1, framebuffer[j*options.width + i].z));
            ofs << r << " " << g << " " << b << "\n ";
        }
    }
    ofs.close();

/*
    ofs.open(outfile);

    ofs << "P6\n" << options.width << " " << options.height << "\n255\n";
    for (uint32_t i = 0; i < options.height * options.width; ++i) {
        char r = (char)(255 * clamp(0, 1, framebuffer[i].x));
        char g = (char)(255 * clamp(0, 1, framebuffer[i].y));
        char b = (char)(255 * clamp(0, 1, framebuffer[i].z));
        ofs << r << g << b;
    }
    ofs.close();
*/


    delete [] framebuffer;
}


// [comment]
// In the main function of the program, we create the scene (create objects and lights)
// as well as set the options for the eyeRender (image widht and height, maximum recursion
// depth, field-of-view, etc.). We then call the eyeRender function().
// [/comment]
int main(int argc, char **argv)
{
    // creating the scene (adding objects and lights)
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Light>> lights;
    
    
    Sphere *sph1 = new Sphere("sph1", DIFFUSE_AND_GLOSSY, Vec3f(-4, 0, -8), 2);
    sph1->ior = 1.3;
    sph1->Kd  = 0.8;
    sph1->diffuseColor = Vec3f(0.6, 0.7, 0.8);
    //sph1->enableRecorder();
    objects.push_back(std::unique_ptr<Sphere>(sph1));


/*
    Sphere *sph2 = new Sphere("sph2", REFLECTION_AND_REFRACTION, Vec3f(4, 0, -8), 2);
    sph2->ior = 1.7;
    sph2->Kd  = 0.0;
    sph2->diffuseColor = Vec3f(0.6, 0.7, 0.8);
    //sph2->enableRecorder();
    objects.push_back(std::unique_ptr<Sphere>(sph2));
*/


    Vec3f verts[4] = {{-10,-2,0}, {10,-2,0}, {10,-2,-14}, {-10,-2,-14}};
    uint32_t vertIndex[6] = {0, 1, 3, 1, 2, 3};
    Vec2f st[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    MeshTriangle *mesh1 = new MeshTriangle("mesh1", REFLECTION, verts, vertIndex, 2, st);
    mesh1->ior = 1.5;
    mesh1->Kd  = 0.1;
    mesh1->localDiffuseColor = Vec3f{0.3843, 0.3569, 0.3412};
    objects.push_back(std::unique_ptr<MeshTriangle>(mesh1));

    

    Vec3f verts2[4] = {{-10,-2,-14}, {10,-2,-14}, {10,18,-14}, {-10,18,-14}};
    uint32_t vertIndex2[6] = {0, 1, 3, 1, 2, 3};
    Vec2f st2[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    MeshTriangle *mesh2 = new MeshTriangle("mesh2", DIFFUSE_AND_GLOSSY, verts2, vertIndex2, 2, st2);
    mesh2->ior = 1.3;
    mesh2->Kd  = 0.8;
    mesh2->localDiffuseColor = -1.;
    objects.push_back(std::unique_ptr<MeshTriangle>(mesh2));



    lights.push_back(std::unique_ptr<Light>(new Light(Vec3f(20, 25, 8), 1)));
//    lights.push_back(std::unique_ptr<Light>(new Light(Vec3f(1, 1, 10), 1)));

    // setting up options
    Options options[100];
    std::memset(options, 0, sizeof(options));
    // no diffuse at all
    options[0].diffuseSpliter = 3;
    options[0].maxDepth = 5;
    options[0].spp = 1;
    options[0].width = VIEW_WIDTH*options[0].spp;
    options[0].height = VIEW_HEIGHT*options[0].spp;
    options[0].fov = 90;
    //options[0].backgroundColor = Vec3f(0.235294, 0.67451, 0.843137);
    options[0].backgroundColor = Vec3f(0.95, 0.95, 0.95);
    //options[0].backgroundColor = Vec3f(0.0);
    //options[0].bias = 0.001;
    options[0].bias = 0.001;
    options[0].doTraditionalRender = true;
    options[0].doRenderAfterDiffusePreprocess = true;
    options[0].doRenderAfterDiffuseAndReflectPreprocess = true;

/*
    options[0].viewpoints[0] = Vec3f(0, 5, 0);
    options[0].viewpoints[1] = Vec3f(-5, 0, -4);
    //options[0].viewpoints[2] = Vec3f(5, 0, 0);
    options[0].viewpoints[3] = Vec3f(2, 0, 0);
    options[0].viewpoints[4] = Vec3f(-2, 0, 0);
*/
    //Vec3f viewpoints[4] = {{0,0,0}, {0,0,1}, {1,1,1}, {6,6,1}};

#if 0
    // 10 diffuse at each first hitpoint, no further diffuse
    options[1].diffuseSpliter = 10;
    options[1].maxDepth = 1;
    options[1].spp = 1;
    options[1].width = VIEW_WIDTH*options[1].spp;
    options[1].height = VIEW_HEIGHT*options[1].spp;
    options[1].fov = 90;
    options[1].backgroundColor = Vec3f(0.0);
    options[1].bias = 0.001;

    // 10 diffuse at each first hitpoint, 1 depth further diffuse
    options[2].diffuseSpliter = 1;
    options[2].maxDepth = 1;
    options[2].spp = 1;
    options[2].width = VIEW_WIDTH*options[2].spp;
    options[2].height = VIEW_HEIGHT*options[2].spp;
    options[2].fov = 90;
    options[2].backgroundColor = Vec3f(0.0);
    options[2].bias = 0.0001;

    // 10 diffuse at each first hitpoint, 3 depth further diffuse
    options[3].diffuseSpliter = 1;
    options[3].maxDepth = 3;
    options[3].spp = 1;
    options[3].width = VIEW_WIDTH*options[3].spp;
    options[3].height = VIEW_HEIGHT*options[3].spp;
    options[3].fov = 90;
    options[3].backgroundColor = Vec3f(0.0);
    options[3].bias = 0.0001;

    // 10 diffuse at each first hitpoint, 3 depth further diffuse
    options[4].diffuseSpliter = 1;
    options[4].maxDepth = 9;
    options[4].spp = 1;
    options[4].width = VIEW_WIDTH*options[4].spp;
    options[4].height = VIEW_HEIGHT*options[4].spp;
    options[4].fov = 90;
    options[4].backgroundColor = Vec3f(0.0);
    options[4].bias = 0.0001;

    // 10 diffuse at each first hitpoint, 3 depth further diffuse
    options[5].diffuseSpliter = 100;
    options[5].maxDepth = 1;
    options[5].spp = 1;
    options[5].width = VIEW_WIDTH*options[5].spp;
    options[5].height = VIEW_HEIGHT*options[5].spp;
    options[5].fov = 90;
    options[5].backgroundColor = Vec3f(0.0);
    options[5].bias = 0.0001;

    // 10 diffuse at each first hitpoint, 3 depth further diffuse
    options[6].diffuseSpliter = 1000;
    options[6].maxDepth = 1;
    options[6].spp = 1;
    options[6].width = VIEW_WIDTH*options[6].spp;
    options[6].height = VIEW_HEIGHT*options[6].spp;
    options[6].fov = 90;
    options[6].backgroundColor = Vec3f(0.0);
    options[6].bias = 0.0001;
#endif

    // setting up ray store
    //RayStore rayStore;

    char outfile[256];
    RayStore *rayStore;

    std::printf("%-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s\n", 
                "split", "depth", 
                "origin", "reflect", "refract", "diffuse", 
                "nohit", "invis", "weak", "overflow", "CPU(Rays)", "TIME(S)", "MEM(GB)");
    time_t start, end;
    //std::printf("split\t depth\t total\t origin\t reflect\t refract\t diffuse\t nohit\t invis\t overflow\t CPUConsumed\n");
    for (int i =0; i<sizeof(options)/sizeof(struct Options); i++){
        if(options[i].width == 0) break;

        for (uint32_t i=0; i<objects.size(); i++) {
            objects[i]->reset();
        }

        if (options[i].doRenderAfterDiffusePreprocess == true || options[i].doRenderAfterDiffuseAndReflectPreprocess == true) {
            // do lightRender
            // setting up ray store
            rayStore = new RayStore(options[i]);
            // caculate time consumed
            start = time(NULL);
            lightRender(*rayStore, options[i], objects, lights);
            end = time(NULL);
            std::printf("###pre render for doRenderAfterDiffusePreprocess & doRenderAfterDiffuseAndReflectPreprocess from light###\n");
            std::printf("%-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10.0f %-10.2f\n",
                        options[i].diffuseSpliter, options[i].maxDepth,
                        rayStore->originRays, rayStore->reflectionRays, rayStore->refractionRays, rayStore->diffuseRays,
                        rayStore->nohitRays, rayStore->invisibleRays, rayStore->weakRays, rayStore->overflowRays, 
                        rayStore->totalRays, difftime(end, start), rayStore->totalMem*1.0/(1024.0*1024.0*1024.0));
            delete rayStore;
        }
        if (options[i].doRenderAfterDiffusePreprocess == true) {
            // do post render from eyes after lightRender
            // calcule all the viewpoints with same options 
            std::printf("###post render for doRenderAfterDiffusePreprocess###\n");
            for (int j =0; j<sizeof(options[i].viewpoints)/sizeof(Vec3f); j++) {
                rayStore = new RayStore(options[i]);
                //std::memset(&rayStore, 0, sizeof(rayStore));
                std::sprintf(outfile,
                    "afterDiffusePreprocess_x.%d_y.%d_z.%d_density.%.2f_dep.%d_spp.%d_split.%d.ppm", (int)options[i].viewpoints[j].x,
                    (int)options[i].viewpoints[j].y, (int)options[i].viewpoints[j].z, RAY_CAST_DESITY, options[i].maxDepth, options[i].spp,
                    options[i].diffuseSpliter);
                // caculate time consumed
                start = time(NULL);
                // finally, eyeRender
                //std::memset(&rayStore, 0, sizeof(rayStore));
                /* start eyeRender after lightRender */
                eyeRender(*rayStore, outfile, options[i], options[i].viewpoints[j], objects, lights, true, false);
                end = time(NULL);
                std::printf("%-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10.0f %-10.2f\n",
                            options[i].diffuseSpliter, options[i].maxDepth,
                            rayStore->originRays, rayStore->reflectionRays, rayStore->refractionRays, rayStore->diffuseRays,
                            rayStore->nohitRays, rayStore->invisibleRays, rayStore->weakRays, rayStore->overflowRays, 
                            rayStore->totalRays, difftime(end, start), rayStore->totalMem*1.0/(1024.0*1024.0*1024.0));

                delete rayStore;
                // (0,0,0) is the default viewpoint, and it means the end of the list
                if (options[i].viewpoints[j] == 0)
                    break;
            }
        }

        if (options[i].doRenderAfterDiffuseAndReflectPreprocess == true) {
            // do objectRender
            // setting up ray store
            rayStore = new RayStore(options[i]);
            // caculate time consumed
            std::printf("###pre render for doRenderAfterDiffuseAndReflectPreprocess from object surface angle###\n");
            start = time(NULL);
            objectRender(*rayStore, options[i], objects, lights);
            end = time(NULL);
            std::printf("%-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10.0f %-10.2f\n",
                        options[i].diffuseSpliter, options[i].maxDepth,
                        rayStore->originRays, rayStore->reflectionRays, rayStore->refractionRays, rayStore->diffuseRays,
                        rayStore->nohitRays, rayStore->invisibleRays, rayStore->weakRays, rayStore->overflowRays, 
                        rayStore->totalRays, difftime(end, start), rayStore->totalMem*1.0/(1024.0*1024.0*1024.0));
            delete rayStore;

            // do post render from eyes after lightRender
            // calcule all the viewpoints with same options 
            std::printf("###post render for doRenderAfterDiffuseAndReflectPreprocess###\n");
            for (int j =0; j<sizeof(options[i].viewpoints)/sizeof(Vec3f); j++) {
                rayStore = new RayStore(options[i]);
                //std::memset(&rayStore, 0, sizeof(rayStore));
                std::sprintf(outfile,
                    "afterReflectPreprocess_x.%d_y.%d_z.%d_density.%.2f_dep.%d_spp.%d_split.%d.ppm", (int)options[i].viewpoints[j].x,
                    (int)options[i].viewpoints[j].y, (int)options[i].viewpoints[j].z, RAY_CAST_DESITY, options[i].maxDepth, options[i].spp,
                    options[i].diffuseSpliter);
                // caculate time consumed
                start = time(NULL);
                // finally, eyeRender
                //std::memset(&rayStore, 0, sizeof(rayStore));
                /* start eyeRender after lightRender */
                eyeRender(*rayStore, outfile, options[i], options[i].viewpoints[j], objects, lights, true, true);
                end = time(NULL);
                std::printf("%-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10.0f %-10.2f\n",
                            options[i].diffuseSpliter, options[i].maxDepth,
                            rayStore->originRays, rayStore->reflectionRays, rayStore->refractionRays, rayStore->diffuseRays,
                            rayStore->nohitRays, rayStore->invisibleRays, rayStore->weakRays, rayStore->overflowRays, 
                            rayStore->totalRays, difftime(end, start), rayStore->totalMem*1.0/(1024.0*1024.0*1024.0));

                delete rayStore;
                // (0,0,0) is the default viewpoint, and it means the end of the list
                if (options[i].viewpoints[j] == 0)
                    break;
            }
        }

        if (options[i].doTraditionalRender == true) {
            // do pure render from eyes without lightRender
            // calcule all the viewpoints with same options 
            std::printf("###traditional render from eye###\n");
            for (int j =0; j<sizeof(options[i].viewpoints)/sizeof(Vec3f); j++) {
                //std::memset(&rayStore, 0, sizeof(rayStore));
                rayStore = new RayStore(options[i]);
                std::sprintf(outfile,
                    "traditional_x.%d_y.%d_z.%d_density.%.2f_dep.%d_spp.%d_split.%d.ppm", (int)options[i].viewpoints[j].x,
                    (int)options[i].viewpoints[j].y, (int)options[i].viewpoints[j].z, RAY_CAST_DESITY, options[i].maxDepth, options[i].spp,
                    options[i].diffuseSpliter);
                // caculate time consumed
                start = time(NULL);
                // finally, eyeRender
                //std::memset(&rayStore, 0, sizeof(rayStore));
                /* start eyeRender after lightRender */
                eyeRender(*rayStore, outfile, options[i], options[i].viewpoints[j], objects, lights, false, false);
                end = time(NULL);
                std::printf("%-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10.0f %-10.2f\n",
                            options[i].diffuseSpliter, options[i].maxDepth,
                            rayStore->originRays, rayStore->reflectionRays, rayStore->refractionRays, rayStore->diffuseRays,
                            rayStore->nohitRays, rayStore->invisibleRays, rayStore->weakRays, rayStore->overflowRays, 
                            rayStore->totalRays, difftime(end, start), rayStore->totalMem*1.0/(1024.0*1024.0*1024.0));

                rayStore->dumpEyeTraceLink(222, 340);

                delete rayStore;
                // (0,0,0) is the default viewpoint, and it means the end of the list
                if (options[i].viewpoints[j] == 0)
                    break;
            }
        }
    }

    return 0;
}
