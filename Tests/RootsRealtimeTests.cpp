#include "../Source/RootsAudioEngine.h"
#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>
using namespace prism::roots::v2;
constexpr double sr=48000.0;
constexpr int n=16384;
bool approx(double a,double b,double tol){return std::abs(a-b)<tol;}
double amplitude(const std::vector<float>& a,double hz) {
    double re=0.0,im=0.0;
    for(int i=n/2;i<n;++i) {
        const double ang=2.0*std::numbers::pi*hz*i/sr;
        re+=a[(size_t)i]*std::cos(ang); im+=a[(size_t)i]*std::sin(ang);
    }
    return 2.0*std::hypot(re,im)/(n/2);
}
double rms(const std::vector<float>& a) {
    double v=0;
    for(int i=n/2;i<n;++i)v+=(double)a[(size_t)i]*a[(size_t)i];
    return std::sqrt(v/(n/2));
}
void sine(std::vector<float>& l,std::vector<float>& r,double hz,double amp,bool anti=false) {
    for(int i=0;i<n;++i){
        const float x=(float)(amp*std::sin(2.0*std::numbers::pi*hz*i/sr));
        l[(size_t)i]=x;r[(size_t)i]=anti?-x:x;
    }
}
int main(){
    // PROOF A: exact bypass (zero-gain filters, no compression, SPACE50).
    {
        Engine engine(sr);std::vector<float> l(n),r(n);
        for(int i=0;i<n;++i){l[i]=(float)(std::sin(i*0.071)*0.2);r[i]=(float)(std::cos(i*0.113)*0.1);}
        const auto beforeL=l,beforeR=r;
        engine.process(l.data(),r.data(),n);
        if(l!=beforeL||r!=beforeR){std::cerr<<"A FAIL bypass\n";return 1;}
        std::cout<<"A PASS bit-exact neutral bypass\n";
    }
    // PROOF B: overlapping two +6 dB peaking nodes yield approximately
    // +12 dB at center on both channels, with -60..+60 bounds.
    {
        Engine engine(sr);std::vector<float> l(n),r(n);
        sine(l,r,1000.0,0.01);
        const auto dry=l;
        for(int i=0;i<2;++i){
            Config c=engine.get(i);c.hz=1000.0;c.q=3.0;c.gainDb=6.0;
            engine.config(i,c);
        }
        engine.process(l.data(),r.data(),n);
        const double db=20.0*std::log10(amplitude(l,1000.0)/amplitude(dry,1000.0));
        Config c=engine.get(0);c.gainDb=10000.0;engine.config(0,c);
        if(!approx(db,12.0,0.6)||engine.get(0).gainDb!=60.0){
            std::cerr<<"B FAIL gain "<<db<<"\n";return 2;
        }
        std::cout<<"B PASS overlapping surgical EQ and gain clamps\n";
    }
    // PROOF C: exclusive frequency audition and an 8-ms crossfade on
    // switching to a different root; not source separation.
    {
        Engine engine(sr);std::vector<float> l(n),r(n);
        for(int i=0;i<n;++i){
            const float x=(float)(0.15*std::sin(2*std::numbers::pi*100*i/sr)
                      +0.15*std::sin(2*std::numbers::pi*1000*i/sr));
            l[i]=r[i]=x;
        }
        Config c=engine.get(0);c.hz=1000;c.q=8;c.solo=true;
        engine.config(0,c);engine.process(l.data(),r.data(),n);
        const auto high=amplitude(l,1000.0);
        const auto low=amplitude(l,100.0);
        if(!(high>0.08&&high>8.0*low)){
            std::cerr<<"C FAIL solo specificity "<<high<<","<<low<<"\n";return 3;
        }
        Config old=engine.get(0);old.solo=false;engine.config(0,old);
        Config next=engine.get(3);next.solo=true;next.hz=100.0;next.q=8.0;
        engine.config(3,next);
        std::vector<float> smallL(128),smallR(128);
        for(int i=0;i<128;++i)smallL[(size_t)i]=smallR[(size_t)i]=(float)
            (0.15*std::sin(2*std::numbers::pi*100.0*(i+n)/sr)
             +0.15*std::sin(2*std::numbers::pi*1000.0*(i+n)/sr));
        engine.process(smallL.data(),smallR.data(),smallL.size());
        if(!(engine.soloTransitionSamplesRemaining()>0)){
            std::cerr<<"C FAIL: no root-to-root crossfade\n";return 3;
        }
        std::vector<float> restL(512),restR(512);
        engine.process(restL.data(),restR.data(),restL.size());
        if(engine.soloTransitionSamplesRemaining()!=0){
            std::cerr<<"C FAIL: crossfade did not finish\n";return 3;
        }
        std::cout<<"C PASS exclusive audible root solo with 8ms crossfade\n";
    }
    // PROOF D: selected FET-inspired compressor attenuates a strong
    // sustained band, mono L/R exact; phase guard preserves default anti-phase.
    {
        Engine engine(sr);std::vector<float> l(n),r(n);
        sine(l,r,1000,0.65);
        const double dry=rms(l);
        Config c=engine.get(0);c.hz=1000;c.q=2.0;c.comp=Compressor::fet4;
        c.thresholdDb=-30.0;c.detector=Detector::linked;
        engine.config(0,c);
        engine.process(l.data(),r.data(),n);
        if(!(rms(l)<dry*0.96 && engine.meter().largestGrDb>1.0)){
            std::cerr<<"D FAIL compression "<<rms(l)<<" / "<<dry<<"\n";return 4;
        }
        engine.prepare(sr);engine.setMono(true);sine(l,r,1000,0.23,true);
        engine.process(l.data(),r.data(),n);
        for(int i=0;i<n;++i) if(l[i]!=r[i]){
            std::cerr<<"D FAIL mono\n";return 4;
        }
        engine.prepare(sr);engine.setMono(false);engine.setSpace(50.0);
        sine(l,r,1000,0.2,true);
        engine.process(l.data(),r.data(),n);
        if(engine.meter().correlation> -0.99){
            std::cerr<<"D FAIL existing antiphase preserved\n";return 4;
        }
        std::cout<<"D PASS compressor gain reduction and mono/phase\n";
    }
    std::cout<<"FOUR ROOTS v2 AUDIO PROOFS PASSED\n";
    return 0;
}
