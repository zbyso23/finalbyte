# FINALBYTE Alpha — Weekend Hardware
## C64 PAL reference build, Rev A.1a

**Purpose:** smallest practical hardware that lets the existing FINALBYTE Alpha demo prove three things at once:

1. C64 -> FINALBYTE command transport
2. stable PAL line/frame synchronization
3. binary overlay (`host` / `FINALBYTE`) plus external audio

This is deliberately a **prototype**, not a production cartridge. **Rev A.1a adds explicit voltage domains, a 3.3 V/open-drain READY acknowledge path, and a pin-level weekend netlist.** It assumes a later 8-pin C64 A/V connector with separate luma and chroma. It does not digitize the host video and it does not generate composite/PAL from scratch.

---

# 1. What the weekend build actually does

The C64 keeps generating its normal picture and SID sound.

FINALBYTE adds:

- a write/read port at `$DF00`
- a synchronized 1-bit overlay mask
- one fixed overlay luminance level (white/gray, adjustable)
- chroma blanking wherever the overlay pixel is opaque
- I2S audio through a PCM5102A module

The output is **S-Video/Y-C**, not recombined composite.

```text
C64 expansion port                       ESP32-S3
     |                                      |
     | D0..D7                               | commands
     +--> 8-bit latch --------------------->|
     |                                      |
     | /IO2, R/W --> write/read logic       |
     |                    |                 |
     |                    +--> READY <------+
     |                                      |
     |<----------- status buffer -----------+

C64 A/V pin 1 Y ----+-----------------------------> video mux ----> S-Video Y out
                    +--> LM1881 --> H/V sync --> ESP32

C64 A/V pin 6 C ---------------------------------> chroma gate ---> S-Video C out

ESP32 MASK + SPI-CS --> binary select logic ------> video mux/gate
ESP32 I2S ----------------------------------------> PCM5102A ------> audio out
```

---

# 2. Reference board

Use an **ESP32-S3-DevKitC-1 with ESP32-S3-WROOM-1, N8 / 3.3 V GPIO** for the first build.

Avoid WROOM variants where GPIO47/48 are tied to a 1.8 V VDD_SPI domain. The Alpha firmware intentionally does not require PSRAM.

Power the ESP32 board from its own USB supply. The C64-side 5 V logic may use the cartridge port +5 V. **Common the grounds. Do not hot-plug the cartridge or A/V wiring.**

---

# 3. Bill of materials

## Core

| Qty | Part | Purpose |
|---:|---|---|
| 1 | ESP32-S3-DevKitC-1 (WROOM-1 N8) | FINALBYTE firmware |
| 1 | SN74LVC574A, preferably DIP-20 | C64 write-byte latch, 5 V tolerant inputs, 3.3 V outputs |
| 1 | 74HCT74, DIP-14 | hardware READY flip-flop |
| 1 | 74AHCT245 or 74HCT245, DIP-20 | 3.3 V/5 V status bus driver to C64 |
| 1 | 74HCT32 or 74AHCT32, DIP-14 | write/read qualification logic; TTL-compatible C64 inputs |
| 1 | 74HCT04, DIP-14 | R/W and SPI-CS inversion |
| 1 | 74HCT08 or 74AHCT08, DIP-14 | mask AND active-video gate |
| 1 | LM1881N, DIP-8 | sync recovery from C64 luma+sync |
| 1 | CD74HC4053E, DIP-16 | analog Y/C switching |
| 1 | PCM5102A I2S DAC breakout | Alpha audio output |
| 1 | 2N3904 / BC547 | low-impedance fixed luma reference |
| 1 | C64 cartridge edge breakout | safe access to expansion port |
| 1 | 8-pin DIN C64 A/V plug/breakout | Y/C input |
| 1 | 4-pin mini-DIN S-Video socket/breakout | Y/C output |

## Passives

