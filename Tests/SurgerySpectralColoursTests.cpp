#include "../Source/SurgerySpectralColours.h"
#include <cmath>
#include <iostream>
int main(){
    using namespace surgery::spectrum;
    if(plotFraction(20)!=0||plotFraction(20000)!=1)return 1;
    if(!(plotFraction(200)>0.32f&&plotFraction(200)<0.35f))return 2;
    if(brightness(-144)!=0||brightness(-108)!=0)return 3;
    if(!(brightness(-8)>brightness(-30)&&brightness(-30)>brightness(-70)))return 4;
    if(brightness(NAN)!=0)return 5;
    const auto sub=colourAt(40),mid=colourAt(1000),air=colourAt(16000);
    if(std::abs(sub.r-air.r)<0.20f)return 6;
    if(std::abs(mid.g-air.g)<0.10f)return 7;
    const auto lo=colourAt(0),hi=colourAt(40000);
    if(lo.r!=stops.front().value.r||hi.b!=stops.back().value.b)return 8;
    std::cout<<"Frequency hue mapping and measured-energy brightness tests passed\n";
}
