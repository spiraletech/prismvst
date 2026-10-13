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
void rotary(juce::Slider& s,juce::String suffix) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,87,18);
    s.setTextValueSuffix(suffix);
    s.setDoubleClickReturnValue(true,0.0);
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
float RootsGraph::gainToY(float db)const {
    auto r=area();
    return r.getCentreY()-juce::jlimit(-60.0f,60.0f,db)
                              *r.getHeight()/120.0f;
}
float RootsGraph::yToGain(float y)const {
    auto r=area();
    return juce::jlimit(-60.0f,60.0f,
              (r.getCentreY()-y)*120.0f/r.getHeight());
}
float RootsGraph::param(int i,const char* suffix)const {
    return pval(processor_.apvts,PRISMVSTAudioProcessor::rootsNodeId(i,suffix));
}
void RootsGraph::write(int i,const char* suffix,float value) {
    change(processor_,PRISMVSTAudioProcessor::rootsNodeId(i,suffix),value);
}
int RootsGraph::hitNode(juce::Point<float> p)const{
    int hit=-1;float best=18.0f;
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        auto x=freqToX(param(i,"hz")),y=gainToY(param(i,"gain_db"));
        const float distance=p.getDistanceFrom({x,y});
        if(distance<best){best=distance;hit=i;}
    }
    return hit;
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
    g.drawText("CUBE GRID   /   14 INTERSECTING ROOTS",r.getX(),
               7.0f,r.getWidth(),15.0f,juce::Justification::centredLeft);
    // Frequency and calibrated dBFS dimensions.
    static constexpr std::array<float,14> freqs{
        1,2,5,10,20,50,100,250,500,1000,2500,5000,10000,20000
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
        g.drawText(juce::String(60-15*i),5,juce::roundToInt(y-8),
                   37,16,juce::Justification::centredRight);
        g.drawText(juce::String(-12*i),juce::roundToInt(r.getRight()+5),
                   juce::roundToInt(y-8),34,16,juce::Justification::centredLeft);
    }
    // Display dBFS always uses the established 0..-96 view; EQ gain uses
    // independent -60..60 vertical mapping, explicitly labeled.
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(juce::Colour::fromRGB(184,197,209));
    g.drawText("EQ dB",4,16,52,16,juce::Justification::centredLeft);
    g.drawText("dBFS",juce::roundToInt(r.getRight()),16,44,16,juce::Justification::centredRight);
    std::array<prism::roots::visual::Node,prism::roots::visual::kNodes> zones{};
    for(int node=0;node<prism::roots::visual::kNodes;++node)
    {
        zones[(size_t)node].hz=param(node,"hz");
        zones[(size_t)node].q=param(node,"q");
        zones[(size_t)node].enabled=param(node,"on")>=0.5f;
    }
    for(int i=0;i<PRISMVSTAudioProcessor::labBars;++i){
        // The analyzer resolves 20–20k. Its x coordinate is aligned with
        // the full 1Hz–20k ROOTS graph, leaving 1–20Hz deliberately blank.
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
    // One gain-stage spectral graph, no separate transfer maps.
    for(int i=0;i<prism::roots::v2::kNodes;++i){
        const float hz=param(i,"hz"),gain=param(i,"gain_db");
        const float x=freqToX(hz),y=gainToY(gain);
        const bool enabled=param(i,"on")>=0.5f;
        const bool solo=param(i,"solo")>=0.5f;
        const auto col=territory(i/2);
        // Tree roots: paired nodes connect back to each frequency territory.
        const float groupX=freqToX((prism::roots::v2::Engine::defaults[(size_t)((i/2)*2)]
                                  +prism::roots::v2::Engine::defaults[(size_t)((i/2)*2+1)])/2.0f);
        g.setColour(col.withAlpha(enabled?0.23f:0.06f));
        g.drawLine(groupX,r.getBottom(),x,y,0.65f);
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
            const juce::String detail=juce::String(i+1)+"  "+hzText(hz)+" Hz  "
                                    +juce::String(gain,1)+" dB";
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
        const juce::String text=freq<20.0f
            ?hzText(freq)+" Hz  /  SUB-20 Hz: NO CALIBRATED FFT DATA"
            :hzText(freq)+" Hz  /  BAR "
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
        for(auto suffix:{"hz","gain_db"})
            if(auto* p=processor_.apvts.getParameter(PRISMVSTAudioProcessor::rootsNodeId(dragging_,suffix)))
                p->beginChangeGesture();
    }
}
void RootsGraph::mouseDrag(const juce::MouseEvent& e){
    if(dragging_<0)return;
    const float newHz=xToFreq(e.position.x);
    const float newGain=yToGain(e.position.y);
    write(dragging_,"hz",newHz);
    write(dragging_,"gain_db",newGain);
    repaint();
}
void RootsGraph::mouseUp(const juce::MouseEvent&){
    if(dragging_>=0){
        for(auto suffix:{"hz","gain_db"})
            if(auto* p=processor_.apvts.getParameter(PRISMVSTAudioProcessor::rootsNodeId(dragging_,suffix)))
                p->endChangeGesture();
    }
    dragging_=-1;
}
void RootsGraph::mouseMove(const juce::MouseEvent& e){mouse_=e.position;hover_=true;repaint();}
void RootsGraph::mouseExit(const juce::MouseEvent&){hover_=false;repaint();}
void RootsGraph::mouseDoubleClick(const juce::MouseEvent& e){
    const int hit=hitNode(e.position);
    if(hit>=0){selectNode(hit);write(hit,"gain_db",0.0f);repaint();}
}
void RootsGraph::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w){
    if(!area().contains(e.position))return;
    const float q=param(selected_,"q");
    write(selected_,"q",juce::jlimit(0.15f,24.0f,q*(1.0f+w.deltaY*0.15f)));
}
void RootsGraph::popup(){
    juce::PopupMenu menu;
    menu.addSectionHeader("ROOT "+juce::String(selected_+1)+"  /  "+PRISMVSTAudioProcessor::sectionName(selected_/2));
    menu.addItem(1,"ENABLED",true,param(selected_,"on")>=0.5f);
    menu.addItem(2,"SOLO — EXCLUSIVE / OFF",true,param(selected_,"solo")>=0.5f);
    menu.addSeparator();
    menu.addSectionHeader("EQ CHANNEL");
    menu.addItem(10,"STEREO",true,param(selected_,"domain")<0.5f);
    menu.addItem(11,"MID",true,juce::roundToInt(param(selected_,"domain"))==1);
    menu.addItem(12,"SIDE",true,juce::roundToInt(param(selected_,"domain"))==2);
    menu.addSeparator();
    menu.addSectionHeader("DYNAMICS PROFILE (original models; no licensed emulation)");
    static const char* names[]{"OFF","OPTICAL 3:1","OPTICAL 4:1","CLB1 TUBE 3:1","FET 4:1"};
    for(int i=0;i<5;++i)menu.addItem(20+i,names[i],true,
                               juce::roundToInt(param(selected_,"comp"))==i);
    menu.addSeparator();
    menu.addSectionHeader("COMPRESSOR STEREO DOMAIN");
    static const char* dn[]{"STEREO LINK","DUAL MID/SIDE","MID ONLY","SIDE ONLY"};
    for(int i=0;i<4;++i)menu.addItem(30+i,dn[i],true,
                          juce::roundToInt(param(selected_,"detector"))==i);
    menu.addSeparator();
    menu.addItem(40,"BAND MONO (SPACE 0)");
    menu.addItem(41,"BAND NEUTRAL (SPACE 50)");
    menu.addItem(42,"BAND WIDE (SPACE 100)");
    const juce::Component::SafePointer<RootsGraph> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [safe](int id){
            if(safe==nullptr||id<=0)return;
            auto& graph=*safe;
            const int n=graph.selected_;
            if(id==1)graph.write(n,"on",graph.param(n,"on")>=0.5f?0.0f:1.0f);
            if(id==2)graph.write(n,"solo",graph.param(n,"solo")>=0.5f?0.0f:1.0f);
            if(id>=10&&id<=12)graph.write(n,"domain",(float)(id-10));
            if(id>=20&&id<=24)graph.write(n,"comp",(float)(id-20));
            if(id>=30&&id<=33)graph.write(n,"detector",(float)(id-30));
            if(id>=40&&id<=42)graph.write(n,"space",(float)((id-40)*50));
            graph.repaint();
        });
}

