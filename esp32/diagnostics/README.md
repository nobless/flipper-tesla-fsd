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
