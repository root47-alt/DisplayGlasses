#!/usr/bin/env python3
"""Assembles the project website (GitHub Pages) into one folder.

  python3 web/build_site.py build/site

Expects the firmware to be built already (pio run) so each env has a
merged.bin. Copies the browser flasher with one manifest per build, the
optics calculator, and the project home page. The screen pictures are
rendered straight into <site>/screens by tools/hudsim/render.py.
"""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# env name -> (chip family for ESP Web Tools, what it is)
BUILDS = {
    "feather": ("ESP32-S3", "ESP32-S3 Feather"),
    "feather_demo": ("ESP32-S3", "ESP32-S3 Feather, demo mode"),
    "feather_v2": ("ESP32", "ESP32 Feather V2"),
    "feather_v2_demo": ("ESP32", "ESP32 Feather V2, demo mode"),
}


def version() -> str:
    sha = os.environ.get("GITHUB_SHA")
    if sha:
        return sha[:7]
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return "dev"


def wrap_page(fragment: str) -> str:
    """The calculator is written as an artifact fragment; give it a full document."""
    return ("<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, viewport-fit=cover\">\n"
            "</head>\n<body>\n" + fragment + "\n</body>\n</html>\n")


def main() -> None:
    site = Path(sys.argv[1] if len(sys.argv) > 1 else "build/site")
    (site / "flash" / "bin").mkdir(parents=True, exist_ok=True)
    (site / "optics").mkdir(parents=True, exist_ok=True)
    ver = version()

    missing = []
    for env, (chip, label) in BUILDS.items():
        merged = ROOT / "firmware" / ".pio" / "build" / env / "merged.bin"
        if not merged.exists():
            missing.append(env)
            continue
        shutil.copy(merged, site / "flash" / "bin" / f"{env}.bin")
        manifest = {
            "name": f"DisplayGlasses ({label})",
            "version": ver,
            "new_install_prompt_erase": True,
            "builds": [{"chipFamily": chip, "parts": [{"path": f"bin/{env}.bin", "offset": 0}]}],
        }
        (site / "flash" / f"{env}.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if missing:
        sys.exit(f"Build these first (pio run -e ...): {', '.join(missing)}")

    shutil.copy(ROOT / "web" / "flash" / "index.html", site / "flash" / "index.html")
    shutil.copy(ROOT / "web" / "index.html", site / "index.html")
    calc = (ROOT / "tools" / "optics" / "calculator.html").read_text()
    (site / "optics" / "index.html").write_text(wrap_page(calc))
    shutil.copy(ROOT / "tools" / "optics" / "optics.js", site / "optics" / "optics.js")
    (site / ".nojekyll").write_text("")
    print(f"Site ready in {site} (firmware version {ver})")


if __name__ == "__main__":
    main()