RootsEditor::RootsEditor(PRISMVSTAudioProcessor& p)
: juce::AudioProcessorEditor(&p),processor_(p),graph_(p) {
    setSize(1120,755);setResizable(true,true);
    setResizeLimits(840,610,1800,1100);
    addAndMakeVisible(graph_);
    graph_.onSelectNode=[this](int n){bindNode(n);};
    for(auto* label:{&title_,&selection_,&status_,&gainLabel_,&qLabel_,&bandSpaceLabel_,
                     &spaceLabel_,&thresholdLabel_}){
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId,juce::Colour::fromRGB(195,216,223));
    }
    title_.setText("PRISM  /  ROOTS PROTO 001",juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold)));
    for(auto* s:{&gain_,&q_,&bandSpace_,&space_,&threshold_}){
        addAndMakeVisible(*s);rotary(*s,"");
        s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour::fromRGB(143,192,204));
        s->setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour::fromRGB(38,55,65));
        s->setColour(juce::Slider::textBoxTextColourId,juce::Colour::fromRGB(239,243,245));
        s->setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour::fromRGB(13,19,25));
        s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colour::fromRGB(38,55,65));
    }
    gainLabel_.setText("GAIN  ±60 dB",juce::dontSendNotification);
    qLabel_.setText("SURGICAL Q",juce::dontSendNotification);
    bandSpaceLabel_.setText("BAND SPACE",juce::dontSendNotification);
    spaceLabel_.setText("MASTER SPACE",juce::dontSendNotification);
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
    gainA_.reset();qA_.reset();bandSpaceA_.reset();thresholdA_.reset();
    onA_.reset();soloA_.reset();
    const auto id=[this](const char* suffix){
        return PRISMVSTAudioProcessor::rootsNodeId(node_,suffix);
    };
    gainA_=std::make_unique<SliderA>(processor_.apvts,id("gain_db"),gain_);
    qA_=std::make_unique<SliderA>(processor_.apvts,id("q"),q_);
    bandSpaceA_=std::make_unique<SliderA>(processor_.apvts,id("space"),bandSpace_);
    thresholdA_=std::make_unique<SliderA>(processor_.apvts,id("threshold"),threshold_);
    onA_=std::make_unique<ButtonA>(processor_.apvts,id("on"),on_);
    soloA_=std::make_unique<ButtonA>(processor_.apvts,id("solo"),solo_);
    selection_.setText("ROOT "+juce::String(node_+1)+" / "+
        PRISMVSTAudioProcessor::sectionName(node_/2),juce::dontSendNotification);
}
void RootsEditor::timerCallback(){
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
    g.drawText("L-DRAG NODE: Hz + GAIN   /   WHEEL: Q   /   RIGHT-CLICK: COMP + M/S + SOLO   /   DOUBLE-CLICK: RESET GAIN",
               24,getHeight()-28,getWidth()-48,18,juce::Justification::centredLeft);
}
void RootsEditor::resized(){
    const int w=getWidth(),h=getHeight();
    title_.setBounds(35,13,330,37);
    graph_.setBounds(16,58,w-32,h-238);
    const int panelY=h-153;
    selection_.setBounds(31,panelY+2,210,26);
    status_.setBounds(w-450,panelY+2,425,27);
    const int left=24, usable=w-48, cw=usable/7;
    struct Entry{juce::Slider* slider;juce::Label* label;};
    const std::array<Entry,5> controls{{
        {&gain_,&gainLabel_},{&q_,&qLabel_},{&bandSpace_,&bandSpaceLabel_},
        {&threshold_,&thresholdLabel_},{&space_,&spaceLabel_}
    }};
    for(int i=0;i<5;++i){
        const int x=left+(i+1)*cw;
        controls[(size_t)i].label->setBounds(x,panelY+24,cw,18);
        controls[(size_t)i].slider->setBounds(x,panelY+40,cw,74);
    }
    on_.setBounds(left,panelY+36,cw-2,25);
    solo_.setBounds(left,panelY+66,cw-2,25);
    mono_.setBounds(left+6*cw,panelY+72,cw-2,25);
}
