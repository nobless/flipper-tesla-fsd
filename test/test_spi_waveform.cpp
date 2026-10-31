#include <cassert>
#include "diagnostic_stubs/Arduino.h"
std::vector<std::pair<int,int>> writes;
std::vector<int> inputBits;
unsigned readIndex=0;
SerialStub Serial;
#include "../esp32/diagnostics/main.cpp"
int main(){
    // Exercise the actual software SPI implementation for CPOL/CPHA 0 and 3.
    for(uint8_t mode : {uint8_t(0),uint8_t(3)}) {
        active={true,mode,10000,20}; writes.clear(); readIndex=0;
        inputBits={1,0,1,0,0,1,0,1};
        assert(softwareByte(0x96)==0xA5);
        assert(readIndex==8); assert(writes.size()==24);
        for(unsigned bit=0;bit<8;++bit){
            const size_t i=bit*3;
            const int data=(0x96 & (0x80>>bit))!=0;
            if(mode==0){
                assert(writes[i]==std::make_pair(MOSI,data));
                assert(writes[i+1]==std::make_pair(SCK,HIGH));
                assert(writes[i+2]==std::make_pair(SCK,LOW));
            }else{
                assert(writes[i]==std::make_pair(SCK,LOW));
                assert(writes[i+1]==std::make_pair(MOSI,data));
                assert(writes[i+2]==std::make_pair(SCK,HIGH));
            }
        }
    }
    // A missing controller cannot pass verification or cause RAM writes.
    active={false,0,100000,20}; writes.clear();
    assert(!verify());
    assert(!configConfirmed());
}
