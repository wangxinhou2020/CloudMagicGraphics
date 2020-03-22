#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

class Light
{
public:
    Light(const Vec3f &p, const Vec3f &i) : position(p), intensity(i) {}
    Vec3f position;
    Vec3f intensity;
};

