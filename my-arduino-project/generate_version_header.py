#!/usr/bin/env python3
"""Generate version header from git describe or fallback to commit hash.

Outputs include/version.h with:
  FW_VERSION_GIT   full describe string
  FW_VERSION_SHORT first tag/segment
  FW_BUILD_UNIX    build epoch seconds
"""
import subprocess
import pathlib
import time

ROOT = pathlib.Path(__file__).resolve().parent
INC = ROOT / 'include'
OUT = INC / 'version.h'

def run(cmd):
    return subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode().strip()

def main():
    try:
        desc = run(['git', 'describe', '--tags', '--long', '--dirty', '--always'])
    except Exception:
        try:
            desc = run(['git', 'rev-parse', '--short', 'HEAD'])
        except Exception:
            desc = 'unknown'
    short = desc.split('-')[0] if '-' in desc else desc
    build_unix = int(time.time())
    content = f"// Auto-generated. Do not edit.\n#pragma once\n#define FW_VERSION_GIT   \"{desc}\"\n#define FW_VERSION_SHORT \"{short}\"\n#define FW_BUILD_UNIX    {build_unix}UL\n"
    if OUT.exists():
        old = OUT.read_text()
        if old == content:
            return
    OUT.write_text(content)
    print(f"[version] Wrote {OUT}")

if __name__ == '__main__':
    main()
