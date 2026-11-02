#pragma once
#include "Arduino.h"
constexpr int FSPI=1,MSBFIRST=1,SPI_MODE0=0,SPI_MODE3=3;
struct SPISettings{SPISettings(uint32_t,int,int){}};
struct SPIClass{explicit SPIClass(int){} void begin(int,int,int,int){} void* bus(){return this;} void end(){} void beginTransaction(SPISettings){} void endTransaction(){} uint8_t transfer(uint8_t){return 0;}};
