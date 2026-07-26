#!/usr/bin/env python3
"""Shared command construction for nRF52840 Intel HEX to UF2 conversion."""

from __future__ import annotations

import sys
from pathlib import Path

NRF52840_FAMILY_ID = "0xADA52840"


def uf2_conversion_command(
    framework_dir: str | Path,
    hex_file: str | Path,
    uf2_file: str | Path,
) -> list[str]:
    """Return the Adafruit BSP conversion command, validating its inputs."""
    framework_dir = Path(framework_dir)
    hex_file = Path(hex_file)
    uf2conv = framework_dir / "tools" / "uf2conv" / "uf2conv.py"

    if not uf2conv.is_file():
        raise FileNotFoundError(f"Adafruit nRF52 UF2 converter not found: {uf2conv}")
    if not hex_file.is_file():
        raise FileNotFoundError(f"nRF52 firmware HEX not found: {hex_file}")

    return [
        sys.executable,
        str(uf2conv),
        str(hex_file),
        "-c",
        "-f",
        NRF52840_FAMILY_ID,
        "-o",
        str(Path(uf2_file)),
    ]
