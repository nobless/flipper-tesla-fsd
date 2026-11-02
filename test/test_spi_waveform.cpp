#include <cassert>
#include "diagnostic_stubs/Arduino.h"
std::vector<std::pair<int,int>> writes;
std::vector<int> inputBits;
unsigned readIndex=0;
std::vector<std::pair<size_t,unsigned>> pauses;
SerialStub Serial;
#include "../esp32/diagnostics/main.cpp"
int main(){
    // Exercise the actual software SPI implementation for CPOL/CPHA 0 and 3.
    for(uint8_t mode : {uint8_t(0),uint8_t(3)}) {
        active={true,mode,10000,20}; writes.clear(); pauses.clear(); readIndex=0;
        inputBits={1,0,1,0,0,1,0,1};
        assert(softwareByte(0x96)==0xA5);
        assert(readIndex==8); assert(writes.size()==24);
        // Each mode must allow MOSI to settle before the rising sample edge.
        for(unsigned bit=0;bit<8;++bit) {
            bool setup=false;
            const size_t beforeEdge=bit*3+(mode==0?1:2);
            for(const auto& p:pauses) if(p.first==beforeEdge && p.second>=50) setup=true;
            assert(setup);
        }
        for(unsigned bit=0;bit<8;++bit){
            const size_t i=bit*3;
            const int data=(0x96 & (0x80>>bit))!=0;
            if(mode==0){
                assert(writes[i]==std::make_pair(DIAG_MOSI,data));
                assert(writes[i+1]==std::make_pair(DIAG_SCK,HIGH));
                assert(writes[i+2]==std::make_pair(DIAG_SCK,LOW));
            }else{
                assert(writes[i]==std::make_pair(DIAG_SCK,LOW));
                assert(writes[i+1]==std::make_pair(DIAG_MOSI,data));
                assert(writes[i+2]==std::make_pair(DIAG_SCK,HIGH));
            }
        }
    }
    // A missing controller cannot pass verification or cause RAM writes.
    active={false,0,100000,20}; writes.clear();
    assert(!verify());
    assert(!configConfirmed());
}
