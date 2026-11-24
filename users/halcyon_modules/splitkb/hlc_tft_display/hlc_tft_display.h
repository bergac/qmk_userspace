// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qp.h"
#include "qp_surface.h"

// All values (including hue) are scaled to 0-255
#define HSV_SPLITKB 145, 235, 155
#define HSV_CAPS_OFF 17, 104, 77
#define HSV_CAPS_ON 17, 191, 245
#define HSV_SCROLL_OFF 202, 104, 77
#define HSV_SCROLL_ON 202, 191, 245
#define HSV_NUM_OFF 142, 104, 77
#define HSV_NUM_ON 142, 191, 245

// Dark blue ish
#define HSV_LAYER_0 0, 0, 160
// Orange
#define HSV_LAYER_1 25, 255, 245
// Red
#define HSV_LAYER_2 0, 177, 245
// Purple
#define HSV_LAYER_3 201, 162, 204
#define HSV_LAYER_4 77, 64, 255
#define HSV_LAYER_5 176, 77, 255
#define HSV_LAYER_6 131, 99, 255
#define HSV_LAYER_7 154, 94, 255
// #define HSV_LAYER_8 213, 56, 255
#define HSV_LAYER_UNDEF 0, 255, 255

// Grey
#define HSV_LAYER_NAME_OFF 153, 153, 153

extern painter_device_t lcd;
extern painter_device_t lcd_surface;

void draw_grid(void);
void update_grid(void);
void init_grid(void);
void add_cell_cluster(void);
uint8_t get_random_color_index(void);
void update_display(void);
void backlight_wakeup(void);
void backlight_suspend(void);
