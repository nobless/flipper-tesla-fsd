"""Conservative project-frame sum plus a 4 KiB core/library allowance, 8 KiB stack."""
from pathlib import Path
import sys
build = Path(sys.argv[1])
files = [p for p in build.rglob("*.su") if "diagnostics" in p.parts]
if not files:
    raise SystemExit("No project stack-usage files; refusing unchecked image")
frames = []
for path in files:
    for line in path.read_text().splitlines():
        columns = line.split("\t")
        if len(columns) < 3:
            continue
        if columns[2] == "dynamic":
            raise SystemExit(f"Unbounded stack usage: {line}")
        frames.append(int(columns[1]))
if not frames:
    raise SystemExit("No function frames found")
budget = sum(frames) + 4096
print(f"Standalone stack bound: all project frames {sum(frames)} + core/library allowance 4096 = {budget}/8192")
if budget > 8192:
    raise SystemExit("Standalone stack budget exceeded")
