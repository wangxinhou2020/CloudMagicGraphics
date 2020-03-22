#include "Vec2.h"
#include "Vec3.h"
#include "Matrix44.h"
#include "utils.h"
#define MY_UINT64_T     uint64_t
#define VIEW_WIDTH      640
#define VIEW_HEIGHT     480
#define RAY_CAST_DESITY 0.1


int main(){
    std::cout<<"OK"<<"\n";
    Vec2f a=Vec2f(9);
    std::cout<<a<<std::endl;
    Vec3f b=Vec3f(1,1,1);
    b = normalize(b);
    std::cout<<b<<std::endl;
    Matrix44f c;
    std::cout<<c<<std::endl;
    float d = rad2deg(0.5);
    std::cout<<d<<std::endl;

}

