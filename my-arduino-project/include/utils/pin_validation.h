// Pin validation helpers: compile-time and runtime checks for critical signals.
#pragma once

#include <Arduino.h>
#include "config/config.h"
#include "config/feature_flags.h"

namespace PinValidation {

// Run lightweight runtime validation (called during self-test)
void run();

// Returns bitmask of issues detected (0 = OK)
uint32_t getFaultMask();

// Bitmask definitions
enum : uint32_t {
    FAULT_NONE = 0,
    FAULT_I2C_SDA_INPUT_ONLY      = 1u << 0,
    FAULT_I2C_SCL_INPUT_ONLY      = 1u << 1,
    FAULT_TOUCH_IRQ_STRAP         = 1u << 2,
    FAULT_DISPLAY_HSYNC_STRAP     = 1u << 3,
    FAULT_DISPLAY_VSYNC_STRAP     = 1u << 4,
    FAULT_DISPLAY_DE_STRAP        = 1u << 5,
    FAULT_DISPLAY_PCLK_STRAP      = 1u << 6,
    FAULT_RTC_INT_STRAP           = 1u << 7,
};

} // namespace PinValidation

// --- Compile-time advisory warnings for known ESP32-S3 strapping / sensitive pins ---
#if (DISPLAY_HSYNC_PIN == 46)
#warning "HSYNC on GPIO46 (strap / input-only). Ensure board routing supports output drive or relocate."
#endif
#if (DISPLAY_VSYNC_PIN == 3)
#warning "VSYNC on GPIO3 (strap: MTDO). Confirm no boot mode conflict."
#endif
#if (LCD_PIN_G3 == 0)
#warning "Using GPIO0 for G3 (boot strap). Must be held HIGH at reset."
#endif
#if (LCD_PIN_G4 == 45)
#warning "Using GPIO45 (input-only) for data – verify panel timing tolerates input-only pin (should be avoided)."
#endif
#if (LCD_PIN_G2 == 39)
// GPIO39 is OK (RTC IO) but note for analog capabilities only.
#endif
