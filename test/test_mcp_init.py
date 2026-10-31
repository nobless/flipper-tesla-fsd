"""Compile the actual firmware bring-up methods against a synthetic SPI device."""
from pathlib import Path
import subprocess
import tempfile
import os

root = Path(__file__).resolve().parents[1]
source = (root / 'esp32/.firmware/can_driver.cpp').read_text()
def method(name):
    start = source.index('    bool ' + name + '(')
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

harness = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <vector>
#include "can_mcp2518fd.h"
static unsigned elapsed;
void delay(unsigned ms) { elapsed += ms; }
struct Logger {
 template<class... T> void printf(const char*, T...) {}
} Serial;
#define MCP2518FD_OSC_HZ 40000000u
struct Fake {
 const char *label_ = "test";
 static constexpr unsigned kSlowSpiHz=800000, kMcp2515ResetMs=5;
 uint32_t stuck=0, clock=40000000, ram=0x12345678;
 bool silent=false, corrupt=false, osc=true, fd_reset=false, classic=false;
 unsigned config_after=0, config_reads=0, oscillator_after=0, oscillator_reads=0;
 unsigned resets=0, classic_resets=0, fast=0, pin_writes=0;
 bool cleanup=false;
 uint32_t read_reg32(uint16_t a, uint32_t hz) {
   if (silent) return stuck;
   if(a==0) return ++config_reads > config_after ? 0x04980760 : 0;
   if(a==0xE00) return osc && ++oscillator_reads > oscillator_after ? 0x460 : 0x60;
   if(a==0x400) { if(hz>1000000) ++fast; return corrupt && ram==0xA5A5A5A5u ? 0 : ram; }
   return 1;
 }
 void write_reg32(uint16_t a,uint32_t v,uint32_t) {
   if(a==0x400) ram=v;
   else { assert(a==0x040 || a==0x050); assert(v==0); }
 }
 void write_reg8(uint16_t a,uint8_t v,uint32_t hz) {
   assert(hz==1000000);
   if(a==3) assert(v==4); // Never request normal CAN mode.
   else if(a==0xE00) assert(v==0x60);
   else { assert(a==0xE07 || a==0); ++pin_writes; }
 }
 void transfer(uint8_t *b,size_t n,uint32_t) {
   if(n==2) {assert(b[0]==0 && b[1]==0); fd_reset=true; ++resets;}
   else if(n==1) {assert(b[0]==0xC0); ++classic_resets;}
   else { assert(n==4 && b[0]==3 && b[1]==0x0E);
     b[2]=classic ? 0x80 : uint8_t(stuck); b[3]=classic ? 0x87 : uint8_t(stuck); }
 }
 void spi_reset() {cleanup=true;}
 uint32_t measure_sysclk_hz() { assert(fd_reset); return clock; }
METHODS
};
int main() {
 Fake ok; elapsed=0; assert(ok.battery_emulator_prepare());
 assert(ok.resets==1 && elapsed>=5 && ok.ram==0x12345678 && ok.pin_writes==2);
 for(uint32_t stuck : {0u,0xffffffffu}) {
   Fake f; f.silent=true; f.stuck=stuck; elapsed=0;
   assert(!f.battery_emulator_prepare()); assert(f.fast==0 && f.pin_writes==0);
   assert(elapsed<100); assert(!f.probe_mcp2515()); assert(f.cleanup);
 }
 Fake delayed; delayed.config_after=15; delayed.oscillator_after=20;
 assert(delayed.battery_emulator_prepare());
 Fake timeout; timeout.osc=false; elapsed=0;
 assert(!timeout.battery_emulator_prepare()); assert(timeout.fast==0 && elapsed<100);
 Fake wrong; wrong.clock=20000000; assert(!wrong.battery_emulator_prepare()); assert(wrong.fast==0);
 Fake corrupt; corrupt.corrupt=true; assert(!corrupt.battery_emulator_prepare());
 assert(corrupt.pin_writes==0 && corrupt.ram==0x12345678);
 Fake classic; classic.classic=true; assert(classic.probe_mcp2515());
 assert(classic.classic_resets==1 && classic.cleanup);
 puts("MCP initialization: success, delayed startup, stuck-low/high, oscillator timeout, wrong clock, RAM corruption, classic identification PASS");
}
'''
harness = harness.replace('METHODS', method('battery_emulator_prepare') + '\n' + method('probe_mcp2515'))
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp)/'test.cpp'; exe = Path(tmp)/'test'
    src.write_text(harness)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'esp32/.firmware'), str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
