#pragma once

#include "core/config_defines.h"

/**
 * Toshiba LTM09C362V  8.9" 1024x600  LVDS 1ch 6bit
 * 出典: LTM09C362T データシート (127569.pdf)
 *   NCLK typ 50.4MHz (max 52.6) / HTotal 1320-1344 / VTotal 610-625typ-635
 *   HSW=8ts / VSW 3-7 / tfd(VBP)>=7 / tvd=600固定 / 56-60Hz
 * 1344 x 625 x 60Hz = 50.4MHz ちょうど → typ値の組で確定
**/
#define PANEL_TYPE LVDS
#define PANEL_18_BIT      TRUE     // 6bit/color = 18bit
#define PANEL_DOUBLE_PORT FALSE    // 1ch

// Panel pixel clock (Hz)
#define PANEL_DCLK          50400000UL

// Horizontal (合計 1344)
#define PANEL_H_SYNC_WIDTH  8      // HPW  データシート値
#define PANEL_H_BACK_PORCH  24     // HBP 160から136減らした（巻き込み量が追従するか検証）
#define PANEL_H_ACTIVE      1024   // HACT
#define PANEL_H_FRONT_PORCH 288    // HFP 152から136増やした。HTotal 1344は不変

// Vertical (合計 625)
#define PANEL_V_SYNC_WIDTH  3      // VPW  データシート範囲3-7の下限
#define PANEL_V_BACK_PORCH  13     // VBP  tfd>=7を満たす。active開始=16行目
#define PANEL_V_ACTIVE      600    // VACT
#define PANEL_V_FRONT_PORCH 9      // VFP  残り

/**
 * Panel signal polarity and data order
**/
#define PANEL_INVERT_DCLK       FALSE
#define PANEL_INVERT_DHS        FALSE
#define PANEL_INVERT_DVS        FALSE
#define PANEL_INVERT_DEN        FALSE

#define PANEL_SWAP_BIT_ORDER    FALSE  // MAP2(TRUE)は改悪だったので戻す
#define PANEL_SWAP_RED_BLU      FALSE
#define PANEL_SWAP_ODD_EVEN     FALSE
