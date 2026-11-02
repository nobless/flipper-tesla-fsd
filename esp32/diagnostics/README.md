# Standalone H784-A SPI diagnostics

USB bench only; disconnect the vehicle. These images replace the dashboard application.
No WiFi, filesystem, TWAI, vehicle features or CAN send calls are included. Pins 6/7 and
interrupt pins 3/8/9 are untouched. Only CS10/SCK12/MOSI11/MISO13 are used.

The default `tesla-fsd-lilygo-t2can-fd-merged.bin` runs an isolated SPI test matrix.
The `-lilygo-merged.bin` and `-longan-merged.bin` variants additionally run their pinned
upstream driver before the same matrix. Both drivers are requested to stay in Configuration
mode. Driver return codes are printed but never accepted as proof of successful communication.
The pinless upstream SPI.begin() is preceded by explicit FSPI pin setup; independent verification
reattaches the same pins afterward. Drivers are not patched or silently substituted.

The installer serves the default image. Download comparison images from the release and flash
at offset 0 with a compatible ESP32-S3 flashing tool. Do not use an app-only image at offset 0.

Serial: 115200 baud. Wait for the test matrix to complete (it can take several seconds).
- `r`: repeat all raw tests.
- `c`: toggle one OSC register read per second, framed by CAPTURE BEGIN/END serial markers.
- Reset: repeat startup and, in comparison variants, the vendor initialization.

Matrix: FSPI at 10/100/400 kHz, software SPI at nominal 10/50 kHz (GPIO overhead lowers rate),
modes 0/3, CS setup/hold/high gaps of 2/20 us, reset waits of 10/50/100 ms. Startup also samples
registers after cumulative additional waits of 10/50/100/500 ms; these are MCU-relative delays,
not measurements of controller power-on. Weak pulls are applied while reading CiCON three times.

VERIFIED requires stable nonconstant CiCON in Configuration mode, RAM readback of four patterns,
and successful restoration. It does not establish actual CAN reception, bit timing, oscillator
accuracy or physical controller identity. DEVID and OSC are raw observations; zeros alone are
inconclusive. No RAM write is attempted unless Configuration mode has been confirmed.

For logic analyzer: probe controller-side CS/SCK/SDI/SDO with a common ground; capture mode reads
OSC with TX bytes 3E 00 00 00 00 00 in SPI mode0, 100 kHz, MSB first. The first two RX bytes are
command-phase data and must not be interpreted as register contents.

Pinned sources and Git blob integrity checks are in vendor.py and vendor-lock.json.
Build: pio run -e lilygo-t2can-fd-spi-diag -e lilygo-t2can-fd-spi-lilygo -e lilygo-t2can-fd-spi-longan

## Field reference logs

[Beta.16: three USB-only SPI captures and analysis](references/beta16/README.md) document repeated all-zero/all-one register reads, 60 failed verification combinations, and undriven MISO observations.


## Beta 17: independent Battery-Emulator backend

The fourth installer image (`battery`) uses the complete MCP2518FD_Lite and
AsyncSpiBus implementation pinned in be-lock.json to Battery-Emulator commit
5d4ead79f8cbaa8496df26cb61225259908dc52b. This includes the final shared-bus
race fix from PR3077 and the driver state after PR3136. Sources are verified
against their Git blob hashes at build time; the upstream GPLv3 license is
retained. A Serial logging shim and explicit IDF 4.4 compatibility replacements are
applied by be_vendor.py: the peripheral-claim header name, typed GPIO casts,
the equivalent master-start call (which applies config before starting), and
workarounds for IDF 4.4 SPI header C++ incompatibilities (unused enum operators
inside extern-C and the SPI_LL_GET_HW null branch).
The controller initialization and PR3077 race fix are unchanged.

This image owns SPI2_HOST directly, with SCK12/MOSI11/MISO13/CS10, and never
constructs Arduino SPIClass. GPIO3/8/9 remain untouched. No interrupt pin is
attached. It first reads registers at 100k/400k/1MHz, then calls the upstream
initialization with a 40MHz oscillator and 500k nominal/data rates, requesting
internal loopback. No CAN frames are sent. Three matching CON reads must
confirm mode 2; a successful begin alone is not accepted as verification.
After success, the driver is paused in Configuration mode. Commands: `r` retry,
`c` toggle one-second raw register snapshots. This tests initialization only;
it does not validate physical CAN wiring, interrupts, or asynchronous traffic.

### Fork review

- The original driver's SPI argument order matches Arduino's SCK/MISO/MOSI/CS
  order. Board pin assignments agree with the LilyGO FD pin configuration.
- The application bench option deliberately leaves can1 stopped even when a
  probe succeeds. It is not a usable vehicle CAN firmware configuration.
- Standalone diagnostics contain no WiFi, filesystem, TWAI or Tesla logic;
  failures there cannot be attributed to those application components.
- Beta 17 checks the hardware SPI begin result and stops if peripheral setup
  fails. Software mode 0 now gives MOSI an explicit setup interval before the
  rising sampling edge. Neither change is a proven explanation for earlier
  all-zero reads.
- PR3136's second external controller pin fix does not directly describe this
  board: it has one external MCP2518FD and one internal TWAI controller.
- Host tests and compilation cannot establish that the physical MCP2518FD
  responds. Previous zero/all-one reads and pull-following MISO results remain
  inconclusive about the exact electrical or software cause.

References: https://github.com/dalathegreat/Battery-Emulator/pull/3077 and
https://github.com/dalathegreat/Battery-Emulator/pull/3136
