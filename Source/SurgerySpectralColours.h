#pragma once
// SURGERY EQ — frequency-addressed spectral colour and calibrated intensity.
// No random colour cycling, ROOT-themed spectrum or simulated activity.
// Pure functions usable in GUI paint and unit tests; NOT audio DSP.
#include <algorithm>
#include <array>
#include <cmath>
namespace surgery::spectrum {
struct RGB {float r,g,b;};
struct Stop {float hz;RGB value;};
// Broad hue zones emulate the readably colour-coded frequency distribution
// of modern spectrum analyzers, without copying a third-party GUI.
inline constexpr std::array<Stop,8> stops{{
    {20.0f,{0.93f,0.27f,0.36f}},     // sub: deep coral
    {65.0f,{0.98f,0.32f,0.42f}},     // bass: warm red
    {170.0f,{0.91f,0.32f,0.58f}},    // low: magenta
    {420.0f,{0.78f,0.34f,0.69f}},    // lower mids: plum
    {1200.0f,{0.90f,0.32f,0.64f}},   // mids: saturated rose
    {3500.0f,{0.98f,0.39f,0.57f}},   // presence: pink/red
    {8000.0f,{0.60f,0.47f,0.85f}},   // high: violet
    {20000.0f,{0.43f,0.58f,0.86f}}   // air: indigo blue
}};
inline RGB colourAt(float hz) noexcept {
    const float f=std::clamp(hz,20.0f,20000.0f);
    for(std::size_t i=1;i<stops.size();++i){
        if(f>stops[i].hz)continue;
        const auto& a=stops[i-1];const auto& b=stops[i];
        const float t=std::clamp(
            std::log(f/a.hz)/std::log(b.hz/a.hz),0.0f,1.0f);
        return {a.value.r+(b.value.r-a.value.r)*t,
                a.value.g+(b.value.g-a.value.g)*t,
                a.value.b+(b.value.b-a.value.b)*t};
    }
    return stops.back().value;
}
// Relative highlight factor. No signal = dark background; stronger FFT
// energy = brighter colour. Display normalization does not change audio.
inline float brightness(float db) noexcept {
    if(!std::isfinite(db))return 0.0f;
    const float u=std::clamp((db+108.0f)/102.0f,0.0f,1.0f);
    return std::pow(u,1.25f);
}
inline float plotFraction(float hz) noexcept {
    return std::clamp(std::log(std::clamp(hz,20.0f,20000.0f)/20.0f)/
                      std::log(1000.0f),0.0f,1.0f);
}
} // namespace surgery::spectrum
