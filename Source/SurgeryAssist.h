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
};
template<std::size_t N> class AssistScanner final {
public:
    void reset() noexcept {last_=0;stable_=0;finding_={};}
    void observe(const std::array<float,N>& bins) noexcept {
        if constexpr(N<21)return;
        int candidate=-1;float strongest=6.0f;
        for(std::size_t i=9;i+9<N;++i){
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
        finding_.ready=stable_>=5;
        finding_.suggestedDepthDb=std::clamp(0.35f*strongest,1.0f,6.0f);
    }
    Finding finding() const noexcept {return finding_;}
private:
    int last_=0,stable_=0;
    Finding finding_{};
};
} // namespace surgery
