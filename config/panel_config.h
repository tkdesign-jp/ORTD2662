#pragma once

#include "core/config_defines.h"

/**
 * Panel interface select
**/
#define PANEL_TYPE TTL
#define PANEL_18_BIT      FALSE
#define PANEL_DOUBLE_PORT FALSE // Only applicable when PANEL_TYPE==LVDS

/**
 * Panel resolution and timings
**/
//          |HPW|   HBP   |     HACT     | HFP |
//          ┐   ┌───────────────────────────────
//          └───┘         |              |     |
// ---- └─┐ +----------------------------------+
//  VPW   │ |             |              |     |
// ---- ┌─┘ |                                  |
//  VBP │   |             |              |     |
// ---- │- -|- - - - - - -+--------------+     |
//      │   |             |              |     |
// VACT │   |             | ACTIVE VIDEO |     |
//      │   |             |              |     |
// ---- │- -|- - - - - - -+--------------+     |
//  VFP │   |                                  |
// ---- │   +----------------------------------+
//

// Panel pixel clock (aka data clock) (Hz)
#define PANEL_DCLK          33000000UL // NOTE:  for my display 40.8 - min; 67.2 - max

// Horizontal timings
#define PANEL_H_SYNC_WIDTH  41     // HPW
#define PANEL_H_BACK_PORCH  82     // HBP
#define PANEL_H_ACTIVE      800    // HACT, AKA: Horizontal resolution
#define PANEL_H_FRONT_PORCH 4      // HFP

// Vertical timings
#define PANEL_V_SYNC_WIDTH  6      // VPW
#define PANEL_V_BACK_PORCH  27     // VBP
#define PANEL_V_ACTIVE      480    // VACT, AKA: Vertical resolution
#define PANEL_V_FRONT_PORCH 22     // VFP

/**
 * Panel signal polarity and data order
**/
#define PANEL_INVERT_DCLK       FALSE
#define PANEL_INVERT_DHS        TRUE   //BRICE FALSE
#define PANEL_INVERT_DVS        TRUE   //BRICE FALSE
#define PANEL_INVERT_DEN        FALSE

#define PANEL_SWAP_BIT_ORDER    FALSE
#define PANEL_SWAP_RED_BLU      FALSE
#define PANEL_SWAP_ODD_EVEN     FALSE
