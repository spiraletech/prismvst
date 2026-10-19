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
        // Original supplied artwork remains byte-for-byte intact. Only these
        // glass/hardware regions are sampled; no reference UI is displayed.
        const auto reference=juce::ImageCache::getFromMemory(BinaryData::approvedreference_png,BinaryData::approvedreference_pngSize);
        auto sample=[&](juce::Image& image,juce::Rectangle<int> source){juce::Graphics g(image);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(reference,0,0,image.getWidth(),image.getHeight(),source.getX(),source.getY(),source.getWidth(),source.getHeight());};
        sample(topOptics_,{88,20,786,35});sample(leftOptics_,{10,75,39,521});sample(rightOptics_,{911,75,39,521});
        for(int i=0;i<4;++i){screws_[(size_t)i]=juce::Image(juce::Image::RGB,40,40,false,juce::SoftwareImageType{});sample(screws_[(size_t)i],{i%2?917:3,i/2?630:11,40,40});}
    }
    struct State { double energy=0, glow=.18, phase=0, temperature=.5; };
    static juce::Rectangle<float> top(int w) { return {70.f,6.f,w-140.f,28.f}; }
    static juce::Rectangle<float> side(int w,int h,bool right) { return {right?w-61.f:5.f,52.f,56.f,h-104.f}; }
    const State& state() const noexcept { return state_; }
    // The reference glass stays illuminated at rest. Only measured energy
    // advances flow, with elapsed-time decay and no independent animation.
    bool update(const weather::Meaning& measured,double dt,double decay,bool frozen,bool reduced) {
        if(frozen)return false;
        dt=std::clamp(std::isfinite(dt)?dt:0.,0.,.25);
        const double target=weather::unit(.30*measured.level+.35*measured.bass+.20*measured.flux+.15*measured.width);
        const auto previous=state_;
        const double tau=target>state_.energy?.025:std::clamp(std::isfinite(decay)?decay:.18,.05,10.);
        state_.energy=target+(state_.energy-target)*std::exp(-dt/tau);
        if(state_.energy<1.e-6)state_.energy=0;
        state_.glow=.18+.82*std::pow(state_.energy,1.35);
        const double temperature=measured.level>1.e-6?weather::unit(measured.centroid):.5;
        state_.temperature=temperature+(state_.temperature-temperature)*std::exp(-dt/tau);
        if(!reduced&&state_.energy>1.e-5)
            state_.phase=std::fmod(state_.phase+dt*(.08+.55*measured.flux+.25*measured.bass)*state_.energy,64.);
        const bool changed=std::abs(requestedGlow_-state_.glow)>.00015||std::abs(requestedTemperature_-state_.temperature)>.001||previous.phase!=state_.phase;
        if(changed){dirty_=true;requestedGlow_=state_.glow;requestedTemperature_=state_.temperature;}
        return changed;
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
        chamber(g,topOptics_,top(w),false,juce::Colour(0xff21bfff));
        chamber(g,leftOptics_,side(w,h,false),true,juce::Colour(0xff078dff));
        chamber(g,rightOptics_,side(w,h,true),true,juce::Colour(0xfff32baf));
        for(bool right:{false,true})for(bool bottom:{false,true})hardware(g,right?w-23.f:23.f,bottom?h-23.f:23.f,(right?1:0)+(bottom?2:0));
        g.setFont(juce::Font(juce::FontOptions(9.f,juce::Font::bold)));
        g.setColour(juce::Colour(0xff82939f).withAlpha(.68f));
        g.drawText("SPIRAL ETHER TECH   /   JUKE JUICE",76,h-39,w-152,13,juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(8.f)));
        g.setColour(juce::Colour(0xff657580));
        g.drawText("E(t) = .30L + .35B + .20F + .15W    /    G = .18 + .82 E^1.35    /    DECAY tau",76,h-24,w-152,11,juce::Justification::centred);
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
        if(dirty_){render(topLiquid_,0);render(leftLiquid_,1);render(rightLiquid_,2);dirty_=false;}
        // Rounded alpha is precomputed only when bounds change. Chrome/glass
        // reflections are static-cache work; no per-frame vector path clips.
        g.setOpacity(1);g.drawImage(topLiquid_,top(w));g.drawImage(leftLiquid_,side(w,h,false));g.drawImage(rightLiquid_,side(w,h,true));
    }
