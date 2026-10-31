# LilyGO T-2CAN FD (H784-A): MCP2518FD SPI bench checks

This checklist applies to the **FD** board with an ESP32-S3 and an MCP2518FD.
It is for diagnosing a secondary controller that never returns register data.
It does **not** enable CAN1 or fix a defective board.

## Safety and scope

- Disconnect the board from the **vehicle and all CAN wiring**. Power only from USB during the firmware diagnostics.
- The `MCP2518FD_BENCH_DIAGNOSTICS` build keeps CAN1 disabled and forces CAN0 to Listen-Only. Do not disable those guards while troubleshooting.
- For continuity/short checks, unplug USB and any other power source first.
- Do not connect vehicle 12 V to SPI or 3.3 V test points. Do not bridge MOSI and MISO while the MCP2518FD is connected.
- A firmware log cannot measure the MCP2518FD's VDD. If you cannot identify its pins and the correct ground point, get the board checked by someone with suitable electronics equipment.

## Official H784-A pin map

| Signal | ESP32-S3 GPIO | MCP2518FD function |
| --- | ---: | --- |
| CS | 10 | SPI chip select |
| SCK | 12 | SPI clock |
| MOSI | 11 | SPI data into controller (SI) |
| MISO | 13 | SPI data out of controller (SO) |
| INT | 8 | Interrupt |
| INT0 | 9 | Additional interrupt; **not reset** |
| INT1 | 3 | Additional interrupt |

Sources: [LilyGO pin_config.h](https://github.com/Xinyuan-LilyGO/T-2Can/blob/main/libraries/private_library/pin_config.h), [FD schematic](https://github.com/Xinyuan-LilyGO/T-2Can/blob/main/project/T-2Can-Fd_V1.0.pdf), and [LilyGO CAN example](https://github.com/Xinyuan-LilyGO/T-2Can/blob/main/examples/can/can.ino).

## Interpret USB diagnostic output

1. `[CAN] BENCH GPIO SCK/MOSI/CS ... PASS (ESP32 pad only)` means the ESP32 can read back its commanded output levels. **It does not prove continuity to the MCP2518FD** or that the signal reaches the chip.
2. `[CAN] INIT SPI=... value=00000000` from every register/speed means no valid response was observed in that test; do not infer the chip's identity from zeros.
3. `[CAN] BENCH backend=0/1/2 ... RAM=FAIL` means bulk hardware SPI, byte-at-a-time hardware SPI, and software GPIO SPI all failed their write/readback test. This substantially reduces the likelihood of a PlatformIO SPI transfer API issue.
4. `[CAN] BENCH MISO weak-pull test: CS=HIGH down/up=0/1; CS=LOW down/up=0/1` means GPIO13 followed the ESP32's weak pulls in both states. MISO might be floating/undriven, **but this result alone does not prove a disconnected trace or dead chip**.

## Hardware tests in order

1. **With power removed**, inspect the board revision, solder joints and continuity of GPIO10/11/12/13 to the corresponding controller pins; check for shorts to GND or adjacent traces.
2. **With USB powering the disconnected board**, use the actual schematic and the MCP2518FD datasheet to find the correct supply and ground pins. Measure supply voltage **at the controller** and verify that its ground is shared with the ESP32. Do not assume the controller is powered just because the ESP32 boots.
3. Use a logic analyzer or oscilloscope on **CS, SCK, MOSI, MISO and GND**, preferably at the controller pins. During a raw SPI read, verify a low-going CS, clock on GPIO12, command bytes on GPIO11, and whether the controller drives GPIO13. Example read of `DEVID` at `0xE14` begins `3E 14` followed by four dummy bytes.
4. If a waveform is visible at the ESP32 pins but absent at the MCP2518FD, investigate board routing/soldering. If power and input waveforms reach the chip but MISO is never driven, suspect controller/reset/oscillator hardware and compare with the vendor's unmodified [Longan_CANFD example](https://github.com/Xinyuan-LilyGO/T-2Can/tree/main/examples/can) before attempting driver replacement.
5. Only after a controller returns plausible nonzero register values should firmware attempt normal MCP2518FD configuration. Keep the first bus test Listen-Only.

**Known beta.13 observation:** At 100/400/800 kHz, all three SPI backends returned zero for `CiCON`, `OSC` and `DEVID`; RAM patterns read back as zero. MISO followed weak pull-down and pull-up with CS selected and deselected. This is a hardware/power/signalling investigation, not evidence that SPI bit timing is wrong.
