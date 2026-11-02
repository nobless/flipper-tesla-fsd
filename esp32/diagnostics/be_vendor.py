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
# The only adaptation is the host application's logging include.
logging = dest / "src/src/devboard/utils/logging.h"
logging.parent.mkdir(parents=True, exist_ok=True)
logging.write_text('#pragma once\n#include <Arduino.h>\n#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)\n')
(dest / "library.json").write_text(json.dumps({"name":"BatteryEmulatorDiagnostic", "version":"1.0.0", "build":{"flags":["-fstack-usage"]}}))
