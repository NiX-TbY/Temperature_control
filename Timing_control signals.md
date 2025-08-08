| Function | ESP32‑S3 GPIO | Schematic label                                 |
| -------- | ------------: | ----------------------------------------------- |
| HSYNC    |        **46** | `IO46 HSYNC` (Mapping area)                     |
| VSYNC    |         **3** | `IO3 VSYNC` (Mapping area)                      |
| PCLK     |         **7** | `IO7 PCLK` (Mapping area)                       |
| DE       |         **5** | `IO5` → `DE` (Mapping area shows `NLIO5 NLDE`)  |
| Bit | ESP32‑S3 GPIO | Evidence                      |
| --: | ------------: | ----------------------------- |
|  R3 |         **1** | `NLIO1 NLR3` (Mapping pins)   |
|  R4 |         **2** | `NLIO2 NLR4` (Mapping pins)   |
|  R5 |        **42** | `NLIO42 NLR5` (Mapping pins)  |
|  R6 |        **41** | `NLIO41 NLR6` (Mapping pins)  |
|  R7 |        **40** | `NLIO40 NLR7` (Mapping pins)  |
| Bit | ESP32‑S3 GPIO | Evidence                      |
| --: | ------------: | ----------------------------- |
|  G2 |        **39** | `NLIO39 NLG2` (Mapping pins)  |
|  G3 |         **0** | `NLIO0 NLG3` (Mapping pins)   |
|  G4 |        **45** | `NLIO45 NLG4` (Mapping pins)  |
|  G5 |        **48** | `NLIO48 NLG5` (Mapping pins)  |
|  G6 |        **47** | `NLIO47 NLG6` (Mapping pins)  |
|  G7 |        **21** | `NLIO21 NLG7` (Mapping pins)  |
| Bit | ESP32‑S3 GPIO | Evidence                      |
| --: | ------------: | ----------------------------- |
|  B3 |        **14** | `NLIO14 NLB3` (Mapping pins)  |
|  B4 |        **38** | `NLIO38 NLB4` (Mapping pins)  |
|  B5 |        **18** | `NLIO18 NLB5` (Mapping pins)  |
|  B6 |        **17** | `NLIO17 NLB6` (Mapping pins)  |
|  B7 |        **10** | `NLIO10 NLB7` (Mapping pins)  |

The schematic’s “Mapping Pins” table and labels around PORT1 show this 16‑bit RGB565 wiring (R3–R7, G2–G7, B3–B7) plus HSYNC/VSYNC/DE/PCLK