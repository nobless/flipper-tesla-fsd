#include <Arduino.h>
#include <initializer_list>
#include <mcp2518fd_lite/mcp2518fd_lite.h>
#include "probe.h"
#ifndef FW_VERSION
#error Release version must be embedded
#endif
// Independent image: never constructs Arduino SPIClass. Explicit first-device pins
// avoid the second-addon HAL defaults corrected by Battery-Emulator PR3136.
static AsyncSpiBus* backend=nullptr;
static bool IRAM_ATTR idleStep(AsyncSpiDevice*) { return false; }
static AsyncSpiDevice raw(idleStep);
static MCP2518FD_Lite* controller=nullptr;
uint32_t beRead(uint16_t address) {
    uint8_t b[]={uint8_t(0x30|(address>>8)),uint8_t(address),0,0,0,0};
    backend->transferBlocking(&raw,b,sizeof b); return decodeWord(b+2);
}
void snapshot(const char* stage) {
    Serial.printf("[BE] %s CON=%08lX OSC=%08lX DEVID=%08lX\n",stage,
        (unsigned long)beRead(0),(unsigned long)beRead(0xE00),(unsigned long)beRead(0xE14));
}
void attempt() {
    Serial.println("[BE] begin internal-loopback, nominal/data=500000, clock=40000000, INT=-1; no CAN frames sent");
    const bool ok=controller->begin({500000,500000,40000000},3,true);
    snapshot("after begin");
    const uint32_t a=beRead(0),b=beRead(0),c=beRead(0);
    const bool confirmed=ok && a==b && b==c && a!=0 && a!=0xFFFFFFFFu && ((a>>21)&7)==2;
    Serial.printf("[BE RESULT] begin=%u stable-internal-loopback=%u\n",unsigned(ok),unsigned(confirmed));
    char state[512]; controller->debugState(state,sizeof state); Serial.println(state);
    if(ok) { controller->pause(true); snapshot("paused in configuration"); }
    Serial.println("[BE DONE] 'r' retry initialization; 'c' toggle 1Hz read captures. No physical CAN communication verified.");
}
void setup() {
    Serial.begin(115200); delay(2000);
    Serial.printf("\n[FSD] Version: %s\n[BE] Battery-Emulator backend; USB-only, vehicle disconnected\n",FW_VERSION);
    Serial.println("[BE] SPI2_HOST SCK=12 MOSI=11 MISO=13 CS=10; Arduino SPI absent; GPIO3/8/9 untouched");
    backend=AsyncSpiBus::get(SPI2_HOST,12,11,13);
    if(!backend || !backend->add(&raw,10,100000)) { Serial.println("[BE FAIL] SPI peripheral claim/device allocation"); backend=nullptr; return; }
    for(uint32_t hz : {100000u,400000u,1000000u}) { backend->setClock(&raw,hz); Serial.printf("[BE] raw SPI=%lu\n",(unsigned long)hz); snapshot("before driver"); }
    backend->setClock(&raw,100000);
    controller=new MCP2518FD_Lite(SPI2_HOST,12,11,13,10,-1,0);
    attempt();
}
void loop() {
    static bool capture=false;
    static uint32_t last=0;
    if(backend && Serial.available()) { int c=Serial.read(); if(c=='r') attempt(); if(c=='c') capture=!capture; }
    if(backend && capture && millis()-last>=1000) { last=millis(); Serial.println("[BE CAPTURE BEGIN] SPI mode0 100kHz reads CON, OSC, DEVID"); snapshot("capture"); Serial.println("[BE CAPTURE END]"); }
    delay(10);
}
