#pragma once
// PRISM ROOTS realtime engine v2: 14 independent overlapping EQ bands, stereo
// correlation guard, frequency-band audition and inspired compressor profiles.
// Standalone C++20; no allocation, locks, logging or DAW calls in process().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace prism::roots::v2 {
constexpr int kTerritories=7,kNodes=14;
enum class Domain:int {stereo=0,mid=1,side=2};
enum class Compressor:int {off=0,optical3=1,optical4=2,tube3=3,fet4=4};
enum class Detector:int {linked=0,dual=1,mid=2,side=3};
struct Config {
    double hz=1000.0,gainDb=0.0,q=1.0,space=50.0,thresholdDb=-18.0;
    Domain domain=Domain::stereo;
    Compressor comp=Compressor::off;
    Detector detector=Detector::linked;
    bool enabled=true,solo=false;
};
struct Meter { double correlation=1.0,midRms=0.0,sideRms=0.0,spaceApplied=1.0,largestGrDb=0.0; };

class Filter {
public:
    void peak(double sr,const Config& c) noexcept {
        const double f=std::clamp(c.hz,1.0,std::min(20000.0,sr*0.45));
        const double w=2.0*std::numbers::pi*f/sr;
        const double a=std::sin(w)/(2.0*std::clamp(c.q,0.15,24.0));
        const double g=std::pow(10.0,std::clamp(c.gainDb,-60.0,60.0)/40.0);
        const double norm=1.0/(1.0+a/g);
        b0_=(1.0+a*g)*norm; b1_=-2.0*std::cos(w)*norm;
        b2_=(1.0-a*g)*norm; a1_=b1_; a2_=(1.0-a/g)*norm;
    }
    // RBJ constant skirt-gain band pass: peak is Q, so normalization by
    // Q gives unity at center frequency. Spectral audition is frequency-only,
    // NOT blind instrument/source separation.
    void tap(double sr,const Config& c) noexcept {
        const double w=2.0*std::numbers::pi*std::clamp(
                         c.hz,1.0,std::min(20000.0,sr*0.45))/sr;
        const double a=std::sin(w)/(2.0*std::clamp(c.q,0.15,24.0));
        const double norm=1.0/(1.0+a);
        b0_=a*norm;b1_=0.0;b2_=-a*norm;
        a1_=-2.0*std::cos(w)*norm;a2_=(1.0-a)*norm;
    }
    double sample(double v) noexcept {
        const double out=b0_*v+z1_;
        z1_=b1_*v-a1_*out+z2_;
        z2_=b2_*v-a2_*out;
        return out;
    }
    void reset() noexcept {z1_=z2_=0.0;}
private:
    double b0_=1.0,b1_=0.0,b2_=0.0,a1_=0.0,a2_=0.0,z1_=0.0,z2_=0.0;
};

