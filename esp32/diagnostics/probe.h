#pragma once
#include <stdint.h>
inline uint32_t decodeWord(const uint8_t *b) { return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24); }
inline bool validConfig(uint32_t c) { return c!=0 && c!=0xFFFFFFFFu && ((c>>21)&7)==4; }
inline bool stableConfig(uint32_t a,uint32_t b,uint32_t c) { return a==b && b==c && validConfig(a); }
inline bool followsPulls(const uint32_t (&down)[3],const uint32_t (&up)[3]) {
    for(unsigned i=0;i<3;++i) if(down[i]!=0 || up[i]!=0xFFFFFFFFu) return false;
    return true;
}
