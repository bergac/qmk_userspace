# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A **QMK userspace** (external overlay) that adds **splitkb Halcyon module** support (TFT display, rotary encoder, Cirque trackpad) to split keyboards without forking `qmk_firmware` or `vial-qmk`. It is compiled *against* a separate local clone of the firmware, not on its own.

- Branch `halcyon` = builds against **Vial** (`vial-kb/vial-qmk`, `vial` branch). This is the default branch.
- Branch `halcyon-qmk` = builds against **upstream QMK** (`qmk/qmk_firmware`, `master`).
- The two never merge cleanly; keep changes on the branch that matches the target firmware, or apply to both.

## Prerequisites

There is no build without a firmware checkout. QMK CLI must know where it is:

```bash
qmk config user.overlay_dir="$(realpath .)"   # run once, from this repo root
qmk config user.qmk_home                       # must point at a vial-qmk / qmk_firmware clone
```

## Common commands

The target keyboard for this checkout is **`splitkb/halcyon/corne/rev2`** with the
**TFT display module** (`HLC_TFT_DISPLAY`). Prefer these exact commands:

```bash
# Compile the Corne rev2 + display firmware (Vial keymap):
qmk compile -kb splitkb/halcyon/corne/rev2 -km vial_hlc -e HLC_TFT_DISPLAY=1 -e TARGET=halcyon_corne_display

# Same, upstream-QMK keymap instead of Vial:
qmk compile -kb splitkb/halcyon/corne/rev2 -km default_hlc -e HLC_TFT_DISPLAY=1 -e TARGET=halcyon_corne_display

qmk userspace-list                 # show configured build targets (from qmk.json)
qmk userspace-compile              # build every target in qmk.json
qmk userspace-compile -n           # print the exact compile commands without building

# Add / remove a target (edits qmk.json for you):
qmk userspace-add    -kb <kb> -km <km> -e <HLC_MODULE>=1 -e TARGET=<filename>
qmk userspace-remove -kb <kb> -km <km> -e <HLC_MODULE>=1 -e TARGET=<filename>

make <target>                      # proxies to $(qmk config user.qmk_home) with QMK_USERSPACE set
```

The compiled `.uf2` lands in the repo root and in `$(qmk config user.qmk_home)`.

### Hardware setup and flashing (this user's board)

- **Left half = master** (USB cable goes here) with the **TFT display**; **right half** has the **Cirque trackpad**. Each half needs its own build, flashed separately (double-tap reset, copy the `.uf2` to the drive):
  - left: `-e HLC_TFT_DISPLAY=1 -e TARGET=halcyon_corne_display`
  - right: `-e HLC_CIRQUE_TRACKPAD=1 -e TARGET=halcyon_corne_trackpad`
- The user runs the **plain QMK build (`default_hlc`)**, not Vial. The layout (Colemak-DH, home-row mods, `LT()` thumbs, 4 layers) was converted from a Vial export; edit `default_hlc/keymap.json` to change it.
- `$(qmk config user.qmk_home)` is `~/code/vial-qmk`. `~/code/qmk_firmware` is an empty folder, so there is no upstream-QMK checkout; `default_hlc` is compiled against vial-qmk.
- **Combos live in `default_hlc/keymap.c`** as a hand-written `combo_t key_combos[]` (with `COMBO_ENABLE = yes`). vial-qmk ignores `combos.def` and the `combos` key in `keymap.json`, and `key_combos` must be in `keymap.c` (a separate `SRC` file fails to link). Tapping term / permissive hold / chordal hold are in `default_hlc/config.h`.
- In `keymap.json` use `SC_LCPO` / `SC_RSPC`, not `KC_LCPO` / `KC_RSPC`.
- The Vial app is not needed and its stored EEPROM keymap is ignored by non-Vial firmware.

### Display gotchas (`hlc_tft_display`)

- `HSV_*` macros in `hlc_tft_display.h` are **QMK HSV (0-255 each), not RGB**. The last value is brightness: `V = 0` draws black on black (an RGB triple pasted as HSV made the symbol layer invisible). Convert RGB first (e.g. `colorsys.rgb_to_hsv`, scaled by 255).
- Fonts are loaded once in `module_post_init_kb`; `update_display()` returns early if a font failed to load (an unchecked NULL font dereference hard-faults the whole keyboard). Layer images are loaded/closed per layer change with NULL checks.

**Do not overwrite `keyboards/splitkb/halcyon/corne/keymaps/vial_hlc/keymap.json` with a
Vial-app export** (a `.vil`-style file with `vial_protocol` / `tap_dance` / `settings`
keys and per-row `layers` arrays containing `-1`). That format is not a compilable QMK
keymap and breaks the build with `Invalid JSON keymap: ... is not of type 'string'`.
`keymap.json` must stay a flat QMK keymap (one array of keycode strings per layer);
keep personal Vial layouts as separate `*.vil` files.

`<HLC_MODULE>` is exactly one of `HLC_NONE`, `HLC_ENCODER`, `HLC_ENCODER_REV2`, `HLC_TFT_DISPLAY`, `HLC_CIRQUE_TRACKPAD`. `splitkb/rules.mk` hard-errors if a Halcyon keyboard is built with none of them set.

