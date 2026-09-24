# FINALBYTE — Tech Alpha
## Minimal Hardware / Firmware Prototype

**Status:** proposed Alpha technical profile  
**Parent specification:** FINALBYTE 1.3  
**Reference host for first bring-up:** Commodore 64 PAL  

---

# 1. Purpose

FINALBYTE Tech Alpha is the smallest useful hardware and firmware implementation that proves the core FINALBYTE architecture in real hardware.

The Alpha is not intended to demonstrate the complete FINALBYTE 1.3 feature set. Its purpose is to validate four fundamental paths:

1. **8-bit host → FINALBYTE command communication**
2. **stable horizontal / vertical synchronization to the host video**
3. **binary host / FINALBYTE video selection**
4. **basic external audio playback**

If these four paths work reliably, the remaining FINALBYTE features are primarily firmware, renderer and asset-system expansion rather than unknown hardware architecture.

---

# 2. Alpha design rule

The Alpha must stay deliberately small.

It uses:

- one ESP32-S3 module,
- only the bus-interface logic required to communicate safely with a 5 V retro host,
- only the analog / mixed-signal hardware required to recover video timing and switch the video path,
- one minimal external audio output stage,
- no FPGA,
- no full graphics accelerator ASIC,
- no digitization of the host video.

A tiny CPLD is **not required** for Alpha. If later measurements show that discrete latch/decode logic is insufficient, a CPLD may replace that logic without changing the software protocol.

---

# 3. Explicitly excluded from Alpha

The following FINALBYTE 1.3 features are intentionally postponed:

- USB mouse
- USB keyboard
- Wi-Fi
- network services
- Sprite Kitchen operations
- Sample Kitchen operations
- load-time blend modes
- internal 4-bit sprite alpha
- true host-video alpha blending
- tile engine
- scrolling engine
- collision accelerator
- passive SID / POKEY / AY sniffing
- multiple asset banks
- SD-card asset loading
- advanced DSP effects
- reverb / delay / filters
- arbitrary sample processing
- interrupts from FINALBYTE to the host
- command FIFO deeper than one command
- NTSC support in the first hardware bring-up

These are extensions, not prerequisites for proving the architecture.

---

# 4. Alpha system block diagram

```text
                      FINALBYTE TECH ALPHA

 Commodore 64
 ┌──────────────┐
 │              │
 │  Expansion   │      5 V / bus-safe      ┌─────────────────┐
 │  Port        ├──────────────────────────►│ Host Interface  │
 │              │                           │ decode + latches│
 └──────────────┘                           └────────┬────────┘
                                                   │
                                                   │ 3.3 V logic
                                                   ▼
                                          ┌──────────────────┐
                                          │    ESP32-S3      │
                                          │                  │
                                          │ command parser   │
                                          │ test renderer    │
                                          │ H/V timing       │
                                          │ binary mask      │
                                          │ PCM player       │
                                          └──────┬─────┬─────┘
                                                 │     │
                            timing               │     │ audio
                                                 │     ▼
 C64 Luma + Sync ──► Sync recovery ──────────────┘   I2S DAC
      │                                                  │
      │                                                  ▼
      │                                               AUDIO OUT
      │
      ├─────────────────────────────┐
      │                             │
      ▼                             ▼
 Host luma                    FINALBYTE luma
      │                             │
      └─────────────┬───────────────┘
                    ▼
              analog video mux
                    │
 C64 chroma ──► chroma gate / blank
                    │
                    ▼
                 VIDEO OUT
```

---

# 5. Reference host: Commodore 64 PAL

The first Alpha board targets a PAL Commodore 64 because it provides a comparatively clean path for validating both host communication and video synchronization.

The Alpha does **not** attempt to be electrically universal across Atari, C64 and ZX Spectrum on the first board.

The shared FINALBYTE protocol and firmware architecture remain portable, while later host adapters provide the machine-specific electrical interface.

For Alpha:

- host communication uses the C64 expansion port,
- FINALBYTE registers live in **IO2 / `$DFxx`**,
- video timing is recovered from the C64 video output,
- the first video path uses C64 luma/chroma rather than attempting universal composite processing.

---

# 6. Minimal hardware

## 6.1 ESP32-S3

Primary controller:

- ESP32-S3 module
- onboard flash for firmware and Alpha test assets
- PSRAM optional, not required for the minimal profile

The ESP32 performs:

- command parsing,
- basic rendering,
- raster timing state,
- binary overlay-mask generation,
- audio sample playback,
- diagnostics.

