#include "../Source/SurgeryAssist.h"
#include <array>
#include <iostream>
int main(){
    surgery::AssistScanner<256> scanner;
    std::array<float,256> bins{};bins.fill(-65.0f);
    scanner.observe(bins);
    if(scanner.finding().ready)return 1;
    bins[120]=-20.0f;
    for(int i=0;i<8;++i)scanner.observe(bins,250.0f,2000.0f);
    auto f=scanner.finding();
    if(!f.ready||f.hz<250||f.hz>2000||
       f.prominenceDb<10||f.suggestedDepthDb>4.5)return 2;
    // A candidate outside the selected ROOT must never become a suggestion.
    scanner.reset();bins.fill(-65.0f);bins[230]=-10.0f;
    for(int i=0;i<12;++i)scanner.observe(bins,250.0f,2000.0f);
    if(scanner.finding().ready)return 3;
    bins.fill(-120.0f);scanner.observe(bins);
    if(scanner.finding().ready)return 5;
    scanner.reset();
    if(scanner.finding().ready)return 4;
    std::cout<<"Surgery Assist four proof groups passed\n";return 0;
}