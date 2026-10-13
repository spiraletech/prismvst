#pragma once
// Visual-only dynamic spectral palette. Both nodes of each root contribute.
// Frequency positions and Q follow the parameters, not fixed analyzer bins.
// No allocation; numeric helpers allow independent calibration tests.
#include <algorithm>
#include <array>
#include <cmath>
namespace prism::roots::visual {
constexpr int kRoots=7, kNodes=14;
constexpr double kGraphLowHz=1.0,kGraphHighHz=20000.0;
struct Node {double hz=1000.0,q=1.5;bool enabled=true;};
struct ColourWeights {
    std::array<double,kRoots> weight{};
    int strongest=-1;
    double confidence=0.0;
};
inline double graphUnit(double hz) noexcept {
    const double f=std::clamp(hz,kGraphLowHz,kGraphHighHz);
    return std::log(f/kGraphLowHz)/std::log(kGraphHighHz/kGraphLowHz);
}
inline double graphFrequency(double u) noexcept {
    return kGraphLowHz*std::pow(kGraphHighHz/kGraphLowHz,std::clamp(u,0.0,1.0));
}
inline ColourWeights at(double hz,const std::array<Node,kNodes>& nodes) noexcept {
    ColourWeights out{};
    const double f=std::clamp(hz,kGraphLowHz,kGraphHighHz);
    double sum=0.0,best=0.0;
    for(int i=0;i<kNodes;++i) {
        const auto& node=nodes[(size_t)i];
        if(!node.enabled)continue;
        // A real node's Q limits its footprint; the soft tails blend where
        // neighbouring roots overlap. It does not alter measured FFT values.
        const double sigma=std::clamp(1.25/std::sqrt(std::max(0.15,node.q)),0.18,2.5);
        const double octaves=std::log2(f/std::clamp(node.hz,kGraphLowHz,kGraphHighHz));
        const double z=octaves/sigma;
        const double influence=std::exp(-0.5*z*z);
        const int root=i/2;
        out.weight[(size_t)root]+=influence;
    }
    for(int r=0;r<kRoots;++r) {
        sum+=out.weight[(size_t)r];
        if(out.weight[(size_t)r]>best){best=out.weight[(size_t)r];out.strongest=r;}
    }
    out.confidence=sum>1e-9?best/sum:0.0;
    if(sum>1e-9)for(auto& w:out.weight)w/=sum;
    return out;
}
} // namespace prism::roots::visual
