#include "RootsEditor.h"
#include "RootsSpectrumColours.h"
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
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,87,18);
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
    bands_=values;peaks_=pk;
    heat_.update(values,1.0/30.0,pval(processor_.apvts,"aura_memory"));
    assist_.observe(values);
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
    g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
    g.setColour(juce::Colour::fromRGB(145,172,182));
    g.drawText("CUBE GRID   /   7 ROOT TERRITORIES  /  2 BOUNDARY HANDLES EACH",r.getX(),
               7.0f,r.getWidth(),15.0f,juce::Justification::centredLeft);
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
        g.drawText(juce::String(-12*i),juce::roundToInt(r.getRight()+5),
                   juce::roundToInt(y-8),34,16,juce::Justification::centredLeft);
    }
    // ROOT handle height is constant; vertical axis is exclusively dBFS.
    // ROOT gain is a separate, single control in the bottom panel.
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(juce::Colour::fromRGB(184,197,209));
    g.drawText("ROOT Hz",4,16,52,16,juce::Justification::centredLeft);
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
    for(int i=0;i<PRISMVSTAudioProcessor::labBars;++i){
        // Every FFT bar and each EQ handle share the same 20–20k log axis.
        const float frequency=prism::analysis::LabBands::centreFrequency(i);
        const float x=freqToX(frequency);
        const auto weights=prism::roots::visual::at(frequency,zones);
        float red=0.0f,green=0.0f,blue=0.0f;
        for(int root=0;root<prism::roots::visual::kRoots;++root)
        {
            const auto c=colour(root);
            const float w=(float)weights.weight[(size_t)root];
            red+=w*c.getFloatRed();green+=w*c.getFloatGreen();blue+=w*c.getFloatBlue();
        }
        const auto barColour=weights.strongest>=0
            ? juce::Colour::fromFloatRGBA(red,green,blue,1.0f)
            : juce::Colour::fromRGB(96,112,121);
        const float db=juce::jlimit(-96.0f,0.0f,bands_[(size_t)i]);
        const float peak=juce::jlimit(-96.0f,0.0f,peaks_[(size_t)i]);
        const float y=r.getY()+(0.0f-db)*r.getHeight()/96.0f;
        const float peakY=r.getY()+(0.0f-peak)*r.getHeight()/96.0f;
        if(y<r.getBottom()-1.0f){
            g.setColour(barColour.withAlpha(0.38f));
            g.drawVerticalLine(juce::roundToInt(x),y,r.getBottom());
        }
        if(peakY<r.getBottom()-1.0f){
            g.setColour(barColour.brighter(0.55f).withAlpha(0.62f));
            g.drawHorizontalLine(juce::roundToInt(peakY),x-1.0f,x+1.0f);
        }
    }
    // SONAR FIELD: measured stationary heat/persistence under the spectrum.
    // It is display-only; raw spectral dB values remain untouched above.
    const float sonarTop=r.getY()+r.getHeight()*0.79f;
    g.setColour(juce::Colour(0xff081617).withAlpha(0.68f));
    g.fillRect(r.getX(),sonarTop,r.getWidth(),r.getBottom()-sonarTop);
    for(int i=0;i<PRISMVSTAudioProcessor::labBars;++i) {
        const float x=freqToX(prism::analysis::LabBands::centreFrequency(i));
        const float intensity=heat_.heat((size_t)i);
        const float motion=heat_.transient((size_t)i);
        auto hue=colour(juce::jlimit(0,6,i*7/PRISMVSTAudioProcessor::labBars));
        if(motion>0.16f)hue=hue.interpolatedWith(juce::Colour(0xffffd991),motion);
        const float height=(r.getBottom()-sonarTop)*intensity;
        g.setColour(hue.withAlpha(0.10f+0.77f*intensity));
        g.drawVerticalLine(juce::roundToInt(x),r.getBottom()-height,r.getBottom());
    }
    g.setColour(juce::Colour(0xff8ab5b3));
    g.setFont(juce::Font(juce::FontOptions(9.0f,juce::Font::bold)));
    g.drawText("SONAR  /  ENERGY PERSISTENCE",r.getX()+8,sonarTop+3,210,14,
               juce::Justification::centredLeft);
    // Both boundary handles always live on one horizontal rail.
    g.setColour(juce::Colour::fromRGB(159,182,191).withAlpha(0.14f));
    g.drawHorizontalLine(juce::roundToInt(r.getCentreY()),r.getX(),r.getRight());
    // Two-pass z-order keeps the selected ROOT boundary pair above neighbors.
    const int selectedRoot=selected_/2;
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
        if(i==selected_){
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
            const juce::String detail="ROOT "+juce::String(i/2+1)+"  "
                  +(i%2==0?"A ":"B ")+hzText(hz)+" Hz";
            g.drawText(detail,juce::roundToInt(juce::jlimit(r.getX(),r.getRight()-120,x-48)),
                       juce::roundToInt(juce::jlimit(r.getY(),r.getBottom()-20,y-30)),
                       125,19,juce::Justification::centredLeft);
        }
    }
    // Focused ROOT foreground rail: all four slots remain identifiable.
    // OFF slots are selectable but never represented as active audio filters.
    const auto focusedColor=colour(selectedRoot);
    g.setColour(focusedColor.withAlpha(0.18f));
    g.fillRoundedRectangle(r.getX(),24.0f,171.0f,20.0f,4.0f);
    g.setColour(focusedColor.brighter(0.45f));
    g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
    g.drawFittedText("ROOT "+juce::String(selectedRoot+1)+" / "+
        PRISMVSTAudioProcessor::sectionName(selectedRoot)+"  CARVE",
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
        if(!enabled)continue;
        const float x=freqToX(pval(processor_.apvts,id("hz")));
        const float depth=pval(processor_.apvts,id("depth"));
        const float y=surgery::ui::carveY(r.getCentreY(),depth);
        const auto outline=focusedColor.brighter(0.35f);
        g.setColour(focusedColor.withAlpha(0.42f));
        g.drawVerticalLine(juce::roundToInt(x),y,r.getBottom());
        g.setColour(juce::Colour(0xff0c1820));
        g.fillEllipse(x-10.5f,y-10.5f,21.0f,21.0f);
        g.setColour(outline);
        g.drawEllipse(x-10.5f,y-10.5f,21.0f,21.0f,slot==slot_?2.8f:1.8f);
        g.setColour(slot==slot_?juce::Colours::white:focusedColor);
        g.fillEllipse(x-6.0f,y-6.0f,12.0f,12.0f);
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
        const juce::String text=hzText(freq)+" Hz  /  BAR "
                +juce::String(bands_[(size_t)bar],1)+" dBFS  /  PEAK "
                +juce::String(peaks_[(size_t)bar],1)+" dBFS";
        const float left=juce::jlimit(r.getX(),r.getRight()-274.0f,mouse_.x+12);
        const float top=juce::jlimit(r.getY(),r.getBottom()-29.0f,mouse_.y-39.0f);
        g.setColour(juce::Colour::fromRGB(6,11,18).withAlpha(0.97f));
        g.fillRoundedRectangle(left,top,274,27,4.0f);
        g.setColour(juce::Colour::fromRGB(236,243,247));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
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
        if(pval(processor_.apvts,id("on"))<0.5f)continue;
        const float x=freqToX(pval(processor_.apvts,id("hz")));
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
            draggingCarve_=slot;
            if(onSelectCarve)onSelectCarve(slot);
            const int root=selected_/2;
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
        const float lower=juce::jmax(20.0f,std::min(a,b)*0.79f);
        const float upper=juce::jmin(20000.0f,std::max(a,b)*1.26f);
        change(processor_,id("hz"),juce::jlimit(lower,upper,xToFreq(e.position.x)));
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
    }
    if(dragging_>=0){
        if(auto* p=processor_.apvts.getParameter(
             PRISMVSTAudioProcessor::rootsNodeId(dragging_,"hz")))
            p->endChangeGesture();
    }
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
    if(carve>=0){
        if(onSelectCarve)onSelectCarve(carve);
        processor_.resetCarveNode(selected_/2,carve);
        repaint();
        return;
    }
    if(boundary>=0){
        selectNode(boundary);
        processor_.resetRootBoundary(boundary);
        repaint();
    }
}
void RootsGraph::mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&){
    // Frequency boundaries cannot secretly change Q or gain via the wheel.
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
    menu.addSectionHeader("ROOT "+juce::String(root+1)+" / "+
                          PRISMVSTAudioProcessor::sectionName(root));
    menu.addItem(1,"ROOT ENABLED",true,current("on")>=0.5f);
    const bool solo=pval(processor_.apvts,
        PRISMVSTAudioProcessor::rootsRootSoloId(root))>=0.5f;
    menu.addItem(2,"SOLO ENTIRE ROOT / OFF",true,solo);
    menu.addSeparator();
    menu.addSectionHeader("ROOT STEREO / EQ DOMAIN");
    menu.addItem(10,"STEREO",true,juce::roundToInt(current("domain"))==0);
    menu.addItem(11,"MID",true,juce::roundToInt(current("domain"))==1);
    menu.addItem(12,"SIDE",true,juce::roundToInt(current("domain"))==2);
    menu.addSeparator();
    menu.addSectionHeader("ROOT COMPRESSOR CHANNEL");
    static const char* dn[]{"STEREO LINK","DUAL MID/SIDE","MID ONLY","SIDE ONLY"};
    for(int i=0;i<4;++i)
        menu.addItem(30+i,dn[i],true,juce::roundToInt(current("detector"))==i);
    menu.addSeparator();
    menu.addItem(40,"ROOT SPACE: MONO (0)");
    menu.addItem(41,"ROOT SPACE: NEUTRAL (50)");
    menu.addItem(42,"ROOT SPACE: WIDE (100)");
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
            if(id>=30&&id<=33)set("detector",(float)(id-30));
            if(id>=40&&id<=42)set("space",(float)((id-40)*50));
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
        b.setButtonText("ROOT "+juce::String(root+1)+"  "+
            PRISMVSTAudioProcessor::sectionName(root));
        b.onClick=[this,root](){graph_.selectNode(root*2);};
        b.onContext=[this,root](){graph_.showRootMenu(root);};
        b.setColour(juce::TextButton::textColourOffId,
                    juce::Colour::fromRGB(192,213,220));
        addAndMakeVisible(b);
    }
    for(auto* label:{&title_,&selection_,&status_,&gainLabel_,&bandSpaceLabel_,
                     &spaceLabel_,&thresholdLabel_,&carveFrequencyLabel_,
                     &carveDepthLabel_,&carveQLabel_,&carveThresholdLabel_}){
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId,juce::Colour::fromRGB(195,216,223));
    }
    title_.setText("SURGERY EQ  /  v1.0",juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold)));
    for(auto* s:{&gain_,&bandSpace_,&space_,&threshold_}){
        addAndMakeVisible(*s);rotary(*s,"",s==&gain_?0.0:
             s==&threshold_?-18.0:50.0);
        s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour::fromRGB(143,192,204));
        s->setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour::fromRGB(38,55,65));
        s->setColour(juce::Slider::textBoxTextColourId,juce::Colour::fromRGB(239,243,245));
        s->setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour::fromRGB(13,19,25));
        s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colour::fromRGB(38,55,65));
    }
    gainLabel_.setText("ROOT GAIN (dB)",juce::dontSendNotification);
    gain_.setTooltip("Entire ROOT frequency range gain: -60 to +60 dB");
    bandSpaceLabel_.setText("ROOT SPACE",juce::dontSendNotification);
    spaceLabel_.setText("STEREO SPACE",juce::dontSendNotification);
    bandSpace_.setTooltip("Selected ROOT only: 0 mono, 50 neutral, 100 wide");
    space_.setTooltip("Global stereo: 0 mono, 50 neutral, 100 widened if phase permits");
    thresholdLabel_.setText("COMP THRESHOLD",juce::dontSendNotification);
    for(auto* b:{&mono_,&on_,&solo_}){
        b->setClickingTogglesState(true);
        b->setColour(juce::TextButton::buttonColourId,juce::Colour::fromRGB(24,38,47));
        b->setColour(juce::TextButton::buttonOnColourId,juce::Colour::fromRGB(43,153,166));
        b->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
        b->setColour(juce::TextButton::textColourOnId,juce::Colours::white);
        addAndMakeVisible(*b);
    }
    static constexpr const char* labels[]{"ROOT","CARVE","SPACE","DYN","ANALYZE"};
    for(int i=0;i<5;++i){
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
    carveFrequencyLabel_.setText("FREQUENCY Hz",juce::dontSendNotification);
    carveDepthLabel_.setText("CUT DEPTH dB",juce::dontSendNotification);
    carveQLabel_.setText("Q / PRECISION",juce::dontSendNotification);
    carveThresholdLabel_.setText("TRIGGER dBFS",juce::dontSendNotification);
    for(auto* b:{&carveOn_,&carveDynamic_}){
        b->setClickingTogglesState(true);
        addAndMakeVisible(*b);
    }
    addAndMakeVisible(assistApply_);
    assistApply_.onClick=[this]{
        const auto suggestion=graph_.finding();
        if(!suggestion.ready)return;
        const int root=node_/2;
        const float low=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2,"hz"));
        const float high=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2+1,"hz"));
        if(suggestion.hz<std::min(low,high)||suggestion.hz>std::max(low,high))return;
        // Explicit user-approved parameter edit, no autonomous audio changes.
        int target=0;
        for(int slot=0;slot<4;++slot)
            if(pval(processor_.apvts,PRISMVSTAudioProcessor::carveId(root,slot,"on"))<0.5f){
                target=slot;break;
            }
        carveSlot_=target;
        const auto id=[root,target](const char* s){
            return PRISMVSTAudioProcessor::carveId(root,target,s);
        };
        change(processor_,id("hz"),suggestion.hz);
        change(processor_,id("depth"),suggestion.suggestedDepthDb);
        change(processor_,id("q"),5.0f);
        change(processor_,id("on"),1.0f);
        bindCarve();
        setMode(1);
    };
    assistApply_.setTooltip("Manually approve a SONAR-evidenced corrective CARVE on selected ROOT");
    addAndMakeVisible(dynButton_);
    dynButton_.onClick=[this]{
        const auto id=PRISMVSTAudioProcessor::rootsRootControlId(node_/2,"comp");
        const bool active=pval(processor_.apvts,id)>=0.5f;
        change(processor_,id,active?0.0f:4.0f);
        timerCallback();
    };
    dynButton_.setTooltip("Enable surgical fast-response ROOT compression (internal FET-style response)");
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
    monoA_=std::make_unique<ButtonA>(processor_.apvts,"roots_mono",mono_);
    spaceA_=std::make_unique<SliderA>(processor_.apvts,"roots_space",space_);
    bindNode(0);
    setMode(0);
    startTimerHz(30);
}
RootsEditor::~RootsEditor(){stopTimer();setLookAndFeel(nullptr);}
void RootsEditor::bindNode(int n){
    node_=juce::jlimit(0,prism::roots::v2::kNodes-1,n);
    gainA_.reset();bandSpaceA_.reset();thresholdA_.reset();
    onA_.reset();soloA_.reset();
    const auto rid=[this](const char* suffix){
        return PRISMVSTAudioProcessor::rootsRootControlId(node_/2,suffix);
    };
    gainA_=std::make_unique<SliderA>(processor_.apvts,
       PRISMVSTAudioProcessor::rootsRootGainId(node_/2),gain_);
    bandSpaceA_=std::make_unique<SliderA>(processor_.apvts,rid("space"),bandSpace_);
    thresholdA_=std::make_unique<SliderA>(processor_.apvts,rid("threshold"),threshold_);
    onA_=std::make_unique<ButtonA>(processor_.apvts,rid("on"),on_);
    soloA_=std::make_unique<ButtonA>(processor_.apvts,
        PRISMVSTAudioProcessor::rootsRootSoloId(node_/2),solo_);
    selection_.setText("ROOT "+juce::String(node_/2+1)+" / "+
        PRISMVSTAudioProcessor::sectionName(node_/2)+
        "  BOUNDARY "+juce::String(node_%2+1),juce::dontSendNotification);
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
    carveFrequency_.setDoubleClickReturnValue(true,
        std::sqrt((double)juce::jmax(20.0f,a)*(double)juce::jmax(20.0f,b)));
    const auto id=[root,slot](const char* s){
        return PRISMVSTAudioProcessor::carveId(root,slot,s);
    };
    carveFrequencyA_=std::make_unique<SliderA>(processor_.apvts,id("hz"),carveFrequency_);
    carveDepthA_=std::make_unique<SliderA>(processor_.apvts,id("depth"),carveDepth_);
    carveQA_=std::make_unique<SliderA>(processor_.apvts,id("q"),carveQ_);
    carveThresholdA_=std::make_unique<SliderA>(processor_.apvts,id("threshold"),carveThreshold_);
    carveOnA_=std::make_unique<ButtonA>(processor_.apvts,id("on"),carveOn_);
    carveDynamicA_=std::make_unique<ButtonA>(processor_.apvts,id("dynamic"),carveDynamic_);
    for(int i=0;i<4;++i){
        carveSlotButtons_[(size_t)i].setToggleState(i==slot,juce::dontSendNotification);
        carveSlotButtons_[(size_t)i].setColour(juce::TextButton::buttonOnColourId,
            territory(root).withAlpha(0.66f));
    }
    graph_.setPresentation(themeIndex_,mode_,carveSlot_);
}
void RootsEditor::setMode(int next){
    mode_=juce::jlimit(0,4,next);
    for(int i=0;i<5;++i){
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
    assistApply_.setVisible(mode_==4);
    gain_.setVisible(mode_==0);gainLabel_.setVisible(mode_==0);
    bandSpace_.setVisible(mode_==0||mode_==2);
    bandSpaceLabel_.setVisible(mode_==0||mode_==2);
    threshold_.setVisible(mode_==0||mode_==3);
    thresholdLabel_.setVisible(mode_==0||mode_==3);
    space_.setVisible(mode_==0||mode_==2);
    spaceLabel_.setVisible(mode_==0||mode_==2);
    dynButton_.setVisible(mode_==0||mode_==3);
    on_.setVisible(mode_!=4);solo_.setVisible(mode_!=4);
    mono_.setVisible(mode_==0||mode_==2);
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
    dynButton_.setButtonText(comp==0?"DYN OFF":"DYN ON");
    dynButton_.setColour(juce::TextButton::buttonColourId,
                         comp==0?juce::Colour(0xff1d2c36):juce::Colour(0xff187a83));
    thresholdLabel_.setText(comp==0?"THRESHOLD (COMP OFF)":"COMP THRESHOLD",
                            juce::dontSendNotification);
    updateRootButtons();
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
    if(mode_==4){
        const auto finding=graph_.finding();
        const int root=node_/2;
        const float low=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2,"hz"));
        const float high=pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(root*2+1,"hz"));
        assistApply_.setEnabled(finding.ready &&
            finding.hz>=std::min(low,high) && finding.hz<=std::max(low,high));
        if(finding.ready)
            status_.setText("ASSIST  "+hzText(finding.hz)+" Hz  /  "+
                juce::String(finding.prominenceDb,1)+" dB PROMINENCE  /  VERIFY BY EAR",
                juce::dontSendNotification);
        else status_.setText("ASSIST  /  NO STABLE NARROWBAND TARGET",juce::dontSendNotification);
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
    for(int i=1;i<7;++i){
        const float x=24+(w-48)*(float)i/7.0f;
        g.setColour((lab?juce::Colour(0xffd7e1e4):juce::Colour(0xff334957)).withAlpha(.55f));
        g.drawVerticalLine(juce::roundToInt(x),h-113,h-49);
    }
    const auto label=lab?juce::Colour(0xff24404a):juce::Colour(0xff93b1bd);
    g.setColour(label);
    g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
    const auto headerLayout=surgery::ui::headerLayout(getWidth());
    g.drawFittedText("SPIRAL ETHER TECH  /  SONAR ROOTS PROCESSOR",
               headerLayout.brand.x,headerLayout.brand.y,
               headerLayout.brand.width,headerLayout.brand.height,
               juce::Justification::centredRight,1);
    g.setColour(label.withAlpha(.85f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("SURGERY ENGINE / ROOTS / CARVE / SPACE / DYN / ANALYZE",
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
    const int modeWidth=(w-36-4*modeGap)/5;
    for(int i=0;i<5;++i)
        modeButtons_[(size_t)i].setBounds(18+i*(modeWidth+modeGap),modeTop,modeWidth,29);
    const int panelY=h-153;
    selection_.setBounds(31,panelY+2,210,26);
    status_.setBounds(w-450,panelY+2,425,27);
    const int left=24, usable=w-48, cw=usable/7;
    struct Entry{juce::Slider* slider;juce::Label* label;};
    const std::array<Entry,4> controls{{
        {&gain_,&gainLabel_},{&bandSpace_,&bandSpaceLabel_},
        {&threshold_,&thresholdLabel_},{&space_,&spaceLabel_}
    }};
    for(int i=0;i<4;++i){
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
    assistApply_.setBounds(260,panelY+51,220,34);
    dynButton_.setBounds(left+6*cw,panelY+36,cw-2,26);
}
