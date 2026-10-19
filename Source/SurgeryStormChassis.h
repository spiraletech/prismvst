#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "SurgeryChassisLayout.h"
#include "SurgeryWeatherInterpretation.h"

namespace surgery::rack {
// JUKE JUICE: a border-only renderer. No parameters, analyzer writes or DSP.
class StormChassis final {
public:
    StormChassis() {
        // The supplied artwork is used for the four X-head screws only.
        // All energy is generated from current measurements, not neon artwork.
        const auto reference=juce::ImageCache::getFromMemory(BinaryData::approvedreference_png,BinaryData::approvedreference_pngSize);
        auto sample=[&](juce::Image& image,juce::Rectangle<int> source){juce::Graphics g(image);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(reference,0,0,image.getWidth(),image.getHeight(),source.getX(),source.getY(),source.getWidth(),source.getHeight());};
        // Plasma pixels from the reference are deliberately not used.
        for(int i=0;i<4;++i){screws_[(size_t)i]=juce::Image(juce::Image::RGB,40,40,false,juce::SoftwareImageType{});sample(screws_[(size_t)i],{i%2?917:3,i/2?630:11,40,40});}
    }
    struct State { double energy=0,glow=.012,phase=0,temperature=.5,level=0,bass=0,flux=0,width=0,flash=0,flashPosition=.5; };
    struct Statistics { double meanDensity=0,litFraction=0,renderedLitFraction=0;float maximum=0; };
    static juce::Rectangle<float> top(int w) { return {70.f,6.f,w-140.f,28.f}; }
    static juce::Rectangle<float> side(int w,int h,bool right) { return {right?w-61.f:5.f,52.f,56.f,h-104.f}; }
    const State& state() const noexcept { return state_; }
    const auto& densityField() const noexcept {return density_;}
    Statistics statistics() const noexcept {
        Statistics out;for(float d:density_){out.meanDensity+=d;out.litFraction+=d>.05f?1:0;out.maximum=std::max(out.maximum,d);}
        out.meanDensity/=density_.size();out.litFraction/=density_.size();out.renderedLitFraction=visiblePixels_/(double)density_.size();return out;
    }
    // Existing Storm values only. SONAR OFF overrides visibility even while
    // frozen; it never alters the held field. No bright decorative base fill.
    bool update(const weather::Meaning& measured,double dt,double decay,bool frozen,bool reduced,bool active=true) {
        const bool gateChanged=enabled_!=active;enabled_=active;frozen_=frozen;
        if(frozen){pendingDt_=0;if(gateChanged)dirty_=true;return gateChanged;}
        reduced_=reduced;dt=std::clamp(std::isfinite(dt)?dt:0.,0.,.25);
        decay_=std::clamp(std::isfinite(decay)?decay:.18,.05,10.);
        const auto previous=state_;
        const double level=active?weather::unit(measured.level):0;
        const double gate=weather::unit(level/.08);
        const double target=gate*weather::unit(.30*level+.35*measured.bass+.20*measured.flux+.15*measured.width);
        auto smooth=[&](double& value,double goal){const double tau=goal>value?.025:decay_;value=goal+(value-goal)*std::exp(-dt/tau);if(std::abs(value)<1.e-6)value=0;};
        smooth(state_.energy,target);smooth(state_.level,level);
        smooth(state_.bass,active?weather::unit(measured.bass):0);smooth(state_.flux,active?weather::unit(measured.flux):0);smooth(state_.width,active?weather::unit(measured.width):0);
        smooth(state_.temperature,level>1.e-6?weather::unit(measured.centroid):.5);
        state_.flash=active&&!reduced?std::min(.10f,measured.flash):0;state_.flashPosition=weather::unit(measured.flashPosition);
        state_.glow=.012+.988*std::pow(state_.energy,1.35);
        if(active&&state_.level>1.e-5)
            state_.phase=std::fmod(state_.phase+dt*(.25+.60*state_.flux+.20*state_.bass)*state_.energy*(reduced?.035:1.),64.);
        pendingDt_=std::min(.25,pendingDt_+dt);
        const bool changed=gateChanged||std::abs(previous.glow-state_.glow)>1.e-6||previous.phase!=state_.phase||activity_>1.e-5;
        if(changed)dirty_=true;return changed;
    }
    // Static carbon, brushed socket hardware and engraved micro-equations.
    // The inner hole and instrument transform never change.
    void paintStatic(juce::Graphics& g,int w,int h) {
        juce::Graphics::ScopedSaveState protectedFace(g);g.excludeClipRegion(instrumentArea(w,h).toNearestInt());
        juce::Path shell;shell.setUsingNonZeroWinding(false);
        shell.addRoundedRectangle(2,2,w-4.f,h-4.f,17);
        shell.addRoundedRectangle(68,38,w-136.f,h-90.f,15);
        g.setTiledImageFill(carbon_,0,0,1);g.fillPath(shell);
        g.setColour(juce::Colour(0xff06080b));g.drawRoundedRectangle({1.5f,1.5f,w-3.f,h-3.f},17,3);
        juce::ColourGradient rim(juce::Colour(0xff8b969e),0,0,juce::Colour(0xff151a21),0,(float)h,false);
        rim.addColour(.05,juce::Colour(0xff303b43));rim.addColour(.94,juce::Colour(0xff64727b));
        g.setGradientFill(rim);g.drawRoundedRectangle({3.5f,3.5f,w-7.f,h-7.f},15,1.1f);
        g.setColour(juce::Colour(0xff02070b));g.drawRoundedRectangle({66,36,w-132.f,h-86.f},16,4);
        g.setColour(juce::Colour(0xff48565f));g.drawRoundedRectangle({68,38,w-136.f,h-90.f},15,1);
        g.setColour(juce::Colour(0xff14252d));g.drawRoundedRectangle({70,40,w-140.f,h-94.f},14,.7f);
        chamber(g,top(w),false,juce::Colour(0xff21bfff));
        chamber(g,side(w,h,false),true,juce::Colour(0xff078dff));
        chamber(g,side(w,h,true),true,juce::Colour(0xfff32baf));
        for(bool right:{false,true})for(bool bottom:{false,true})hardware(g,right?w-23.f:23.f,bottom?h-23.f:23.f,(right?1:0)+(bottom?2:0));
        g.setFont(juce::Font(juce::FontOptions(9.f,juce::Font::bold)));
        g.setColour(juce::Colour(0xff82939f).withAlpha(.68f));
        g.drawText("SPIRAL ETHER TECH   /   JUKE JUICE",76,h-39,w-152,13,juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(8.f)));
        g.setColour(juce::Colour(0xff657580));
        g.drawText("E(t) = .30L + .35B + .20F + .15W    /    G = .012 + .988 E^1.35    /    DECAY tau",76,h-24,w-152,11,juce::Justification::centred);
    }
    void paintLiquid(juce::Graphics& g,int w,int h) {
        if(!g.clipRegionIntersects(top(w).expanded(6).toNearestInt())&&!g.clipRegionIntersects(side(w,h,false).expanded(6).toNearestInt())&&!g.clipRegionIntersects(side(w,h,true).expanded(6).toNearestInt()))return;
        juce::Graphics::ScopedSaveState protectedFace(g);g.excludeClipRegion(instrumentArea(w,h).toNearestInt());
        if(maskWidth_!=w||maskHeight_!=h){
            maskWidth_=w;maskHeight_=h;int offset=0;
            for(int index=0;index<3;++index){const auto r=index==0?top(w):side(w,h,index==2);const auto& image=index==0?topLiquid_:index==1?leftLiquid_:rightLiquid_;
                const float radius=std::min(r.getWidth(),r.getHeight())*.5f,pixel=std::max(r.getWidth()/image.getWidth(),r.getHeight()/image.getHeight());
                for(int y=0;y<image.getHeight();++y)for(int x=0;x<image.getWidth();++x){
                    const float px=(x+.5f)*r.getWidth()/image.getWidth(),py=(y+.5f)*r.getHeight()/image.getHeight();
                    const float dx=std::max({radius-px,0.f,px-(r.getWidth()-radius)}),dy=std::max({radius-py,0.f,py-(r.getHeight()-radius)});
                    masks_[(size_t)offset++]=(std::uint8_t)std::round(255*weather::unit(.5f-(std::sqrt(dx*dx+dy*dy)-radius)/pixel));
                }
            }
            dirty_=true;
        }
        if(dirty_){activity_=0;visiblePixels_=0;render(topLiquid_,0);render(leftLiquid_,1);render(rightLiquid_,2);dirty_=false;pendingDt_=0;}
        // Rounded alpha is precomputed only when bounds change. Chrome/glass
        // reflections are static-cache work; no per-frame vector path clips.
        g.setOpacity(1);g.drawImage(topLiquid_,top(w));g.drawImage(leftLiquid_,side(w,h,false));g.drawImage(rightLiquid_,side(w,h,true));
    }
private:
    State state_{};bool dirty_=true,enabled_=false,frozen_=false,reduced_=true;
    double pendingDt_=0,decay_=.18;float activity_=0;int visiblePixels_=0;
    std::array<float,30720> density_{};
    std::array<std::uint8_t,30720> masks_{};int maskWidth_=0,maskHeight_=0;
    juce::Image topLiquid_{juce::Image::ARGB,512,24,false};
    juce::Image leftLiquid_{juce::Image::ARGB,24,384,false},rightLiquid_{juce::Image::ARGB,24,384,false};
    std::array<juce::Image,4> screws_;
    juce::Image carbon_=makeCarbon();
    static juce::Image makeCarbon() {
        juce::Image image(juce::Image::RGB,64,64,false,juce::SoftwareImageType{});
        juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<64;++y)for(int x=0;x<64;++x){
            const int a=(x+y)%16,b=(x-y+128)%16;
            const bool over=((x/8+y/8)&1)==0;
            const float fibre=(over?(a<8?a:15-a):(b<8?b:15-b))/7.f;
            const float v=.035f+.075f*fibre+((over?x:y)%2?.012f:0);
            pixels.setPixelColour(x,y,juce::Colour::fromFloatRGBA(v,v*1.10f,v*1.23f,1));
        }
        return image;
    }
    static juce::Colour currentColour(float u,int orientation,float centroid) {
        if(orientation==1)return juce::Colour::fromHSV(.54f+.085f*centroid+.015f*std::sin(u*6),.85f,1,1);
        if(orientation==2)return juce::Colour::fromHSV(std::fmod(.86f+.15f*centroid+.06f*u,1.f),.8f,1,1);
        const std::array<juce::Colour,5> colours{juce::Colour(0xff27ddff),juce::Colour(0xff316bff),juce::Colour(0xffa076ff),juce::Colour(0xfff250c7),juce::Colour(0xffffa078)};
        const float position=weather::unit(u+(centroid-.5f)*.18f)*4;const int a=std::min(3,(int)position);float t=position-a;t=t*t*(3-2*t);return colours[(size_t)a].interpolatedWith(colours[(size_t)a+1],t);
    }
    void render(juce::Image& image,int orientation) {
        juce::Image::BitmapData output(image,juce::Image::BitmapData::writeOnly);
        const int length=orientation?image.getHeight():image.getWidth(),breadth=orientation?image.getWidth():image.getHeight();
        const int offset=orientation==0?0:orientation==1?12288:21504;
        const float phase=(float)state_.phase,energy=(float)(state_.energy*state_.level),bass=(float)state_.bass,flux=(float)state_.flux,width=(float)state_.width;
        const float spread=.065f+.22f*width,thickness=.022f+.032f*bass+(reduced_?.0015f:.009f)*flux;
        const double dt=frozen_?0:pendingDt_,attack=std::exp(-dt/.035),release=std::exp(-dt/decay_);
        for(int i=0;i<length;++i){
            const float u=i/(float)(length-1);const auto colour=currentColour(u,orientation,(float)state_.temperature);
            std::array<float,3> centre{},pulse{};
            for(int strand=0;strand<3;++strand){
                const float direction=strand==1?-1.f:1.f;
                centre[(size_t)strand]=.5f+spread*std::sin(u*(9+strand*3.f)+phase*direction*4+strand*2.1f)+.025f*flux*(reduced_?.12f:1.f)*std::sin(u*29-phase*3);
                const float position=std::fmod(phase*(.7f+strand*.16f)+strand*.33f+orientation*.13f,1.f);
                const float raw=std::abs(u-position),distance=std::min(raw,1-raw),q=distance/(.035f+.045f*bass),z=1+q*q;
                pulse[(size_t)strand]=1/(z*z*z)*(strand==2?.15f+.65f*width:1.f);
            }
            const float flashQ=(u-(float)state_.flashPosition)/.045f,flash=.5f*(float)state_.flash/(1+flashQ*flashQ);
            for(int j=0;j<breadth;++j){
                const float v=j/(float)(breadth-1);float target=0;
                for(int strand=0;strand<3;++strand){const float d=(v-centre[(size_t)strand])/thickness,z=1+d*d;target+=pulse[(size_t)strand]/(z*z*z);}
                target=enabled_?weather::unit(energy*(1.1f+.55f*bass+.45f*flux)*target):0;
                const int ox=orientation?j:i,oy=orientation?i:j,index=offset+oy*image.getWidth()+ox;
                auto& density=density_[(size_t)index];density=(float)(target+(density-target)*(target>density?attack:release));if(density<1.e-6f)density=0;
                activity_=std::max(activity_,density);
                const float illumination=enabled_?std::min(1.f,density*(.6f+.4f*(float)state_.glow)+flash*density):0;
                const float alpha=masks_[(size_t)index]/255.f*weather::unit(3.f*std::sqrt(illumination));
                visiblePixels_+=alpha>.10f?1:0;
                const float white=weather::unit((illumination-.28f)*1.1f);
                output.setPixelColour(ox,oy,juce::Colour::fromFloatRGBA(colour.getFloatRed()+(1-colour.getFloatRed())*white,colour.getFloatGreen()+(1-colour.getFloatGreen())*white,colour.getFloatBlue()+(1-colour.getFloatBlue())*white,alpha));
            }
        }
    }
    void hardware(juce::Graphics& g,float x,float y,int index) {
        juce::Graphics::ScopedSaveState saved(g);juce::Path clip;clip.addEllipse(x-19,y-19,38,38);g.reduceClipRegion(clip);
        g.setOpacity(1);g.drawImage(screws_[(size_t)index],{x-19,y-19,38,38});
    }
    static void chamber(juce::Graphics& g,juce::Rectangle<float> r,bool vertical,juce::Colour accent) {
        const float radius=std::min(r.getWidth(),r.getHeight())*.5f;
        g.setColour(juce::Colour(0xff010308));g.fillRoundedRectangle(r.expanded(5),radius+5);
        juce::ColourGradient rim(juce::Colour(0xff9cabb5),r.getX(),r.getY(),juce::Colour(0xff13212d),r.getRight(),r.getBottom(),false);
        rim.addColour(.3,juce::Colour(0xff273846));rim.addColour(.6,juce::Colour(0xff78909e));rim.addColour(.8,juce::Colour(0xff111c25));
        g.setGradientFill(rim);g.drawRoundedRectangle(r.expanded(3),radius+3,1.5f);
        g.setColour(accent.withAlpha(.10f));g.drawRoundedRectangle(r.expanded(1),radius+1,1.2f);
        g.setColour(juce::Colour(0xff02080d).withAlpha(.72f));g.fillRoundedRectangle(r,radius);
        // Contained cylindrical glass: shaded edges, polished seams and a
        // narrow reflection. No bloom may escape into the instrument face.
        juce::ColourGradient glass(juce::Colours::black.withAlpha(.64f),r.getX(),r.getY(),juce::Colours::black.withAlpha(.62f),vertical?r.getRight():r.getX(),vertical?r.getY():r.getBottom(),false);
        glass.addColour(.16,juce::Colours::white.withAlpha(.075f));glass.addColour(.25,juce::Colours::white.withAlpha(.02f));glass.addColour(.5,juce::Colours::transparentBlack);glass.addColour(.80,juce::Colours::white.withAlpha(.04f));
        g.setGradientFill(glass);g.fillRoundedRectangle(r,radius);
        g.setColour(juce::Colour(0xffe6f8ff).withAlpha(.28f));g.drawRoundedRectangle(r.reduced(2),radius-2,.85f);
        g.setColour(accent.withAlpha(.045f));g.drawRoundedRectangle(r.reduced(4),radius-4,.7f);
    }
};
} // namespace surgery::rack
