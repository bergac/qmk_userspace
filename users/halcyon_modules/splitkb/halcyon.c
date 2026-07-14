// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "halcyon.h"
#include "transactions.h"
#include "split_util.h"
#include "_wait.h"

#include "matrix.h"

__attribute__((weak)) void module_suspend_power_down_kb(void);
__attribute__((weak)) void module_suspend_wakeup_init_kb(void);

__attribute__((weak)) bool module_post_init_kb(void) {
    return module_post_init_user();
}
__attribute__((weak)) bool module_housekeeping_task_kb(void) {
    return module_housekeeping_task_user();
}
__attribute__((weak)) bool display_module_housekeeping_task_kb(bool second_display) {
    return display_module_housekeeping_task_user(second_display);
}

__attribute__((weak)) bool module_post_init_user(void) {
    return true;
}
__attribute__((weak)) bool module_housekeeping_task_user(void) {
    return true;
}
__attribute__((weak)) bool display_module_housekeeping_task_user(bool second_display) {
    return true;
}

module_t module_master;
module_t module;
#ifdef HLC_NONE
    module_t module = hlc_none;
#endif
#ifdef HLC_CIRQUE_TRACKPAD
    module_t module = hlc_cirque_trackpad;
#endif
#ifdef HLC_ENCODER
    module_t module = hlc_encoder;
#endif
#ifdef HLC_TFT_DISPLAY
    module_t module = hlc_tft_display;
#endif

#ifdef SPLIT_KEYBOARD
#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

bool backlight_off = false;

#ifndef HALCYON_LEGACY
#   define VIRTUAL_COL_START (MATRIX_COLS - 5)
#endif // HALCYON_LEGACY

extern matrix_row_t matrix[MATRIX_ROWS];

#ifndef BUTTON_PINS
#   define BUTTON_PINS  (const uint8_t[]){ }
#endif

// Timeout handling
void backlight_wakeup(void) {
    backlight_off = false;
    backlight_enable();
    if (get_backlight_level() == 0) {
        backlight_level(BACKLIGHT_LEVELS);
    }
}

// Timeout handling
void backlight_suspend(void) {
    backlight_off = true;
    backlight_disable();
}

void module_sync_slave_handler(uint8_t initiator2target_buffer_size, const void* initiator2target_buffer, uint8_t target2initiator_buffer_size, void* target2initiator_buffer) {
    if (initiator2target_buffer_size == sizeof(module)) {
        memcpy(&module_master, initiator2target_buffer, sizeof(module_master));
    }
}

void suspend_power_down_kb(void) {
    module_suspend_power_down_kb();

    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    module_suspend_wakeup_init_kb();

    suspend_wakeup_init_user();
}

void keyboard_post_init_kb(void) {
    // Register module sync split transaction
    transaction_register_rpc(MODULE_SYNC, module_sync_slave_handler);

    // Do any post init for modules
    module_post_init_kb();

    // User post init
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    if (is_keyboard_master()) {
        static bool synced = false;

        if (!synced) {
            if(is_transport_connected()) {
                transaction_rpc_send(MODULE_SYNC, sizeof(module), &module); // Sync to slave
                wait_ms(10);
                // Good moment to make sure the backlight wakes up after boot for both halves
                backlight_wakeup();
                synced = true;
            }
        }

        display_module_housekeeping_task_kb(false); // Is master so can never be the second display
    }

    if (!is_keyboard_master()) {
        display_module_housekeeping_task_kb(module_master == hlc_tft_display);
    }

    // Backlight feature
    if (last_input_activity_elapsed() <= HLC_BACKLIGHT_TIMEOUT) {
        if (backlight_off) {
            backlight_wakeup();
        }
    } else {
        if (!backlight_off) {
            backlight_suspend();
        }
    }

    module_housekeeping_task_kb();

    housekeeping_task_user();
}

