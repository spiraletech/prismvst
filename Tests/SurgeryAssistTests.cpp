#include "../Source/SurgeryAssist.h"
#include <array>
#include <iostream>
int main(){
    surgery::AssistScanner<256> scanner;
    std::array<float,256> bins{};bins.fill(-65.0f);
    scanner.observe(bins);
    if(scanner.finding().ready)return 1;
    bins[120]=-20.0f;
    for(int i=0;i<5;++i)scanner.observe(bins);
    auto f=scanner.finding();
    if(!f.ready||f.hz<200||f.prominenceDb<10||f.suggestedDepthDb>6)return 2;
    bins.fill(-120.0f);scanner.observe(bins);
    if(scanner.finding().ready)return 3;
    scanner.reset();
    if(scanner.finding().ready)return 4;
    std::cout<<"Surgery Assist four proof groups passed\n";return 0;
}