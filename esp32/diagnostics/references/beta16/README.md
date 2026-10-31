# Beta.16 H784-A MCP2518FD: three hardware bench captures

Captured 2026-10-31 (device local timestamps 16:28–16:31); firmware reports
`v2.17-beta.16 standalone SPI diagnostics`. Source release:
[beta.16](https://github.com/nobless/flipper-tesla-fsd/releases/tag/v2.17-beta.16).

These are **three actual serial captures**, not three successful builds. Each
attached text file includes a short **tail from a previous test iteration before
the ESP-ROM reboot**. The per-boot matrix counts below start from the
`[FSD] Version:` line after that reboot. The original captures are preserved
verbatim.

## Files and observations

| Capture | USB reboot / run | Extra diagnostic | Matrix tests | Verified |
| --- | --- | --- | ---: | ---: |
| [1: Longan comparison](capture-1-longan-162839.txt) | 16:28:37 / 16:28:39 | `[VENDOR] begin=0 mode=0` but `verified=0`; duplicate `addApbChangeCallback` warning | 20 | 0 |
| [2: unlabelled variant](capture-2-162953.txt) | 16:29:51 / 16:29:53 | No `[VENDOR]` line in this capture | 20 | 0 |
| [3: unlabelled variant](capture-3-163050.txt) | 16:30:48 / 16:30:50 | No `[VENDOR]` line in this capture | 20 | 0 |

The release offers three standalone comparison images (plain matrix, LilyGO,
Longan). Only capture 1 explicitly identifies Longan. Because captures 2–3
do not identify a vendor variant in their serial output, **do not assume a
one-to-one mapping** to the other two images without a firmware-image hash
or a version/variant banner. A vendor `begin=0` return value is **not**
evidence the chip answered: the independent `verified` check failed.

## Aggregate results (after the reboot in each file)

- **60 test configurations, 60 reported `configuration=0 verified=0`.**
- Hardware FSPI: **36/36 failures** (12 configurations per capture).
- GPIO/software SPI: **24/24 failures** (8 configurations per capture).
- SPI modes 0 and 3, hardware at 10/100/400 kHz, software at nominal
  10/50 kHz; chip-select setup/hold/high delays of 2/20 us; RESET checks
  after 10/50/100 ms. No verified register response in any tested combination.
- **60/60** pull-assisted reads reported `follows-pulls=1`:
  `down=00000000/00000000/00000000` and
  `up=FFFFFFFF/FFFFFFFF/FFFFFFFF`. GPIO13/MISO is behaving as an
  undriven or high-impedance input under these test conditions.
- All reboot-time 10/50/100/500 ms startup samples read
  `CiCON=0 OSC=0`. Waiting longer after MCU boot did not make the
  controller respond. These samples are **not measurements of controller power**.
- A few irregular intermediate reads were seen in captures 1 and 3
  (examples `OSC=81F1FFFF`, `CON=E8FFFFFF`,
  `CON=FEFFFFFF OSC=E8FAF7ED`). They were not reproducible or
  accepted by verification. Treat as unvalidated/possibly floating-bus
  data, **not a device signature**.
- Capture 1 reports an ESP32
  `addApbChangeCallback(): duplicate func` warning in the Longan
  comparison, but the subsequent isolated raw SPI tests also fail.
  Investigate this warning separately; it does not establish why MISO
  is undriven.

## Conclusion and hardware next steps

**No software-side MCP2518FD initialization was verified in beta.16.**
Testing both vendor-library and independently clocked hardware/software SPI
reduces the probability of a simple SPI API/clock-mode mistake, but cannot
prove hardware failure.

Keep the board **disconnected from the vehicle**. With power removed,
check continuity/shorts on ESP32 GPIO10 (CS), GPIO12 (SCK), GPIO11
(MOSI) and GPIO13 (MISO) all the way to controller pins, using the
[LilyGO FD schematic](https://github.com/Xinyuan-LilyGO/T-2Can/blob/main/project/T-2Can-Fd_V1.0.pdf).
With USB power and appropriate probing, measure supply and ground
**at the MCP2518FD**. Then capture CS/SCK/MOSI/MISO with a common
ground and verify signal presence **at the controller**. A
logic analyzer can use the standalone firmware's `c` command:
OSC read TX `3E 00 00 00 00 00`, SPI mode 0 at 100 kHz, with
capture markers for synchronization.

These are observations, not proof of a dead controller or a firmware fix.
See [../README.md](../../README.md) for standalone diagnostics architecture
and [H784-A hardware checklist](../../../H784A_SPI_DIAGNOSTICS.md).
