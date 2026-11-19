# FINALBYTE Weekend Alpha — Rev A.1 changes

- `READY_PRE_N` pull-up moved from +5 V to +3.3 V.
- ESP32 GPIO39 is configured as open-drain; LOW asserts HCT74 `/PRE`, released state is pulled to 3.3 V.
- Added explicit supply / logic-domain table.
- Replaced `74HC32` recommendation with `74HCT32` / `74AHCT32` for TTL-compatible C64-side inputs.
- Marked CD74HC4053 as the Rev A.1 candidate analog switch, subject to Stage C oscilloscope validation.
- Added pin-level PDIP wiring tables for U1–U8 and a separate `NETLIST.csv`.
- Added pre-power electrical checklist and clarified which portions are digitally frozen vs. measurement-dependent.