- 100 nF ceramic at every logic IC supply pair
- 10 uF near ESP32/logic rails
- LM1881: 0.1 uF input coupling capacitor, 680 kΩ RSET, 0.1 uF from RSET node as shown in TI reference circuit
- 10 kΩ / 20 kΩ divider for the 5 V write-strobe -> ESP32 GPIO12
- 10 kΩ pull-up from `READY_PRE_N` to **+3.3 V**
- 10 kΩ trimmer for FINALBYTE luma level
- 1 kΩ base resistor for 2N3904
- 1 kΩ emitter-to-ground resistor for the luma reference source
- 330 Ω to 1 kΩ series resistor in the C64 chroma path; start at 1 kΩ
- optional 33–68 Ω series resistors on SPI MASK/SCLK/CS if breadboard ringing is visible

---


# 3.1 Power and logic domains

| Device / block | Supply | Logic domain | Notes |
|---|---:|---|---|
| ESP32-S3 DevKit | USB / board regulator | 3.3 V | GPIOs are **not 5 V tolerant** |
| SN74LVC574A | 3.3 V | 3.3 V outputs; 5 V-tolerant inputs | C64 D0..D7 and `WRITE_N` may drive its inputs directly |
| 74HCT32 / 74AHCT32 | 5 V | TTL-compatible 5 V logic | receives C64 `/IO2`, `R/W`; also receives 5 V HCT signals |
| 74HCT74 | 5 V | TTL-compatible 5 V logic | `/PRE` high level comes from 3.3 V pull-up; valid for HCT |
| 74HCT04 | 5 V | TTL-compatible 5 V logic | accepts C64 and 3.3 V ESP32 control signals |
| 74HCT08 / 74AHCT08 | 5 V | TTL-compatible 5 V logic | accepts 3.3 V `MASK_OUT` / `MASK_ACTIVE` |
| 74AHCT245 / 74HCT245 | 5 V | TTL-compatible inputs, 5 V outputs | translates ESP32 status to C64 data bus |
| LM1881 | 5 V | 5 V output logic | outputs go to ESP32 only through resistor dividers |
| CD74HC4053 | 5 V, VEE=0 V | 5 V control / analog Y-C | Rev A.1a candidate; validate on scope |
| PCM5102A breakout | board-dependent | 3.3 V I2S-compatible | verify the exact module supply/jumper configuration |

**Domain rule:** no raw 5 V logic output is connected to an ESP32 pin. The deliberate exceptions are inputs of the 3.3 V SN74LVC574A, whose specified inputs tolerate 5.5 V.

# 4. C64 expansion-port signals used

The weekend build uses only these C64 signals:

| C64 edge pin | Signal | FINALBYTE use |
|---|---|---|
| 10 | `/IO2` | `$DF00-$DFFF` select |
| 5 | `R/W` | distinguish read/write |
| 14..21 | `D7..D0` | command/status byte |
| 2 or 3 | `+5 V` | C64-side TTL logic only |
| 1 / 22 / A / Z | GND | common ground |

No GAME/EXROM/ROM lines are required. The Alpha therefore does **not** replace BASIC/KERNAL/cartridge ROM.

For Rev A, any access in `$DFxx` is seen by the byte bridge; the demo only accesses `$DF00`. Full A0..A7 decoding can be added later.

---

# 5. C64 -> ESP32 write path

## 5.1 Write qualification

Define:

```text
WRITE_N = /IO2 OR R/W
```

Using one gate of **74HCT32/AHCT32**:

```text
/IO2 ----\
          OR ---- WRITE_N
R/W  ----/
```

`WRITE_N` is low only during a C64 write to IO2. It rises at the end of the write cycle.

Use `WRITE_N` for two things:

1. **SN74LVC574A CLK** — latches C64 D0..D7 on the rising edge.
2. **74HCT74 /CLR** — clears hardware READY while the C64 is writing.

The SN74LVC574A is powered from **3.3 V**. Its D inputs are connected directly to C64 D0..D7; this specific LVC family accepts 5 V input levels while powered at 3.3 V. Tie `/OE` low.

Map latch outputs to ESP32:

```text
Q0 -> GPIO4
Q1 -> GPIO5
Q2 -> GPIO6
Q3 -> GPIO7
Q4 -> GPIO8
Q5 -> GPIO9
Q6 -> GPIO10
Q7 -> GPIO11
```

## 5.2 ESP32 strobe

Feed `WRITE_N` to ESP32 GPIO12 through a divider:

