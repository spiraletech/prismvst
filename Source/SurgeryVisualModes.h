#pragma once
#include "SurgerySynesthesia.h"
#include "SurgeryWeatherField.h"
namespace surgery::visual {
inline weather::Pixel style(int mode,const weather::Meaning& f,float phase,float u,float v,weather::Pixel storm) noexcept {
    if(mode==1)return storm;
    const auto base=synesthesia::RGB{11/255.f,17/255.f,24/255.f};
    auto colour=synesthesia::read(f,u,.12f);float amount=0;
    const float local=synesthesia::localEnergy(f,u),level=f.level;
    switch(mode){
        case 2: amount=.23f*level*(.25f+.75f*local)*std::pow(1-v,1.4f);break;
        case 3: colour=synesthesia::palette(.36f+.17f*f.centroid);amount=.20f*level*(.35f+.65f*f.bass)*std::exp(-((v-.78f)*(v-.78f)/.08f))*(.7f+.3f*std::sin(u*9-phase));break;
        case 4: {const float wave=.73f+.09f*std::sin(u*12-phase*2);const float d=(v-wave)/(.04f+.08f*f.bass);amount=.25f*level*(.25f+.75f*std::max(f.flux,f.crest))/(1+d*d);break;}
        case 5: {const float x=(u-(.25f+.5f*f.centroid))/(.18f+.3f*f.width),y=(v-.65f)/(.16f+.14f*f.bass);colour=synesthesia::read(f,f.centroid,.08f);amount=.24f*level*std::exp(-x*x-y*y);break;}
        case 6: {const float d=(u-f.centroid)/.055f;amount=.19f*level*local/(1+d*d)*(1-v);break;}
        case 7: amount=.08f*level*local*(1-v);break;
        default: break;
    }
    amount=weather::unit(amount);
    auto encode=[&](float b,float c){return (std::uint8_t)std::round(255*weather::unit(b+(c-b)*amount));};
    return {encode(base.r,colour.r),encode(base.g,colour.g),encode(base.b,colour.b)};
}
} // namespace surgery::visual
