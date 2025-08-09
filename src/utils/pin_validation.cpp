// Runtime pin validation implementation
#include "utils/pin_validation.h"
#include <driver/gpio.h>

namespace {
uint32_t g_faultMask = 0;

bool isInputOnly(int gpio) {
    // On ESP32-S3, GPIOs 34-39 were input-only on classic ESP32; S3 differs but keep heuristic for portability.
    // Adjust list if needed for S3 specifics.
    switch (gpio) {
        case 34: case 35: case 36: case 37: case 38: case 39:
            return true; // treat as potentially input-only across variants
        default: return false;
    }
}

bool isStrap(int gpio) {
    switch (gpio) {
        case 0: case 3: case 45: case 46: case 48: return true; // include key boot/strap pins for caution
        default: return false;
    }
}
}

namespace PinValidation {

uint32_t getFaultMask() { return g_faultMask; }

void run() {
    g_faultMask = 0;
    // I2C lines should be output-capable open-drain
    if (isInputOnly(I2C_SDA_PIN)) g_faultMask |= FAULT_I2C_SDA_INPUT_ONLY;
    if (isInputOnly(I2C_SCL_PIN)) g_faultMask |= FAULT_I2C_SCL_INPUT_ONLY;

    // Touch IRQ on strap pin? (should be okay but flag if sensitive)
    if (isStrap(TOUCH_IRQ_PIN)) g_faultMask |= FAULT_TOUCH_IRQ_STRAP;

    // Display control lines strap risk
#ifdef DISPLAY_HSYNC_PIN
    if (isStrap(DISPLAY_HSYNC_PIN)) g_faultMask |= FAULT_DISPLAY_HSYNC_STRAP;
#endif
#ifdef DISPLAY_VSYNC_PIN
    if (isStrap(DISPLAY_VSYNC_PIN)) g_faultMask |= FAULT_DISPLAY_VSYNC_STRAP;
#endif
#ifdef DISPLAY_DE_PIN
    if (isStrap(DISPLAY_DE_PIN)) g_faultMask |= FAULT_DISPLAY_DE_STRAP;
#endif
#ifdef DISPLAY_PCLK_PIN
    if (isStrap(DISPLAY_PCLK_PIN)) g_faultMask |= FAULT_DISPLAY_PCLK_STRAP;
#endif

    if (isStrap(RTC_INT_PIN)) g_faultMask |= FAULT_RTC_INT_STRAP;

    if (g_faultMask) {
        Serial.println("[PIN_VALIDATION] Warnings detected:");
        if (g_faultMask & FAULT_I2C_SDA_INPUT_ONLY) Serial.println(" - I2C SDA on potential input-only pin");
        if (g_faultMask & FAULT_I2C_SCL_INPUT_ONLY) Serial.println(" - I2C SCL on potential input-only pin");
        if (g_faultMask & FAULT_TOUCH_IRQ_STRAP) Serial.println(" - TOUCH IRQ on strap pin (review boot behavior)");
        if (g_faultMask & FAULT_DISPLAY_HSYNC_STRAP) Serial.println(" - HSYNC on strap pin (GPIO46)");
        if (g_faultMask & FAULT_DISPLAY_VSYNC_STRAP) Serial.println(" - VSYNC on strap pin (GPIO3 / MTDO)");
        if (g_faultMask & FAULT_DISPLAY_DE_STRAP) Serial.println(" - DE on strap pin");
        if (g_faultMask & FAULT_DISPLAY_PCLK_STRAP) Serial.println(" - PCLK on strap pin");
        if (g_faultMask & FAULT_RTC_INT_STRAP) Serial.println(" - RTC INT on strap pin");
    } else {
        Serial.println("[PIN_VALIDATION] All critical pins pass heuristic checks.");
    }
}

} // namespace PinValidation
