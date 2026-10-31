#include <Arduino.h>
#include <SPI.h>
#include <initializer_list>
#include "probe.h"
#ifdef DIAG_VENDOR
#include <mcp2518fd_can.h>
#endif
#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif
// Only the board's four documented SPI pins are driven. GPIO9 is INT0, not reset.
constexpr int DIAG_CS=10, DIAG_SCK=12, DIAG_MOSI=11, DIAG_MISO=13;
SPIClass diagnosticSPI(FSPI);
struct Link { bool software; uint8_t mode; uint32_t hz; uint16_t guard; };
Link active{false,0,100000,10};
void attach(const Link &l) {
    diagnosticSPI.end();
    digitalWrite(DIAG_CS,HIGH); pinMode(DIAG_CS,OUTPUT);
    if(l.software) {
        pinMode(DIAG_SCK,OUTPUT); digitalWrite(DIAG_SCK,l.mode==3);
        pinMode(DIAG_MOSI,OUTPUT); digitalWrite(DIAG_MOSI,LOW); pinMode(DIAG_MISO,INPUT);
    } else diagnosticSPI.begin(DIAG_SCK,DIAG_MISO,DIAG_MOSI,DIAG_CS);
}
uint8_t softwareByte(uint8_t tx) {
    uint8_t rx=0;
    const bool idle=active.mode==3;
    const uint32_t half=500000/active.hz;
    for(unsigned bit=0;bit<8;++bit) {
        if(!idle) digitalWrite(DIAG_MOSI,(tx & (0x80>>bit))!=0);
        digitalWrite(DIAG_SCK,!idle);
        if(idle) digitalWrite(DIAG_MOSI,(tx & (0x80>>bit))!=0);
        delayMicroseconds(half);
        if(!idle) rx=uint8_t((rx<<1)|digitalRead(DIAG_MISO));
        digitalWrite(DIAG_SCK,idle);
        if(idle) rx=uint8_t((rx<<1)|digitalRead(DIAG_MISO));
        delayMicroseconds(half);
    }
    return rx;
}
void exchange(uint8_t *b,size_t n) {
    if(!active.software) diagnosticSPI.beginTransaction(SPISettings(active.hz,MSBFIRST,active.mode==3?SPI_MODE3:SPI_MODE0));
    delayMicroseconds(active.guard);
    digitalWrite(DIAG_CS,LOW); delayMicroseconds(active.guard);
    for(size_t i=0;i<n;++i) b[i]=active.software?softwareByte(b[i]):diagnosticSPI.transfer(b[i]);
    delayMicroseconds(active.guard); digitalWrite(DIAG_CS,HIGH);
    delayMicroseconds(active.guard);
    if(!active.software) diagnosticSPI.endTransaction();
}
uint32_t readWord(uint16_t a) {
    uint8_t b[]={uint8_t(0x30|(a>>8)),uint8_t(a),0,0,0,0};
    exchange(b,sizeof b); return decodeWord(b+2);
}
void writeWord(uint16_t a,uint32_t v) {
    uint8_t b[]={uint8_t(0x20|(a>>8)),uint8_t(a),uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};
    exchange(b,sizeof b);
}
void writeByte(uint16_t a,uint8_t v) {
    uint8_t b[]={uint8_t(0x20|(a>>8)),uint8_t(a),v}; exchange(b,sizeof b);
}
void reset(unsigned wait) { uint8_t b[]={0,0}; exchange(b,2); delay(wait); }
bool configConfirmed() {
    writeByte(3,4);
    for(unsigned i=0;i<100;++i) { if(validConfig(readWord(0))) return true; delay(1); }
    return false;
}
bool verify() {
    const uint32_t a=readWord(0),b=readWord(0),c=readWord(0);
    const bool stable=stableConfig(a,b,c);
    Serial.printf("[VERIFY] CON=%08lX/%08lX/%08lX stable-config=%u OSC=%08lX DEVID=%08lX\n",(unsigned long)a,(unsigned long)b,(unsigned long)c,unsigned(stable),(unsigned long)readWord(0xE00),(unsigned long)readWord(0xE14));
    if(!stable) return false;
    // No RAM writes until Configuration mode is positively confirmed.
    const uint32_t saved=readWord(0x400);
    bool ok=true;
    for(uint32_t pattern : {0xA5A5A5A5u,0x5A5A5A5Au,0x00000000u,0xFFFFFFFFu}) {
        writeWord(0x400,pattern); const uint32_t actual=readWord(0x400);
        Serial.printf("[RAM] wrote=%08lX read=%08lX\n",(unsigned long)pattern,(unsigned long)actual);
        ok=ok && actual==pattern;
    }
    writeWord(0x400,saved); const bool restored=readWord(0x400)==saved;
    Serial.printf("[VERIFY] RAM=%s restored=%u identity-not-inferred-from-DEVID-alone\n",ok?"PASS":"FAIL",unsigned(restored));
    return ok && restored;
}
void pullRead() {
    uint32_t down[3],up[3];
    pinMode(DIAG_MISO,INPUT_PULLDOWN); delay(2);
    for(auto &v:down) v=readWord(0);
    pinMode(DIAG_MISO,INPUT_PULLUP); delay(2);
    for(auto &v:up) v=readWord(0);
    pinMode(DIAG_MISO,INPUT);
    Serial.printf("[PULL READ] down=%08lX/%08lX/%08lX up=%08lX/%08lX/%08lX follows-pulls=%u (suggests undriven input; not proof of chip failure)\n",(unsigned long)down[0],(unsigned long)down[1],(unsigned long)down[2],(unsigned long)up[0],(unsigned long)up[1],(unsigned long)up[2],unsigned(followsPulls(down,up)));
}
void suite() {
    Serial.println("[SUITE] USB bench; disconnect vehicle. No WiFi/storage/TWAI/application tasks. No CAN transmit calls.");
    for(bool sw : {false,true}) for(uint8_t mode : {uint8_t(0),uint8_t(3)}) {
        // Software SPI: 10/50 kHz; hardware SPI: 10/100/400 kHz.
        for(uint32_t rate : {10000u,50000u,100000u,400000u}) {
            if((sw && rate>50000)||(!sw && rate==50000)) continue;
            for(uint16_t guard : {uint16_t(2),uint16_t(20)}) {
                active={sw,mode,rate,guard}; attach(active);
                Serial.printf("[LINK] backend=%s peripheral=FSPI mode=%u hz=%lu DIAG_CS-setup/hold/high=%uus pins=10/12/11/13\n",sw?"software":"hardware",unsigned(mode),(unsigned long)rate,unsigned(guard));
                for(unsigned wait : {10u,50u,100u}) {
                    reset(wait);
                    Serial.printf("[RESET] wait=%ums CON=%08lX OSC=%08lX\n",wait,(unsigned long)readWord(0),(unsigned long)readWord(0xE00));
                }
                pullRead();
                const bool configured=configConfirmed();
                Serial.printf("[RESULT] configuration=%u verified=%u\n",unsigned(configured),unsigned(configured && verify()));
                yield();
            }
        }
    }
    active={false,0,100000,20}; attach(active);
    Serial.println("[DONE] 'r' repeats suite; 'c' toggles 1Hz capture reads (3E 00 + 4 data bytes). Reset repeats startup.");
}
#ifdef DIAG_VENDOR
void vendorComparison() {
    active={false,0,100000,20}; attach(active);
    reset(100);
    // Both upstream APIs permit Configuration mode, avoiding a transition to Normal.
    mcp2518fd controller(DIAG_CS); controller.setSPI(&diagnosticSPI);
    controller.setMode(CAN_CONFIGURATION_MODE);
    Serial.printf("[VENDOR] %s starting; return code alone is not a hardware check\n",DIAG_VENDOR);
    const byte result=controller.begin(CAN20_500KBPS,MCP2518FD_40MHz);
    Serial.printf("[VENDOR] begin=%u mode=%u\n",unsigned(result),unsigned(controller.getMode()));
    // Reattach explicitly: upstream begin() may call begin() without pin arguments.
    attach(active);
    const bool configured=configConfirmed();
    Serial.printf("[VENDOR] verified=%u\n",unsigned(configured && verify()));
    reset(100);
}
#endif
bool capture=false;
void setup() {
    Serial.begin(115200); delay(2000);
    Serial.printf("\n[FSD] Version: %s standalone SPI diagnostics\n",FW_VERSION);
    Serial.println("[BOARD] LilyGO T-2CAN FD H784-A; GPIO8/9/3 untouched; GPIO7/6 untouched");
    active={false,0,100000,20}; attach(active);
    // Delays are since MCU setup, not measured controller power-on times.
    for(unsigned wait : {10u,50u,100u,500u}) {
        delay(wait); Serial.printf("[STARTUP] additional-wait=%ums CON=%08lX OSC=%08lX\n",wait,(unsigned long)readWord(0),(unsigned long)readWord(0xE00));
    }
#ifdef DIAG_VENDOR
    vendorComparison();
#endif
    suite();
}
void loop() {
    if(Serial.available()) { const int c=Serial.read(); if(c=='r') suite(); if(c=='c') capture=!capture; }
    static uint32_t last=0;
    if(capture && millis()-last>=1000) {
        last=millis(); Serial.println("[CAPTURE BEGIN] OSC read; CS10 SCK12 MOSI11 MISO13; hardware mode0 100kHz");
        const uint32_t osc=readWord(0xE00);
        Serial.printf("[CAPTURE END] OSC=%08lX\n",(unsigned long)osc);
    }
    delay(10);
}
