#pragma once
#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <vector>
#include <utility>
using byte=uint8_t;
constexpr int HIGH=1,LOW=0,INPUT=0,OUTPUT=1,INPUT_PULLDOWN=2,INPUT_PULLUP=3;
extern std::vector<std::pair<int,int>> writes;
extern std::vector<int> inputBits;
extern unsigned readIndex;
inline void digitalWrite(int p,int v){writes.emplace_back(p,v);}
inline int digitalRead(int){return inputBits.at(readIndex++);}
inline void pinMode(int,int){}
inline void delay(unsigned){}
inline void delayMicroseconds(unsigned){}
inline void yield(){}
inline uint32_t millis(){return 0;}
struct SerialStub {void begin(int){} void println(const char*){} template<class... T> void printf(const char*,T...){} bool available(){return false;} int read(){return -1;}};
extern SerialStub Serial;