```text
WRITE_N -- 10k --+--> GPIO12
                 |
                20k
                 |
                GND
```

GPIO12 triggers on the positive edge. By this moment the external latch already holds the byte, so ESP32 interrupt latency is irrelevant to the C64 bus timing.

---

# 6. Hardware READY handshake

Use one half of **74HCT74** as an asynchronous SR-like READY latch.

```text
Q      = READY
/CLR   = WRITE_N
/PRE   = READY_PRE_N from ESP32 GPIO39
D      = GND
CLK    = GND
```

Operation:

```text
C64 begins write    -> WRITE_N = 0 -> /CLR = 0 -> READY = 0
C64 ends write      -> byte is latched
ESP32 copies byte
ESP32 pulses GPIO39 low
                    -> /PRE = 0 -> READY = 1
```

Put a **10 kΩ pull-up** from `/PRE` to **+3.3 V**. Configure ESP32 GPIO39 as **open-drain**: driving low asserts `/PRE`; releasing the pin lets the 3.3 V pull-up produce a valid HIGH for the 5 V HCT74. This avoids exposing an ESP32 GPIO to a 5 V pull-up. A small NPN/open-collector stage may be substituted for additional isolation.

### READY startup sequence

Do **not** assume a defined 74HCT74 Q state after power-up. Firmware shall establish READY explicitly after GPIO initialization:

```text
configure GPIO39 as open-drain and release it
configure WRITE_N input
wait until WRITE_N reads HIGH
pulse READY_PRE_N LOW briefly
release READY_PRE_N
=> READY = 1
```

This sequence is part of the Rev A.1a bring-up contract.

Do not allow `/PRE` and `/CLR` low at the same time. Firmware only sends ACK after the write cycle has ended.

---

# 7. ESP32 -> C64 status read path

Use a **74AHCT245 / 74HCT245 powered from +5 V**.

- `DIR`: fixed ESP32/status side -> C64 data side
- `/OE`: enabled only during an IO2 read

Generate:

```text
RW_N = NOT(R/W)                 (74HCT04)
STATUS_OE_N = /IO2 OR RW_N      (74HCT32/AHCT32)
```

Thus the 245 drives the C64 data bus only when `/IO2=0` and `R/W=1`.

Status A-side:

```text
A0 <- hardware READY Q
A1 <- ESP32 GPIO13   BUSY
A2 <- ESP32 GPIO14   ERROR
A3 <- ESP32 GPIO15   VIDEO_LOCK
A4 <- ESP32 GPIO16   AUDIO
A5 <- ESP32 GPIO17   reserved
A6 <- ESP32 GPIO18   reserved
A7 <- ESP32 GPIO21   reserved
```

B0..B7 go to C64 D0..D7 respectively.

The C64 therefore sees one bidirectional logical port:

```text
write $DF00 : one outgoing byte
read  $DF00 : status
```

The high-level command packet is:

```text
FB CMD ARG0 ARG1 ARG2 ARG3 DATA
```

where `FB` is marker `$FB`.

---

# 8. Video input and sync recovery

Use a later C64 8-pin A/V connector:

```text
pin 1 = luminance + sync (Y)
pin 2 = GND
pin 6 = chrominance (C)
```

The original C64 Y is also the source for sync recovery.

## LM1881

Use the TI reference connection:

```text
LM1881 pin 8 -> +5 V
LM1881 pin 4 -> GND
C64 Y -> 0.1 uF -> pin 2 CVIN
pin 6 RSET -> 680 kΩ to GND
pin 6 -> 0.1 uF to GND
pin 1 CSOUT -> divider -> ESP32 GPIO1 (HSYNC/composite-sync edge)
pin 3 VSOUT -> divider -> ESP32 GPIO2 (VSYNC)
```

For each LM1881 output, use the same simple 5 V -> 3.3 V divider:

```text
LM1881 out -- 10k --+--> ESP32
                    |
                   20k
                    |
                   GND
```

The Alpha firmware uses one composite-sync edge per line as `HSYNC`; it does not require a separately reconstructed analog H pulse.

---

# 9. Binary Y/C overlay

## 9.1 Why S-Video output

Rev A keeps luminance and chroma separate. That avoids a PAL encoder and is the shortest path to proving stable overlay timing.

