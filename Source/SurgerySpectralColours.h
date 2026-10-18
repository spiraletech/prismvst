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
// Synesthesia V1: an explicit designed log-frequency -> colour correspondence.
// It is not a biological synesthesia model, source classifier or optical sensor.
inline constexpr std::array<Stop,10> stops{{
    {20.0f,{0.93f,0.10f,0.17f}},
    {60.0f,{1.00f,0.33f,0.05f}},
    {160.0f,{1.00f,0.65f,0.03f}},
    {350.0f,{0.95f,0.91f,0.05f}},
    {750.0f,{0.18f,0.87f,0.32f}},
    {1600.0f,{0.05f,0.88f,0.65f}},
    {3500.0f,{0.04f,0.79f,0.95f}},
    {7000.0f,{0.18f,0.39f,1.00f}},
    {12000.0f,{0.55f,0.22f,0.95f}},
    {20000.0f,{0.91f,0.16f,0.72f}}
}};
inline RGB colourAt(float hz) noexcept {
    const float f=std::clamp(std::isfinite(hz)?hz:20.0f,20.0f,20000.0f);
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
struct PitchReading {int note=0,octave=0;float cents=0;bool valid=false;};
inline constexpr std::array<const char*,12> noteNames{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
// Nearest 12-TET label for an FFT band frequency, A4=440 Hz.
// A label for an approximate bin is not a polyphonic pitch estimate.
inline PitchReading pitchAt(float hz) noexcept {
    if(!std::isfinite(hz)||hz<20||hz>20000)return {};
    const double semitones=69+12*std::log2((double)hz/440);
    const int midi=(int)std::round(semitones);
    return {midi%12,midi/12-1,(float)((semitones-midi)*100),true};
}
inline float plotFraction(float hz) noexcept {
    return std::clamp(std::log(std::clamp(std::isfinite(hz)?hz:20.0f,20.0f,20000.0f)/20.0f)/
                      std::log(1000.0f),0.0f,1.0f);
}
} // namespace surgery::spectrum
