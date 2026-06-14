# Spy-Bi-Wire (SBW) Analyzer Plugin for KingstVIS

A protocol analyzer plugin for decoding TI MSP430 **Spy-Bi-Wire (SBW)** 2-wire JTAG signals in the [KingstVIS](https://www.qdkingst.com/en/vis) logic analyzer application.

## Features

- Decodes SBW time-division multiplexed signals (TMS/TDI/TDO slots) from SBWTCK + SBWTDIO
- Full IEEE 1149.1 JTAG TAP state machine tracking (16 states)
- Shift-IR and Shift-DR data accumulation with named MSP430 instruction decoding
- CSV export of decoded JTAG transactions

## Prerequisites

1. Clone or download this repository
2. Download the KingstVIS Analyzer SDK from [https://www.qdkingst.com/download/vis_sdk](https://www.qdkingst.com/download/vis_sdk)
3. Extract the SDK archive to a temporary folder, then copy the `inc/` and `lib/` folders into the repository root:
   ```
   Kingst-Spi-Bi-Wire/
   ├── inc/          ← SDK headers (copied from SDK archive)
   ├── lib/          ← Prebuilt Analyzer library (copied from SDK archive)
   ├── SbwAnalyzer/
   └── ...
   ```

## Project Structure

```
inc/               — KingstVIS SDK headers (do not modify)
lib/               — Prebuilt Analyzer library per platform (Win32, Win64, Linux, Mac)
SbwAnalyzer/       — SBW analyzer plugin (4 source pairs)
  src/             — SbwAnalyzer, Settings, Results, SimulationDataGenerator
  vs2022/          — Visual Studio 2022 project/solution
  Linux/           — Linux Makefile
  Mac/             — macOS Makefile
```

## Building

| Platform | Command | Output |
|---|---|---|
| Windows | `msbuild SbwAnalyzer/vs2022/SbwAnalyzer.vcxproj /p:Configuration=Release /p:Platform=x64` | `SbwAnalyzer.dll` |
| Linux | `make -C SbwAnalyzer/Linux` | `libSbw.so` |
| macOS | `make -C SbwAnalyzer/Mac` | `libSbw.dylib` |

Or open `SbwAnalyzer/vs2022/SbwAnalyzer.sln` in Visual Studio 2022 and build.

## Protocol Reference

Detailed SBW protocol documentation (TDM slot structure, JTAG instruction set, init flows) is maintained in [AGENTS.md](AGENTS.md).

## License

GNU General Public License v2 — see [LICENSE](LICENSE).