`0` mask bit means **host Y/C passes unchanged**.

`1` mask bit means:

- replace host Y with adjustable FINALBYTE fixed luminance
- replace host C with 0 V / no chroma

The result is an opaque monochrome/gray FINALBYTE pixel over the original color C64 picture.

## 9.2 Active gate

Firmware outputs:

```text
GPIO3  = MASK_OUT (SPI MOSI)
GPIO42 = MASK_SCLK
GPIO43 = MASK_CS, active low during each 256-bit active-row burst
GPIO38 = OVERLAY_ARM, firmware enable; external 10 kΩ pull-down keeps it LOW at reset
```

Invert CS with one **74HCT04** gate:

```text
MASK_ACTIVE = NOT(MASK_CS)
```

Then use two gates of **74HCT08/AHCT08**:

```text
MASK_PIXEL = MASK_OUT AND MASK_ACTIVE
FB_SELECT  = MASK_PIXEL AND OVERLAY_ARM
```

`OVERLAY_ARM` has a **10 kΩ pull-down to GND** and is driven from ESP32 GPIO38. This gives a hardware-default OFF state during reset, ROM boot, JTAG/UART activity, or before firmware configures GPIO3/GPIO43. Only after SPI and sync GPIOs are initialized may firmware drive `OVERLAY_ARM=1`.

This is important: outside an actual SPI row burst, or before firmware explicitly arms the overlay, `FB_SELECT` is forced to zero regardless of the last MOSI bit. Therefore sync and blanking remain the original C64 signal.

## 9.2.1 ESP32-S3 boot-state rule

Some selected ESP32-S3 pins have ROM-boot, strapping, UART0, or JTAG-related behavior. Rev A.1a therefore does **not** rely on their reset state for video safety. The external `OVERLAY_ARM` pull-down forces the overlay path off until application firmware explicitly arms it.

Reference firmware order:

```text
1. OVERLAY_ARM remains LOW by hardware pull-down
2. configure OVERLAY_ARM output LOW
3. initialize SPI MASK_OUT / MASK_CLK / MASK_CS
4. initialize HSYNC / VSYNC inputs and ISR handlers
5. clear framebuffer / establish safe mask state
6. drive OVERLAY_ARM HIGH
```

The reference build remains limited to the documented ESP32-S3-WROOM-1 N8 / compatible 3.3 V GPIO configuration; do not substitute a module variant with incompatible VDD_SPI requirements without re-auditing GPIO47/48.

## 9.3 CD74HC4053 — Rev A.1a candidate

**The CD74HC4053 is the Rev A.1a candidate analog switch and is explicitly subject to oscilloscope validation.** Stage C must verify passthrough amplitude, edge quality, sync integrity and visible ringing before overlay operation is accepted. If the passthrough is poor, replace the switch rather than compensating in firmware.

Power the 4053 from +5 V, VEE=GND, INH=GND.

Use two switch sections with their selects tied to `FB_SELECT`:

```text
X0 = C64 Y
X1 = FB_Y_REFERENCE
X common -> S-Video Y output

Y0 = C64 C
Y1 = GND
Y common -> S-Video C output
```

The third section is unused.

Start with **1 kΩ series resistance in the incoming C64 chroma path**. The C64 chroma level is often higher than modern S-Video expectations; adjust downward only after viewing the result.

---

# 10. FINALBYTE fixed-luma source

For Alpha we need only one opaque brightness, so do not build a DAC.

Use a 2N3904 emitter follower:

```text
+5V ---- 10k trim ---- GND
              |
            wiper
              |
             1k
              |
          2N3904 base

collector -> +5V
emitter   -> FB_Y_REFERENCE
emitter   -> 1k -> GND
```

Adjust the trimmer while looking at the terminated S-Video Y output on a scope/monitor. Start dark and increase until the FINALBYTE rectangle is comfortably bright without clipping.

This is intentionally a calibration control, not a production video DAC.

---

# 11. Audio

Use a common **PCM5102A I2S DAC breakout**.

Firmware pin mapping:

```text
ESP32 GPIO44 -> BCK / BCLK
ESP32 GPIO47 -> LRCK / WS
ESP32 GPIO48 -> DIN
GND           -> GND
5V or 3.3V    -> module supply according to the specific breakout
```

