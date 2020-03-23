#include "Vec2.h"
#include "Vec3.h"
#include "Matrix44.h"
#include "utils.h"
#define MY_UINT64_T     uint64_t
#define VIEW_WIDTH      640
#define VIEW_HEIGHT     480
#define RAY_CAST_DESITY 0.1

int main(){
    Vec3f x = Vec3f(1, 0, 0);
    Vec3f y = Vec3f(0, 1, 0);
    Vec3f z = Vec3f(0, 0, 1);
    float low = 0., high = 1., value = 0.65;
    float deg = 90.;
    Vec3f v0 = Vec3f(1,0,0), v1 = Vec3f(0, 1, 0), v2 = Vec3f(0, 0, 1);
    Vec3f orig = Vec3f(0, 0, 0);
    Vec3f dir = Vec3f(2, 2, 2);
    float tnear, u, v;
    bool hitted = rayTriangleIntersect(orig, dir, v0, v1, v2, tnear, u, v);
    if (hitted)
        std::cout<<"hitted"<<std::endl;
    else
        std::cout<<"unhitted"<<std::endl;
    std::cout<<tnear<<std::endl;
}