No ESP32 GPIO connected to the host may be exposed directly to unsafe 5 V bus levels.

---

## 6.2 Host bus interface

The Alpha host interface requires only:

- IO2 selection,
- low register-address bits,
- read/write direction,
- host data bus,
- a small number of write latches,
- a readable status path,
- 5 V ↔ 3.3 V-safe buffering / level translation.

The Alpha uses a **single-command mailbox**, not a hardware FIFO.

Conceptually:

```text
C64 CPU
   │
   │ IO2 + A0..A2 + R/W + D0..D7
   ▼
address decode
   │
   ├── write latches ─────► ESP32
   │
   └── status read buffer ◄ ESP32
```

The external logic guarantees that the C64 sees deterministic bus timing even if the ESP32 is temporarily busy.

The ESP32 never has to win a sub-microsecond software race against a live 6510 bus cycle.

---

# 7. Alpha host register interface

Only eight register addresses are required.

The same address may have different read and write meaning.

| Address | Write | Read |
|---|---|---|
| `$DF00` | `COMMAND` | `STATUS` |
| `$DF01` | `ARG0` | `VERSION_MAJOR` |
| `$DF02` | `ARG1` | `VERSION_MINOR` |
| `$DF03` | `ARG2` | `DEVICE_ID` |
| `$DF04` | `ARG3` | `LAST_ERROR` |
| `$DF05` | `DATA` | `DATA` |
| `$DF06` | `COMMIT` | `FLAGS` |
| `$DF07` | `CONTROL` | `CAPABILITIES` |

## 7.1 Mailbox transaction

Typical command sequence:

```text
1. host reads STATUS
2. wait until READY = 1
3. host writes COMMAND
4. host writes ARG0..ARG3 as needed
5. host writes COMMIT
6. hardware latches the complete command
7. READY becomes 0 / BUSY becomes 1
8. ESP32 consumes command
9. READY becomes 1
```

There is no command queue in Alpha.

If the host submits while `BUSY`, the command may be rejected and an error flag set.

---

# 8. Minimal Alpha command set

The first firmware requires only enough commands to validate the architecture.

```text
NOP
RESET_FB
SET_VIDEO_MODE
SET_PHASE_X
SET_PHASE_Y
CLEAR_FB
DRAW_RECT
SHOW_SPRITE
MOVE_SPRITE
HIDE_SPRITE
PLAY_SAMPLE
STOP_SAMPLE
SET_VOLUME
```

## 8.1 Built-in test assets

Alpha does not load arbitrary sprite or sample files from the host.

Firmware flash contains a tiny fixed test set, for example:

- diagnostic grid,
- FINALBYTE logo,
- 8×8 marker,
- 16×16 sprite,
- 32×32 sprite,
- one short PCM sample.

This removes SD, filesystems, manifests and Kitchen processing from the first hardware test.

---

# 9. Video Alpha: fundamental rule

The Alpha does **not** generate a complete independent PAL/composite signal.

It uses the original host video timing as the master reference.

During synchronization, blanking and other timing-critical portions of the line, the original host signal remains authoritative.

FINALBYTE only replaces selected **active-picture** pixels.

This greatly reduces the amount of analog video hardware required for the first prototype.

---

# 10. C64 Alpha video path

The reference C64 implementation uses the machine's separated luma/chroma output where practical.

## 10.1 Luma path

C64 luma contains the host luminance and synchronization timing.

The Alpha video switch selects:

```text
mask = 0  →  original C64 luma
mask = 1  →  FINALBYTE generated luma level
```

However, host luma is always passed during:

- sync,
- horizontal blanking,
- vertical blanking,
- calibration / guard intervals.

Therefore FINALBYTE does not need to synthesize PAL sync timing itself.

## 10.2 Chroma path

For the first Alpha:

```text
mask = 0  →  original C64 chroma
mask = 1  →  chroma blank / neutral chroma
```

A FINALBYTE-selected pixel is therefore monochrome / grayscale in Alpha.

This is intentional.

The purpose of Alpha is to prove synchronization and pixel replacement, not the final color encoder.

## 10.3 Result

The screen may simultaneously contain:

```text
original colored C64 picture
+
monochrome FINALBYTE HUD / sprite / rectangle
```

with hard 0/1 source selection.

No original host pixel is digitized.

No alpha blend is performed.

---

# 11. Alpha graphics profile

## 11.1 Logical coordinates

Reference logical area:

```text
320 × 200
```

The actual active host window and border alignment are calibrated by the video adapter / firmware.

## 11.2 FINALBYTE pixel data

