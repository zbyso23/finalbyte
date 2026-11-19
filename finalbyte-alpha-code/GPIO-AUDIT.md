# FINALBYTE Weekend Alpha Rev A.2 — GPIO audit

This file is generated/maintained as the human-readable cross-check between
`esp32/main/board_pins.h` and `NETLIST.csv` / Weekend HW Rev A.1.

| GPIO | Firmware role | Rev A.1 hardware net | Direction | Domain at ESP32 |
|---:|---|---|---|---|
| 1 | HSYNC/composite-sync edge | LM1881 CSOUT via 10k/20k divider | in | <=3.3 V |
| 2 | VSYNC | LM1881 VSOUT via 10k/20k divider | in | <=3.3 V |
| 3 | MASK_OUT / SPI MOSI | U6 HCT08 input | out | 3.3 V |
| 4..11 | latched host byte D0..D7 | U1 LVC574 Q0..Q7 | in | 3.3 V |
| 12 | WRITE_STROBE | WRITE_N via 10k/20k divider | in, posedge IRQ | <=3.3 V |
| 13 | BUSY | U3 A2 | out | 3.3 V -> AHCT/HCT |
| 14 | ERROR | U3 A3 | out | 3.3 V -> AHCT/HCT |
| 15 | VIDEO_LOCK | U3 A4 | out | 3.3 V -> AHCT/HCT |
| 16 | AUDIO | U3 A5 | out | 3.3 V -> AHCT/HCT |
| 17 | reserved status | U3 A6 | out | 3.3 V -> AHCT/HCT |
| 18 | reserved status | U3 A7 | out | 3.3 V -> AHCT/HCT |
| 21 | reserved status | U3 A8 | out | 3.3 V -> AHCT/HCT |
| 39 | READY_PRE_N | U2 /PRE + 10k to +3.3V | open-drain out | 0/3.3 V only |
| 42 | MASK_SCLK | video mask SPI clock | out | 3.3 V |
| 43 | MASK_CS | U5 HCT04 -> MASK_ACTIVE | out, active low | 3.3 V |
| 44 | I2S BCLK | PCM5102A BCLK | out | 3.3 V |
| 47 | I2S WS/LRCK | PCM5102A WS/LRCK | out | 3.3 V |
| 48 | I2S DATA | PCM5102A DIN | out | 3.3 V |

No GPIO number is assigned to more than one functional block.

## Important runtime behavior

- GPIO43 is not a decorative chip-select. The SPI peripheral drives it LOW only
  during each 256-bit row transaction. Rev A.1 hardware inverts it to
  `MASK_ACTIVE`; therefore `FB_SELECT = MASK_OUT AND MASK_ACTIVE` and original
  C64 video is forced through outside active row bursts.
- GPIO39 must remain open-drain. It never drives HIGH; HIGH is provided by the
  external +3.3 V pull-up.
- GPIO12 internal pull-up/down is disabled because the external WRITE_N divider
  defines the logic level.