Use the DAC module line output directly into powered speakers / amplifier / monitor audio input.

For the weekend demo, do **not** mix it electrically back into the C64 SID output. Play SID from the C64 and FINALBYTE audio through separate mixer inputs/speakers if desired.

---

# 12. ESP32 pin summary

| GPIO | Function |
|---:|---|
| 1 | sync line input from LM1881 CSOUT |
| 2 | VSYNC input from LM1881 VSOUT |
| 3 | video mask MOSI |
| 4..11 | latched C64 byte D0..D7 |
| 12 | write-strobe input |
| 13 | status bit 1 BUSY |
| 14 | status bit 2 ERROR |
| 15 | status bit 3 VIDEO_LOCK |
| 16 | status bit 4 AUDIO |
| 17,18,21 | reserved status bits 5..7 |
| 39 | READY acknowledge pulse, active low |
| 42 | video mask SPI clock |
| 43 | video mask SPI CS |
| 44 | I2S BCLK |
| 47 | I2S WS |
| 48 | I2S DATA |

These mappings match the accompanying **Weekend** firmware bundle.

---

# 13. Bring-up order

Do not assemble the entire system and debug it at once.

## Stage A — bus only

1. Leave video and audio disconnected.
2. Power C64-side logic and ESP32.
3. Flash FINALBYTE firmware.
4. On a scope/logic analyzer verify `$DF00` write produces:
   - `WRITE_N` low during write
   - rising edge at end
   - LVC574 output equals written byte
   - READY falls during write and rises after ESP32 ACK
5. Run C64 demo and verify packets arrive without VIDEO connected.

**Pass condition:** C64 animation keeps running and FINALBYTE command parser reports no framing/error condition.

## Stage B — sync only

1. Connect C64 Y to LM1881 through 0.1 uF.
2. Verify CSOUT is line-rate and VSOUT is frame-rate.
3. Verify GPIO1/2 remain below 3.3 V after dividers.
4. Confirm firmware `VIDEO_LOCK` bit appears in `$DF00` reads.

## Stage C — passive video path

1. Keep `FB_SELECT=0` permanently.
2. Connect Y/C through the 4053.
3. Confirm picture is indistinguishable enough from the direct S-Video feed.
4. If not, debug this before enabling any overlay.

## Stage D — static overlay

1. Enable mask output.
2. Draw only one rectangle.
3. Tune `ACTIVE_DELAY_US` and `VISIBLE_FIRST_PAL_LINE` with scope/image.
4. Tune fixed luma trimmer.

**Pass condition:** stationary FINALBYTE rectangle remains locked to the same C64 raster location with no tearing.

## Stage E — full demo

Run the supplied C64 program:

- C64 draws its own text/marker
- C64 SID produces its own beep
- C64 moves the FINALBYTE sprite through `$DF00`
- FINALBYTE overlay stays raster locked
- SPACE triggers FINALBYTE I2S sample

At this point Alpha has proved the architecture.

---

# 14. Expected tuning values

The firmware intentionally contains two board-dependent constants:

```c
#define VISIBLE_FIRST_PAL_LINE 50
#define ACTIVE_DELAY_US        10
#define PIXEL_CLOCK_HZ         6000000
```

Do not treat the first two as final values. Tune them on the actual C64 + LM1881 + breadboard combination.

At 6 MHz, 256 mask pixels occupy about **42.7 us** of a PAL line, leaving enough room inside the active picture for a centered Alpha overlay region.

---

# 15. Things that are intentionally not solved in Rev A

- true 320-pixel C64-wide calibration
- color FINALBYTE pixels
- alpha levels other than 0/1
- universal NTSC timing
- A0..A7 decode for only `$DF00`
- Atari/ZX electrical adapters
- C64 cartridge pass-through
- mixing FINALBYTE audio into SID output
- EMC/ESD/production protection
- PCB impedance / proper video layout

The purpose of Rev A.1a is not polish. It is to answer one question:

> Can an ESP32-based FINALBYTE receive commands from a live C64 and place its own opaque pixels and sound in deterministic cooperation with that C64?

