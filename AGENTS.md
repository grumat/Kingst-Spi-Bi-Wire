# KingstVIS Analyzer SDK — SBW (Spy-Bi-Wire) Analyzer

SDK for building protocol analyzer plugins (shared libraries) loaded by the KingstVIS logic analyzer application. Active project: a **Spy-Bi-Wire (SBW)** decoder plugin for MSP430 2-wire JTAG.

## Architecture

- **`inc/`** — Public SDK headers (abstract base classes). Do not modify: binary compatibility with the closed-source `Analyzer` library is critical.
- **`lib/`** — Prebuilt `Analyzer` library per platform (`Win32/`, `Win64/`, `Linux/`, `Mac/`). Source not included.
- **`SbwAnalyzer/`** — SBW analyzer plugin (4 source pairs).
- **PDF guide** — English developer documentation in root (`KingstVIS Analyzer Developer Guide.pdf`, not machine-readable; ask user for excerpts).

## Plugin contract

Every analyzer DLL/SO/dylib must export three C-linkage functions:

```cpp
extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);
```

Each analyzer has 4 source pairs: `<Protocol>Analyzer`, `<Protocol>AnalyzerSettings`, `<Protocol>AnalyzerResults`, `<Protocol>SimulationDataGenerator`.

## Build commands

| Platform | Command | Output |
|---|---|---|
| Linux | `make -C SbwAnalyzer/Linux` | `libSbw.so` |
| macOS | `make -C SbwAnalyzer/Mac` | `libSbw.dylib` |
| Windows | `msbuild SbwAnalyzer/vs2022/SbwAnalyzer.vcxproj /p:Configuration=Release /p:Platform=x64` | `SbwAnalyzer.dll` |

Or open `SbwAnalyzer/vs2022/SbwAnalyzer.sln` in Visual Studio 2022 and build.

## Spy-Bi-Wire (SBW) protocol reference

Source: TI `slau320aj` (MSP430 Programming With the JTAG Interface).

### Physical layer

| SBW Signal | MSP430 Pin | Direction | Notes |
|---|---|---|---|
| SBWTCK | TEST/SBWTCK | OUT | Clock. Internally pulled low. Must not hold low >7µs or SBW deactivates. |
| SBWTDIO | RST/NMI/SBWTDIO | BIDIR | Shared with RST/NMI. Entry: pull high + first SBWTCK clock → 2-wire mode. |

- **Entry (2-wire)**: hold SBWTDIO high, apply first SBWTCK clock → SBW activated.  
- **Entry (4-wire on SBW-capable)**: hold SBWTDIO low, apply SBWTCK clock → 4-wire JTAG mode enabled.  
- **Exit**: hold SBWTCK low >100µs.  
- **SBWTCK frequency**: same as 4-wire TCK frequency. Flash erase/program requires TCLK = 350kHz ±100kHz (for 1xx/2xx/4xx).

### Time-division multiplexing (3 slots per SBWTCK cycle)

Each SBWTCK cycle encodes one of three JTAG signals via the TDM slot. The SBWTCK rising edge defines slot boundaries.

| Slot | Signal | SBWTDIO carries | Direction |
|---|---|---|---|
| **TMS** | TMS | TMS bit value | OUT (to target) |
| **TDI** | TDI | TDI bit value (or TCLK in Run-Test/Idle) | OUT (to target) |
| **TDO** | TDO | TDO bit value | IN (from target) |

- In **Run-Test/Idle** state, the TDI slot doubles as **TCLK** (CPU clock). Generate a full TCLK cycle requires two TDI slots: SetTCLK (TDI high) + ClrTCLK (TDI low).
- TMS slot must stay low during Run-Test/Idle to prevent state exit.
- **TDO read**: master releases SBWTDIO on SBWTCK rising edge of TDI slot; slave drives bus on next SBWTCK falling edge; master re-drives after TDO slot.

### Macros (each maps to specific TMS/TDI/TDO bit patterns)

| Macro | SBWTDIO sequence | Purpose |
|---|---|---|
| `TMSH` | SBWTDIO=H → SBWTCK↓ → SBWTCK↑ | TMS=1 |
| `TMSL` | SBWTDIO=L → SBWTCK↓ → SBWTCK↑ | TMS=0 |
| `TMSLDH` | SBWTDIO=L → SBWTCK↓ → SBWTDIO=H → SBWTCK↑ | TMS=0 with TDI prep for ClrTCLK/SetTCLK |
| `TDIH` | SBWTDIO=H → SBWTCK↓ → SBWTCK↑ | TDI=1 |
| `TDIL` | SBWTDIO=L → SBWTCK↓ → SBWTCK↑ | TDI=0 |
| `TDO_RD` | SBWTDIO=Hi-Z → SBWTCK↓ → read → SBWTCK↑ | Read TDO bit |
| `TDOsbw` | SBWTDIO=Hi-Z → SBWTCK↓ → SBWTCK↑ | Skip TDO (no read) |

### TAP state machine

