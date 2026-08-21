#!/usr/bin/env python3
"""Contract checks for deprecated RAK4631 build-flag aliases."""

import subprocess
from pathlib import Path


ALIASES = (
    "ETHERNET_W5100S",
    "ETH_TCP_PORT",
    "ETH_TOKEN",
    "ETH_HOSTNAME",
    "ETH_USE_DHCP",
    "ETH_DHCP_TIMEOUT_MS",
    "ETH_DHCP_RESPONSE_TIMEOUT_MS",
    "ETH_DHCP_RETRY_MS",
    "ETH_STATIC_FALLBACK_ON_DHCP_FAIL",
    "ETH_STATIC_IP",
    "ETH_GATEWAY",
    "ETH_SUBNET",
    "ETH_DNS",
    "ETH_POWER_PIN",
    "ETH_RESET_PIN",
    "ETH_CS_PIN",
    "ETH_INT_PIN",
    "ETH_SPI_SCK",
    "ETH_SPI_MISO",
    "ETH_SPI_MOSI",
    "ETH_RESET_LOW_MS",
    "ETH_POST_RESET_MS",
    "ETH_HARD_RESET_ON_BEGIN",
    "ETH_ASSUME_UNKNOWN_LINK_UP",
    "RAK4631_SX126X_POWER_EN",
)

root = Path(__file__).resolve().parents[1]
header = root / "include" / "legacy_rak4631_build_flags.h"
text = header.read_text()

for suffix in ALIASES:
    legacy = f"PYMC_{suffix}"
    canonical = f"OPENHOP_{suffix}"
    guard = f"#if defined({legacy}) && !defined({canonical})"
    mapping = f"#  define {canonical} {legacy}"
    assert guard in text, f"missing canonical-wins guard for {legacy}"
    assert mapping in text, f"missing compatibility mapping for {legacy}"

for source in (root / "src" / "main.cpp", root / "src" / "w5100s_ethernet_transport.cpp"):
    source_text = source.read_text()
    assert '#include "legacy_rak4631_build_flags.h"' in source_text, source
    assert source_text.index('#include "legacy_rak4631_build_flags.h"') < source_text.index(
        "#if defined(OPENHOP_ETHERNET_W5100S)"
    ), source


def resolve_token(*defines: str) -> str:
    result = subprocess.run(
        ["cpp", "-P", "-x", "c", "-I", str(root / "include"), *defines, "-"],
        input='#include "legacy_rak4631_build_flags.h"\nOPENHOP_ETH_TOKEN\n',
        text=True,
        capture_output=True,
        check=True,
    )
    return result.stdout.strip()


assert resolve_token('-DPYMC_ETH_TOKEN="legacy-secret"') == '"legacy-secret"'
assert (
    resolve_token('-DPYMC_ETH_TOKEN="legacy-secret"', '-DOPENHOP_ETH_TOKEN="canonical-secret"')
    == '"canonical-secret"'
)

print("Legacy RAK4631 build-flag compatibility contract: PASS")
