#include "RootsEditor.h"
#include "RootsSpectrumColours.h"
#include "SurgerySpectralColours.h"
#include <cmath>
#include <numbers>

namespace {
float pval(const juce::AudioProcessorValueTreeState& ap,const juce::String& id) {
    if(const auto* v=ap.getRawParameterValue(id))return v->load();
    return 0.0f;
}
void change(PRISMVSTAudioProcessor& p,const juce::String& id,float val) {
    if(auto* param=p.apvts.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(val));
}
juce::String hzText(float f) {
    if(f>=1000.0f)return juce::String(f/1000.0f,f>=10000?1:2)+"k";
    if(f<10.0f)return juce::String(f,2);
    return juce::String(juce::roundToInt(f));
}
juce::Colour territory(int i){
    static constexpr unsigned hues[7]{
        0xFF6BBDD4u,0xFFDFA871u,0xFFE6BD6Au,0xFF8DCEA8u,
        0xFF9DADEAu,0xFFD2A2E5u,0xFFE89EADu
    };
    return juce::Colour(hues[(size_t)juce::jlimit(0,6,i)]);
}
void rotary(juce::Slider& s,juce::String suffix,double resetValue) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,22);
    s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe4eef1));
    s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff0d1b23));
    s.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff47616a));
    s.setTextValueSuffix(suffix);
    s.setDoubleClickReturnValue(true,resetValue);
}
}
RootsGraph::RootsGraph(PRISMVSTAudioProcessor& owner):processor_(owner) {
    bands_.fill(-144.0f);peaks_.fill(-144.0f);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}
