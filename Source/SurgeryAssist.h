#pragma once
// SURGERY ASSIST: deterministic, explainable observation, NOT a trained ML model.
// Never executes or alters DSP. Results are proposals requiring explicit approval.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace surgery {
struct Finding {
    bool ready=false;
    float hz=0.0f, measuredDb=-144.0f, prominenceDb=0.0f;
    float suggestedDepthDb=0.0f;
    int observedFrames=0;
    float evidenceScore=0.0f; // bounded heuristic score, not an ML probability
    float lowHz=20.0f,highHz=20000.0f;
};
template<std::size_t N> class AssistScanner final {
public:
    void reset() noexcept {last_=-100;stable_=0;lastLow_=0;lastHigh_=0;finding_={};}
    void observe(const std::array<float,N>& bins,
                 float bandLow=20.0f,float bandHigh=20000.0f) noexcept {
        if constexpr(N<21)return;
        const float lo=std::max(20.0f,std::min(bandLow,bandHigh));
        const float hi=std::min(20000.0f,std::max(bandLow,bandHigh));
        if(lo!=lastLow_||hi!=lastHigh_) {
            lastLow_=lo;lastHigh_=hi;stable_=0;last_=-100;
        }
        int candidate=-1;float strongest=8.0f;
        for(std::size_t i=9;i+9<N;++i){
            const float hz=20.0f*std::pow(1000.0f,
                ((float)i+0.5f)/(float)N);
            if(hz<lo||hz>hi)continue;
            const float v=bins[i];
            if(!std::isfinite(v)||v<-70.0f)continue;
            float neighborhood=0.0f;int count=0;
            for(int j=-9;j<=9;++j){
                if(j>=-2&&j<=2)continue;
                const float sample=bins[(size_t)((int)i+j)];
                if(std::isfinite(sample)){neighborhood+=sample;++count;}
            }
            if(count==0)continue;
            const float prominence=v-neighborhood/(float)count;
            if(prominence>strongest){strongest=prominence;candidate=(int)i;}
        }
        if(candidate<0){stable_=0;finding_={};return;}
        if(std::abs(candidate-last_)<=2)++stable_;
        else stable_=1;
        last_=candidate;
        const float hz=20.0f*std::pow(1000.0f,((float)candidate+0.5f)/(float)N);
        finding_.hz=hz;finding_.measuredDb=bins[(size_t)candidate];
        finding_.prominenceDb=strongest;finding_.observedFrames=stable_;
        finding_.ready=stable_>=8;
        finding_.lowHz=lo;finding_.highHz=hi;
        finding_.evidenceScore=std::clamp(
            (strongest-8.0f)/18.0f,0.0f,1.0f)*
            std::clamp((float)stable_/15.0f,0.0f,1.0f);
        finding_.suggestedDepthDb=std::clamp(0.22f*strongest,1.0f,4.5f);
    }
    Finding finding() const noexcept {return finding_;}
private:
    int last_=-100,stable_=0;
    float lastLow_=0,lastHigh_=0;
    Finding finding_{};
};
} // namespace surgery
