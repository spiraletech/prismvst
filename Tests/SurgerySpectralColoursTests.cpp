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
    const auto a4=pitchAt(440),a5=pitchAt(880),c4=pitchAt(261.625565f);
    if(!a4.valid||a4.note!=9||a4.octave!=4||std::abs(a4.cents)>0.001)return 9;
    if(a5.note!=9||a5.octave!=5||c4.note!=0||c4.octave!=4)return 10;
    if(pitchAt(NAN).valid||pitchAt(0).valid||pitchAt(INFINITY).valid)return 11;
    // Distinct warm, green, cyan and blue zones, not a rose-only palette.
    const auto yellow=colourAt(350),green=colourAt(750),cyan=colourAt(3500),blue=colourAt(7000);
    if(yellow.r<.8f||yellow.g<.8f||green.g<green.r*2||cyan.b<.8f||blue.b<blue.r*2)return 12;
    for(float hz=20.0f;hz<=20000.0f;hz*=1.27f){
        const auto f=colourAt(hz),repeat=colourAt(hz);
        if(!std::isfinite(f.r)||f.r<0||f.r>1||f.g<0||f.g>1||f.b<0||f.b>1)return 13;
        if(f.r!=repeat.r||f.g!=repeat.g||f.b!=repeat.b)return 14;
    }
    if(!std::isfinite(colourAt(NAN).r)||!std::isfinite(plotFraction(NAN)))return 15;
    std::cout<<"Synesthesia V1: full rainbow, fixed hues, level brightness, silence and approximate 12-TET labels passed\n";
}
