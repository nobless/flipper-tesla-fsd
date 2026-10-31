"""Fetch exact upstream driver sources into a separate per-provider library directory."""
Import("env")
from pathlib import Path
import hashlib
import urllib.request
import json
provider = env.GetProjectOption("custom_vendor", "")
if provider:
    sources = {
        "lilygo": ("Xinyuan-LilyGO/T-2Can", "4e9eddd5f39f27d43360de9fb8904734cce9f0cb", "libraries/Longan_CANFD/"),
        "longan": ("Longan-Labs/Longan_CANFD", "6e5566196a1902556e75f10f448172a919bbcdfe", ""),
    }
    repo, commit, prefix = sources[provider]
    destination = Path(env.subst("$PROJECT_DIR")) / ".diagnostic-libs" / provider / "Longan_CANFD"
    files = ["LICENSE", "src/mcp2518fd_can.cpp", "src/mcp2518fd_can.h", "src/mcp2518fd_can_dfs.h", "src/mcp_can_fd.cpp", "src/mcp_can_fd.h"]
    # Pin each file to its Git blob hash as well as the upstream commit.
    hashes = json.loads((Path(env.subst("$PROJECT_DIR")) / "diagnostics/vendor-lock.json").read_text())[provider]
    for name in files:
        target = destination / name
        def digest(data):
            return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if target.exists() and digest(target.read_bytes()) == hashes[name]:
            continue
        url = f"https://raw.githubusercontent.com/{repo}/{commit}/{prefix}{name}"
        with urllib.request.urlopen(url, timeout=60) as response:
            data = response.read()
        if digest(data) != hashes[name]:
            raise RuntimeError(f"Upstream file hash mismatch: {url}")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