private:
    State state_{};bool dirty_=true;double requestedGlow_=.18,requestedTemperature_=.5;
    std::array<std::uint8_t,30720> masks_{};int maskWidth_=0,maskHeight_=0;
    juce::Image topLiquid_{juce::Image::ARGB,512,24,false};
    juce::Image leftLiquid_{juce::Image::ARGB,24,384,false},rightLiquid_{juce::Image::ARGB,24,384,false};
    juce::Image topOptics_{juce::Image::RGB,1024,40,false,juce::SoftwareImageType{}};
    juce::Image leftOptics_{juce::Image::RGB,40,768,false,juce::SoftwareImageType{}},rightOptics_{juce::Image::RGB,40,768,false,juce::SoftwareImageType{}};
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
    void render(juce::Image& image,int orientation) {
        const auto& source=orientation==0?topOptics_:orientation==1?leftOptics_:rightOptics_;
        juce::Image::BitmapData input(source,juce::Image::BitmapData::readOnly),output(image,juce::Image::BitmapData::writeOnly);
        const int length=orientation?image.getHeight():image.getWidth(),breadth=orientation?image.getWidth():image.getHeight();
        const float phase=(float)state_.phase,light=(float)(.82+.18*state_.glow),temperature=(float)(state_.temperature-.5)*.04f;
        std::array<float,40> sine{},wiggle{};
        for(int j=0;j<breadth;++j){const float v=j/(float)(breadth-1);sine[(size_t)j]=std::sin(v*juce::MathConstants<float>::pi);wiggle[(size_t)j]=std::sin(v*6);}
        const float flow=.018f*std::sin(phase*1.4f),cross=.035f*std::sin(phase*2);
        for(int i=0;i<length;++i){
            const float u=i/(float)(length-1),longitudinal=std::sin(u*juce::MathConstants<float>::pi);
            const float transverse=cross*std::sin(u*11);
            for(int j=0;j<breadth;++j){
                const float v=j/(float)(breadth-1);
                const float a=weather::unit(u+flow*wiggle[(size_t)j]*longitudinal);
                const float b=weather::unit(v+transverse*sine[(size_t)j]);
                const float x=(orientation?b:a)*(source.getWidth()-1),y=(orientation?a:b)*(source.getHeight()-1);
                const int x0=std::min(source.getWidth()-2,(int)x),y0=std::min(source.getHeight()-2,(int)y);const float fx=x-x0,fy=y-y0;
                const auto p=input.getPixelColour(x0,y0),q=input.getPixelColour(x0+1,y0),r=input.getPixelColour(x0,y0+1),s=input.getPixelColour(x0+1,y0+1);
                auto mix=[&](float c,float d,float e,float f,float tint){return weather::unit(((1-fy)*((1-fx)*c+fx*d)+fy*((1-fx)*e+fx*f))*light*tint);};
                const int ox=orientation?j:i,oy=orientation?i:j,offset=orientation==0?0:orientation==1?12288:21504;
                const float alpha=masks_[(size_t)(offset+oy*image.getWidth()+ox)]/255.f;
                output.setPixelColour(ox,oy,juce::Colour::fromFloatRGBA(mix(p.getFloatRed(),q.getFloatRed(),r.getFloatRed(),s.getFloatRed(),1+temperature),mix(p.getFloatGreen(),q.getFloatGreen(),r.getFloatGreen(),s.getFloatGreen(),1),mix(p.getFloatBlue(),q.getFloatBlue(),r.getFloatBlue(),s.getFloatBlue(),1-temperature),alpha));
            }
        }
    }
    void hardware(juce::Graphics& g,float x,float y,int index) {
        juce::Graphics::ScopedSaveState saved(g);juce::Path clip;clip.addEllipse(x-19,y-19,38,38);g.reduceClipRegion(clip);
        g.setOpacity(1);g.drawImage(screws_[(size_t)index],{x-19,y-19,38,38});
    }
    static void chamber(juce::Graphics& g,const juce::Image& image,juce::Rectangle<float> r,bool vertical,juce::Colour accent) {
        const float radius=std::min(r.getWidth(),r.getHeight())*.5f;
        g.setColour(juce::Colour(0xff010308));g.fillRoundedRectangle(r.expanded(5),radius+5);
        juce::ColourGradient rim(juce::Colour(0xff9cabb5),r.getX(),r.getY(),juce::Colour(0xff13212d),r.getRight(),r.getBottom(),false);
        rim.addColour(.3,juce::Colour(0xff273846));rim.addColour(.6,juce::Colour(0xff78909e));rim.addColour(.8,juce::Colour(0xff111c25));
        g.setGradientFill(rim);g.drawRoundedRectangle(r.expanded(3),radius+3,1.5f);
        g.setColour(accent.withAlpha(.55f));g.drawRoundedRectangle(r.expanded(1),radius+1,1.2f);
        {juce::Graphics::ScopedSaveState saved(g);juce::Path clip;clip.addRoundedRectangle(r,radius);g.reduceClipRegion(clip);g.setOpacity(1);g.drawImage(image,r);}
        // Contained cylindrical glass: shaded edges, polished seams and a
        // narrow reflection. No bloom may escape into the instrument face.
        juce::ColourGradient glass(juce::Colours::black.withAlpha(.64f),r.getX(),r.getY(),juce::Colours::black.withAlpha(.62f),vertical?r.getRight():r.getX(),vertical?r.getY():r.getBottom(),false);
        glass.addColour(.16,juce::Colours::white.withAlpha(.22f));glass.addColour(.25,juce::Colours::white.withAlpha(.02f));glass.addColour(.5,juce::Colours::transparentBlack);glass.addColour(.80,juce::Colours::white.withAlpha(.10f));
        g.setGradientFill(glass);g.fillRoundedRectangle(r,radius);
        g.setColour(juce::Colour(0xffe6f8ff).withAlpha(.63f));g.drawRoundedRectangle(r.reduced(2),radius-2,.85f);
        g.setColour(accent.withAlpha(.25f));g.drawRoundedRectangle(r.reduced(4),radius-4,.7f);
    }
};
} // namespace surgery::rack
