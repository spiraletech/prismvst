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
    return getLocalBounds().toFloat().reduced(46.0f,34.0f);
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
juce::Colour RootsGraph::colour(int territory) {return ::territory(territory);}
void RootsGraph::updateAnalyzer(
    const std::array<float,PRISMVSTAudioProcessor::labBars>& values,
    const std::array<float,PRISMVSTAudioProcessor::labBars>& pk){
    bands_=values;peaks_=pk;repaint();
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
    juce::ColourGradient bg(juce::Colour::fromRGB(14,21,28),full.getX(),full.getY(),
                            juce::Colour::fromRGB(3,7,12),full.getX(),full.getBottom(),false);
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
        g.setColour(territory(root).withAlpha(root==selected_/2?0.075f:0.025f));
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
            const auto c=territory(root);
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
    // Both boundary handles always live on one horizontal rail.
    g.setColour(juce::Colour::fromRGB(159,182,191).withAlpha(0.14f));
    g.drawHorizontalLine(juce::roundToInt(r.getCentreY()),r.getX(),r.getRight());
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        const float hz=param(i,"hz");
        const float x=freqToX(hz),y=r.getCentreY();
        const bool enabled=pval(processor_.apvts,
            PRISMVSTAudioProcessor::rootsRootControlId(i/2,"on"))>=0.5f;
        const bool solo=pval(processor_.apvts,
             PRISMVSTAudioProcessor::rootsRootSoloId(i/2))>=0.5f;
        const auto col=territory(i/2);
        // Vertical guides represent frequency boundaries, not gain movements.
        g.setColour(col.withAlpha(enabled?0.23f:0.06f));
        g.drawVerticalLine(juce::roundToInt(x),r.getY(),r.getBottom());
        g.setColour(col.withAlpha(enabled?0.10f:0.025f));
        g.fillEllipse(x-10,y-10,20,20);
        g.setColour(enabled?col:juce::Colour::fromRGB(79,90,97));
        g.fillEllipse(x-5.5f,y-5.5f,11,11);
        if(i==selected_||solo){
            g.setColour(solo?juce::Colours::white:col.brighter(0.65f));
            g.drawEllipse(x-9,y-9,18,18,i==selected_?1.9f:1.2f);
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
void RootsGraph::mouseDown(const juce::MouseEvent& e){
    mouse_=e.position;hover_=true;
    const int hit=hitNode(e.position);
    if(hit>=0)selectNode(hit);
    if(e.mods.isPopupMenu()){popup();return;}
    dragging_=hit;
    if(dragging_>=0){
        if(auto* p=processor_.apvts.getParameter(
             PRISMVSTAudioProcessor::rootsNodeId(dragging_,"hz")))
            p->beginChangeGesture();
    }
}
void RootsGraph::mouseDrag(const juce::MouseEvent& e){
    if(dragging_<0)return;
    const float newHz=xToFreq(e.position.x);
    // Deliberately ignore e.position.y: frequency handles have no gain axis.
    write(dragging_,"hz",newHz);
    repaint();
}
void RootsGraph::mouseUp(const juce::MouseEvent&){
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
    const int hit=hitNode(e.position);
    if(hit>=0){selectNode(hit);repaint();} // no per-handle gain reset
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
    menu.addSectionHeader("ROOT COMPRESSOR MODEL");
    static const char* names[]{"OFF","OPTICAL 3:1","OPTICAL 4:1",
                               "CLB1 TUBE 3:1","FET 4:1"};
    for(int i=0;i<5;++i)
        menu.addItem(20+i,names[i],true,juce::roundToInt(current("comp"))==i);
    menu.addSeparator();
    menu.addSectionHeader("ROOT COMPRESSOR CHANNEL");
    static const char* dn[]{"STEREO LINK","DUAL MID/SIDE","MID ONLY","SIDE ONLY"};
    for(int i=0;i<4;++i)
        menu.addItem(30+i,dn[i],true,juce::roundToInt(current("detector"))==i);
    menu.addSeparator();
    menu.addItem(40,"ROOT SPACE: MONO (0)");
    menu.addItem(41,"ROOT SPACE: NEUTRAL (50)");
    menu.addItem(42,"ROOT SPACE: WIDE (100)");
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
            if(id>=20&&id<=24)set("comp",(float)(id-20));
            if(id>=30&&id<=33)set("detector",(float)(id-30));
            if(id>=40&&id<=42)set("space",(float)((id-40)*50));
            graph.repaint();
        });
}
RootsEditor::RootsEditor(PRISMVSTAudioProcessor& p)
: juce::AudioProcessorEditor(&p),processor_(p),graph_(p) {
    setSize(1120,755);setResizable(true,true);
    setResizeLimits(840,610,1800,1100);
    addAndMakeVisible(graph_);
    graph_.onSelectNode=[this](int n){bindNode(n);};
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
                     &spaceLabel_,&thresholdLabel_}){
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId,juce::Colour::fromRGB(195,216,223));
    }
    title_.setText("PRISM  /  ROOTS PROTO 001",juce::dontSendNotification);
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
    for(auto* b:{&mono_,&on_,&solo_})addAndMakeVisible(*b);
    monoA_=std::make_unique<ButtonA>(processor_.apvts,"roots_mono",mono_);
    spaceA_=std::make_unique<SliderA>(processor_.apvts,"roots_space",space_);
    bindNode(0);
    startTimerHz(30);
}
RootsEditor::~RootsEditor(){stopTimer();}
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
    updateRootButtons();
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
    // Threshold has no audio effect unless an actual compressor model is selected.
    const int comp=juce::roundToInt(pval(processor_.apvts,
         PRISMVSTAudioProcessor::rootsRootControlId(node_/2,"comp")));
    threshold_.setEnabled(comp!=0);
    thresholdLabel_.setText(comp==0?"THRESHOLD (COMP OFF)":"COMP THRESHOLD",
                            juce::dontSendNotification);
    updateRootButtons();
    std::array<float,PRISMVSTAudioProcessor::labBars> bars{},peaks{};
    processor_.copyPrismLabBars(bars,peaks);
    graph_.updateAnalyzer(bars,peaks);
    const auto state=processor_.getRootsMeter();
    const juce::String status=
        "IN "+juce::String(processor_.getInputPeakDb(),1)+" dBFS  /  OUT "+
        juce::String(processor_.getPeakDb(),1)+" dBFS  /  CORR "+
        juce::String(state.correlation,2)+"  /  GR "+
        juce::String(state.largestGrDb,1)+" dB";
    status_.setText(status,juce::dontSendNotification);
}
void RootsEditor::paint(juce::Graphics& g){
    const auto bounds=getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour::fromRGB(28,33,39),0,0,
                             juce::Colour::fromRGB(8,12,16),0,bounds.getBottom(),false);
    g.setGradientFill(bg);g.fillRect(bounds);
    g.setColour(juce::Colour::fromRGB(54,69,80));
    g.drawRoundedRectangle(bounds.reduced(5),11.0f,1.5f);
    g.setColour(juce::Colour::fromRGB(122,153,166));
    g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
    g.drawText("SONY DIGITAL-DECK / CINEMA STEREO INSPIRATION  •  INDEPENDENT DESIGN",
          370,24,getWidth()-390,18,juce::Justification::centredRight);
    const int bottomY=getHeight()-158;
    g.setColour(juce::Colour::fromRGB(19,29,37));
    g.fillRoundedRectangle(15.0f,(float)bottomY,(float)getWidth()-30.0f,123.0f,9.0f);
    g.setColour(juce::Colour::fromRGB(57,75,86));
    g.drawRoundedRectangle(15.0f,(float)bottomY,(float)getWidth()-30.0f,123.0f,9.0f,1.0f);
    g.setColour(juce::Colour::fromRGB(142,162,172));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("SELECT ROOT ABOVE  /  DRAG Hz BOUNDARIES  /  RIGHT-CLICK ROOT: SPACE, STEREO & COMP",
               24,getHeight()-28,getWidth()-48,18,juce::Justification::centredLeft);
}
void RootsEditor::resized(){
    const int w=getWidth(),h=getHeight();
    title_.setBounds(35,13,330,37);
    const int buttonY=59,buttonGap=6,buttonX=18;
    const int buttonW=(w-36-6*buttonGap)/7;
    for(int root=0;root<7;++root)
        rootButtons_[(size_t)root].setBounds(
            buttonX+root*(buttonW+buttonGap),buttonY,buttonW,30);
    graph_.setBounds(16,97,w-32,h-277);
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
}