If Stage E passes, the answer is yes.

---

# 16. Practical warnings

- Build and connect everything with both devices powered off.
- Do not connect raw 5 V C64 signals directly to ESP32 GPIO.
- Do not power the ESP32 from the C64 cartridge +5 V rail for this prototype.
- Verify the orientation of the C64 edge connector twice; reversed cartridge wiring can damage hardware.
- Verify every IC package pinout against the exact manufacturer datasheet before power-up.
- Put a 100 nF decoupling capacitor at every TTL/CMOS IC.
- Keep the Y/C analog wiring short and away from ESP32/SPI wiring.
- A breadboard is acceptable for bus bring-up; for the final video stage, perfboard or a tiny PCB is strongly preferable because a 6 MHz switching mask next to analog video is noisy.


# 17. Rev A.1a pin-level wiring / netlist

This section is the build-level reference for the weekend prototype. Pin numbers below assume the listed **PDIP packages, top view**. Verify the exact manufacturer/package datasheet before soldering. `NC` means leave unconnected unless otherwise stated. All unused digital inputs are tied to a defined level; do not leave CMOS inputs floating.

## 17.1 Named nets

```text
+5V_C64       C64 cartridge +5 V; powers 5 V logic only
+3V3          ESP32 DevKit 3.3 V rail; powers U1 and READY pull-up
GND           common C64 / ESP32 / video ground
WRITE_N       qualified IO2 write strobe, active low during write
RW_N          inverted C64 R/W
STATUS_OE_N   status-bus enable, active low on IO2 read
READY_PRE_N   ESP32 open-drain acknowledge to U2 /PRE
READY         hardware READY status bit
MASK_ACTIVE   inverted SPI mask CS
FB_SELECT     opaque-pixel analog-switch select
FB_Y_REF      adjustable FINALBYTE luminance source
```

## 17.2 U1 — SN74LVC574A, PDIP-20, VCC=3.3 V

| Pin | Signal | Connect to |
|---:|---|---|
| 1 | `/OE` | GND |
| 2 | 1D | C64 D0 |
| 3 | 2D | C64 D1 |
| 4 | 3D | C64 D2 |
| 5 | 4D | C64 D3 |
| 6 | 5D | C64 D4 |
| 7 | 6D | C64 D5 |
| 8 | 7D | C64 D6 |
| 9 | 8D | C64 D7 |
| 10 | GND | GND |
| 11 | CLK | `WRITE_N` |
| 12 | 8Q | ESP32 GPIO11 |
| 13 | 7Q | ESP32 GPIO10 |
| 14 | 6Q | ESP32 GPIO9 |
| 15 | 5Q | ESP32 GPIO8 |
| 16 | 4Q | ESP32 GPIO7 |
| 17 | 3Q | ESP32 GPIO6 |
| 18 | 2Q | ESP32 GPIO5 |
| 19 | 1Q | ESP32 GPIO4 |
| 20 | VCC | +3V3 |

Place 100 nF directly between pins 20 and 10.

## 17.3 U2 — 74HCT74, PDIP-14, VCC=5 V

First flip-flop implements READY. Second half is parked in a defined state.

| Pin | Signal | Connect to |
|---:|---|---|
| 1 | 1CLR | `WRITE_N` |
| 2 | 1D | GND |
| 3 | 1CLK | GND |
| 4 | 1PRE | `READY_PRE_N` |
| 5 | 1Q | `READY` -> U3 pin 2 |
| 6 | 1/Q | NC |
| 7 | GND | GND |
| 8 | 2/Q | NC |
| 9 | 2Q | NC |
| 10 | 2PRE | +5V_C64 |
| 11 | 2CLK | GND |
| 12 | 2D | GND |
| 13 | 2CLR | +5V_C64 |
| 14 | VCC | +5V_C64 |

`READY_PRE_N`: 10 kΩ to **+3V3**, and directly to ESP32 GPIO39 configured open-drain. Place 100 nF between pins 14 and 7.

## 17.4 U3 — 74AHCT245 / 74HCT245, PDIP-20, VCC=5 V

DIR is fixed A -> B. A side is status generation; B side drives C64 only during an IO2 read.

