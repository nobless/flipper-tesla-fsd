"""Pinned Battery-Emulator driver, including PR3136 and the final PR3077 race fix."""
Import("env")
from pathlib import Path
import hashlib, json, urllib.request
root = Path(env.subst("$PROJECT_DIR"))
lock = json.loads((root / "diagnostics/be-lock.json").read_text())
dest = root / ".diagnostic-libs/battery/BE"
for name, entry in lock["files"].items():
    url = f'https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/{lock["ref"]}/{entry["source"]}'
    with urllib.request.urlopen(url, timeout=60) as response:
        data = response.read()
    digest = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
    if digest != entry["sha"]:
        raise RuntimeError(f"Source hash mismatch: {url}")
    target = dest / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
# Supply the host application logging include.
logging = dest / "src/src/devboard/utils/logging.h"
logging.parent.mkdir(parents=True, exist_ok=True)
logging.write_text('#pragma once\n#include <Arduino.h>\n#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)\n')
(dest / "library.json").write_text(json.dumps({"name":"BatteryEmulatorDiagnostic", "version":"1.0.0", "build":{"flags":["-fstack-usage"]}}))

# IDF 4.4 compatibility for this project's pinned Arduino 2.0.17 toolchain.
# Each replacement must match the hash-verified upstream source exactly.
patches = {
    "src/mcp2515_lite/async_spi_bus.cpp": [
        ('#include "hal/spi_ll.h"', '#pragma push_macro("FLAG_ATTR")\n#undef FLAG_ATTR\n#define FLAG_ATTR(type)\n#include "hal/spi_ll.h"\n#pragma pop_macro("FLAG_ATTR")'),
        ('SPI_LL_GET_HW(host)', '(host == SPI2_HOST ? &GPSPI2 : &GPSPI3)'),
        ('#include "esp_private/spi_share_hw_ctrl.h"', '#include "driver/spi_common_internal.h"'),
        ('spi_ll_apply_config(_hw);\n  spi_ll_user_start(_hw);', 'spi_ll_master_user_start(_hw);'),
        ('gpio_ll_set_level(&GPIO, dev->cs,', 'gpio_ll_set_level(&GPIO, (gpio_num_t)dev->cs,'),
        ('gpio_ll_set_level(&GPIO, _cur->cs,', 'gpio_ll_set_level(&GPIO, (gpio_num_t)_cur->cs,'),
        ('gpio_ll_set_level(&GPIO, b->_cur->cs,', 'gpio_ll_set_level(&GPIO, (gpio_num_t)b->_cur->cs,'),
    ],
    "src/mcp2518fd_lite/mcp2518fd_lite.cpp": [
        ('gpio_ll_get_level(&GPIO, _int_pin)', 'gpio_ll_get_level(&GPIO, (gpio_num_t)_int_pin)'),
    ],
}
for filename, replacements in patches.items():
    path = dest / filename
    source = path.read_text()
    for old, new in replacements:
        if old not in source:
            raise RuntimeError(f"Compatibility patch did not match: {filename}: {old}")
        source = source.replace(old, new)
    path.write_text(source)
