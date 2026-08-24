#include <stdint.h>
#include <stdio.h>

#include "config/board_config.h"
#include "config/panel_config.h"
#include "config/misc_config.h"

#include "scaler/scaler.h"
#include "scaler/scaler_registers.h"
#include "scaler/scaler_access.h"
#include "scaler/measure.h"
#include "scaler/scaling.h"

#include "peripherals/pins.h"
#include "peripherals/xsfr.h"
#include "peripherals/timer.h"
#include "peripherals/ddc.h"

#include "interfaces/hdmi.h"

#include "osd/osd.h"
#include "osd/osd_ui.h"

void main()
{
#   ifdef __SDCC
    XSFRWriteByte(WDT_CONTROL, 0x00); // Disable watchdog
    InitSysTimer();
    EA = 1;
#   endif

    // Upload EDID to DDC port 1
    UploadEDID(1, testEDID);

#   if BOARD_GPIO_KNOWN
    // Display and backlight power
    SetGPIOShare(DISPLAY_POWER_ENABLE_PIN, PUSH_PULL_OUT);
    SetGPIO(DISPLAY_POWER_ENABLE_PIN, 1^!DISPLAY_POWER_ACTIVE_LEVEL);
    SetGPIOShare(BACKLIGHT_ENABLE_PIN, PUSH_PULL_OUT);
    SetGPIO(BACKLIGHT_ENABLE_PIN, 1^!BACKLIGHT_ENABLE_ACTIVE_LEVEL);
#   endif

#   if BOARD_MIRROR_PINS_KNOWN
    // Display mirroring pins
    SetGPIOShare(MIRROR_HORIZONTAL_PIN, PUSH_PULL_OUT);
    SetGPIOShare(MIRROR_VERTICAL_PIN, PUSH_PULL_OUT);
    SetGPIO(MIRROR_HORIZONTAL_PIN, HOR_MIRRROR);
    SetGPIO(MIRROR_VERTICAL_PIN, VER_MIRRROR);
#   endif

    InitScaler();

    // ---- ガンマ設定 (純正CAdjustGamma FULL_NORMAL, GAMMA_1, 既定index=1) ----
    // 8bit入力を10bit化する段。ディザはこの10bit出力に対して働く（純正では対で初期化される）
    {
        #include "core/gamma_tables.inc"
        ScalerWriteByte(0x67, 0x80 | 1);   // R書き込みモード | FULL_GAMMA_NORMAL_TABLE(=1)
        ScalerWriteBytes(0x66, (uint8_t*)gammaR, 384);
        ScalerWriteByte(0x67, 0x90 | 1);   // G
        ScalerWriteBytes(0x66, (uint8_t*)gammaG, 384);
        ScalerWriteByte(0x67, 0xA0 | 1);   // B
        ScalerWriteBytes(0x66, (uint8_t*)gammaB, 384);
        ScalerWriteByte(0x67, 0x40);       // 書き込み終了 + ガンマ有効(bit6) -> 純正実測CR67=0x40と一致
    }

    // ---- ディザ設定 (純正CAdjustDitherの移植, 18bitパネル用 10->6) ----
    {
        static const uint8_t ditherSeq[24] = {
            0xe4,0xa2,0x05,0x37,0xf6,0x31,0x69,0xcb,0x1f,0xd2,0xb0,0xe6,
            0x45,0x1b,0x87,0xc6,0x9e,0xb4,0xc6,0x38,0xd4,0xdb,0x12,0x1b };
        static const uint8_t ditherTbl[24] = {
            0x07,0xf8,0x69,0x1e,0xad,0x52,0xc3,0xb4,
            0xad,0x52,0xc3,0xb4,0x07,0xf8,0x69,0x1e,
            0xad,0x52,0x69,0x1e,0xc3,0xb4,0x07,0xf8 };
        ScalerWriteBits(S_DITHER_CONTROL, 6, 2, 0b01);
        ScalerWriteBytes(S_DITHER_DATA, (uint8_t*)ditherSeq, 24);
        ScalerWriteBits(S_DITHER_CONTROL, 6, 2, 0b10);
        ScalerWriteBytes(S_DITHER_DATA, (uint8_t*)ditherTbl, 24);
        ScalerWriteByte(S_DITHER_CONTROL, 0x38);
    }
    SetOverlayColor(0xff, 0xff, 0x00);

    OSDInit();
    SetOverlayColor(0x00, 0xff, 0xff);

    // Initialize first TMDS port
    InitHDMI(0);

    // Measure in digital mode
    MeasureSignal(1);
    // After digital measure InputMeasData.HSync = HActive - 1, InputMeasData.VTotal = VActive - 1
    uint32_t hact = InputMeasData.HSync + 1, vact = InputMeasData.VTotal + 1;

    // Scale up and/or down
    ScaleUp  (hact, vact, PANEL_H_ACTIVE, PANEL_V_ACTIVE);
    ScaleDown(hact, vact, PANEL_H_ACTIVE, PANEL_V_ACTIVE);

    // Adjust HStart, VStart, HDelay, VDelay if picture is shifted
    // 垂直18行の補正はV窓開始(CR18/19)のみで行う。VSディレイ(CR1C)は行途中に落ちて半ライン回転を起こすため使わない(v17の教訓)
    SetCaptureWindow(0, 18, hact, vact, 1, 1);
    SetFIFOWindow(hact, vact);

    // Measure in analog mode for Frequencies
    MeasureSignal(0);


    // Adjust display frequency manualy if picture flickering artifacts present
    // Magic formula Fdisplay = DisplayHTotal*InputHFreq*DisplayVActive/InputVActive
    SetDPLLFrequncy((PANEL_H_SYNC_WIDTH + PANEL_H_BACK_PORCH + PANEL_H_ACTIVE + PANEL_H_FRONT_PORCH) *
                    InputMeasData.HFreq / vact * PANEL_V_ACTIVE);

    #   ifdef __SDCC
        // ---- 候補8本すべてを出力Lowで保持（スイープ時のknown-good状態）----
        {
            static const uint8_t cand[] = { PIN064, PIN069, PIN070, PIN071,
                                            PIN112, PIN114, PIN056, PIN057 };
            uint8_t k;
            for (k = 0; k < sizeof(cand); k++)
            {
                SetGPIOShare(cand[k], PUSH_PULL_OUT);
                SetGPIO(cand[k], 0);
            }
            while(1);
        }
    #   else
        getchar();
    #   endif
}


// Leftover junk
/*

    #include "alien/struct_.h"

    // InitComposite(2);

    // InitVGA();

    // ScalerWriteByte(S_SYNC_CONTROL, 0x06); // Select ADC Sync, Select SeperateHSync
    // SetCaptureWindow(216, 27, 800, 600, 0, 0);
    // SetFIFOWindow(1024, 600);
    // SetAPLLFrequncy(40000000UL, 1056);
    // ScaleUp(800, 600, 1024, 600);
    // Magic formula Fdisplay = DisplayHTotal*InputHFreq*DisplayVActive/InputVActive
    // ScalerWriteBit(SCALER_CONTROL, 4, 0b1); // Enable Full line buffer ?

    #ifndef __SDCC
    stModeInfo.IHWidth = 696;
    stModeInfo.IHStartPos = 141;
    stModeInfo.IHTotal = 858;
    stModeInfo.IHFreq = 157;
    stModeInfo.IVHeight = 232;
    stModeInfo.IVStartPos = 26;
    stModeInfo.IVTotal = 300;
    stModeInfo.IVFreq = 60;
    #endif*/