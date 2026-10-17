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
    // Pearlescent shimmer must be silent when audio is silent,
    // responsive to measured energy, and animated only by display phase.
    const auto silent=foil(700.0f,-144.0f,0.25f);
    const auto soft=foil(700.0f,-64.0f,0.25f);
    const auto loud=foil(700.0f,-9.0f,0.25f);
    if(silent.strength!=0.0f)return 9;
    if(!(soft.strength>0.0f&&loud.strength>soft.strength))return 10;
    const auto moved=foil(700.0f,-9.0f,0.72f);
    if(std::abs(moved.colour.r-loud.colour.r)<0.005f &&
       std::abs(moved.colour.g-loud.colour.g)<0.005f)return 11;
    for(float hz=20.0f;hz<=20000.0f;hz*=1.27f){
        const auto f=foil(hz,-12.0f,0.41f);
        if(!std::isfinite(f.strength)||f.strength<0.0f||
           f.strength>0.75f)return 12;
        if(f.colour.r<0||f.colour.r>1||
           f.colour.g<0||f.colour.g>1||
           f.colour.b<0||f.colour.b>1)return 13;
    }
    std::cout<<"Holographic hue, audio-gated shimmer and silent-field proofs passed\n";
}