There is **no test suite** (this is firmware). C is formatted with `.clang-format` (Google base, 4-space indent, `ColumnLimit: 1000`). `.mk`/`Makefile` use tabs + LF.

CI (`.github/workflows/build_binaries.yaml`) builds all `qmk.json` targets against vial-qmk on every push and publishes firmware to the `latest-halcyon-vial` release. It symlinks `converters/promicro_to_halcyon/` into the firmware tree as a preparation step.

## Architecture

### Overlay wiring

- Keymaps live in `keyboards/<vendor>/<kb>/keymaps/`. Each provides `default_hlc` (QMK), `vial_hlc` (current Vial layout), and sometimes `vial_hlc_legacy` (old Vial button/matrix mapping — compiles with `HALCYON_LEGACY`, incompatible with `vial_hlc`).
- Every Halcyon keymap's `rules.mk` sets `USER_NAME := halcyon_modules`, which pulls in `users/halcyon_modules/`.
- `users/halcyon_modules/rules.mk` conditionally includes `splitkb/rules.mk` when `KEYBOARD` is a known Halcyon board, and `splitkb/converter/converter_rules.mk` when `CONVERT_TO=halcyon`.
- `users/halcyon_modules/splitkb/rules.mk` turns on Quantum Painter (`st7789_spi surface`), pointing device (`cirque_pinnacle_spi`), and PWM backlight for *all* Halcyon builds, then includes the per-module `rules.mk` gated on the `-e HLC_*=1` env var.

### `splitkb/halcyon.c` — the module core

- Defines **weak** hooks that modules and keymaps override instead of the real QMK ones, because `keyboard_post_init_kb` and `housekeeping_task_kb` are already consumed here:
  - `module_post_init_kb/_user`
  - `module_housekeeping_task_kb/_user`
  - `display_module_housekeeping_task_kb/_user(bool second_display)`
- `module_t` enum identifies the installed module. Master broadcasts its module to the slave once over a split RPC transaction (`MODULE_SYNC`) into `module_master`; that is how a display half decides whether it is the "second display".
- Owns the backlight idle timeout (`HLC_BACKLIGHT_TIMEOUT`) and a fix in `pointing_device_task_combined_kb` that de-inverts a non-trackpad master on the right half.

### `splitkb/config.h` — injected as `POST_CONFIG_H`

- **Expands `MATRIX_COLS` by 5** to create virtual columns for the Halcyon buttons (unless `HALCYON_LEGACY`).
- Remaps VIK connector SPI/I2C drivers and pins.
- Per-keyboard `ENCODER_A_PINS` / `ENCODER_B_PINS` overrides: the original keyboard pins plus `HLC_ENCODER_A` / `HLC_ENCODER_B`. QMK cannot append encoder pins from userspace, so the whole list is redefined per `KEYBOARD_*` ifdef. Adding a keyboard means adding a block here (see `docs/PORTING.md`).

### `splitkb/halcyon_buttons.c`

Scans `BUTTON_PINS` (defined per keyboard) directly into the matrix: into virtual columns starting at `MATRIX_COLS - 5` (modern) or an extra matrix row (`HALCYON_LEGACY`). `process_record_kb` intercepts those virtual columns and resolves them through `left_halcyon_buttons[10][5]` / `right_halcyon_buttons[10][5]` — a per-layer mini-keymap the user defines in `keymap.c` / `halcyon_keys.c`. Only linked in when the encoder module is enabled.

### Modules (`splitkb/hlc_*`)

- `hlc_tft_display/` — Quantum Painter on a 240×135 ST7789, drawn to an off-screen `qp_surface` so only diffs are pushed. Ships fonts + digit images under `graphics/` (regenerate with `qmk painter-*` CLI tools). `hlc_tft_display.c` renders the layer/LED-state view and a Game-of-Life animation. `hlc_tft_display.h` holds the HSV palette.
- `hlc_encoder/` — adds `config.h`, plus `config_rev2.h` when `HLC_ENCODER_REV2`.
- `hlc_cirque_trackpad/`.

### `converters/promicro_to_halcyon/`

A ChibiOS converter (`CONVERT_TO=halcyon`, RP2040 / `QMK_PM2040`) that lets non-Halcyon Aurora/Kyria boards use modules through the Halcyon→Pro Micro adapter. `splitkb/converter/converter_rules.mk` forces `LTO`/`OLED` off and then includes the normal `splitkb/rules.mk`.

## Customizing module behavior

See `docs/MODULES.md` (display/encoder/cirque examples) and `docs/PORTING.md` (adding a new keyboard). To change compiled-in module code, edit files under `users/halcyon_modules/splitkb/<module>/`; add custom sources with `SRC += $(CURRENT_DIR)/<file>.c` in a keymap `rules.mk`. `examples/display/` is a drop-in-to-your-keymap sample.

`qmk_old.json` is the full official target list kept for reference; `qmk.json` is the active (usually trimmed) target set.