Alpha uses a deliberately small grayscale profile.

Recommended first implementation:

- 2-bit FINALBYTE luminance: 4 levels
- 1-bit host / FINALBYTE selection mask

Conceptually:

```text
FB pixel = 00  black
FB pixel = 01  dark
FB pixel = 10  light
FB pixel = 11  white

MASK = 0      host pixel
MASK = 1      FINALBYTE pixel
```

This is **not alpha**.

`MASK` selects the source. The 2-bit value only selects the brightness of a FINALBYTE pixel.

## 11.3 Memory requirement

For a 320×200 active framebuffer:

```text
2-bit framebuffer  = 16,000 bytes
1-bit mask         =  8,000 bytes
--------------------------------
total              = 24,000 bytes
```

This is small enough to keep the minimal display data in fast MCU memory.

PSRAM is not required for the Alpha framebuffer.

## 11.4 Alpha renderer

The first renderer needs only:

- clear framebuffer,
- clear mask,
- fill rectangle,
- copy built-in sprite,
- move built-in sprite,
- hide sprite,
- full-screen mask enable / disable.

No scaling, rotation, blending, tilemaps or Kitchen operations are required.

---

# 12. H/V synchronization

## 12.1 Timing extraction

The host video input is sent to a high-impedance video/sync front end.

The front end produces normalized logic-level timing information for the ESP32, for example:

- composite sync / horizontal timing event,
- vertical sync / frame timing event.

Exact comparator / sync-separator component choice is an implementation detail and is not frozen by this Alpha architecture document.

## 12.2 Raster state

Firmware maintains:

```text
current line
current frame
active-area state
horizontal phase
vertical phase
```

Each detected horizontal timing event starts a new line timing sequence.

The vertical timing event resets / re-synchronizes the frame position.

## 12.3 Per-line synchronization

FINALBYTE must re-anchor horizontal output to the host on every scanline.

It must **not** free-run an entire frame from an unrelated MCU clock and hope that the two video sources remain aligned.

Conceptually:

```text
host line sync
     ↓
programmable horizontal delay
     ↓
active line start
     ↓
FINALBYTE pixel output
```

This prevents long-term horizontal drift.

## 12.4 Phase calibration

Alpha exposes two calibration values:

```text
PHASE_X
PHASE_Y
```

These allow the FINALBYTE active area to be aligned with the host picture without recompiling firmware.

The values may later become per-platform / per-video-mode defaults.

---

# 13. Video output implementation rule

Pixel timing must not depend on one software interrupt per pixel.

The ESP32 should prepare line data in advance and use a hardware-driven output mechanism such as DMA / parallel peripheral / timer-driven buffered output.

The exact ESP32-S3 peripheral mapping is an implementation choice to be proven during bring-up.

A recommended architecture is:

```text
framebuffer + mask
      ↓
line preparation
      ↓
small line buffer
      ↓
hardware-timed output
      ↓
video mux control + luma level
```

The CPU may prepare the next line while the current line is being emitted.

---

# 14. Analog video switching

The video switch is not a generic slow GPIO multiplexer.

The Alpha requires an analog switch / video mux with sufficient bandwidth and switching behavior for the host video path.

The switching system performs two related functions:

1. **luma source selection**
   - host luma
   - FINALBYTE luma

2. **chroma handling**
   - host chroma when host pixel is selected
   - neutral / blank chroma when FINALBYTE pixel is selected

The exact analog part numbers, terminations and bias network are intentionally left to the host-adapter schematic revision.

They must be verified on an oscilloscope against the actual C64 output before being frozen.

---

# 15. Audio Alpha

Audio remains intentionally simple.

## 15.1 Hardware

Recommended path:

```text
ESP32-S3
   ↓ I2S
small DAC / codec
   ↓
line-level stereo or mono output
```

## 15.2 Firmware profile

Alpha supports:

- one built-in PCM sample,
- one or a few simultaneous voices,
- play,
- stop,
- volume.

No Sample Kitchen, mixing recipes, reverb, echo or advanced DSP is required.

A successful `PLAY_SAMPLE` command is enough to validate host-to-audio latency and the external audio path.

---

# 16. Firmware architecture

The Alpha firmware should remain divided into small deterministic subsystems.

```text
┌─────────────────────────────┐
│ Host mailbox                │
│ command decode / status     │
├─────────────────────────────┤
│ Graphics state              │
│ framebuffer + mask          │
├─────────────────────────────┤
│ Video timing                │
│ line/frame synchronization  │
├─────────────────────────────┤
│ Line output                 │
│ buffered hardware output    │
├─────────────────────────────┤
│ PCM audio                   │
└─────────────────────────────┘
```

