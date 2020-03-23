#ifndef LIGHTH
#define LIGHTH
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "Vec3.h"

class Light
{
public:
    Light(const Vec3f &p, const Vec3f &i) : position(p), intensity(i) {}
    Vec3f position;
    Vec3f intensity;
};

#endif
