// LCD timing
#define LCD_HSYNC_GPIO   46   // HSYNC
#define LCD_VSYNC_GPIO    3   // VSYNC
#define LCD_PCLK_GPIO     7   // PCLK
#define LCD_DE_GPIO       5   // DE (data enable)

// RGB565 bus
// Red bits
#define LCD_R3_GPIO       1
#define LCD_R4_GPIO       2
#define LCD_R5_GPIO      42
#define LCD_R6_GPIO      41
#define LCD_R7_GPIO      40
// Green bits
#define LCD_G2_GPIO      39
#define LCD_G3_GPIO       0
#define LCD_G4_GPIO      45
#define LCD_G5_GPIO      48
#define LCD_G6_GPIO      47
#define LCD_G7_GPIO      21
// Blue bits
#define LCD_B3_GPIO      14
#define LCD_B4_GPIO      38
#define LCD_B5_GPIO      18
#define LCD_B6_GPIO      17
#define LCD_B7_GPIO      10

// Touch (CTP)
#define CTP_SDA_GPIO      8   // I2C SDA
#define CTP_SCL_GPIO      9   // I2C SCL
#define CTP_IRQ_GPIO      4   // Touch interrupt
// CTP_RST is driven by the CH422G expander (EXIO1)

// SD‑Card (SPI mode)
#define SD_MOSI_GPIO     11
#define SD_MISO_GPIO     13
#define SD_SCK_GPIO      12
// SD_CS is driven by the CH422G expander (EXIO4)

// Other peripherals
#define RS485_RXD_GPIO   43
#define RS485_TXD_GPIO   44
#define CAN_TX_GPIO      15
#define CAN_RX_GPIO      16
#define RTC_INT_GPIO      6   // PCF85063A INT
// LCD_RST and DISP (panel enable) are on the CH422G expander (EXIO3, EXIO2)
Notes
• USB uses native S3 pins ESP_USB_P / ESP_USB_N (not regular GPIOs).
• IO0, IO45, IO46 are strapping pins (see “Strapping Pins” box on the schematic); keep their states in mind during boot. 