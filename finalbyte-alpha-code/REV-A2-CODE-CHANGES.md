# FINALBYTE Weekend Alpha — Rev A.2 code alignment

Rev A.2 does **not** change the Rev A.1 hardware architecture. It aligns all
source code with that hardware and makes pin ownership explicit.

## Fixed

1. ESP32 host transport is the Rev A one-byte bridge only:
   - write `$DF00` = one byte,
   - read `$DF00` = status,
   - hardware READY clears on C64 write,
   - ESP32 acknowledges each consumed byte through GPIO39.
2. C64 `fb_send()` emits `0xFB + command + arg0..arg3 + data` through `$DF00`.
   The demo/game-facing API did not change.
3. Video SPI uses GPIO43 as the real active-low SPI CS. The hardware therefore
   gates FINALBYTE pixels only while a row transaction is physically active.
4. I2S BCLK is GPIO44, matching Rev A.1 hardware.
5. All ESP32 GPIO definitions are centralized in `esp32/main/board_pins.h`.
6. Removed obsolete `$DF07 CONTROL` comments from the protocol header.
7. Added `GPIO-AUDIT.md` matching code pin ownership to the Rev A.1 netlist.

## Deliberately still experimental

The HSYNC -> task notification -> calibrated delay -> polling SPI transaction
path remains an **Alpha measurement target**, not a production timing claim.
Stage D must scope-test horizontal jitter and missed/late rows. If it is not
stable enough, the next revision should move row launch into a more hardware-
driven path (e.g. GPTimer/RMT/LCD or dedicated logic) without changing the C64
transport or command API.
