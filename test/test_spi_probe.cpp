#include <cassert>
#include "../esp32/diagnostics/probe.h"
int main() {
    const uint8_t bytes[]={0x60,0x07,0x98,0x04};
    assert(decodeWord(bytes)==0x04980760u);
    assert(validConfig(0x04980760u));
    assert(!validConfig(0)); assert(!validConfig(0xFFFFFFFFu));
    assert(!validConfig(0x04180760u));
    assert(stableConfig(0x04980760u,0x04980760u,0x04980760u));
    assert(!stableConfig(0x04980760u,0x04980761u,0x04980760u));
    assert(!stableConfig(0,0,0));
    const uint32_t down[]={0u,0u,0u},up[]={0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    assert(followsPulls(down,up));
    const uint32_t driven[]={0x04980760u,0x04980760u,0x04980760u};
    assert(!followsPulls(driven,driven));
    assert(!followsPulls(down,driven));
}