Standard IEEE 1149.1 JTAG TAP controller. Key states: `Test-Logic-Reset → Run-Test/Idle → Select-DR-Scan → Capture-DR → Shift-DR → Exit1-DR → Update-DR` (and IR equivalents).

- **Fuse check**: after power-up, toggle TMS twice (two TMS slots, low phase ≥5µs). On SBW, this leaves TAP in Exit2-DR; need 2 dummy TCK cycles (TMS=H then TMS=L) to return to Run-Test/Idle.
- **ResetTAP**: ≥6 TCK clocks with TMS=H, then 1 TCK with TMS=L → Run-Test/Idle.

### JTAG instruction set

| Instruction | Code | Description |
|---|---|---|
| `IR_JMB_WRITE_32BIT_MODE` | 0x11 | JTAG mailbox 32-bit write mode (5xx/6xx) |
| `IR_ADDR_CAPTURE` | 0x21 | Read MAB into DR |
| `IR_DATA_PSA` | 0x22 | PSA (signature analysis) mode |
| `IR_EX_BLOW` | 0x24 | Execute fuse blow |
| `IR_CNTRL_SIG_CAPTURE` | 0x28 | Capture control signals into DR |
| `IR_EMEX_WRITE_CONTROL` | 0x30 | EMEX write control |
| `IR_FLASH_UPDATE` | 0x38 | Flash update |
| `IR_ADDR_LOW_BYTE` | 0x41 | Load MAB low byte from DR |
| `IR_DATA_CAPTURE` | 0x42 | Capture data into DR |
| `IR_PREPARE_BLOW` | 0x44 | Prepare fuse blow |
| `IR_JSTATE_ID` | 0x46 | JTAG state ID |
| `IR_CNTRL_SIG_LOW_BYTE` | 0x48 | Read/write control signal low byte |
| `IR_EMEX_READ_TRIGGER` | 0x50 | EMEX read trigger |
| `IR_TEST_REG` | 0x54 | Test register access |
| `IR_FLASH_CAPTURE` | 0x58 | Flash capture |
| `IR_CAPTURE_CPU_REG` | 0x61 | Capture CPU register |
| `IR_SHIFT_OUT_PSA` | 0x62 | Shift out PSA signature |
| `IR_ADDR_HIGH_BYTE` | 0x81 | Load MAB high byte from DR |
| `IR_DATA_16BIT` | 0x82 | Set MDB from DR (addressed by PC) |
| `IR_JMB_EXCHANGE` | 0x86 | JTAG mailbox exchange (5xx/6xx) |
| `IR_CNTRL_SIG_HIGH_BYTE` | 0x88 | Read/write control signal high byte |
| `IR_EMEX_DATA_EXCHANGE` | 0x90 | EMEX data exchange |
| `IR_CONFIG_FUSES` | 0x94 | Configure fuses |
| `IR_FLASH_16BIT_UPDATE` | 0x98 | Flash 16-bit update |
| `IR_DATA_TO_ADDR` | 0xA1 | Write MDB from DR (addressed by MAB) |
| `IR_DATA_16BIT_OPT` | 0xA2 | Optional 16-bit data access |
| `IR_CNTRL_SIG_RELEASE` | 0xA8 | Release CPU from JTAG control |
| `IR_EMEX_DATA_EXCHANGE32` | 0xB0 | EMEX 32-bit data exchange |
| `IR_ADDR_16BIT` | 0xC1 | Load MAB from DR_SHIFT16/20 |
| `IR_DATA_QUICK` | 0xC2 | Auto-incrementing access via PC |
| `IR_CNTRL_SIG_16BIT` | 0xC8 | Read/write control signal register |
| `IR_EMEX_READ_CONTROL` | 0xD0 | EMEX read control |
| `IR_FLASH_16BIT_IN` | 0xD8 | Flash 16-bit in |
| `IR_DEVICE_ID` | 0xE1 | Read device ID |
| `IR_DTA` | 0xE2 | Direct memory access |
| `IR_CORE_IP_ID` | 0xE8 | Read core IP ID |
| `IR_TEST_3VREG` | 0xF4 | Test 3V regulator |
| `IR_BYPASS` | 0xFF | Bypass (also releases CPU) |

- IR: 8-bit, **LSB first**.
- DR (data register): 16-bit for data, 20-bit for MSP430X addresses, **MSB first**.
- JTAG ID readback on every IR_SHIFT: 0x89 for most 1xx/2xx/4xx, 0x91/0x98/0x99 for FRxx.

### CPU control register (1xx/2xx/4xx)

| Bit | Name | Description |
|---|---|---|
| 0 | R/W | 1=Read, 0=Write |
| 3 | HALT_JTAG | 1=CPU stopped |
| 4 | BYTE | 1=Byte, 0=Word access |
| 7 | INSTR_LOAD | RO: 1=Instruction fetch |
| 9 | TCE | RO: 1=CPU synchronized |
| 10 | TCE1 | 1=CPU under JTAG control |
| 11 | POR | 1=Perform POR |
| 12 | RELEASE_LOW | 1=CPU controls RW/BYTE |