| Pin | Signal | Connect to |
|---:|---|---|
| 1 | DIR | +5V_C64 |
| 2 | A1 | `READY` |
| 3 | A2 | ESP32 GPIO13 BUSY |
| 4 | A3 | ESP32 GPIO14 ERROR |
| 5 | A4 | ESP32 GPIO15 VIDEO_LOCK |
| 6 | A5 | ESP32 GPIO16 AUDIO |
| 7 | A6 | ESP32 GPIO17 reserved |
| 8 | A7 | ESP32 GPIO18 reserved |
| 9 | A8 | ESP32 GPIO21 reserved |
| 10 | GND | GND |
| 11 | B8 | C64 D7 |
| 12 | B7 | C64 D6 |
| 13 | B6 | C64 D5 |
| 14 | B5 | C64 D4 |
| 15 | B4 | C64 D3 |
| 16 | B3 | C64 D2 |
| 17 | B2 | C64 D1 |
| 18 | B1 | C64 D0 |
| 19 | `/OE` | `STATUS_OE_N` |
| 20 | VCC | +5V_C64 |

Place 100 nF between pins 20 and 10.

## 17.5 U4 — 74HCT32 / 74AHCT32, PDIP-14, VCC=5 V

| Pin | Gate | Connect to |
|---:|---|---|
| 1 | 1A | C64 `/IO2` |
| 2 | 1B | C64 `R/W` |
| 3 | 1Y | `WRITE_N` |
| 4 | 2A | C64 `/IO2` |
| 5 | 2B | `RW_N` |
| 6 | 2Y | `STATUS_OE_N` |
| 7 | GND | GND |
| 8 | 3Y | NC |
| 9 | 3A | GND |
| 10 | 3B | GND |
| 11 | 4Y | NC |
| 12 | 4A | GND |
| 13 | 4B | GND |
| 14 | VCC | +5V_C64 |

Place 100 nF between pins 14 and 7.

## 17.6 U5 — 74HCT04, PDIP-14, VCC=5 V

| Pin | Gate | Connect to |
|---:|---|---|
| 1 | 1A | C64 `R/W` |
| 2 | 1Y | `RW_N` |
| 3 | 2A | ESP32 GPIO43 `MASK_CS` |
| 4 | 2Y | `MASK_ACTIVE` |
| 5 | 3A | GND |
| 6 | 3Y | NC |
| 7 | GND | GND |
| 8 | 4Y | NC |
| 9 | 4A | GND |
| 10 | 5Y | NC |
| 11 | 5A | GND |
| 12 | 6Y | NC |
| 13 | 6A | GND |
| 14 | VCC | +5V_C64 |

Place 100 nF between pins 14 and 7.

## 17.7 U6 — 74HCT08 / 74AHCT08, PDIP-14, VCC=5 V

| Pin | Gate | Connect to |
|---:|---|---|
| 1 | 1A | ESP32 GPIO3 `MASK_OUT` |
| 2 | 1B | `MASK_ACTIVE` |
| 3 | 1Y | `MASK_PIXEL` |
| 4 | 2A | `MASK_PIXEL` |
| 5 | 2B | ESP32 GPIO38 `OVERLAY_ARM` |
| 6 | 2Y | `FB_SELECT` |
| 7 | GND | GND |
| 8 | 3Y | NC |
| 9 | 3A | GND |
| 10 | 3B | GND |
| 11 | 4Y | NC |
| 12 | 4A | GND |
| 13 | 4B | GND |
| 14 | VCC | +5V_C64 |

Place 100 nF between pins 14 and 7.

## 17.8 U7 — LM1881N, PDIP-8, VCC=5 V

| Pin | Signal | Connect to |
|---:|---|---|
| 1 | CSOUT | 10 kΩ -> ESP32 GPIO1 node; node 20 kΩ -> GND |
| 2 | CVIN | C64 Y through 0.1 µF series capacitor |
| 3 | VSOUT | 10 kΩ -> ESP32 GPIO2 node; node 20 kΩ -> GND |
| 4 | GND | GND |
| 5 | BPOUT | NC |
| 6 | RSET | 680 kΩ -> GND and 0.1 µF -> GND, per Rev A reference network |
| 7 | OEOUT | NC |
| 8 | VCC | +5V_C64 |

