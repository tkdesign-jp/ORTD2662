#pragma once

#include "core/config_defines.h"

/**
 * X@RTD2662 board (VGA + HDMI + AV)
 * ★このファイルの値は未実測。第1弾では GPIO 出力を一切行わない設定で焼く。
**/

#define RTD_FREQ                        27000000UL   // ★未確認: 基板の水晶を要目視

// --- GPIO: 未実測のため無効化。特定後に 1 にする ---
#define BOARD_GPIO_KNOWN                0
#define BOARD_MIRROR_PINS_KNOWN         0

#define DISPLAY_POWER_ENABLE_PIN        PIN110   // P3_3 (PCB800099系の慣行)
#define DISPLAY_POWER_ACTIVE_LEVEL      ACTIVE_LEVEL_HIGH
#define BACKLIGHT_ENABLE_PIN            PIN070   // P1_6/GPIO16 実測で特定
#define BACKLIGHT_ENABLE_ACTIVE_LEVEL   ACTIVE_LEVEL_LOW
#define MIRROR_VERTICAL_PIN             PIN098
#define MIRROR_HORIZONTAL_PIN           PIN099
#define KEYBOARD_ADC_PIN                PIN053

// --- Interface ---
// TMDS Port 0 = HDMI
#define TMDS0_TYPE      TMDS_HDMI
#define TMDS0_DDC       DDC1
#define TMDS0_SWAP_RB   TRUE    // ★未確認: 基板配線依存。赤青が入れ替わったら反転
#define TMDS0_SWAP_PN   TRUE    // ★未確認: 映らない場合の第一候補

// TMDS Port 1 (未使用)
#define TMDS1_TYPE      TMDS_DVI
#define TMDS1_DDC       DDC2
#define TMDS1_SWAP_RB   TRUE
#define TMDS1_SWAP_PN   FALSE
