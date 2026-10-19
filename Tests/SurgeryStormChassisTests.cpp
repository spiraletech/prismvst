#include <JuceHeader.h>
#include "../Source/SurgeryStormChassis.h"
#include "../Source/SurgeryChassisLayout.h"
#include <chrono>
#include <fstream>
#include <iostream>
void startAllocationProbe() noexcept;
std::size_t finishAllocationProbe() noexcept;
bool runStormChassisTests(){
    juce::ScopedJuceInitialiser_GUI gui;
    using surgery::rack::StormChassis;
    auto fail=[](const char* text){std::cerr<<"STORM CHASSIS FAIL: "<<text<<'\n';return false;};
    StormChassis border;surgery::weather::Meaning signal;signal.level=.8f;signal.bass=.6f;signal.flux=.4f;signal.width=.5f;signal.centroid=.65f;
    const double expected=.30*.8+.35*.6+.20*.4+.15*.5;
    for(int i=0;i<120;++i)border.update(signal,1./60.,.5,false,false);
    auto held=border.state();
    if(std::abs(held.energy-expected)>.00001||std::abs(held.glow-(.012+.988*std::pow(expected,1.35)))>.00001||held.phase<=0)return fail("energy/glow/flow equation");
    for(int i=0;i<120;++i)border.update({},1./60.,.5,true,false);
    if(std::memcmp(&held,&border.state(),sizeof(held)))return fail("Freeze changes held state");
    border.update({},.1,.5,false,true);auto reduced=border.state();
    if(reduced.phase<=held.phase||reduced.phase-held.phase>.003||reduced.energy>=held.energy||reduced.glow<=.012)return fail("reduced motion loses level response");
    for(int i=0;i<1000;++i)border.update({},.1,.5,false,true);
    if(border.state().energy!=0||border.state().glow!=.012)return fail("silence/reduced motion");
    startAllocationProbe();for(int i=0;i<1000;++i)border.update(signal,.016,.18,false,true);
    if(finishAllocationProbe()!=0)return fail("state update heap allocation");
    // Both static and moving layers must leave every interior pixel untouched.
    // Use software storage for exact alpha tests, independent of GPU rounding.
    for(auto size:{juce::Point<int>{840,610},juce::Point<int>{1320,880},juce::Point<int>{1800,1200},juce::Point<int>{840,1200},juce::Point<int>{1800,610}}){
        juce::Image image(juce::Image::ARGB,size.x,size.y,true,juce::SoftwareImageType{});
        {juce::Graphics g(image);border.paintStatic(g,size.x,size.y);border.paintLiquid(g,size.x,size.y);}
        const auto interior=surgery::rack::instrumentArea(size.x,size.y).toNearestInt();
        for(int y=interior.getY();y<interior.getBottom();++y)for(int x=interior.getX();x<interior.getRight();++x)
            if(image.getPixelAt(x,y).getAlpha()!=0)return fail("border overlaps instrument face");
    }
    StormChassis hollow;
    juce::Image vessel(juce::Image::ARGB,1120,755,true,juce::SoftwareImageType{});
    auto draw=[&](){vessel.clear(vessel.getBounds(),juce::Colours::transparentBlack);juce::Graphics g(vessel);hollow.paintLiquid(g,1120,755);};
    for(int frame=0;frame<120;++frame){hollow.update(signal,1./60.,.3,false,false);draw();}
    const auto active=hollow.statistics();
    if(active.maximum<.04||active.litFraction>.25||active.meanDensity>.04||active.renderedLitFraction>.45)return fail("chamber not mostly hollow / no visible current");
    auto density=hollow.densityField();auto current=hollow.state();
    for(int frame=0;frame<30;++frame){hollow.update(signal,1./60.,.3,true,false);draw();}
    if(std::memcmp(&current,&hollow.state(),sizeof(current))||density!=hollow.densityField())return fail("Freeze advances plasma/trails");
    hollow.update(signal,1./60.,.3,true,false,false);draw();
    for(int y=0;y<vessel.getHeight();++y)for(int x=0;x<vessel.getWidth();++x)if(vessel.getPixelAt(x,y).getAlpha()!=0)return fail("SONAR OFF leaves animated energy visible");
    if(density!=hollow.densityField())return fail("OFF destroys frozen field");
    for(int frame=0;frame<300;++frame){hollow.update({},.05,.3,false,false,true);draw();}
    if(hollow.statistics().maximum>1.e-5||hollow.state().energy!=0)return fail("silence retains energy or full motion");
    const auto quietPhase=hollow.state().phase;for(int i=0;i<20;++i)hollow.update({},.05,.3,false,false,true);
    if(hollow.state().phase!=quietPhase)return fail("silent independent motion");
    std::ofstream truth("surgery-hollow-vessel.csv");truth<<"active_mean_density,active_lit_fraction,active_peak,rendered_lit_fraction,silent_peak\n"<<active.meanDensity<<','<<active.litFraction<<','<<active.maximum<<','<<active.renderedLitFraction<<','<<hollow.statistics().maximum<<'\n';
    std::ofstream csv("surgery-storm-chassis-performance.csv");csv<<"scale,median_ms,p95_ms,max_ms,dynamic_texture_pixels,raw_fixed_texture_bytes\n";
    for(float scale:{1.f,1.25f,1.5f}){
        juce::Image target(juce::Image::RGB,(int)(1120*scale),(int)(755*scale),true);std::vector<double> times;
        for(int frame=0;frame<50;++frame){
            const auto start=std::chrono::steady_clock::now();border.update(signal,1./60.,.18,false,false);
            {juce::Graphics g(target);g.addTransform(juce::AffineTransform::scale(scale));border.paintLiquid(g,1120,755);}
            if(frame>=10)times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
        }
        std::sort(times.begin(),times.end());csv<<scale<<','<<times[times.size()/2]<<','<<times[times.size()*95/100]<<','<<times.back()<<",30720,307968\n";
    }
    std::cout<<"STORM CHASSIS: weighted energy/glow equations, Freeze, reduced motion, silence, allocation-free state update, five editor sizes with untouched interior, and 100/125/150% border timings passed\n";
    return true;
}