juce::Rectangle<float> RootsGraph::area() const {
    // Reserve a header rail for ROOT-specific C1-C4 foreground selectors.
    return getLocalBounds().toFloat().reduced(46.0f,46.0f);
}
float RootsGraph::freqToX(float f) const {
    const auto r=area();
    return r.getX()+r.getWidth()*(float)prism::roots::visual::graphUnit(f);
}
float RootsGraph::xToFreq(float x)const {
    const auto r=area();
    return (float)prism::roots::visual::graphFrequency(
        juce::jlimit(0.0f,1.0f,(x-r.getX())/juce::jmax(1.0f,r.getWidth())));
}
float RootsGraph::param(int i,const char* suffix)const {
    return pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(i,suffix));
}
void RootsGraph::write(int i,const char* suffix,float value) {
    change(processor_,PRISMVSTAudioProcessor::rootsNodeId(i,suffix),value);
}
int RootsGraph::hitNode(juce::Point<float> p)const{
    // Adjacent ROOT boundary handles may occupy the identical x/y point.
    // Cycle among geometrically coincident handles instead of making the
    // later ROOT permanently impossible to select.
    float nearest=18.0f;
    std::array<float,prism::roots::v2::kNodes> distances{};
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        const auto x=freqToX(param(i,"hz")),y=area().getCentreY();
        distances[(size_t)i]=p.getDistanceFrom({x,y});
        nearest=juce::jmin(nearest,distances[(size_t)i]);
    }
    if(nearest>=18.0f)return -1;
    int first=-1,next=-1;
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        if(distances[(size_t)i]>nearest+1.25f)continue;
        if(first<0)first=i;
        if(i>selected_&&next<0)next=i;
    }
    return next>=0?next:first;
}
juce::Colour RootsGraph::colour(int root) const {
    const auto id="surgery_root"+juce::String(root+1)+"_rgb";
    return juce::Colour(0xff000000u |
        (unsigned)juce::roundToInt(pval(processor_.apvts,id)));
}
void RootsGraph::updateAnalyzer(
    const std::array<float,PRISMVSTAudioProcessor::labBars>& values,
    const std::array<float,PRISMVSTAudioProcessor::labBars>& pk){
    if(frozen_)return;
    // Analyzer worker owns the FFT. Only 256 display values arrive here.
    // Message-thread smoothing is fast-attack with calibrated decay; never
    // change the underlying audio or the diagnostic measurement data.
    spectrumEnvelope_.update(values,1.0/60.0,
        pval(processor_.apvts,"aura_memory"));
    bands_=spectrumEnvelope_.frame();
    peaks_=pk;
    // Holographic foil swipes only while measured post-DSP spectrum is active.
    // No synthetic spectral peaks: a silent signal holds the decorative phase.
    float loudest=-144.0f;
    for(const float db:values)loudest=std::max(loudest,db);
    if(loudest>-95.0f){
        chromaPhase_+=0.0038f;
        if(chromaPhase_>=1.0f)chromaPhase_-=1.0f;
    }
    repaint();
}
void RootsGraph::selectNode(int i){
    const int target=juce::jlimit(0,prism::roots::v2::kNodes-1,i);
    if(target==selected_)return;
    selected_=target;
    // Once solo audition is engaged it travels with the selected root.
    processor_.transferRootsSoloTo(selected_);
    if(onSelectNode)onSelectNode(selected_);
    repaint();
}
void RootsGraph::showRootMenu(int root){
    selectNode(juce::jlimit(0,6,root)*2);
    popup();
}
void RootsGraph::paint(juce::Graphics& g){
    const auto full=getLocalBounds().toFloat();
    const auto r=area();

    const juce::Colour top=theme_==1?juce::Colour(0xff152b31):
                            theme_==2?juce::Colour(0xff092820):
                            theme_==3?juce::Colour(0xff142038):
                                       juce::Colour(0xff0e151c);
    const juce::Colour bottom=theme_==2?juce::Colour(0xff030f0c):
                               theme_==3?juce::Colour(0xff070d19):
                                          juce::Colour(0xff03070c);
    juce::ColourGradient bg(top,full.getX(),full.getY(),
                            bottom,full.getX(),full.getBottom(),false);
    g.setGradientFill(bg);g.fillRoundedRectangle(full.reduced(1.0f),10.0f);
    g.setColour(juce::Colour::fromRGB(58,74,86));
    g.drawRoundedRectangle(full.reduced(1.0f),10.0f,1.0f);
    g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
    g.setColour(juce::Colour::fromRGB(145,172,182));
    // Measured frequency/intensity colouring, independent of ROOT or theme.
    g.drawText(frozen_?"SONAR / FROZEN":"SONAR / LIVE FREQUENCY ENERGY",
               r.getX(),7.0f,r.getWidth(),16.0f,juce::Justification::centredLeft);
    // Frequency and calibrated dBFS dimensions.
    static constexpr std::array<float,11> freqs{
        20,40,80,160,315,630,1250,2500,5000,10000,20000
    };
    for(const float f:freqs){
        const float x=freqToX(f);
        g.setColour(juce::Colour::fromRGB(42,55,69).withAlpha(0.74f));
        g.drawVerticalLine(juce::roundToInt(x),r.getY(),r.getBottom());
        g.setColour(juce::Colour::fromRGB(135,161,175));
        g.drawText(hzText(f),juce::roundToInt(x-19),juce::roundToInt(r.getBottom()+6),
                   38,17,juce::Justification::centred);
    }
    for(int i=0;i<=8;++i){
        const float y=r.getY()+r.getHeight()*i/8.0f;
        g.setColour(juce::Colour::fromRGB(53,67,84).withAlpha(i==4?0.74f:0.34f));
        g.drawHorizontalLine(juce::roundToInt(y),r.getX(),r.getRight());
        g.setColour(juce::Colour::fromRGB(131,155,169));
        const int choice=juce::roundToInt(pval(processor_.apvts,"analyzer_depth"));
        const int rangeDb=choice==0?96:choice==1?120:144;
        g.drawText(juce::String(-rangeDb*i/8),juce::roundToInt(r.getRight()+5),
                   juce::roundToInt(y-8),34,16,juce::Justification::centredLeft);
    }
    // ROOT handle height is constant; vertical axis is exclusively dBFS.
    // ROOT gain is a separate, single control in the bottom panel.
    g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::plain)));
    g.setColour(juce::Colour::fromRGB(184,197,209));
    g.drawText("Hz",8,16,34,16,juce::Justification::centredLeft);
    g.drawText("dBFS",juce::roundToInt(r.getRight()),16,44,16,juce::Justification::centredRight);
    std::array<prism::roots::visual::Node,prism::roots::visual::kNodes> zones{};
    for(int node=0;node<prism::roots::visual::kNodes;++node)
    {
        zones[(size_t)node].hz=param(node,"hz");
        zones[(size_t)node].q=param(node,"q");
        zones[(size_t)node].enabled=pval(processor_.apvts,
            PRISMVSTAudioProcessor::rootsRootControlId(node/2,"on"))>=0.5f;
    }
    // The visible coverage is defined by each ROOT's two boundary handles,
    // not Q distance or a fixed index/37 palette. Regions can intersect.
    for(int root=0;root<prism::roots::visual::kRoots;++root) {
        const auto interval=prism::roots::visual::region(root,zones);
        if(!interval.enabled)continue;
        const float x1=freqToX((float)interval.lowHz);
        const float x2=freqToX((float)interval.highHz);
        g.setColour(colour(root).withAlpha(root==selected_/2?0.075f:0.025f));
        g.fillRect(x1,r.getY(),juce::jmax(0.0f,x2-x1),r.getHeight());
    }
    // Single live FFT envelope. X is 20-20k logarithmic Hz; Y is measured dBFS.
    // The area beneath the one curve is closed at the -rangeDb graph floor.
    // Display slope or spectral colouring never changes the audio path.
    const int depthMode=juce::roundToInt(pval(processor_.apvts,"analyzer_depth"));
    const float rangeDb=depthMode==0?96.0f:depthMode==1?120.0f:144.0f;
    const float slope=pval(processor_.apvts,"analyzer_slope");
    const bool heatOn=pval(processor_.apvts,"aura_color")>=0.5f;
    juce::Path spectrumPath,filledPath,peakPath;
    const int N=PRISMVSTAudioProcessor::labBars;
    for(int i=0;i<N;++i){
        const float frequency=prism::analysis::LabBands::centreFrequency(i);
        const float x=freqToX(frequency);
        const float db=surgery::displayDb(bands_[(size_t)i],frequency,slope,rangeDb);
        const float y=r.getY()-db*r.getHeight()/rangeDb;
        const float peak=surgery::displayDb(peaks_[(size_t)i],frequency,slope,rangeDb);
        const float py=r.getY()-peak*r.getHeight()/rangeDb;
        if(i==0){
            spectrumPath.startNewSubPath(x,y);
            peakPath.startNewSubPath(x,py);
            filledPath.startNewSubPath(x,r.getBottom());
            filledPath.lineTo(x,y);
        }else{
            spectrumPath.lineTo(x,y);
            peakPath.lineTo(x,py);
            filledPath.lineTo(x,y);
        }
    }
    filledPath.lineTo(freqToX(20000.0f),r.getBottom());
    filledPath.closeSubPath();
    g.saveState();
    g.reduceClipRegion(r.toNearestInt());
    if(heatOn){
        // Frequency-addressed, measured-energy heat is clipped to ONE
        // closed dBFS spectrum, as in a parametric-EQ spectral background.
        // No theme/palette choice, scrolling spectrogram, or stacked bars.
        g.saveState();
        g.reduceClipRegion(filledPath);
        juce::ColourGradient frequencyRamp(
            juce::Colour(0xffef4357),r.getX(),r.getY(),
            juce::Colour(0xff7194df),r.getRight(),r.getY(),false);
        for(std::size_t i=1;i<surgery::spectrum::stops.size()-1;++i){
            const auto& stop=surgery::spectrum::stops[i];
            const auto& rgb=stop.value;
            const auto tint=juce::Colour::fromFloatRGBA(rgb.r,rgb.g,rgb.b,1.0f);
            frequencyRamp.addColour(surgery::spectrum::plotFraction(stop.hz),tint);
        }
        g.setGradientFill(frequencyRamp);
        g.fillRect(r);
        // Vertical shadow gives full-bodied colour at the envelope with
        // darker energy below, without imposing one global rose/ROOT shade.
        juce::ColourGradient shade(juce::Colours::transparentBlack,
            r.getCentreX(),r.getY(),
            juce::Colour(0xff050b16).withAlpha(0.80f),
            r.getCentreX(),r.getBottom(),false);
        shade.addColour(0.55,juce::Colour(0xff110e1f).withAlpha(0.25f));
        g.setGradientFill(shade);
        g.fillRect(r);
        // Iridescent holographic foil: restrained cross-frequency diffraction
        // and a smooth optical swipe. Each glint is multiplied by measured
        // spectral energy: silence remains completely dark.
        // Entire layer is clipped to the SINGLE dBFS envelope.
        constexpr int spectralSlices=160;
        for(int i=0;i<spectralSlices;++i){
            const float u=((float)i+0.5f)/(float)spectralSlices;
            const float hz=20.0f*std::pow(1000.0f,u);
            const int bin=juce::jlimit(0,N-1,(int)(u*(float)N));
            const float energy=surgery::spectrum::brightness(bands_[(size_t)bin]);
            if(energy<0.009f)continue;
            const auto base=surgery::spectrum::colourAt(hz);
            const auto foil=surgery::spectrum::foil(hz,bands_[(size_t)bin],
                                                    chromaPhase_);
            const float x1=r.getX()+r.getWidth()*(float)i/spectralSlices;
            const float x2=r.getX()+r.getWidth()*(float)(i+1)/spectralSlices;
            const auto tint=juce::Colour::fromFloatRGBA(
                base.r,base.g,base.b,0.065f+0.16f*energy);
            g.setColour(tint);
            g.fillRect(x1,r.getY(),x2-x1+0.4f,r.getHeight());
            if(foil.strength>0.012f){
                const auto highlight=juce::Colour::fromFloatRGBA(
                    foil.colour.r,foil.colour.g,foil.colour.b,foil.strength);
                juce::ColourGradient foilFade(
                    highlight,x1,r.getY(),
                    highlight.withAlpha(foil.strength*0.10f),
                    x1,r.getBottom(),false);
                g.setGradientFill(foilFade);
                g.fillRect(x1,r.getY(),x2-x1+0.45f,r.getHeight());
            }
        }
        g.restoreState();
    }
    // A single fine readout follows the spectrum edge, never a 2D waterfall.
    juce::ColourGradient edge(
        juce::Colour(0xffffa5ad),r.getX(),r.getY(),
        juce::Colour(0xffa8c6ff),r.getRight(),r.getY(),false);
    edge.addColour(0.34,juce::Colour(0xffe4a7d4));
    edge.addColour(0.66,juce::Colour(0xffff9bbb));
    // Broad low-opacity pearlescent edge glow, then one crisp live contour.
    juce::ColourGradient glow(
        juce::Colour(0xffc2fff5).withAlpha(0.13f),
        r.getX(),r.getY(),
        juce::Colour(0xfff6c8ff).withAlpha(0.14f),
        r.getRight(),r.getY(),false);
    glow.addColour(0.48,juce::Colour(0xffddedff).withAlpha(0.19f));
    g.setGradientFill(glow);
    g.strokePath(spectrumPath,juce::PathStrokeType(3.5f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setGradientFill(edge);
    g.strokePath(spectrumPath,juce::PathStrokeType(1.3f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    // A restrained peak-hold readout; the audio measurements are unchanged.
    g.setColour(juce::Colour(0xffe3edf2).withAlpha(0.31f));
    g.strokePath(peakPath,juce::PathStrokeType(0.8f));
    g.restoreState();

    // Predict the EXACT static RBJ peak-filter response used in the CARVE DSP.
    // It shows the maximum possible response for dynamic nodes, not measured GR.
    const int selectedRoot=selected_/2;
    const double sr=processor_.getSampleRate()>8000.0?
                    processor_.getSampleRate():48000.0;
    const float baseY=r.getCentreY()+23.0f;
    const float lowHz=param(selectedRoot*2,"hz");
    const float highHz=param(selectedRoot*2+1,"hz");
    for(int slot=0;slot<surgery::kSlots;++slot){
        const auto id=[selectedRoot,slot](const char* suffix){
            return PRISMVSTAudioProcessor::carveId(selectedRoot,slot,suffix);
        };
        if(pval(processor_.apvts,id("on"))<0.5f)continue;
        const double fc=surgery::precision::boundedFrequency(
            pval(processor_.apvts,id("hz")),lowHz,highHz);
        const double q=pval(processor_.apvts,id("q"));
        const double depth=pval(processor_.apvts,id("depth"));
        if(depth<0.001)continue;
        juce::Path contour,footprint;
        const float firstX=freqToX(20.0f);
        footprint.startNewSubPath(firstX,baseY);
        // High Q requires sub-pixel-aware sampling; old 256 samples across
        // 10 octaves could miss the entire ultra-narrow notch!
        for(int i=0;i<=2048;++i){
            const float f=20.0f*std::pow(1000.0f,(float)i/2048.0f);
            const float x=freqToX(f);
            const double scale=surgery::CarveEngine::boundaryWeight(fc,
                std::min(lowHz,highHz),std::max(lowHz,highHz));
            // Static-response preview: approximate dB interpolation for dry/wet
            // blend; exact RBJ biquad curve tested separately.
            const double response=surgery::carveMagnitudeDb(sr,fc,q,depth,f)*scale;
            const float y=baseY-(float)response*1.10f;
            if(i==0)contour.startNewSubPath(x,y);
            else contour.lineTo(x,y);
            footprint.lineTo(x,y);
        }
        footprint.lineTo(freqToX(20000.0f),baseY);
        footprint.closeSubPath();
        const auto tint=colour(selectedRoot).brighter(slot==slot_?0.6f:0.10f);
        g.setColour(tint.withAlpha(slot==slot_?0.20f:0.08f));
        g.fillPath(footprint);
        g.setColour(tint.withAlpha(slot==slot_?0.96f:0.58f));
        g.strokePath(contour,juce::PathStrokeType(slot==slot_?2.4f:1.35f));
    }
    g.setColour(juce::Colour(0xffacc3cf));
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText("SONAR / IRIDESCENT ENERGY FIELD     CARVE / STATIC RESPONSE",
               r.getX(),r.getBottom()+23.0f,r.getWidth(),16.0f,
               juce::Justification::centredLeft);
    // Both boundary handles always live on one horizontal rail.
    g.setColour(juce::Colour::fromRGB(159,182,191).withAlpha(0.14f));
    g.drawHorizontalLine(juce::roundToInt(r.getCentreY()),r.getX(),r.getRight());
    // Two-pass z-order keeps the selected ROOT boundary pair above neighbors.
    for(int pass=0;pass<2;++pass)
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        if((i/2==selectedRoot)!=(pass==1))continue;
        const float hz=param(i,"hz");
        const float x=freqToX(hz),y=r.getCentreY();
        const bool enabled=pval(processor_.apvts,
            PRISMVSTAudioProcessor::rootsRootControlId(i/2,"on"))>=0.5f;
        const bool solo=pval(processor_.apvts,
             PRISMVSTAudioProcessor::rootsRootSoloId(i/2))>=0.5f;
        const auto col=colour(i/2);
        // Vertical guides represent frequency boundaries, not gain movements.
        const bool focused=i/2==selectedRoot;
        g.setColour(col.withAlpha(enabled?(focused?0.55f:0.15f):0.045f));
        g.drawVerticalLine(juce::roundToInt(x),r.getY(),r.getBottom());
        g.setColour(col.withAlpha(enabled?(focused?0.19f:0.045f):0.025f));
        g.fillEllipse(x-10,y-10,20,20);
        g.setColour(enabled?col:juce::Colour::fromRGB(79,90,97));
        g.fillEllipse(x-5.5f,y-5.5f,11,11);
        if(i==selected_||solo||focused){
            g.setColour(solo?juce::Colours::white:col.brighter(0.65f));
            g.drawEllipse(x-9,y-9,18,18,i==selected_?2.0f:1.2f);
        }
        // Precise ROOT selection details live in the contextual control shelf,
        // not in floating text that masks live spectrum or CARVE contours.
    }
    // Focused ROOT foreground rail: all four slots remain identifiable.
    // OFF slots are selectable but never represented as active audio filters.
    const auto focusedColor=colour(selectedRoot);
    g.setColour(focusedColor.withAlpha(0.18f));
    g.fillRoundedRectangle(r.getX(),24.0f,171.0f,20.0f,4.0f);
    g.setColour(focusedColor.brighter(0.45f));
    g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
    g.drawFittedText("ROOT "+juce::String(selectedRoot+1)+"  /  C1–C4",
        juce::Rectangle<int>((int)r.getX()+6,25,160,18),
        juce::Justification::centredLeft,1);

    // Selected ROOT CARVE markers always render AFTER boundary handles and heat.
    // Active cuts use their actual host frequency and depth; inactive slots
    // exist only as OFF selectors in the top rail (no fabricated filter plot).
    for(int slot=0;slot<surgery::ui::kSlotsPerRoot;++slot){
        const auto id=[selectedRoot,slot](const char* suffix){
            return PRISMVSTAudioProcessor::carveId(selectedRoot,slot,suffix);
        };
        const bool enabled=pval(processor_.apvts,id("on"))>=0.5f;
        if(!enabled)continue; // Disabled CARVEs are genuinely invisible on the graph.
        const float x=freqToX(surgery::precision::boundedFrequency(
            pval(processor_.apvts,id("hz")),
            param(selectedRoot*2,"hz"),param(selectedRoot*2+1,"hz")));
        const float depth=pval(processor_.apvts,id("depth"));
        const float y=surgery::ui::carveY(r.getCentreY(),depth);
        const auto outline=focusedColor.brighter(0.35f);
        if(enabled) {
            g.setColour(focusedColor.withAlpha(0.42f));
            g.drawVerticalLine(juce::roundToInt(x),y,r.getBottom());
        }
        g.setColour(juce::Colour(0xff0c1820));
        g.fillEllipse(x-10.5f,y-10.5f,21.0f,21.0f);
        g.setColour(enabled?outline:juce::Colour(0xff788b93));
        g.drawEllipse(x-10.5f,y-10.5f,21.0f,21.0f,slot==slot_?2.8f:1.8f);
        if(enabled) {
            g.setColour(slot==slot_?juce::Colours::white:focusedColor);
            g.fillEllipse(x-6.0f,y-6.0f,12.0f,12.0f);
        }
        // The marker centre stays at the measured host parameter position.
        // Small nearby identification plates may overlap, not the node data.
        const int labelX=juce::jlimit((int)r.getX(),(int)r.getRight()-30,
                                    juce::roundToInt(x)+9+(slot%2)*2);
        const int labelY=juce::jlimit((int)r.getY(),(int)r.getBottom()-17,
                                    juce::roundToInt(y)-25-(slot%2)*17);
        g.setColour(juce::Colour(0xff07151c).withAlpha(0.94f));
        g.fillRoundedRectangle((float)labelX,(float)labelY,29.0f,17.0f,4.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
        g.drawText("C"+juce::String(slot+1),labelX,labelY,29,17,
                   juce::Justification::centred);
    }
    for(int slot=0;slot<surgery::ui::kSlotsPerRoot;++slot){
        const auto bounds=surgery::ui::carveTabBounds((float)getWidth(),slot);
        const auto id=PRISMVSTAudioProcessor::carveId(selectedRoot,slot,"on");
        const bool enabled=pval(processor_.apvts,id)>=0.5f;
        const bool chosen=slot==slot_;
        g.setColour(juce::Colour(0xff09151b).withAlpha(0.95f));
        g.fillRoundedRectangle(bounds.x,bounds.y,bounds.w,bounds.h,4.0f);
        g.setColour(enabled?focusedColor.withAlpha(chosen?1.0f:0.75f):
                             juce::Colour(0xff68838a).withAlpha(chosen?0.95f:0.55f));
        g.drawRoundedRectangle(bounds.x,bounds.y,bounds.w,bounds.h,4.0f,chosen?2.0f:1.0f);
        g.setColour(enabled?juce::Colours::white:juce::Colour(0xff9bb0b6));
        g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
        const auto label="C"+juce::String(slot+1)+(enabled?"  ON":"  OFF");
        g.drawText(label,juce::roundToInt(bounds.x),juce::roundToInt(bounds.y),
                   juce::roundToInt(bounds.w),juce::roundToInt(bounds.h),
                   juce::Justification::centred);
    }
    if(hover_&&r.contains(mouse_)){
        const int bar=prism::analysis::LabBands::indexForFrequency(xToFreq(mouse_.x));
        const float freq=xToFreq(mouse_.x);
        g.setColour(juce::Colours::white.withAlpha(0.26f));
        g.drawVerticalLine(juce::roundToInt(mouse_.x),r.getY(),r.getBottom());
        g.drawHorizontalLine(juce::roundToInt(mouse_.y),r.getX(),r.getRight());
        juce::String text=hzText(freq)+" Hz / "+
                juce::String(bands_[(size_t)bar],1)+" dBFS / HOLD "+
                juce::String(peaks_[(size_t)bar],1)+" dBFS";
        const int hoveredCarve=hitCarveNode(mouse_);
        if(hoveredCarve>=0) {
            const int root=selected_/2;
            const auto id=[root,hoveredCarve](const char* suffix){
                return PRISMVSTAudioProcessor::carveId(root,hoveredCarve,suffix);
            };
            text="C"+juce::String(hoveredCarve+1)+" / "+
                 hzText(pval(processor_.apvts,id("hz")))+" Hz / Q "+
                 juce::String(pval(processor_.apvts,id("q")),2)+" / CUT "+
                 juce::String(pval(processor_.apvts,id("depth")),1)+" dB";
        }
        const float left=juce::jlimit(r.getX(),r.getRight()-274.0f,mouse_.x+12);
        const float top=juce::jlimit(r.getY(),r.getBottom()-29.0f,mouse_.y-39.0f);
        g.setColour(juce::Colour::fromRGB(6,11,18).withAlpha(0.97f));
        g.fillRoundedRectangle(left,top,274,27,4.0f);
        g.setColour(juce::Colour::fromRGB(236,243,247));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawFittedText(text,(int)left+6,(int)top+3,262,21,
                         juce::Justification::centredLeft,1);
    }
}
int RootsGraph::hitCarveTab(juce::Point<float> point) const {
    for(int slot=0;slot<surgery::ui::kSlotsPerRoot;++slot)
        if(surgery::ui::carveTabBounds((float)getWidth(),slot).contains(point.x,point.y))
            return slot;
    return -1;
}
int RootsGraph::hitCarveNode(juce::Point<float> point) const {
    const int root=selected_/2;
    float nearest=15.0f;
    int target=-1;
    for(int slot=0;slot<surgery::ui::kSlotsPerRoot;++slot){
        const auto id=[root,slot](const char* suffix){
            return PRISMVSTAudioProcessor::carveId(root,slot,suffix);
        };
        if(pval(processor_.apvts,id("on"))<0.5f)continue; // Invisible means not hit-testable.
        const float x=freqToX(surgery::precision::boundedFrequency(
            pval(processor_.apvts,id("hz")),
            param(root*2,"hz"),param(root*2+1,"hz")));
        const float y=surgery::ui::carveY(area().getCentreY(),
                                          pval(processor_.apvts,id("depth")));
        const float distance=point.getDistanceFrom({x,y});
        // Prefer the selected slot for overlapping real audio nodes.
        if(distance<nearest-0.5f || (slot==slot_&&distance<=nearest+0.5f)){
            nearest=distance;target=slot;
        }
    }
    return target;
}
void RootsGraph::mouseDown(const juce::MouseEvent& e){
    mouse_=e.position;hover_=true;
    if(!e.mods.isPopupMenu()){
        const int tab=hitCarveTab(e.position);
        if(tab>=0){
            if(onSelectCarve)onSelectCarve(tab);
            repaint();
            return;
        }
        const int slot=hitCarveNode(e.position);
        if(slot>=0){
            // Dragging only moves an explicitly enabled, visible CARVE.
            draggingCarve_=slot;
            const int root=selected_/2;
            dragStartX_=e.position.x;
            dragStartHz_=pval(processor_.apvts,
                PRISMVSTAudioProcessor::carveId(root,slot,"hz"));
            fineDragging_=e.mods.isCtrlDown();
            if(onSelectCarve)onSelectCarve(slot);
            for(const auto* suffix:{"hz","depth"})
                if(auto* parameter=processor_.apvts.getParameter(
                       PRISMVSTAudioProcessor::carveId(root,slot,suffix)))
                    parameter->beginChangeGesture();
            return;
        }
    }
    const int hit=hitNode(e.position);
    if(hit>=0)selectNode(hit);
    if(e.mods.isPopupMenu()){
        if(hit<0){
            const float f=xToFreq(e.position.x);
            int target=6;
            for(int root=0;root<7;++root){
                const float low=param(root*2,"hz");
                const float high=param(root*2+1,"hz");
                if(f>=low&&f<=high){target=root;break;}
            }
            selectNode(target*2);
        }
        popup();return;
    }
    dragging_=hit;
    if(dragging_>=0){
        if(auto* p=processor_.apvts.getParameter(
             PRISMVSTAudioProcessor::rootsNodeId(dragging_,"hz")))
            p->beginChangeGesture();
    }
}
void RootsGraph::mouseDrag(const juce::MouseEvent& e){
    if(draggingCarve_>=0){
        const int root=selected_/2,slot=draggingCarve_;
        const auto id=[root,slot](const char* s){
            return PRISMVSTAudioProcessor::carveId(root,slot,s);
        };
        const float a=param(root*2,"hz"),b=param(root*2+1,"hz");
        const bool fine=e.mods.isCtrlDown();
        if(fine&&!fineDragging_){
            // Ctrl may be pressed in the middle of a drag: anchor current value.
            dragStartX_=e.position.x;
            dragStartHz_=pval(processor_.apvts,id("hz"));
        }
        fineDragging_=fine;
        const float next=surgery::precision::scrubFrequency(
            dragStartHz_,e.position.x-dragStartX_,xToFreq(e.position.x),
            fine,a,b);
        change(processor_,id("hz"),next);
        if(!fine)
            change(processor_,id("depth"),surgery::ui::depthFromY(
                area().getCentreY(),e.position.y));
        repaint();return;
    }
    if(dragging_<0)return;
    const float newHz=xToFreq(e.position.x);
    // Deliberately ignore e.position.y: frequency handles have no gain axis.
    write(dragging_,"hz",newHz);
    repaint();
}
void RootsGraph::mouseUp(const juce::MouseEvent&){
    if(draggingCarve_>=0){
        const int root=selected_/2,slot=draggingCarve_;
        for(const auto* name:{"hz","depth"})
            if(auto* p=processor_.apvts.getParameter(
                PRISMVSTAudioProcessor::carveId(root,slot,name)))
                p->endChangeGesture();
        draggingCarve_=-1;
        fineDragging_=false;
    }
    if(dragging_>=0){
        if(auto* p=processor_.apvts.getParameter(
             PRISMVSTAudioProcessor::rootsNodeId(dragging_,"hz")))
            p->endChangeGesture();
    }
    if(dragging_>=0)clampCarvesToRoot(dragging_/2);
    dragging_=-1;
}
void RootsGraph::mouseMove(const juce::MouseEvent& e){mouse_=e.position;hover_=true;repaint();}
void RootsGraph::mouseExit(const juce::MouseEvent&){hover_=false;repaint();}
void RootsGraph::mouseDoubleClick(const juce::MouseEvent& e){
    if(e.mods.isPopupMenu())return;
    // Finish any gesture opened by the second mouse-down before resetting.
    const int tab=hitCarveTab(e.position);
    const int carve=tab>=0?tab:hitCarveNode(e.position);
    const int boundary=carve<0?hitNode(e.position):-1;
    if(draggingCarve_>=0||dragging_>=0)mouseUp(e);
    if(tab>=0&&pval(processor_.apvts,
        PRISMVSTAudioProcessor::carveId(selected_/2,tab,"on"))<0.5f){
        if(onSelectCarve)onSelectCarve(tab);
        return; // An OFF selector cannot resurrect its invisible node.
    }
    if(carve>=0){
        if(onSelectCarve)onSelectCarve(carve);
        processor_.resetCarveNode(selected_/2,carve);
        repaint();
        return;
    }
    if(boundary>=0){
        selectNode(boundary);
        processor_.resetRootBoundary(boundary);
        clampCarvesToRoot(boundary/2);
        repaint();
    }
}
void RootsGraph::mouseWheelMove(const juce::MouseEvent& e,
                                const juce::MouseWheelDetails& wheel){
    const int slot=hitCarveNode(e.position);
    if(slot<0)return; // wheel over ROOT boundary never changes another parameter
    const int root=selected_/2;
    const auto id=[root,slot](const char* suffix){
        return PRISMVSTAudioProcessor::carveId(root,slot,suffix);
    };
    if(onSelectCarve)onSelectCarve(slot);
    if(e.mods.isCtrlDown()){
        const float a=param(root*2,"hz"),b=param(root*2+1,"hz");
        const auto frequency=surgery::precision::scrubWheelFrequency(
            pval(processor_.apvts,id("hz")),wheel.deltaY*10.0f,a,b);
        change(processor_,id("hz"),frequency);
    }else{
        // Wheel changes mathematical quality factor; Shift grants finer steps.
        const float q=surgery::precision::adjustQ(
            pval(processor_.apvts,id("q")),wheel.deltaY*10.0f,
            e.mods.isShiftDown());
        change(processor_,id("q"),q);
    }
    repaint();
}
void RootsGraph::clampCarvesToRoot(int root){
    if(root<0||root>=7)return;
    const auto a=param(root*2,"hz"),b=param(root*2+1,"hz");
    for(int slot=0;slot<4;++slot){
        const auto id=PRISMVSTAudioProcessor::carveId(root,slot,"hz");
        const float raw=pval(processor_.apvts,id);
        const auto bound=surgery::precision::boundedFrequency(raw,a,b);
        if(std::abs(raw-bound)>0.005f)change(processor_,id,bound);
    }
}
void RootsGraph::popup(){
    const int root=selected_/2;
    const auto rid=[root](const char* suffix){
        return PRISMVSTAudioProcessor::rootsRootControlId(root,suffix);
    };
    const auto current=[this,&rid](const char* suffix){
        return pval(processor_.apvts,rid(suffix));
    };
    juce::PopupMenu menu;
    menu.addSectionHeader(PRISMVSTAudioProcessor::sectionName(root));
    menu.addItem(1,"ROOT ENABLED",true,current("on")>=0.5f);
    const bool solo=pval(processor_.apvts,
        PRISMVSTAudioProcessor::rootsRootSoloId(root))>=0.5f;
    menu.addItem(2,"SOLO ENTIRE ROOT / OFF",true,solo);
    menu.addSeparator();
    menu.addSectionHeader("ROOT GAIN + CARVE / EQ DOMAIN");
    menu.addItem(10,"STEREO",true,juce::roundToInt(current("domain"))==0);
    menu.addItem(11,"MID",true,juce::roundToInt(current("domain"))==1);
    menu.addItem(12,"SIDE",true,juce::roundToInt(current("domain"))==2);
    menu.addSeparator();
    menu.addSectionHeader("ROOT COMPRESSOR CHANNEL");
    static const char* dn[]{"STEREO LINK","DUAL MID/SIDE","MID ONLY","SIDE ONLY"};
    for(int i=0;i<4;++i)
        menu.addItem(30+i,dn[i],true,juce::roundToInt(current("detector"))==i);
    menu.addSeparator();
    menu.addItem(15,"MONO SELECTED ROOT",true,current("mono")>=0.5f);
    menu.addSeparator();
    juce::PopupMenu palette;
    static constexpr const char* names[]{"CYAN","AMBER","JADE","VIOLET","ROSE","BLUE","LIME","WHITE"};
    for(int i=0;i<8;++i)palette.addItem(80+i,names[i]);
    palette.addItem(99,"CUSTOM HEX...");
    menu.addSubMenu("ROOT COLOUR",palette);
    const juce::Component::SafePointer<RootsGraph> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [safe,root](int id){
            if(safe==nullptr||id<=0)return;
            auto& graph=*safe;
            const auto rid=[root](const char* suffix){
                return PRISMVSTAudioProcessor::rootsRootControlId(root,suffix);
            };
            const auto value=[&graph,&rid](const char* suffix){
                return pval(graph.processor_.apvts,rid(suffix));
            };
            const auto set=[&graph,&rid](const char* suffix,float v){
                change(graph.processor_,rid(suffix),v);
            };
            if(id==1)set("on",value("on")>=0.5f?0.0f:1.0f);
            if(id==2){
                const auto key=PRISMVSTAudioProcessor::rootsRootSoloId(root);
                const bool on=pval(graph.processor_.apvts,key)>=0.5f;
                change(graph.processor_,key,on?0.0f:1.0f);
            }
            if(id>=10&&id<=12)set("domain",(float)(id-10));
            if(id==15)set("mono",value("mono")>=0.5f?0.0f:1.0f);
            if(id>=30&&id<=33)set("detector",(float)(id-30));

            const auto colorId="surgery_root"+juce::String(root+1)+"_rgb";
            static constexpr unsigned swatches[]{
                0x6BBDD4,0xDFA871,0x8DCEA8,0xD2A2E5,
                0xE89EAD,0x9DADEA,0xBFD678,0xE5F4F3};
            if(id>=80&&id<=87)change(graph.processor_,colorId,(float)swatches[id-80]);
            if(id==99){
                auto* dialog=new juce::AlertWindow(
                    "ROOT COLOUR","Enter a 6-digit RGB hexadecimal value",
                    juce::MessageBoxIconType::NoIcon);
                dialog->addTextEditor("hex","6BBDD4","RRGGBB");
                dialog->addButton("APPLY",1);
                dialog->addButton("CANCEL",0);
                dialog->enterModalState(true,
                    juce::ModalCallbackFunction::create(
                        [safe,dialog,colorId](int result) {
                            if(result!=1||safe==nullptr)return;
                            auto str=dialog->getTextEditorContents("hex").trim().removeCharacters("#");
                            if(str.length()!=6||!str.containsOnly("0123456789aAbBcCdDeEfF"))return;
                            const unsigned value=(unsigned)str.getHexValue32()&0x00ffffffu;
                            change(safe->processor_,colorId,(float)value);
                            safe->repaint();
                        }),true);
            }
            graph.repaint();
        });
}
RootsEditor::RootsEditor(PRISMVSTAudioProcessor& p)
: juce::AudioProcessorEditor(&p),processor_(p),graph_(p) {
    setLookAndFeel(&lookAndFeel_);
    setSize(1120,755);setResizable(true,true);
    setResizeLimits(840,610,1800,1100);
    addAndMakeVisible(graph_);
    graph_.onSelectNode=[this](int n){bindNode(n);};
    graph_.onSelectCarve=[this](int slot){
        carveSlot_=juce::jlimit(0,3,slot);
        bindCarve();
        if(mode_!=1)setMode(1); // Foreground slot selection opens its real controls.
    };
    for(int root=0;root<7;++root){
        auto& b=rootButtons_[(size_t)root];
        b.setButtonText(PRISMVSTAudioProcessor::sectionName(root));
        b.onClick=[this,root](){graph_.selectNode(root*2);};
        b.onContext=[this,root](){graph_.showRootMenu(root);};
        b.setColour(juce::TextButton::textColourOffId,
                    juce::Colour::fromRGB(192,213,220));
        addAndMakeVisible(b);
    }
    for(auto* label:{&title_,&selection_,&status_,&gainLabel_,
                     &thresholdLabel_,&carveFrequencyLabel_,&carveDepthLabel_,
                     &carveQLabel_,&carveThresholdLabel_,&analyzerDepthLabel_,
                     &analyzerSlopeLabel_,&analyzerMemoryLabel_,&analyzerLegend_}){
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId,juce::Colour::fromRGB(205,224,229));
        label->setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::plain)));
        label->setJustificationType(juce::Justification::centredLeft);
    }
    title_.setText("SURGERY EQ  /  v1.0",juce::dontSendNotification);
    analyzerLegend_.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));
    analyzerLegend_.setText("CHROMATIC SONAR   /   FREQUENCY × ENERGY",
                            juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions(24.0f,juce::Font::bold)));
    selection_.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));
    status_.setFont(juce::Font(juce::FontOptions(11.5f,juce::Font::plain)));
    status_.setJustificationType(juce::Justification::centredRight);
    for(auto* s:{&gain_,&threshold_}){
        addAndMakeVisible(*s);rotary(*s,"",s==&gain_?0.0:-18.0);
        s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour::fromRGB(143,192,204));
        s->setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour::fromRGB(38,55,65));
        s->setColour(juce::Slider::textBoxTextColourId,juce::Colour::fromRGB(239,243,245));
        s->setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour::fromRGB(13,19,25));
        s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colour::fromRGB(38,55,65));
    }
    gainLabel_.setText("ROOT GAIN (dB)",juce::dontSendNotification);
    gain_.setTooltip("Entire ROOT frequency range gain: -60 to +60 dB");

    thresholdLabel_.setText("COMP THRESHOLD",juce::dontSendNotification);
    for(auto* b:{&mono_,&on_,&solo_}){
        b->setClickingTogglesState(true);
        b->setColour(juce::TextButton::buttonColourId,juce::Colour::fromRGB(24,38,47));
        b->setColour(juce::TextButton::buttonOnColourId,juce::Colour::fromRGB(43,153,166));
        b->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
        b->setColour(juce::TextButton::textColourOnId,juce::Colours::white);
        addAndMakeVisible(*b);
    }
    static constexpr const char* labels[]{"ROOT","CARVE","ONYX","ANALYZE"};
    for(int i=0;i<4;++i){
        auto& b=modeButtons_[(size_t)i];
        b.setButtonText(labels[i]);
        b.onClick=[this,i]{setMode(i);};
        addAndMakeVisible(b);
    }
    for(int i=0;i<4;++i){
        auto& b=carveSlotButtons_[(size_t)i];
        b.setButtonText("C"+juce::String(i+1));
        b.onClick=[this,i]{carveSlot_=i;bindCarve();graph_.setPresentation(themeIndex_,mode_,carveSlot_);};
        addAndMakeVisible(b);
    }
    for(auto* s:{&carveFrequency_,&carveDepth_,&carveQ_,&carveThreshold_}){
        rotary(*s,"",0.0);addAndMakeVisible(*s);
    }
    carveDepth_.setDoubleClickReturnValue(true,0.0);
    carveQ_.setDoubleClickReturnValue(true,2.0);
    carveThreshold_.setDoubleClickReturnValue(true,-24.0);
    carveFrequency_.setMouseDragSensitivity(2000);
    carveQ_.setMouseDragSensitivity(2800);
    carveQ_.setNumDecimalPlacesToDisplay(3);
    carveFrequency_.setTooltip("Exact Hz value: type in the field. CTRL+drag a CARVE node for fine frequency scrubbing.");
    carveQ_.setTooltip("Ultra-surgical Q 0.300 to 96.000. Higher = tighter notch; 0–100% tightness shown above knob. SHIFT+wheel = fine Q.");
    carveFrequencyLabel_.setText("FREQUENCY Hz",juce::dontSendNotification);
    carveDepthLabel_.setText("CUT DEPTH dB",juce::dontSendNotification);
    carveQLabel_.setText("Q / PRECISION",juce::dontSendNotification);
    carveThresholdLabel_.setText("TRIGGER dBFS",juce::dontSendNotification);
    for(auto* b:{&carveOn_,&carveDynamic_}){
        b->setClickingTogglesState(true);
        addAndMakeVisible(*b);
    }
    const std::array<const char*,4> names{"SPARKLE","SHINE","SMOOTHENING","VINTAGE TAPE"};
    const std::array<const char*,4> ids{
        "surgery_onyx_sparkle","surgery_onyx_shine","surgery_onyx_smooth","surgery_onyx_tape"};
    for(int i=0;i<4;++i){
        auto& s=onyxSliders_[(size_t)i];
        rotary(s," %",0.0);s.setRange(0.0,100.0,0.1);
        s.setMouseDragSensitivity(600);
        s.setDoubleClickReturnValue(true,0.0);
        addAndMakeVisible(s);
        onyxLabels_[(size_t)i].setText(names[(size_t)i],juce::dontSendNotification);
        onyxLabels_[(size_t)i].setJustificationType(juce::Justification::centred);
        onyxLabels_[(size_t)i].setColour(juce::Label::textColourId,juce::Colours::white);
        addAndMakeVisible(onyxLabels_[(size_t)i]);
        onyxAttachments_[(size_t)i]=std::make_unique<SliderA>(processor_.apvts,ids[(size_t)i],s);
    }
    addAndMakeVisible(themeButton_);
    themeButton_.onClick=[this]{
        themeIndex_=(themeIndex_+1)%4;
        change(processor_,"surgery_theme",(float)themeIndex_);
        graph_.setPresentation(themeIndex_,mode_,carveSlot_);
        static const char* names[]{"ONYX","ETHER LAB","ALIEN GRID","CINEMA VOID"};
        themeButton_.setButtonText(juce::String("THEME  /  ")+names[themeIndex_]);
        graph_.repaint();
        repaint();
    };
    themeButton_.setButtonText("THEME  /  ONYX");
    mono_.setTooltip("Frequency-selective MONO on the selected ROOT only");
    analyzerDepthLabel_.setText("VIEW RANGE",juce::dontSendNotification);
    analyzerSlopeLabel_.setText("PIVOT / dB PER OCT",juce::dontSendNotification);
    analyzerMemoryLabel_.setText("VISUAL DECAY / SEC",juce::dontSendNotification);
    analyzerDepth_.addItem("MIX -96 dBFS",1);
    analyzerDepth_.addItem("DEEP -120 dBFS",2);
    analyzerDepth_.addItem("FORENSIC -144 dBFS",3);
    addAndMakeVisible(analyzerDepth_);
    analyzerDepthA_=std::make_unique<ComboA>(processor_.apvts,"analyzer_depth",analyzerDepth_);
    for(auto* s:{&analyzerSlope_,&analyzerMemory_}){
        rotary(*s,"",s==&analyzerSlope_?0.0:1.5);
        addAndMakeVisible(*s);
    }
    analyzerSlopeA_=std::make_unique<SliderA>(processor_.apvts,"analyzer_slope",analyzerSlope_);
    analyzerMemoryA_=std::make_unique<SliderA>(processor_.apvts,"aura_memory",analyzerMemory_);
    heatToggle_.setTooltip("Show or hide frequency-coloured spectral activity from measured FFT levels");
    heatToggle_.setClickingTogglesState(true);
    addAndMakeVisible(heatToggle_);
    heatA_=std::make_unique<ButtonA>(processor_.apvts,"aura_color",heatToggle_);
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.setTooltip("Freeze measured spectrum and SONAR history; DSP remains live");
    freezeButton_.onClick=[this]{
        graph_.setFrozen(freezeButton_.getToggleState());
        freezeButton_.setButtonText(graph_.isFrozen()?"RESUME":"FREEZE");
    };
    addAndMakeVisible(freezeButton_);
    bindNode(0);
    setMode(0);
    startTimerHz(60);
}
RootsEditor::~RootsEditor(){stopTimer();setLookAndFeel(nullptr);}
void RootsEditor::bindNode(int n){
    node_=juce::jlimit(0,prism::roots::v2::kNodes-1,n);
    gainA_.reset();thresholdA_.reset();monoA_.reset();
    onA_.reset();soloA_.reset();
    const auto rid=[this](const char* suffix){
        return PRISMVSTAudioProcessor::rootsRootControlId(node_/2,suffix);
    };
    gainA_=std::make_unique<SliderA>(processor_.apvts,
       PRISMVSTAudioProcessor::rootsRootGainId(node_/2),gain_);

    thresholdA_=std::make_unique<SliderA>(processor_.apvts,rid("threshold"),threshold_);
    monoA_=std::make_unique<ButtonA>(processor_.apvts,rid("mono"),mono_);
    onA_=std::make_unique<ButtonA>(processor_.apvts,rid("on"),on_);
    soloA_=std::make_unique<ButtonA>(processor_.apvts,
        PRISMVSTAudioProcessor::rootsRootSoloId(node_/2),solo_);
    selection_.setText("ROOT "+juce::String(node_/2+1)+
        "  /  BOUNDARY "+juce::String(node_%2+1),juce::dontSendNotification);
    bindCarve();
    updateRootButtons();
}
void RootsEditor::bindCarve(){
    carveFrequencyA_.reset();carveDepthA_.reset();
    carveQA_.reset();carveThresholdA_.reset();
    carveOnA_.reset();carveDynamicA_.reset();
    const int root=node_/2,slot=carveSlot_;
    const float a=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2,"hz"));
    const float b=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2+1,"hz"));
    const double low=juce::jmax(20.0f,std::min(a,b));
    const double high=juce::jmax((float)low,std::max(a,b));
    carveFrequency_.setDoubleClickReturnValue(true,
        std::exp(std::log(low)+(slot+1.0)/5.0*std::log(high/low)));
    const auto id=[root,slot](const char* s){
        return PRISMVSTAudioProcessor::carveId(root,slot,s);
    };
    carveFrequency_.onValueChange=[this]{
        const int root=node_/2;
        const auto a=pval(processor_.apvts,
            PRISMVSTAudioProcessor::rootsNodeId(root*2,"hz"));
        const auto b=pval(processor_.apvts,
            PRISMVSTAudioProcessor::rootsNodeId(root*2+1,"hz"));
        const double value=carveFrequency_.getValue();
        const auto limited=surgery::precision::boundedFrequency((float)value,a,b);
        if(std::abs(value-limited)>0.001)
            carveFrequency_.setValue(limited,juce::sendNotificationSync);
    };
    carveFrequencyA_=std::make_unique<SliderA>(processor_.apvts,id("hz"),carveFrequency_);
    carveDepthA_=std::make_unique<SliderA>(processor_.apvts,id("depth"),carveDepth_);
    carveQA_=std::make_unique<SliderA>(processor_.apvts,id("q"),carveQ_);
    carveThresholdA_=std::make_unique<SliderA>(processor_.apvts,id("threshold"),carveThreshold_);
    carveOnA_=std::make_unique<ButtonA>(processor_.apvts,id("on"),carveOn_);
    carveDynamicA_=std::make_unique<ButtonA>(processor_.apvts,id("dynamic"),carveDynamic_);
    carveOn_.setButtonText(pval(processor_.apvts,id("on"))>=0.5f?
                           "CARVE ON":"CARVE OFF");
    for(int i=0;i<4;++i){
        carveSlotButtons_[(size_t)i].setToggleState(i==slot,juce::dontSendNotification);
        carveSlotButtons_[(size_t)i].setColour(juce::TextButton::buttonOnColourId,
            territory(root).withAlpha(0.66f));
    }
    graph_.setPresentation(themeIndex_,mode_,carveSlot_);
}
void RootsEditor::setMode(int next){
    mode_=juce::jlimit(0,3,next);
    for(int i=0;i<4;++i){
        auto& b=modeButtons_[(size_t)i];
        b.setToggleState(i==mode_,juce::dontSendNotification);
        b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff1a2933));
        b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff247a83));
        b.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
        b.setColour(juce::TextButton::textColourOnId,juce::Colours::white);
    }
    const bool carve=mode_==1;
    for(auto* s:{&carveFrequency_,&carveDepth_,&carveQ_,&carveThreshold_})s->setVisible(carve);
    for(auto* l:{&carveFrequencyLabel_,&carveDepthLabel_,&carveQLabel_,&carveThresholdLabel_})l->setVisible(carve);
    for(auto& b:carveSlotButtons_)b.setVisible(carve);
    carveOn_.setVisible(carve);carveDynamic_.setVisible(carve);
    analyzerLegend_.setVisible(mode_==3);
    gain_.setVisible(mode_==0);gainLabel_.setVisible(mode_==0);
    threshold_.setVisible(mode_==0);
    thresholdLabel_.setVisible(mode_==0);
    for(int i=0;i<4;++i){
        onyxSliders_[(size_t)i].setVisible(mode_==2);
        onyxLabels_[(size_t)i].setVisible(mode_==2);
    }
    on_.setVisible(mode_==0);
    solo_.setVisible(mode_==0);
    mono_.setVisible(mode_==0);
    const bool analyze=mode_==3;
    analyzerDepth_.setVisible(analyze);
    analyzerDepthLabel_.setVisible(analyze);
    analyzerSlope_.setVisible(analyze);
    analyzerSlopeLabel_.setVisible(analyze);
    analyzerMemory_.setVisible(analyze);
    analyzerMemoryLabel_.setVisible(analyze);
    heatToggle_.setVisible(analyze);
    freezeButton_.setVisible(analyze);
    graph_.setPresentation(themeIndex_,mode_,carveSlot_);
    resized();
    repaint();
}
void RootsEditor::updateRootButtons(){
    for(int root=0;root<7;++root){
        const bool selected=graph_.selectedNode()/2==root;
        auto& button=rootButtons_[(size_t)root];
        button.setToggleState(selected,juce::dontSendNotification);
        button.setColour(juce::TextButton::buttonOnColourId,
            territory(root).withAlpha(0.52f));
        button.setColour(juce::TextButton::buttonColourId,
            juce::Colour::fromRGB(22,32,41));
    }
}
void RootsEditor::timerCallback(){
    themeIndex_=juce::jlimit(0,3,juce::roundToInt(pval(processor_.apvts,"surgery_theme")));
    lookAndFeel_.setTheme(themeIndex_);
    static const char* themeNames[]{"ONYX","ETHER LAB","ALIEN GRID","CINEMA VOID"};
    themeButton_.setButtonText(juce::String("THEME / ")+themeNames[themeIndex_]);
    // Show when compression is disabled; never present inactive controls as working.
    const int comp=juce::roundToInt(pval(processor_.apvts,
         PRISMVSTAudioProcessor::rootsRootControlId(node_/2,"comp")));
    threshold_.setEnabled(comp!=0);
thresholdLabel_.setText(comp==0?"THRESHOLD (COMP OFF)":"COMP THRESHOLD",
                            juce::dontSendNotification);
    updateRootButtons();
    const float factor=pval(processor_.apvts,
        PRISMVSTAudioProcessor::carveId(node_/2,carveSlot_,"q"));
    const float tight=surgery::precision::tightnessPercent(factor);
    carveQLabel_.setText("Q / "+juce::String(tight,1)+"% TIGHT",
                         juce::dontSendNotification);
    carveOn_.setButtonText(pval(processor_.apvts,
       PRISMVSTAudioProcessor::carveId(node_/2,carveSlot_,"on"))>=0.5f?
       "CARVE ON":"CARVE OFF");
    heatToggle_.setButtonText(pval(processor_.apvts,"aura_color")>=0.5f?
                              "SONAR ON":"SONAR OFF");
    graph_.setPresentation(themeIndex_,mode_,carveSlot_);
    std::array<float,PRISMVSTAudioProcessor::labBars> bars{},peaks{};
    processor_.copyPrismLabBars(bars,peaks);
    graph_.updateAnalyzer(bars,peaks);
    const auto state=processor_.getRootsMeter();
    const juce::String status=
        "IN "+juce::String(processor_.getInputPeakDb(),1)+" dBFS  /  OUT "+
        juce::String(processor_.getPeakDb(),1)+" dBFS  /  CORR "+
        juce::String(state.correlation,2)+"  /  GR "+
        juce::String(state.largestGrDb,1)+" dB";
    if(mode_==3){
        const auto metrics=graph_.spectralMetrics();
        const juce::String line=metrics.signalPresent?
            "POST FFT  /  PEAK "+hzText(metrics.loudestHz)+"Hz  "+
            juce::String(metrics.loudestDb,1)+" dBFS  /  CENTROID ~"+
            hzText(metrics.approximateCentroidHz)+"Hz":
            "POST FFT  /  SILENCE OR BELOW DISPLAY FLOOR";
        status_.setText(line,juce::dontSendNotification);
    } else status_.setText(status,juce::dontSendNotification);
}
void RootsEditor::paint(juce::Graphics& g){
    const auto w=(float)getWidth(),h=(float)getHeight();
    const bool lab=themeIndex_==1;
    const auto top=lab?juce::Colour(0xffe0e8e9):
                   themeIndex_==2?juce::Colour(0xff102923):
                   themeIndex_==3?juce::Colour(0xff263448):
                                  juce::Colour(0xff1b2731);
    const auto bottom=lab?juce::Colour(0xffa5b6bb):
                      themeIndex_==2?juce::Colour(0xff071612):
                      themeIndex_==3?juce::Colour(0xff0e1724):
                                     juce::Colour(0xff0a121b);
    juce::ColourGradient chassis(top,0,0,bottom,w,h,false);
    chassis.addColour(.45,top.interpolatedWith(bottom,.32f));
    g.setGradientFill(chassis);
    g.fillRect(getLocalBounds());
    const auto accent=themeIndex_==2?juce::Colour(0xff81dfbb):
                      themeIndex_==3?juce::Colour(0xffe0a878):
                                     juce::Colour(0xff84d1df);
    // Hardware header: thin structural rails rather than ornamental neon.
    const auto header=juce::Rectangle<float>(12,9,w-24,43);
    g.setColour(lab?juce::Colour(0xffc2d1d4):juce::Colour(0xff121e27));
    g.fillRoundedRectangle(header,5.0f);
    g.setColour(accent.withAlpha(.58f));
    g.drawRoundedRectangle(header,5.0f,1.0f);
    g.fillRect(header.getX()+8,header.getBottom()-2,56.0f,2.0f);
    // Reserved grouping lines, never crossing numeric fields or labels.
    // Mode bank and modular instrument shelf.
    const auto mode=juce::Rectangle<float>(12,h-226,w-24,42);
    g.setColour(lab?juce::Colour(0xff7d969e):juce::Colour(0xff0a141d));
    g.fillRoundedRectangle(mode,6.0f);
    g.setColour(accent.withAlpha(.33f));
    g.drawRoundedRectangle(mode,6.0f,1.0f);
    const auto shelf=juce::Rectangle<float>(12,h-158,w-24,124);
    g.setColour(lab?juce::Colour(0xff8a9fa5):juce::Colour(0xff14232c));
    g.fillRoundedRectangle(shelf,7.0f);
    g.setColour(accent.withAlpha(.44f));
    g.drawRoundedRectangle(shelf,7.0f,1.1f);
    // No internal vertical bars under control labels: one coherent shelf.
    const auto label=lab?juce::Colour(0xff24404a):juce::Colour(0xff93b1bd);
    g.setColour(label);
    g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
    const auto headerLayout=surgery::ui::headerLayout(getWidth());
    g.drawFittedText("SPIRAL ETHER TECH  /  SONAR ROOTS PROCESSOR",
               headerLayout.brand.x,headerLayout.brand.y,
               headerLayout.brand.width,headerLayout.brand.height,
               juce::Justification::centredRight,1);
    g.setColour(label.withAlpha(.85f));
    g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::plain)));
    g.drawText("SURGERY ENGINE / ROOTS / CARVE / ONYX / ANALYZE",
               24,getHeight()-27,getWidth()-48,18,juce::Justification::centredLeft);
}
void RootsEditor::resized(){
    const int w=getWidth(),h=getHeight();
    const auto headerLayout=surgery::ui::headerLayout(w);
    title_.setBounds(headerLayout.title.x,headerLayout.title.y,
                     headerLayout.title.width,headerLayout.title.height);
    themeButton_.setBounds(headerLayout.theme.x,headerLayout.theme.y,
                           headerLayout.theme.width,headerLayout.theme.height);
    const int buttonY=headerLayout.rootsY,buttonGap=6,buttonX=18;
    const int buttonW=(w-36-6*buttonGap)/7;
    for(int root=0;root<7;++root)
        rootButtons_[(size_t)root].setBounds(
            buttonX+root*(buttonW+buttonGap),buttonY,buttonW,30);
    graph_.setBounds(16,97,w-32,h-326);
    const int modeTop=h-220;
    const int modeGap=7;
    const int modeWidth=(w-36-3*modeGap)/4;
    for(int i=0;i<4;++i)
        modeButtons_[(size_t)i].setBounds(18+i*(modeWidth+modeGap),modeTop,modeWidth,29);
    const int panelY=h-153;
    selection_.setBounds(32,panelY+4,220,27);
    status_.setBounds(w-470,panelY+4,440,27);
    const int left=24, usable=w-48, cw=usable/7;
    struct Entry{juce::Slider* slider;juce::Label* label;};
    const std::array<Entry,2> controls{{
        {&gain_,&gainLabel_},{&threshold_,&thresholdLabel_}
    }};
    for(int i=0;i<2;++i){
        const int x=left+(i+1)*cw;
        controls[(size_t)i].label->setBounds(x,panelY+24,cw,18);
        controls[(size_t)i].slider->setBounds(x,panelY+40,cw,74);
    }
    on_.setBounds(left,panelY+36,cw-2,25);
    solo_.setBounds(left,panelY+66,cw-2,25);
    mono_.setBounds(left+6*cw,panelY+72,cw-2,25);
    for(int i=0;i<4;++i)
        carveSlotButtons_[(size_t)i].setBounds(255+i*62,panelY+1,56,27);
    const std::array<juce::Slider*,4> cs{
        &carveFrequency_,&carveDepth_,&carveQ_,&carveThreshold_
    };
    const std::array<juce::Label*,4> cl{
        &carveFrequencyLabel_,&carveDepthLabel_,&carveQLabel_,&carveThresholdLabel_
    };
    for(int i=0;i<4;++i){
        const int x=left+(i+1)*cw;
        cl[(size_t)i]->setBounds(x,panelY+24,cw,18);
        cs[(size_t)i]->setBounds(x,panelY+40,cw,74);
    }
    carveOn_.setBounds(left,panelY+38,cw-2,25);
    carveDynamic_.setBounds(left,panelY+69,cw-2,25);
    analyzerLegend_.setBounds(left+12,panelY+40,2*cw-12,38);
    for(int i=0;i<4;++i){
        const int x=left+(i+1)*cw;
        onyxLabels_[(size_t)i].setBounds(x,panelY+24,cw,18);
        onyxSliders_[(size_t)i].setBounds(x,panelY+40,cw,74);
    }
    analyzerDepthLabel_.setBounds(left+2*cw,panelY+27,cw,18);
    analyzerDepth_.setBounds(left+2*cw,panelY+50,cw,28);
    analyzerSlopeLabel_.setBounds(left+3*cw,panelY+23,cw,18);
    analyzerSlope_.setBounds(left+3*cw,panelY+42,cw,75);
    analyzerMemoryLabel_.setBounds(left+4*cw,panelY+23,cw,18);
    analyzerMemory_.setBounds(left+4*cw,panelY+42,cw,75);
    heatToggle_.setBounds(left+6*cw,panelY+41,cw-2,28);
    freezeButton_.setBounds(left+6*cw,panelY+76,cw-2,28);
}
