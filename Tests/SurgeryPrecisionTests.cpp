#include "../Source/SurgeryPrecision.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using namespace surgery::precision;
    assert(rootBounds(600.0f,250.0f).lo==250.0f);
    assert(boundedFrequency(100.0f,250.0f,600.0f)==250.0f);
    assert(boundedFrequency(4000.0f,250.0f,600.0f)==600.0f);
    assert(scrubFrequency(400.0f,800.0f,10000.0f,true,250,600)<=600);
    assert(scrubFrequency(400.0f,100.0f,10000.0f,true,250,600)<420.0f);
    assert(scrubFrequency(400.0f,100.0f,10000.0f,false,250,600)==600.0f);
    assert(scrubWheelFrequency(250,1000,250,600)==600.0f);
    assert(adjustQ(4.0f,0.1f,true)>4.0f);
    assert(adjustQ(24.0f,10,false)==24.0f);
    assert(bandwidthOctaves(15.0f)<bandwidthOctaves(1.0f));
    std::cout<<"ROOT frequency clamp, CTRL scrub and numerical Q proofs passed\n";
}
