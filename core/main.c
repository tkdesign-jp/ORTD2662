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

    // Display and backlight power
    SetGPIOShare(DISPLAY_POWER_ENABLE_PIN, PUSH_PULL_OUT);
    SetGPIO(DISPLAY_POWER_ENABLE_PIN, 1^!DISPLAY_POWER_ACTIVE_LEVEL);
    SetGPIOShare(BACKLIGHT_ENABLE_PIN, PUSH_PULL_OUT);
    SetGPIO(BACKLIGHT_ENABLE_PIN, 1^!BACKLIGHT_ENABLE_ACTIVE_LEVEL);

    // Display mirroring pins
    SetGPIOShare(MIRROR_HORIZONTAL_PIN, PUSH_PULL_OUT);
    SetGPIOShare(MIRROR_VERTICAL_PIN, PUSH_PULL_OUT);
    SetGPIO(MIRROR_HORIZONTAL_PIN, HOR_MIRRROR);
    SetGPIO(MIRROR_VERTICAL_PIN, VER_MIRRROR);

    InitScaler();
    SetOverlayColor(0xff, 0xff, 0xff);

/////////////////////////////////////

    // TODO: Modify scaler_registers.h to use these macro names
#   define SP_TCON_VSTA_LO(n)       (0x08 + n * 8) // TCON[n] Vertical Start LByte
#   define SP_TCON_VEND_VSTA_HI(n)  (0x09 + n * 8) // TCON[n] Vertical End/Start HByte
#   define SP_TCON_VEND_LO(n)       (0x0A + n * 8) // TCON[n] Vertical End LByte
#   define SP_TCON_HSTA_LO(n)       (0x0B + n * 8) // TCON[n] Horizontal Start LByte
#   define SP_TCON_HEND_HSTA_HI(n)  (0x0C + n * 8) // TCON[n] Horizontal End/Start HByte
#   define SP_TCON_HEND_LO(n)       (0x0D + n * 8) // TCON[n] Horizontal End LByte
#   define SP_TCON_CONTROL(n)       (0x0E + n * 8) // TCON[n] Control

#   define TCON_VSTA (PANEL_V_SYNC_WIDTH + PANEL_V_BACK_PORCH - 1)
#   define TCON_VEND (PANEL_V_SYNC_WIDTH + PANEL_V_BACK_PORCH + PANEL_V_ACTIVE + 1)
#   define TCON_HSTA (0x000)
#   define TCON_HEND (0xFFF)

    SetGPIOShare(PIN097, 0b100); // DVS on pin 97
    SetGPIOShare(PIN096, 0b100); // DCK on pin 96
    ScalerWritePortBit(S_TCON_PORT, SP_TCON_CONTROL0, 7, 0b1); // Enable timing controller function

    // Vertical timings for TCON[0]
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VSTA_LO(0), TCON_VSTA); // Set signal start line number low
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VEND_VSTA_HI(0),
        ((TCON_VEND & 0xF00) >> 4) | // Set signal stop line number high
        (TCON_VSTA >> 8));   // Set signal start line number high
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VEND_LO(0), TCON_VEND); // Set signal stop line number low
    // Horizontal timings for TCON[0]
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HSTA_LO(0), TCON_HSTA); // Set signal start pixel number low
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HEND_HSTA_HI(0),
        ((TCON_HEND & 0xF00) >> 4) | // Set signal stop pixel number high
        (TCON_HSTA >> 8));   // Set signal start pixel number high
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HEND_LO(0), TCON_HEND); // Set signal stop pixel number low
    // Control register for TCON[0]
    ScalerWritePortBit(S_TCON_PORT, SP_TCON_CONTROL(0), 7, 0b1); // Enable TCON
    ScalerWritePortBits(S_TCON_PORT, SP_TCON_CONTROL(0), 0, 3, 0b100); // Invert data bus when TCON[0] is 0

/////////////////////////////////////

#if 0
    // Set TCON[1] to TCON[0] and output to extra pin for debugging

    SetGPIOShare(PIN065, 0b100);
    // Vertical timings for TCON[1]
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VSTA_LO(1), TCON_VSTA); // Set signal start line number low
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VEND_VSTA_HI(1),
        ((TCON_VEND & 0xF00) >> 4) | // Set signal stop line number high
        (TCON_VSTA >> 8));   // Set signal start line number high
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_VEND_LO(1), TCON_VEND); // Set signal stop line number low
    // Horizontal timings for TCON[1]
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HSTA_LO(1), TCON_HSTA); // Set signal start pixel number low
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HEND_HSTA_HI(1),
        ((TCON_HEND & 0xF00) >> 4) | // Set signal stop pixel number high
        (TCON_HSTA >> 8));   // Set signal start pixel number high
    ScalerWritePortByte(S_TCON_PORT, SP_TCON_HEND_LO(1), TCON_HEND); // Set signal stop pixel number low
    // Control register for TCON[1]
    ScalerWritePortBit(S_TCON_PORT, SP_TCON_CONTROL(1), 7, 0b1); // Enable TCON[0]
    ScalerWritePortBits(S_TCON_PORT, SP_TCON_CONTROL(1), 0, 3, 0b000); // Data inversion mode (?)
#endif

    while(1) ; // Wait on overlay output

    // On screen display example
    OSDInit();
    char* entries[]= { "scaling", "other" };
    OSDCreateMenu("Settings", entries, 2);
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
    SetCaptureWindow(0, 0, hact, vact, 1, 1);
    SetFIFOWindow(hact, vact);

    // Measure in analog mode for Frequencies
    MeasureSignal(0);

    // Adjust display frequency manualy if picture flickering artifacts present
    // Magic formula Fdisplay = DisplayHTotal*InputHFreq*DisplayVActive/InputVActive
    SetDPLLFrequncy((PANEL_H_SYNC_WIDTH + PANEL_H_BACK_PORCH + PANEL_H_ACTIVE + PANEL_H_FRONT_PORCH) *
                    InputMeasData.HFreq / vact * PANEL_V_ACTIVE);

    #   ifdef __SDCC
        while(1);
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
