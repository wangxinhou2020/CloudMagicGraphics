#ifndef UTILSH
#define UTILSH

#include "Values.h"
#include "Vec3.h"

inline
Vec3f normalize(const Vec3f &v)
{
    float mag2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (mag2 > 0) {
        float invMag = 1 / sqrtf(mag2);
        return Vec3f(v.x * invMag, v.y * invMag, v.z * invMag);
    }

    return v;
}

inline
float dotProduct(const Vec3f &a, const Vec3f &b)
{ return a.x * b.x + a.y * b.y + a.z * b.z; }

inline
Vec3f crossProduct(const Vec3f &a, const Vec3f &b)
{
    return Vec3f(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline
float clamp(const float &lo, const float &hi, const float &v)
{ return std::max(lo, std::min(hi, v)); }

inline
float deg2rad(const float &deg)
{ return deg * M_PI / 180; }

inline
float rad2deg(const float &rad)
{ 
    float deg = rad * 180 / M_PI; 
    if (deg < 0) 
        deg += 360.0;
    return deg;
}

inline
Vec3f mix(const Vec3f &a, const Vec3f& b, const float &mixValue)
{ return a * (1 - mixValue) + b * mixValue; }

inline
bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
            -0.5 * (b + sqrt(discr)) :
            -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

bool rayTriangleIntersect(
    const Vec3f &orig, const Vec3f &dir,
    const Vec3f &v0, const Vec3f &v1, const Vec3f &v2,
    float &t, float &u, float &v)
{
    Vec3f v0v1 = v1 - v0;
    Vec3f v0v2 = v2 - v0;
    Vec3f pvec = dir.crossProduct(v0v2);
    float det = v0v1.dotProduct(pvec);

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < kEpsilon) return false;

    float invDet = 1 / det;

    Vec3f tvec = orig - v0;
    u = tvec.dotProduct(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vec3f qvec = tvec.crossProduct(v0v1);
    v = dir.dotProduct(qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    t = v0v2.dotProduct(qvec) * invDet;

    return t < 0 ? false: true;
}

// [comment]
// Compute reflection direction
// [/comment]
Vec3f reflect(const Vec3f &I, const Vec3f &N)
{
    return I - 2 * dotProduct(I, N) * N;
}

// [comment]
// Compute diffuse direction
// tanScale is tan(output)/tan(input)
// we choose [-2,2] as typical tanscale
// [/comment]
Vec3f diffuse(const Vec3f &I, const Vec3f &N, const float tanScale)
{
    return tanScale * I - (tanScale + 1) * dotProduct(I, N) * N;
}

// [comment]
// Compute refraction direction using Snell's law
//
// We need to handle with care the two possible situations:
//
//    - When the ray is inside the object
//
//    - When the ray is outside.
//
// If the ray is outside, you need to make cosi positive cosi = -N.I
//
// If the ray is inside, you need to invert the refractive indices and negate the normal N
// [/comment]
Vec3f refract(const Vec3f &I, const Vec3f &N, const float &ior)
{
    float cosi = clamp(-1, 1, dotProduct(I, N));
    float etai = 1, etat = ior;
    Vec3f n = N;
    if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
}

// [comment]
// Compute Fresnel equation
//
// \param I is the incident view direction
//
// \param N is the normal at the intersection point
//
// \param ior is the mateural refractive index
//
// \param[out] kr is the amount of light reflected
// [/comment]
inline
void fresnel(const Vec3f &I, const Vec3f &N, const float &ior, float &kr)
{
    float cosi = clamp(-1, 1, dotProduct(I, N));
    float etai = 1, etat = ior;
    if (cosi > 0) {  std::swap(etai, etat); }
    // Compute sini using Snell's law
    float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
    // Total internal reflection
    if (sint >= 1) {
        kr = 1;
    }
    else {
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
}

inline
Matrix44f lookAt(const Vec3f& from, const Vec3f& to, const Vec3f& tmp = Vec3f(0, 1, 0))
{
    Vec3f forward = normalize(from - to);
    Vec3f right = crossProduct(normalize(tmp), forward);
    Vec3f up = crossProduct(forward, right);

    Matrix44f camToWorld;

    camToWorld[0][0] = right.x;
    camToWorld[0][1] = right.y;
    camToWorld[0][2] = right.z;
    camToWorld[1][0] = up.x;
    camToWorld[1][1] = up.y;
    camToWorld[1][2] = up.z;
    camToWorld[2][0] = forward.x;
    camToWorld[2][1] = forward.y;
    camToWorld[2][2] = forward.z;

    camToWorld[3][0] = from.x;
    camToWorld[3][1] = from.y;
    camToWorld[3][2] = from.z;

    return camToWorld;
}
#endif