No network stack or USB-host stack is required in the Alpha firmware build.

---

# 17. Alpha status flags

Minimum `STATUS` bits:

```text
bit 0   READY
bit 1   BUSY
bit 2   VIDEO_SYNC
bit 3   VIDEO_ACTIVE
bit 4   AUDIO_ACTIVE
bit 5   ERROR
bit 6   reserved
bit 7   reserved
```

`VIDEO_SYNC` is especially useful during bring-up: the host can determine whether FINALBYTE currently considers synchronization stable.

---

# 18. Alpha startup behavior

On power-up:

```text
1. initialize ESP32
2. initialize bus mailbox
3. keep video mux in safe HOST PASS-THROUGH state
4. initialize sync detector
5. wait for stable video sync
6. initialize framebuffer and mask to transparent-to-host
7. expose READY
```

The critical rule is:

> A crashed, resetting or uninitialized FINALBYTE must default to passing the original host video whenever the hardware design permits it.

This fail-safe behavior should be implemented in hardware bias / mux defaults where practical, not only in firmware.

---

# 19. Alpha video modes

Only three modes are required.

## PASS_THROUGH

```text
mask ignored
100% host video
```

## OVERLAY

```text
mask 0 → host
mask 1 → FINALBYTE grayscale pixel
```

## FULL

```text
active picture → FINALBYTE grayscale framebuffer
sync / blanking → host timing
```

`FULL` remains synchronized to the host and still relies on the host timing signal.

---

# 20. Bring-up sequence

The prototype should be brought up in stages rather than attempting the complete Alpha in one step.

## A0 — host detection

Prove:

- `$DFxx` decode,
- read/write registers,
- `DEVICE_ID`,
- `READY/BUSY`.

No video switching.

## A1 — audio

Prove:

```text
C64 command → ESP32 → PCM sample output
```

## A2 — sync detector

Prove:

- stable line detection,
- stable frame detection,
- PAL frame count,
- `VIDEO_SYNC` status.

Video still passes through unchanged.

## A3 — static video replacement

Replace one fixed rectangle inside the active picture with a FINALBYTE luminance level.

This is the first real binary mux test.

## A4 — framebuffer

Display:

- test grid,
- moving rectangle,
- built-in sprite.

Verify alignment over many minutes without visible horizontal or vertical drift.

## A5 — host-controlled graphics

C64 software controls:

- sprite position,
- rectangle position,
- video mode,
- sample playback.

At this point FINALBYTE Tech Alpha is considered successful.

---

# 21. Alpha acceptance criteria

The Alpha passes when all of the following work reliably on the reference PAL C64:

- FINALBYTE can be detected through the host register interface.
- The host can issue commands without corrupting or freezing the C64 bus.
- Original video passes through normally when FINALBYTE overlay is disabled.
- FINALBYTE detects and maintains stable video synchronization.
- A FINALBYTE rectangle can remain locked to a fixed screen position.
- A built-in sprite can be moved under host control.
- Binary host / FINALBYTE source selection has clean, stable boundaries.
- FINALBYTE can replace the complete active area in `FULL` mode.
- A host command can play a PCM sample through the external audio path.
- Reset or firmware failure returns or leaves the video path in safe host-pass-through state where practical.

No other FINALBYTE feature is required to declare Alpha successful.

---

# 22. What comes immediately after Alpha

Once Alpha is stable, development can expand without redesigning the fundamental architecture.

Likely next steps:

```text
Alpha
  ↓
color FINALBYTE video
  ↓
more sprites / animation
  ↓
asset loading / banks
  ↓
Sprite + Sample Kitchen
  ↓
tile renderer
  ↓
USB mouse / keyboard
  ↓
Atari adapter
  ↓
ZX Spectrum adapter
```

True host-video multi-level alpha blending remains outside the FINALBYTE 1.3 architecture.

---

# 23. Core architectural boundary proven by Alpha

A successful Alpha demonstrates the essential FINALBYTE proposition:

```text
8-bit computer
     │
     │ game commands
     ▼
FINALBYTE ESP32
     │
     ├── synchronized generated graphics
     └── external digital audio

host video ───────────────┐
                          ├── binary 0/1 selection ──► display
FINALBYTE video layer ────┘
```

The host remains the computer running the game.

FINALBYTE is the synchronized external graphics/audio coprocessor.

That boundary is the foundation on which the later FINALBYTE feature set is built.