Place 100 nF between pins 8 and 4. The exact sync-input/RSET network remains a scope-validated prototype detail.

## 17.9 U8 — CD74HC4053E, PDIP-16, VCC=5 V — candidate

Channel A switches luma; channel B switches chroma. The C channel is grounded and unused.

| Pin | Signal | Connect to |
|---:|---|---|
| 1 | B1 | GND (blank chroma) |
| 2 | B0 | C64 C through initial 1 kΩ series resistor |
| 3 | C1 | GND |
| 4 | COM C | GND |
| 5 | C0 | GND |
| 6 | `/E` | GND |
| 7 | VEE | GND |
| 8 | GND | GND |
| 9 | S2 | GND |
| 10 | S1 | `FB_SELECT` |
| 11 | S0 | `FB_SELECT` |
| 12 | A0 | C64 Y |
| 13 | A1 | `FB_Y_REF` |
| 14 | COM A | S-Video Y output |
| 15 | COM B | S-Video C output |
| 16 | VCC | +5V_C64 |

Place 100 nF between pins 16 and 8. **Do not call this section frozen until Stage C passthrough has been inspected on an oscilloscope and monitor.**

## 17.10 Q1 — 2N3904 luma reference

Use a **2N3904** for the reference build. Connect collector to +5V_C64, base through 1 kΩ to the wiper of a 10 kΩ trimmer across +5V_C64/GND, and emitter to `FB_Y_REF`; place 1 kΩ from emitter to GND. If substituting BC547 or another transistor, verify its package pinout first.

## 17.11 ESP32 interconnect

| ESP32 pin | Net / destination | Electrical note |
|---:|---|---|
| GPIO1 | LM1881 CSOUT divider node | max approx. 3.3 V |
| GPIO2 | LM1881 VSOUT divider node | max approx. 3.3 V |
| GPIO3 | U6 pin 1 `MASK_OUT` | 3.3 V -> HCT input |
| GPIO4..11 | U1 Q0..Q7 | 3.3 V |
| GPIO12 | `WRITE_N` through 10k/20k divider | 5 V -> approx. 3.3 V |
| GPIO13..18,21 | U3 A2..A8 | 3.3 V -> AHCT/HCT inputs |
| GPIO38 | U6 pin 5 `OVERLAY_ARM` | 10 kΩ pull-down to GND; firmware drives HIGH only after video init |
| GPIO39 | `READY_PRE_N` | **open-drain**, 10 kΩ pull-up to +3V3 |
| GPIO42 | mask SPI clock | firmware-only path |
| GPIO43 | U5 pin 3 `MASK_CS` | 3.3 V -> HCT input |
| GPIO44 | PCM5102A BCLK | module label, not fixed module pin number |
| GPIO47 | PCM5102A WS/LRCK | module label |
| GPIO48 | PCM5102A DIN | module label |

## 17.12 Power checklist before inserting into the C64

With the C64 disconnected, verify:

1. no continuity between +5V_C64 and +3V3;
2. common GND continuity everywhere;
3. U1 pin 20 is 3.3 V, not 5 V;
4. U2/U3/U4/U5/U6/U7/U8 VCC pins are on the 5 V rail;
5. `OVERLAY_ARM` is LOW with ESP32 held in reset / unpowered;
6. `READY_PRE_N` rises only to approximately 3.3 V;
6. ESP32-facing LM1881 and `WRITE_N` divider nodes never exceed the 3.3 V domain;
7. U3 `/OE` is high except during a C64 IO2 read;
8. U8 `FB_SELECT=0` gives host passthrough before any ESP32 overlay is enabled.

---

# 18. Rev A.1a freeze status

**Digitally frozen for weekend prototype:** command latch, READY handshake, status buffer, voltage domains and ESP32 pin assignment.

**Experimentally frozen only after measurement:** LM1881 phase/timing constants, CD74HC4053 passthrough quality, chroma series resistor and FINALBYTE luminance calibration.

The Rev A.1a success criterion remains Stage E: a live C64 executes its own program and SID activity while commanding a raster-locked FINALBYTE overlay and independent FINALBYTE audio.
