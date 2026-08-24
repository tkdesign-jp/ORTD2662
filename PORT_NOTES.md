# Port: X@RTD2662 board + Toshiba LTM09C362V (1024x600)

This fork ports [KerJoe/ORTD2662](https://github.com/KerJoe/ORTD2662) to the
generic **X@RTD2662** controller board driving a **Toshiba LTM09C362V**
9.0" 1024x600 18-bit single-channel LVDS panel, used as a standalone HDMI
monitor.

Status: **fully working.** The firmware serves a native 1024x600@60.15 EDID,
the display is pixel-perfect 1:1 with correct colors. Verified with two
different HDMI sources with zero source-side configuration:

* Raspberry Pi 3B (Raspberry Pi OS, vc4-kms-v3d)
* Orange Pi PC2 (Armbian 26.2.5)

To our knowledge this is the first documented third-party port of ORTD2662
to a different board and panel.

## Build & flash

Plain SDCC (tested 4.2.0), no other dependencies:

```
make firmware          # -> output/firmware.bin (32 KB)
```

Known-good build md5: `dab06e957b8bca27de369c9ab5abb3cb`

Flash with [RTDMultiProg](https://github.com/KerJoe/RTDMultiProg) (here via a
Raspberry Pi's I2C on the board's HDMI DDC pins):

```
python3 rtdmultiprog.py -i i2cdev -d 1 -w firmware.bin
python3 rtdmultiprog.py -i i2cdev -d 1 -r verify.bin -z 32768 && md5sum verify.bin
```

Keep a full dump of your vendor firmware before flashing
(`-r dump.bin -z 524288`); restoring it is a single `-w dump.bin`.

## Changed files

* `config/panel_config.h` — LTM09C362V timing (1024x600, DCLK 49.088 MHz,
  HTotal 1312 / VTotal 622, 18-bit, no bit-order swap).
* `config/board_config.h`, `config/misc_config.h` — board specifics.
* `peripherals/ddc.c` — `testEDID` replaced with a panel-native
  1024x600@60.15 EDID (DTD: 49.09 MHz, H 1024/40/104/144, V 600/3/10/9).
* `core/main.c` — backlight/panel-power GPIO bring-up for this board,
  gamma LUT + dither initialization ported from the vendor reference code,
  vertical capture-window offset (V start = 18 lines).
* `scaler/scaling.c` — at 1:1 the UZD line buffer is bypassed
  (`BUFFER_MODE=00`, `SBUFF_EXT=0`); the buffered path with `SBUFF_EXT`
  is only configured when actually downscaling (see notes).
* `scaler/scaler.c` — `SetDPLLFrequncy()` uses a high-VCO configuration
  (DPN=8, output Div4, VCO ≈ 400 MHz) for the 10–100 MHz range, matching
  the configuration the vendor firmware runs (measured on live hardware).
* `scaler/scaler_tables.c` — LVDS control register init, including **the
  single bit that cost us a whole day** (finding 1 below).

## Findings that may save you days

1. **TCON register 0xA3 (LVDS_CTRL3) bit 0 = BMTS, the LVDS bit-mapping
   table select. It is panel-dependent, and the reset default may be
   wrong for your panel.** The LTM09C362V needs Table 2 (bit0 = 1). With
   the default Table 1, *only intermediate pixel values are corrupted* —
   colored per-pixel speckle on antialiased text — while pure 0x00/0xFF
   content (1-px stripe patterns, solid color bars) renders **perfectly**,
   because any bit-position permutation is invisible when all bits of a
   channel are equal. This makes the bug almost immune to classic test
   patterns. If your panel shows colored noise on text but passes every
   stripe test you throw at it: try the other BMTS table.

2. **LVDS_CTRL1 (TCON 0xA1) lower bits are not don't-care.** Upstream's
   `DisplayInitTable` writes `0xC0` (the two clock-polarity inversion
   bits), which also clears STSTL (default 010) and the LVDS output
   common-mode setting (default 100). On this panel that produced visible
   flicker; restoring the datasheet default lower bits (`0xD4`) fixed it.
   The vendor firmware runs `0xD7` (common-mode 111); both are stable here.

3. **UZD "2-tap" (page 6, 0xE3 bit 4) is not an image filter — it is
   SBUFF_EXT**, a line-buffer width extension (960 → 1920 px, RTD2660
   datasheet p150-151). In the buffered path it is mandatory for widths
   over 960 (clearing it wraps the image); at 1:1 the correct
   configuration is to take the buffer out of the path entirely
   (`BUFFER_MODE=00`), which is what the vendor firmware does.

4. **The firmware-served EDID is enough.** With a valid DTD both Raspberry
   Pi OS (KMS) and Armbian pick the panel-native mode with no
   `config.txt` / kernel-cmdline overrides. If you inherited
   `hdmi_timings=`, `hdmi_ignore_edid=` or `drm.edid_firmware=` overrides
   from earlier experiments, remove them — a stale 592-line override cost
   us hours (it silently re-enables vertical scaling and smears text).

5. **Backlight on this board is not the usual PCB800099-style P3_3/P3_4**
   (those pins are a bit-banged I2C bus here). A group of scaler GPIOs
   must be configured as outputs and driven low; see `core/main.c`.

## Known cosmetic issue

Input (60.15 Hz) and output (60.00 Hz, VTotal 625) are not frame-locked;
a slowly drifting beat line can be seen on a 1-px checkerboard test
pattern (invisible in normal content). Deriving the output VTotal from
the input (622) should lock them — left as a TODO.

## Thanks

Huge thanks to **KerJoe** for ORTD2662 and RTDMultiProg. A from-scratch,
plain-SDCC, readable firmware for these scalers is what made this port
possible at all — the vendor Keil source tree does not even build without
a paid toolchain. 感謝！
