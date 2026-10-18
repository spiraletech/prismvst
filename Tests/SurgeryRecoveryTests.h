#pragma once
#include "../Source/RootsEditor.h"
inline bool verifySonarRecovery(){
    auto processor=std::make_unique<PRISMVSTAudioProcessor>();processor->prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());editor->setSize(1120,755);
    RootsGraph* graph=nullptr;for(auto* child:editor->getChildren())if(auto* g=dynamic_cast<RootsGraph*>(child))graph=g;
    if(!graph)return false;
    juce::AudioBuffer<float> block(2,512);juce::MidiBuffer midi;juce::Random random(1015);
    for(int n=0;n<60;++n){for(int i=0;i<512;++i){const float v=(random.nextFloat()*2-1)*.3f;block.setSample(0,i,v);block.setSample(1,i,v);}processor->processBlock(block,midi);}
    juce::MessageManager::getInstance()->runDispatchLoopUntil(150);graph->setFrozen(true);
    std::array<float,PRISMVSTAudioProcessor::labBars> measured{},peaks{};processor->copyPrismLabBars(measured,peaks);
    std::cout<<"RECOVERY: worker maximum "<<*std::max_element(measured.begin(),measured.end())<<"\n";
    const auto metrics=graph->spectralMetrics();if(!metrics.signalPresent){std::cerr<<"Recovery: analyzer pipeline has no signal\n";return false;}
    auto* parameter=processor->apvts.getParameter("aura_color");
    parameter->setValueNotifyingHost(1);auto on=graph->createComponentSnapshot(graph->getLocalBounds());
    parameter->setValueNotifyingHost(0);auto off=graph->createComponentSnapshot(graph->getLocalBounds());
    int visible=0;double contrast=0;int count=0;
    for(int y=70;y<graph->getHeight()-60;++y)for(int x=65;x<graph->getWidth()-65;++x){
        auto a=on.getPixelAt(x,y),b=off.getPixelAt(x,y);
        const int difference=std::max({std::abs(a.getRed()-b.getRed()),std::abs(a.getGreen()-b.getGreen()),std::abs(a.getBlue()-b.getBlue())});
        if(difference>=5)++visible;contrast+=difference;++count;
    }
    std::cout<<"RECOVERY: measured peak "<<metrics.loudestDb<<" dBFS; heat pixels >=5/255 contrast "<<visible<<"; mean contrast "<<contrast/count<<"\n";
    if(visible<10000||contrast/count<2){std::cerr<<"RECOVERY FAIL: enabled heat field is not visibly distinct despite measured input\n";return false;}
    return true;
}
