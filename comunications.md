| Signal |     ESP32‑S3 GPIO | Notes                                |
| ------ | ----------------: | ------------------------------------ |
| MOSI   |            **11** | `NLIO11 NLMOSI`.                     |
| MISO   |            **13** | `NLIO13 NLMISO`.                     |
| SCK    |            **12** | `NLIO12 NLSCK`.                      |
| CS     | — (via **EXIO4**) | `SDCS` is on CH422G expander EXIO4.  |
| Signal | ESP32‑S3 GPIO | Notes                  |
| ------ | ------------: | ---------------------- |
| RXD    |        **43** | `NLIO43 NLRS4850RXD`.  |
| TXD    |        **44** | `NLIO44 NLRS4850TXD`.  |
| Signal | ESP32‑S3 GPIO | Notes              |
| ------ | ------------: | ------------------ |
| CANTX  |        **15** | `NLIO15 NLCANTX`.  |
| CANRX  |        **16** | `NLIO16 NLCANRX`.  |
RTC (PCF85063A)
| Signal | ESP32‑S3 GPIO | Notes                                              |
| ------ | ------------: | -------------------------------------------------- |
| INT    |         **6** | `NLIO6 NLRTC0INT`. I²C shares SDA/SCL on IO8/IO9.  |
