# Spy-Bi-Wire (SBW) Analyzer Plugin for KingstVIS

A protocol analyzer plugin for decoding TI MSP430 **Spy-Bi-Wire (SBW)** 2-wire JTAG signals in the [KingstVIS](https://www.qdkingst.com/en/vis) logic analyzer application.

## Features

- Decodes SBW time-division multiplexed signals (TMS/TDI/TDO slots) from SBWTCK + SBWTDIO
- Full IEEE 1149.1 JTAG TAP state machine tracking (16 states)
- Shift-IR and Shift-DR data accumulation with named MSP430 instruction decoding
- CSV export of decoded JTAG transactions

## Project Structure

```
inc/               — KingstVIS SDK headers (do not modify)
lib/               — Prebuilt Analyzer library per platform (Win32, Win64, Linux, Mac)
SbwAnalyzer/       — SBW analyzer plugin (4 source files)
  src/             — SbwAnalyzer, Settings, Results, SimulationDataGenerator
  vs2022/          — Visual Studio 2022 project/solution
  Linux/           — Linux Makefile
  Mac/             — macOS Makefile
SerialAnalyzer/    — Example: Serial (UART) analyzer
SpiAnalyzer/       — Example: SPI analyzer
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

MIT — see [LICENSE](LICENSE).
