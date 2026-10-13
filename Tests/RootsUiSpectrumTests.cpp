#include "../Source/RootsSpectrumColours.h"
#include "../Source/RootsAudioEngine.h"
#include <cmath>
#include <iostream>
namespace V=prism::roots::visual;
using prism::roots::v2::Engine;
using prism::roots::v2::Config;
int main(){
  // PROOF 1: Graph endpoints cover 1Hz to 20k and map exactly in both directions.
  if(std::abs(V::graphUnit(1.0))>1e-12 ||
     std::abs(V::graphUnit(20000.0)-1.0)>1e-12 ||
     std::abs(V::graphFrequency(0.0)-1.0)>1e-12 ||
     std::abs(V::graphFrequency(1.0)-20000.0)>1e-6 ||
     std::abs(V::graphFrequency(V::graphUnit(37.0))-37.0)>1e-7){
    std::cerr<<"PROOF 1 FAIL: 1Hz graph calibration\n";return 1;
  }
  std::cout<<"PROOF 1 PASS: 1 Hz to 20 kHz graph coordinates\n";
  // PROOF 2: Root 1 first node is 1Hz, other nodes min 20Hz.
  Engine core(48000);
  if(core.get(0).hz!=1.0)return 2;
  Config n0=core.get(0);n0.hz=0.1;core.config(0,n0);
  Config n1=core.get(1);n1.hz=0.1;core.config(1,n1);
  if(core.get(0).hz!=1.0 || core.get(1).hz!=20.0){
    std::cerr<<"PROOF 2 FAIL: low frequency clamp\n";return 2;
  }
  std::cout<<"PROOF 2 PASS: Root 1 first node minimum 1Hz\n";
  // PROOF 3: both nodes of each root control the same palette as they move.
  std::array<V::Node,V::kNodes> nodes{};
  for(auto& n:nodes)n.enabled=false;
  nodes[0]={60,2.0,true};nodes[1]={90,2.0,true};
  nodes[2]={800,2.0,true};nodes[3]={1200,2.0,true};
  if(V::at(70,nodes).strongest!=0 || V::at(1000,nodes).strongest!=1)
    return 3;
  // Moving BOTH roots changes the chosen palette at fixed 70Hz.
  nodes[0].hz=5000;nodes[1].hz=7000;
  nodes[2].hz=50;nodes[3].hz=100;
  if(V::at(70,nodes).strongest!=1){
    std::cerr<<"PROOF 3 FAIL: bands not following node movement\n";return 3;
  }
  std::cout<<"PROOF 3 PASS: spectral colors respond to both moving nodes\n";
  // PROOF 4: Overlaps yield finite normalized blends, and 1Hz biquad stable.
  for(int i=0;i<V::kNodes;++i)nodes[(size_t)i]={500.0,0.7,true};
  const auto mix=V::at(500,nodes);
  double sum=0;
  for(auto w:mix.weight)sum+=w;
  if(!std::isfinite(sum)||std::abs(sum-1.0)>1e-10) {
    std::cerr<<"PROOF 4 FAIL: blended palette\n";return 4;
  }
  n0=core.get(0);n0.hz=1.0;n0.gainDb=9.0;n0.q=1.5;
  core.config(0,n0);
  float left[256]{},right[256]{};
  left[0]=0.1f;right[0]=0.1f;
  for(int i=0;i<200;++i){
    // Test the filter's impulse tail, not 200 repeated gain stages.
    if(i>0){for(int k=0;k<256;++k){left[k]=0.0f;right[k]=0.0f;}}
    core.process(left,right,256);
    for(int k=0;k<256;++k)if(!std::isfinite(left[k])||!std::isfinite(right[k]))
      return 4;
  }
  std::cout<<"PROOF 4 PASS: overlapping colors, stable 1Hz filter\n";
  return 0;
}
