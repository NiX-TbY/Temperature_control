| Function |     ESP32‑S3 GPIO | Notes                                                 |
| -------- | ----------------: | ----------------------------------------------------- |
| I²C SDA  |             **8** | `NLIO8 NLSDA` → Touch/expander/RTC bus.               |
| I²C SCL  |             **9** | `NLIO9 NLSCL` (same I²C bus).                         |
| IRQ      |             **4** | `NLIO4 NLCTP0IRQ` (interrupt from touch controller).  |
| RST      | — (via **EXIO1**) | `CTP_RST` is on CH422G expander pin EXIO1.            |
