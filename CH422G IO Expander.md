CH422G I/O Expander (on I²C)
These board functions are not on ESP32 GPIOs; they’re on the CH422G’s expanded outputs (“EXIOx”), controlled over the same I²C bus as the touch/RTC:
| Expander pin      | Board net / purpose                                                       |
| ----------------- | ------------------------------------------------------------------------- |
| **EXIO1**         | `CTP_RST` (Touch reset)                                                   |
| **EXIO2**         | `DISP` (Panel enable / display power)                                     |
| **EXIO3**         | `LCD_RST` (LCD controller reset)                                          |
| **EXIO4**         | `SDCS` (SD‑Card chip‑select)                                              |
| **EXIO0 / EXIO5** | Used for DI/DO or other board control nets (see DI/DO and mapping area).  |