report_mouse_t pointing_device_task_combined_kb(report_mouse_t left_report, report_mouse_t right_report) {
    // Only runs on master
    // Fixes the following bug: If master is right and master is NOT a cirque trackpad, the inputs would be inverted.
    if(module != hlc_cirque_trackpad && !is_keyboard_left()) {
        mouse_xy_report_t x = left_report.x;
        mouse_xy_report_t y = left_report.y;
        left_report.x = -x;
        left_report.y = -y;
    }
    return pointing_device_task_combined_user(left_report, right_report);
}

#ifdef HALCYON_LEGACY
static inline uint8_t legacy_button_row(void) {
#ifdef SPLIT_KEYBOARD
    return is_keyboard_left() ? ROWS_PER_HAND : (MATRIX_ROWS - 1);
#else
    return MATRIX_ROWS - 1;
#endif
}
#endif // HALCYON_LEGACY

void matrix_init_kb(void) {
    size_t num_pins = sizeof(BUTTON_PINS)/sizeof(BUTTON_PINS[0]);
    for (uint8_t i = 0; i < num_pins; i++) {
        gpio_set_pin_input_high(BUTTON_PINS[i]);
    }
}

#ifdef HALCYON_LEGACY
static void scan_legacy_buttons(void) {
    size_t num_pins = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);
    if (num_pins == 0) return;

    uint8_t row = is_keyboard_left() ? ROWS_PER_HAND : (MATRIX_ROWS - 1);

    for (size_t i = 0; i < num_pins && i < MATRIX_COLS; i++) {
        if (gpio_read_pin(BUTTON_PINS[i]) == 0) {
            matrix[row] |= ((matrix_row_t)1 << i);
        } else {
            matrix[row] &= ~((matrix_row_t)1 << i);
        }
    }
}

#else

static void scan_buttons(void) {
    size_t num_pins = sizeof(BUTTON_PINS)/sizeof(BUTTON_PINS[0]);
    if (num_pins == 0) return;

    uint8_t row = is_keyboard_left() ? 0 : ROWS_PER_HAND;

    for (size_t i = 0; i < num_pins; i++) {
        if (gpio_read_pin(BUTTON_PINS[i]) == 0) {
            matrix[row] |= (1 << (VIRTUAL_COL_START + i));
        } else {
            matrix[row] &= ~(1 << (VIRTUAL_COL_START + i));
        }
    }
}
#endif

void matrix_scan_kb(void) {
#ifdef HALCYON_LEGACY
    scan_legacy_buttons();
#else
    scan_buttons();
#endif

    matrix_scan_user();
}

void matrix_slave_scan_kb(void) {
#ifdef HALCYON_LEGACY
    scan_legacy_buttons();
#else
    scan_buttons();
#endif

    matrix_slave_scan_user();
}

#ifndef HALCYON_LEGACY
#if (defined(HALCYON_BUTTONS_ENABLE) || !defined(VIAL_ENABLE))
__attribute__((weak)) const uint16_t left_halcyon_buttons[10][5];
__attribute__((weak)) const uint16_t right_halcyon_buttons[10][5];

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.key.col >= VIRTUAL_COL_START) {
        uint8_t btn = record->event.key.col - VIRTUAL_COL_START;

        if (btn < 5) {
            uint8_t max_layer = get_highest_layer(layer_state | default_layer_state);

            for (int8_t l = max_layer; l >= 0; l--) {
                if (!((layer_state | default_layer_state) & (1UL << l))) continue;

                uint16_t code = KC_TRNS;

                if (is_keyboard_left()) {
                    code = left_halcyon_buttons[l][btn];
                } else {
                    code = right_halcyon_buttons[l][btn];
                }

                if (code != KC_TRNS) {
                    record->event.pressed ? register_code16(code) : unregister_code16(code);
                    break;
                }
            }
        }
        return false;
    }

    return process_record_user(keycode, record);
}
#endif // HALCYON_BUTTONS_ENABLE || VIAL_ENABLE
#endif // HALCYON_LEGACY
