USB (native)
USB D+ / D‑: ESP_USB_P / ESP_USB_N routed directly to the Type‑C connector; use TinyUSB in IDF/Arduino (no separate GPIO assignment). 

Boot / Strapping pins
The schematic calls out IO0, IO45, IO46 as strapping pins (affect boot mode, voltage options, etc.). Avoid pulling them to wrong levels at reset. 

Power / Backlight
Backlight is driven by a boost LED driver; logic enable for the panel/backlight is DISP (via EXIO2). Supply nets: VLED+, VLED− shown in the “LCD Backlight Power” block. 

Schematic references
Page 1: Full board schematic, LCD connector & Mapping Pins table; LCD HSYNC/VSYNC/DE/PCLK and RGB nets to ESP32 GPIOs; I²C and SPI nets; RS‑485, CAN, RTC.

Page 2: Board outline / connector placement (PORT1, SD‑CARD, USB, battery switch, etc.). 