### CPU control register (5xx/6xx)

| Bit | Name | Description |
|---|---|---|
| 0 | R/W | 1=Read, 0=Write |
| 3 | WAIT | RO: 1=CPU clock stopped |
| 4 | BYTE | 1=Byte, 0=Word |
| 7 | INSTR_LOAD | RO: fetch state |
| 8 | CPUSUSP | 1=CPU suspended (pipeline empty) |
| 9 | TCE0 | RO: sync |
| 10 | TCE1 | 1=CPU under JTAG control |
| 11 | POR | 1=POR |

### Typical init flow

```
1. SBW entry: SBWTDIO=H, SBWTCK↑ (first clock)
2. ResetTAP: 6×(TMS H) → 1×(TMS L)  [each = TMS slot]
3. Fuse check: TMS toggle ×2 (≥5µs low each)
4. Get JTAG ID: IR_SHIFT(IR_ADDR_16BIT or any)
5. CPU control: IR_SHIFT(IR_CNTRL_SIG_16BIT) → DR_SHIFT16(0x2401) → verify TCE bit
6. If 5xx/6xx: SyncJtag_AssertPor, GetCoreipIdXv2, GetDeviceID
```

### Memory access flow (read)

```
IR_SHIFT(IR_CNTRL_SIG_16BIT) → DR_SHIFT16(0x2409)  [R/W=Read, HALT]
IR_SHIFT(IR_ADDR_16BIT) → DR_SHIFT16(address)
IR_SHIFT(IR_DATA_TO_ADDR)
SetTCLK → ClrTCLK                         [clock data onto MDB]
DR_SHIFT16(0x0000)                         [read value on TDO]
```

### Flash write flow (1xx/2xx/4xx, via flash controller)

```
HaltCPU → IR_CNTRL_SIG_16BIT(0x2408)      [R/W=Write]
FCTL1=0xA540                               [enable write]
FCTL2=0xA540                               [MCLK/1]
FCTL3=0xA500                               [clear LOCK]
for each word:
  IR_ADDR_16BIT(address) → IR_DATA_TO_ADDR(data) → SetTCLK → ClrTCLK
  repeat 35 TCLK cycles                    [flash program timing at 350kHz]
FCTL1=0xA500                               [disable write]
FCTL3=0xA510                               [set LOCK]
```

## Reference implementation (Saleae, GPL v2)

A reference SBW analyzer by Fredrik Ahlberg (2013) for Saleae Logic SDK 2.0 exists (not in this repo). It is **GPL v2 licensed** — do not copy code, use only as structural/protocol reference.

Key implementation patterns to note:
- **`SbwState` enum**: `SbwTMS → SbwTDI → SbwTDO → SbwTMS` — tracks which TDM slot the next SBWTCK edge belongs to.
- **`JtagState` enum**: Full 16-state IEEE 1149.1 TAP controller.
- **Worker loop**: On each SBWTCK rising edge, call `ProcessStep()` which reads the slot type, captures data, then advances slot. After the rising→falling edge pair, check for timeout (>7µs low → SBW deactivated).
- **Timeout**: `GetSampleRate() / 14286` ≈ 7µs. **TDO skip**: `GetSampleRate() / 1e7` ≈ 100ns delay before sampling TDIO in TDO slot.
- **Frame creation**: One frame per JTAG state transition. `mFlags` stores the JTAG state. `mData1` = TDI shifted data, `mData2` = TDO shifted data.
- **IR decode**: Large table of MSP430 instruction names mapped to 8-bit opcodes.
- **Settings archive string**: Uses `"KongoSbwAnalyzer"` as identifier in `SaveSettings()`/`LoadSettings()`.
- **Simulation**: Not implemented in reference (`#if 0`).

## Key constraints

- **C++03/11 required** — Code uses `std::auto_ptr` (removed in C++17). Do not migrate to `std::unique_ptr` without verifying the closed-source `Analyzer` library headers align.
- **Do not reorder member variables** in classes that inherit from SDK base types (`Channel`, `SimulationChannelDescriptor`, `Analyzer`, `AnalyzerResults`, etc.). Binary layout is serialized between the plugin and KingstVIS host.
- **New virtual methods** on `Analyzer` base class must be appended at the bottom (after existing ones) for binary compatibility.
- **Two export macros**: `LOGICAPI` (for the SDK library itself), `ANALYZER_EXPORT` (for plugin analyzers).
- **`snprintf` compat**: `LogicPublicTypes.h` `#define`s `snprintf` → `_snprintf_s` on MSVC.
- **Windows toolset**: MSVC 2013 (`v120_xp`), targeting Windows XP. Retarget if using a newer VS.

## Makefile quirks

- Linux/Mac makefiles have a typo: `$(HFILe)` instead of `$(HFILE)` in dependency lines. Dependency tracking is broken but builds still succeed.

## Absent from this repo

- No tests, test framework, or test infrastructure
- No CI/CD
- No linting/formatting config
