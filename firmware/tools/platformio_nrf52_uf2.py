# pyright: reportMissingImports=false, reportUndefinedVariable=false
"""PlatformIO post-build hook that emits firmware.uf2 for nRF52840."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

Import("env")  # noqa: F821 - provided by PlatformIO/SCons

tools_dir = Path(env.subst("$PROJECT_DIR")) / "tools"  # noqa: F821
sys.path.insert(0, str(tools_dir))

from nrf52_uf2 import uf2_conversion_command  # noqa: E402


def generate_uf2(source, target, env) -> None:
    del source, target
    build_dir = Path(env.subst("$BUILD_DIR"))
    program_name = env.subst("$PROGNAME")
    framework_dir = env.PioPlatform().get_package_dir(
        "framework-arduinoadafruitnrf52"
    )
    if not framework_dir:
        raise RuntimeError("PlatformIO Adafruit nRF52 framework package is unavailable")

    hex_file = build_dir / f"{program_name}.hex"
    uf2_file = build_dir / f"{program_name}.uf2"
    subprocess.run(
        uf2_conversion_command(framework_dir, hex_file, uf2_file),
        check=True,
    )
    print(f"Generated {uf2_file}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", generate_uf2)  # noqa: F821