class Engine {
public:
    explicit Engine(double sampleRate=48000.0) noexcept { prepare(sampleRate); }
    static constexpr std::array<double,kNodes> defaults{
        1.,60.,85.,120.,175.,245.,370.,525.,790.,1450.,2600.,4450.,8300.,13500.
    };
    void prepare(double sampleRate) noexcept {
        sr_=std::max(8000.0,sampleRate);
        correlationAlpha_=std::exp(-1.0/(0.025*sr_));
        sideAlpha_=std::exp(-1.0/(0.008*sr_));
        globalMidPower_=globalSidePower_=0.0;appliedSpace_=1.0;
        grMax_=0.0;meter_={};
        for(int i=0;i<kNodes;++i) {
            auto& node=nodes_[(size_t)i];
            if(!haveConfig_) {node.config.hz=defaults[(size_t)i];}
            node.midEq.reset();node.sideEq.reset();
            node.midTap.reset();node.sideTap.reset();
            node.e1=node.e2=node.sideEnergy=node.midEnergy=0.0;
            updateFilters(i);
        }
        haveConfig_=true;
    }
    void config(int index, Config c) noexcept {
        if(index<0||index>=kNodes)return;
        c.hz=std::clamp(c.hz,index==0?1.0:20.0,std::min(20000.0,sr_*0.45));
        c.q=std::clamp(c.q,0.15,24.0);
        c.gainDb=std::clamp(c.gainDb,-60.0,60.0);
        c.space=std::clamp(c.space,0.0,100.0);
        c.thresholdDb=std::clamp(c.thresholdDb,-60.0,0.0);
        auto& n=nodes_[(size_t)index];
        const bool changed= n.config.hz!=c.hz || n.config.q!=c.q ||
                            n.config.gainDb!=c.gainDb;
        n.config=c;
        if(changed)updateFilters(index);
    }
    Config get(int index) const noexcept {return nodes_[(size_t)std::clamp(index,0,kNodes-1)].config;}
    void setSpace(double value) noexcept {space_=std::clamp(value,0.0,100.0);}
    void setMono(bool mono) noexcept {mono_=mono;}
    Meter meter() const noexcept {return meter_;}
    void process(float* l,float* r,std::size_t count) noexcept {
        if(!l||!r||count==0)return;
        bool solo=false, active=false;
        for(const auto& n:nodes_) {
            if(!n.config.enabled)continue;
            solo=solo||n.config.solo;
            active=active||n.config.solo||std::abs(n.config.gainDb)>0.00001||
                   n.config.comp!=Compressor::off||n.config.space!=50.0;
        }
        // Truly transparent neutral bypass for pristine A/B.
        if(!solo&&!active&&!mono_&&space_==50.0) {
            measureUnchanged(l,r,count);return;
        }
        double sumL=0.0,sumR=0.0,sumCross=0.0,sumM=0.0,sumS=0.0;
        double greatestGr=0.0;
        const double globalWidth=mono_?0.0:spaceToWidth(space_);
        for(std::size_t i=0;i<count;++i) {
            const double dryM=((double)l[i]+(double)r[i])*0.5;
            const double dryS=((double)l[i]-(double)r[i])*0.5;
            double m=dryM,s=dryS;
            if(solo){m=0.0;s=0.0;}
            for(auto& node:nodes_) {
                const Config& c=node.config;
                if(!c.enabled)continue;
                // In solo mode, multiple active nodes can be combined.
                if(solo && !c.solo)continue;
                const bool listen=solo;
                const bool usesTap=listen||c.comp!=Compressor::off||c.space!=50.0;
                double tapM=0.0,tapS=0.0;
                if(usesTap){
                    if(c.domain!=Domain::side)tapM=node.midTap.sample(dryM);
                    if(c.domain!=Domain::mid)tapS=node.sideTap.sample(dryS);
                }
                // In normal mode, each peaking EQ intersects all other EQ
                // curves by cascading its full signal contribution.
                if(!listen&&std::abs(c.gainDb)>0.00001){
                    if(c.domain!=Domain::side)m=node.midEq.sample(m);
                    if(c.domain!=Domain::mid)s=node.sideEq.sample(s);
                }
                // In audition mode gain applies to the isolated tap.
                const double auditionGain=std::pow(10.0,c.gainDb/20.0);
                if(listen){tapM*=auditionGain;tapS*=auditionGain;}
                if(usesTap){
                    const double ae=1.0-correlationAlpha_;
                    node.midEnergy+=ae*(tapM*tapM-node.midEnergy);
                    node.sideEnergy+=ae*(tapS*tapS-node.sideEnergy);
                    const double localWidth=spaceToWidth(c.space);
                    double allowedLocalWidth=localWidth;
                    if(localWidth>1.0&&node.sideEnergy>1e-15)
                       allowedLocalWidth=std::min(localWidth,
                         std::max(1.0,std::sqrt(0.98*node.midEnergy/node.sideEnergy)));
                    const double widthDiff=(allowedLocalWidth-1.0)*tapS;
                    if(listen)tapS+=widthDiff;else s+=widthDiff;
                }
                if(c.comp!=Compressor::off){
                    const double ratio=c.comp==Compressor::optical4 || c.comp==Compressor::fet4?4.0:3.0;
                    const double atk=c.comp==Compressor::fet4?0.0008:
                          c.comp==Compressor::tube3?0.012:0.010;
                    const double rel=c.comp==Compressor::fet4?0.14:
                          c.comp==Compressor::tube3?0.32:0.65;
                    const double attack=std::exp(-1.0/(atk*sr_));
                    const double release=std::exp(-1.0/(rel*sr_));
                    const double stereoPeak=std::max(std::abs(tapM+tapS),std::abs(tapM-tapS));
                    const double level1=c.detector==Detector::mid?std::abs(tapM):
                        c.detector==Detector::side?std::abs(tapS):
                        c.detector==Detector::linked?stereoPeak:std::abs(tapM);
                    const double level2=c.detector==Detector::dual?std::abs(tapS):level1;
                    node.e1=follow(node.e1,level1,attack,release);
                    node.e2=follow(node.e2,level2,attack,release);
                    const double g1=compressGain(node.e1,c.thresholdDb,ratio,c.comp);
                    const double g2=compressGain(node.e2,c.thresholdDb,ratio,c.comp);
                    const double mGain=c.detector==Detector::side?1.0:g1;
                    const double sGain=c.detector==Detector::mid?1.0:
                         c.detector==Detector::dual?g2:g1;
                    greatestGr=std::max(greatestGr,-20.0*std::log10(std::max(1e-9,std::min(mGain,sGain))));
                    if(listen){tapM*=mGain;tapS*=sGain;}
                    else{m+=tapM*(mGain-1.0);s+=tapS*(sGain-1.0);}
                }
                if(listen){m+=tapM;s+=tapS;}
            }
            globalMidPower_=correlationAlpha_*globalMidPower_+
                            (1.0-correlationAlpha_)*m*m;
            globalSidePower_=correlationAlpha_*globalSidePower_+
                             (1.0-correlationAlpha_)*s*s;
            double allowed=globalWidth;
            if(globalWidth>1.0&&globalSidePower_>1e-15)
                allowed=std::min(globalWidth,
                    std::max(1.0,std::sqrt(0.98*globalMidPower_/globalSidePower_)));
            appliedSpace_=sideAlpha_*appliedSpace_+(1.0-sideAlpha_)*allowed;
            if(mono_)appliedSpace_=0.0;
            const double outL=m+appliedSpace_*s;
            const double outR=m-appliedSpace_*s;
            l[i]=(float)outL;r[i]=(float)outR;
            sumL+=outL*outL;sumR+=outR*outR;sumCross+=outL*outR;
            sumM+=m*m;sumS+=(appliedSpace_*s)*(appliedSpace_*s);
        }
        meter_.correlation=(sumL>1e-20&&sumR>1e-20)?
           std::clamp(sumCross/std::sqrt(sumL*sumR),-1.0,1.0):1.0;
        meter_.midRms=std::sqrt(sumM/(double)count);
        meter_.sideRms=std::sqrt(sumS/(double)count);
        meter_.spaceApplied=appliedSpace_;
        meter_.largestGrDb=greatestGr;
    }
private:
    struct Node {
        Config config{};
        Filter midEq,sideEq,midTap,sideTap;
        double e1=0.0,e2=0.0,midEnergy=0.0,sideEnergy=0.0;
    };
    void updateFilters(int i) noexcept {
        auto& n=nodes_[(size_t)i];
        n.midEq.peak(sr_,n.config);n.sideEq.peak(sr_,n.config);
        n.midTap.tap(sr_,n.config);n.sideTap.tap(sr_,n.config);
    }
    static double spaceToWidth(double space) noexcept {
        return space<=50.0?space/50.0:1.0+(space-50.0)/100.0;
    }
    static double follow(double env,double level,double attack,double release) noexcept {
        const double coefficient=level>env?attack:release;
        return coefficient*env+(1.0-coefficient)*level;
    }
    static double compressGain(double env,double thresholdDb,double ratio,
                               Compressor profile) noexcept {
        const double level=20.0*std::log10(std::max(1e-12,env));
        // Soft knee specifically for opto/tube; FET intentionally firmer.
        const double knee=profile==Compressor::fet4?2.0:8.0;
        const double over=level-thresholdDb;
        const double gainReduction=over<=-knee*0.5?0.0:
            over>=knee*0.5?(1.0-1.0/ratio)*over:
            (1.0-1.0/ratio)*std::pow(over+knee*0.5,2.0)/(2.0*knee);
        return std::pow(10.0,-std::max(0.0,gainReduction)/20.0);
    }
    void measureUnchanged(const float* l,const float* r,std::size_t n) noexcept {
        double a=0,b=0,c=0,mid=0,side=0;
        for(std::size_t i=0;i<n;++i){
            const double L=l[i],R=r[i],m=(L+R)*0.5,s=(L-R)*0.5;
            a+=L*L;b+=R*R;c+=L*R;mid+=m*m;side+=s*s;
        }
        const double count=(double)n;
        meter_.correlation=(a>1e-20&&b>1e-20)?
                           std::clamp(c/std::sqrt(a*b),-1.0,1.0):1.0;
        meter_.midRms=std::sqrt(mid/count);
        meter_.sideRms=std::sqrt(side/count);
        meter_.spaceApplied=1.0;meter_.largestGrDb=0.0;
    }
    std::array<Node,kNodes> nodes_{};
    double sr_=48000.0,correlationAlpha_=0.999,sideAlpha_=0.99;
    double globalMidPower_=0.0,globalSidePower_=0.0,appliedSpace_=1.0,grMax_=0.0;
    double space_=50.0;
    bool mono_=false,haveConfig_=false;
    Meter meter_{};
};
} // namespace prism::roots::v2
