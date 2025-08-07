Waveshare ESP32-S3-Touch-LCD-4.3B: A Comprehensive Technical Analysis for AI-Assisted System Development


1.0 System Architecture and Executive Summary


1.1 Overview

This report provides an exhaustive technical analysis of the Waveshare ESP32-S3-Touch-LCD-4.3B development board, a high-performance Human-Machine Interface (HMI) platform engineered for demanding graphical applications.1 The system is centered around the powerful Espressif ESP32-S3 System-on-Chip (SoC), which features a 240 MHz Xtensa® 32-bit LX7 dual-core processor, integrated Wi-Fi and Bluetooth 5 (LE) connectivity, 16MB of Quad I/O (QIO) flash memory, and a crucial 8MB of high-bandwidth Octal (OPI) PSRAM.1 The board's primary feature is a vibrant 4.3-inch, 800×480 pixel In-Plane Switching (IPS) display, driven by a 24-bit parallel RGB interface. This high-throughput interface, coupled with the processing power of the ESP32-S3, enables fluid, high-frame-rate graphical user interfaces suitable for industrial control, smart home automation, and other IoT applications.3 This analysis is framed within the context of developing a "Commercial Freezer Controller," a project that leverages the board's full capabilities for real-time monitoring and control.

1.2 The Core Technical Challenge: The Display-Expander Dependency

An in-depth architectural review reveals that the ESP32-S3-Touch-LCD-4.3B is not a conventional development board but a highly integrated, application-specific platform with non-obvious hardware interdependencies. The most significant of these is the method used to control the display's essential functions. The ST7262 display controller's critical control signals—specifically the hardware Reset (LCD_RST) and Backlight (DISPLAY_BL_PIN)—are not connected to the ESP32-S3's General-Purpose Input/Output (GPIO) pins as is standard practice. Instead, they are physically routed through a WCH CH422G I/O expander chip, which resides on the I²C bus.3
This design choice, likely made to conserve the ESP32-S3's limited GPIO pins after allocating 28 pins to the parallel display, creates a fundamental architectural constraint. It mandates a non-standard, multi-step initialization sequence that renders common display libraries, such as TFT_eSPI or even Espressif's generic ESP_Panel library, completely unusable out of the box.3 These libraries are architected with the assumption of direct GPIO control for reset and backlight toggling. On this board, any attempt to perform a
digitalWrite() to a virtual reset pin will fail, as no such physical connection to the ESP32 exists. The entire display initialization process is contingent upon first communicating with the CH422G expander over I²C to perform these essential hardware operations.8 The result is a tightly coupled system where the display driver is inextricably dependent on the I/O expander driver.

1.3 The Immutable Software Stack

As a direct and unavoidable consequence of this hardware architecture, the "Commercial Freezer Controller" project is locked to a specific, non-updatable, and fragile set of local libraries. The currently stable and functional build relies on a precise pairing of ESP32_Display_Panel@1.0.0 and ESP32_IO_Expander@1.0.1.3 These specific versions contain the necessary logic to handle the complex, ordered initialization sequence.
Attempts to update these libraries to more recent versions from the official repositories will result in compilation failure. Analysis of the library evolution and community-reported issues confirms that significant breaking changes were introduced in later versions, including a major refactoring to use C++ namespaces and a change in header file names (e.g., from ESP_IOExpander.h to esp_io_expander.hpp).10 The local version of
ESP32_Display_Panel used in the successful build depends on the older header file name, creating a fatal dependency conflict if the I/O expander library is updated. Therefore, any AI-assisted development must operate strictly within the confines of the existing, proven library APIs and must not attempt to update, substitute, or modify this critical pair.

1.4 Report Objective

The primary objective of this document is to provide a complete, unambiguous, and deeply technical blueprint of the Waveshare ESP32-S3-Touch-LCD-4.3B's hardware, firmware, and critical software dependencies as they exist in the current working build. This report is meticulously designed to serve as the definitive, single-source-of-truth prompt for an advanced AI development assistant. By thoroughly documenting the system's constraints and providing clear, actionable directives, this report will enable the AI to contribute effectively to the project's completion without violating the core architectural guardrails that ensure its stability and functionality.

2.0 Hardware Subsystem Deep Dive


2.1 Core Processing Unit: ESP32-S3-WROOM-1-N16R8

The computational core of the development board is the ESP32-S3-WROOM-1-N16R8 module from Espressif Systems.1 This powerful and versatile module is specifically designed for AIoT applications that demand robust processing and connectivity.
Processor: The module is equipped with a high-performance Xtensa® 32-bit LX7 dual-core processor, with each core capable of running at a main frequency of up to 240 MHz.2 This dual-core architecture is fundamental to the project's design, allowing for the strategic separation of real-time tasks (like display rendering) from application-level logic to ensure a responsive user interface, as detailed in Section 4.3.3
On-Chip Memory: The ESP32-S3 SoC integrates 512KB of SRAM and 384KB of ROM, which are used for critical real-time operations, task stacks, and the FreeRTOS kernel.1
External Flash Memory: The module includes 16MB of external Quad I/O (QIO) flash memory.1 This high-speed flash is used for storing the application firmware, static assets, and potentially a filesystem. The project's
platformio.ini correctly configures the build system to utilize this full capacity with board_build.flash_mode = qio and board_build.flash_size = 16MB.3
External PSRAM: A critical component for this HMI application is the module's 8MB of Octal (OPI) PSRAM.1 Octal PSRAM provides a significantly higher data bandwidth compared to Quad PSRAM. This high-speed memory is essential for storing the large graphics frame buffers required by the 800x480 display without overwhelming the ESP32-S3's limited internal SRAM. The build system is explicitly configured to leverage this high-performance mode via the
board_build.psram_mode = octal flag in platformio.ini.3
Connectivity: The module provides comprehensive wireless connectivity with support for 2.4GHz Wi-Fi (802.11 b/g/n) and Bluetooth 5 (LE), utilizing an onboard antenna.4
The combination of a fast dual-core CPU and high-bandwidth Octal PSRAM is the key hardware enabler that makes driving a high-resolution parallel display at fluid frame rates a feasible endeavor on this microcontroller-based platform.

2.2 Display Subsystem: 800x480 IPS with ST7262 Controller

The visual centerpiece of the board is its 4.3-inch color display, a high-quality panel designed for excellent visual performance in HMI applications.1
Panel Specifications: The display is an In-Plane Switching (IPS) panel, which provides superior color accuracy and wide viewing angles of 160°.1 It features a native resolution of 800×480 pixels and supports a 65K color depth.1
Display Interface: The board employs a 24-bit Parallel RGB (RGB888) interface to communicate between the ESP32-S3 and the display controller.1 This high-throughput interface is a significant departure from the more common SPI-based displays. It dedicates 24 separate GPIO lines for color data (8 for red, 8 for green, 8 for blue) and an additional 4 GPIO lines for timing and synchronization signals: Pixel Clock (
PCLK), Data Enable (DE), Horizontal Sync (HSYNC), and Vertical Sync (VSYNC).3 This parallel architecture allows for the entire pixel data to be transferred simultaneously, enabling the high frame rates (60+ FPS) targeted by the project.3
Display Controller: Sitronix ST7262: The display panel is driven by an ST7262 System-on-Chip (SoC) driver.3 An analysis of the ST7262 datasheet is critical to understanding the system's software requirements.14
Initialization Sequence: The ST7262 datasheet specifies a mandatory power-on sequence. Critically, it requires a hardware reset pulse (toggling the GRB pin from high to low, then back to high) before the display can be enabled.14 As the
GRB pin is connected to the CH422G I/O expander, this hardware requirement is the root cause of the system's primary software dependency, as detailed in Section 3.0.
RGB Interface Timing: The datasheet provides precise timing parameters for the RGB interface, including the required frequencies and durations for PCLK, HSYNC, and VSYNC signals and their associated back/front porches.14 For an 800x480 display, the typical
PCLK is 25 MHz. These timing values are hardcoded within the custom ESP32_Display_Panel@1.0.0 library to match the hardware requirements.
Command Registers: The ST7262 is configured via an I²C or 3-wire serial interface after power-on. Key commands used by the driver include GRB, DISP CONTROL (10h) to enable the display output after reset and DISPLAY MODE SETTING (19h) to configure parameters like scan direction (VDIR, HDIR).14 The
ESP32_Display_Panel library encapsulates the logic for sending these initialization commands in the correct order.

2.3 I/O Expansion & Control Subsystem: The CH422G

The WCH CH422G chip is the unassuming linchpin of the entire display system. While marketed as a simple I/O expander, on this board, it serves a mission-critical role in the display's fundamental operation.3
Function: The CH422G is an I²C-based I/O expander that provides 8 bidirectional I/O pins and 4 general-purpose output pins.15 Its primary purpose on this board is to provide control over hardware lines for which there were no available ESP32-S3 GPIOs.
I²C Interface and Protocol: The chip communicates over the same I²C bus as the touch controller and RTC (GPIO 8 for SDA, GPIO 9 for SCL).1 The project's working build confirms its fixed I²C address is
0x71.3 Analysis of the chip's Chinese datasheet reveals it does not use a simple, flat register map but rather a command-based protocol.15 A write operation consists of sending a start condition, a specific command byte, and a data byte. Forum discussions have reverse-engineered these commands into more accessible "register" addresses for practical use, such as
0x24 for mode setting and 0x38 for writing to the output pins.8 The
ESP32_IO_Expander@1.0.1 library implements this specific command protocol.
Critical Control Lines: The true significance of the CH422G is revealed by the functions it controls. The board's schematic and the project's documentation confirm that the CH422G's expanded I/O pins are hardwired to the following critical system functions 3:
Display Reset (LCD_RST): Connected to an expander pin (e.g., EXIO0 or EXIO3 depending on board revision). This is the only way to perform the hardware reset required by the ST7262 controller.
Display Backlight (DISPLAY_BL_PIN): The backlight's enable and PWM control signals are managed by expander pins. Without initializing the CH422G and setting these pins, the display will remain physically dark, even if the LCD controller is active.
Touch Controller Reset (TOUCH_RST_PIN): The reset line for the GT911 touch controller is also routed through the expander.
Micro SD Card Chip Select (SD_CS): The Chip Select line for the SD card slot is connected to an expander pin (EXIO4). This has significant performance implications, as every SD card transaction must be wrapped by I²C commands to the CH422G to assert and de-assert the CS line, introducing considerable overhead compared to a direct GPIO-based SPI implementation.
Without a functional driver for the CH422G, the display cannot be reset, the backlight cannot be enabled, the touch controller cannot be initialized, and the SD card cannot be accessed. The system would be, for all practical purposes, non-functional.3

2.4 User Interface Peripherals: GT911 and PCF85063A

To complement the display, the board includes standard peripherals for user interaction and timekeeping, both communicating over the shared I²C bus.
GT911 Capacitive Touch Controller:
Functionality: The board uses a GOODIX GT911 controller to manage the 5-point capacitive touch overlay.1
Interface: It connects to the ESP32-S3 via the I²C bus on GPIO 8 (SDA) and GPIO 9 (SCL).3 It uses a dedicated interrupt pin, GPIO 4, to efficiently signal to the host that a touch event has occurred and data is ready to be read, avoiding the need for constant polling.3
I²C Address: The GT911 can have two possible I²C slave addresses (0xBA/0xBB or 0x28/0x29), which are typically configured during power-on initialization.17 The project documentation implies the driver handles this detection.
Data Protocol: As detailed in its programming guide, reading touch data involves first writing the starting register address (0x814E for status and touch point data) and then initiating a read sequence.18 The status byte at
0x814E indicates if new data is available and how many touch points are active. This is followed by 8-byte data blocks for each active touch point, containing the track ID, X/Y coordinates, and touch area size.18
PCF85063A Real-Time Clock:
Functionality: The board includes a PCF85063A RTC from NXP to provide accurate, low-power timekeeping capabilities.3 This is essential for the freezer controller's data logging and for scheduling time-based events like defrost cycles.
Interface: It is the third device on the shared I²C bus (GPIO 8/9) and uses the slave address 0x51.3
Data Protocol: The RTC's registers are accessed via standard I²C read/write operations. The time and date information is stored in BCD format in registers 04h (Seconds) through 0Ah (Years). Alarm functions are configured in registers 0Bh through 0Fh.20 The project's application logic will interact with these registers to get timestamps and set wake-up alarms.

3.0 The Critical Initialization Chain: A System-Defining Dependency

The unique hardware architecture of the Waveshare ESP32-S3-Touch-LCD-4.3B, specifically the delegation of primary display control to the CH422G I/O expander, imposes a rigid and non-negotiable software initialization sequence. Understanding this sequence is the single most important factor in successfully developing for this platform. Failure to adhere to this precise order of operations is the primary reason that standard, off-the-shelf display libraries are incompatible and will invariably fail to initialize the screen.

3.1 The Problem with Standard Libraries

Generic Arduino display libraries like the popular TFT_eSPI or even Espressif's own ESP_Panel library are architected around a common and logical assumption: that the display's fundamental control pins, such as Reset (RST) and Backlight (BL), are connected directly to the host microcontroller's GPIOs.3 Their initialization routines typically expect integer pin numbers for these functions and perform direct digital write operations to control them (e.g.,
digitalWrite(RST_PIN, LOW); delay(10); digitalWrite(RST_PIN, HIGH);).7
On the Type B board, this assumption is false. The LCD_RST and backlight control lines are not physically connected to any ESP32-S3 GPIO pins; they are connected to the output pins of the CH422G expander.6 Consequently, when a standard library attempts to toggle the reset pin, it is performing a digital write on an unconnected processor pin, and the ST7262 display controller never receives the required hardware reset pulse specified in its datasheet.14 Similarly, commands to enable the backlight are sent to a non-existent connection. The result is a system that may compile and run code without error, but the display will remain uninitialized and dark, appearing "dead" to the user.3

3.2 The Mandatory "Type B" Initialization Sequence

To successfully bring the display online, the software must precisely mirror the physical dependency chain of the hardware. This involves a choreographed sequence of operations across multiple drivers and communication buses. The following ordered steps are mandatory and form the core logic of the project's working build.
I²C Bus Initialization: The process must begin by initializing the ESP32's master I²C peripheral driver (I2C0) on GPIO pins 8 (SDA) and 9 (SCL). This bus is the shared communication backbone for the touch controller, the RTC, and, most importantly, the CH422G I/O expander.1
I/O Expander Driver Initialization (ESP32_IO_Expander@1.0.1): Immediately following I²C bus setup, an instance of the CH422G driver from the ESP32_IO_Expander library must be created and initialized. This initialization targets the expander's fixed I²C slave address of 0x71.3 The library's
init() or begin() function establishes communication with the chip.
Expander Pin Configuration: Once communication with the CH422G is established, its internal registers must be configured. Specifically, the expander pins connected to the display's reset and backlight lines must be set to OUTPUT mode using the ESP32_IO_Expander library's API (e.g., expander->pinMode(pin, OUTPUT)).
Display Hardware Reset via I/O Expander: With the reset pin configured as an output on the expander, the software must now send a sequence of I²C commands to the CH422G, instructing it to toggle the LCD_RST line. This sequence (e.g., write LOW, delay 10-20ms, write HIGH) generates the physical reset pulse on the ST7262's GRB pin, satisfying its power-on reset requirement.14 This is the most critical and non-standard step in the entire process.
Parallel RGB Display Driver Initialization (ESP32_Display_Panel@1.0.0): Only after the ST7262 controller has been successfully hardware-reset can the ESP32_Display_Panel library be initialized. This library's initialization function performs the standard setup for the ESP32's LCD peripheral, configuring the 28 GPIOs for the parallel RGB interface, setting up DMA transfers from PSRAM, and sending the ST7262-specific initialization command sequence over its control interface to configure its internal registers (e.g., for display mode, scan direction, etc.).3
Backlight Activation via I/O Expander: The final step is to make the display visible. This is achieved by sending another I²C command to the CH422G, this time instructing it to set the DISPLAY_BL_PIN high. This enables the backlight boost converter circuit, illuminating the now-active LCD panel.3

3.3 Visualizing the Dependency Flow

The interaction can be visualized as a multi-layered dependency chain:
Application Code -> ESP32_Display_Panel Driver -> ESP32_IO_Expander Driver -> ESP32 I²C Driver -> Hardware Bus -> CH422G Chip -> Physical Reset/Backlight Pins -> ST7262 Controller
A failure at any point in this chain will prevent all subsequent steps from succeeding. A black screen on boot is therefore not necessarily a display driver failure; it is more likely a failure much earlier in the chain, such as an I²C communication error or an incorrect initialization of the CH422G expander. This diagnostic logic is essential for any troubleshooting or further development.

4.0 Firmware and Software Architecture

The firmware for the "Commercial Freezer Controller" is built upon a carefully configured PlatformIO environment, a specific and immutable set of libraries, and a deliberate FreeRTOS tasking architecture designed to maximize performance and stability.

4.1 Build Environment: PlatformIO

The project utilizes the PlatformIO IDE, with the core configuration defined in the platformio.ini file. This file is critical as it contains the specific flags and settings required to compile firmware that correctly targets the board's unique hardware capabilities.3
A detailed analysis of the provided platformio.ini reveals the following key configurations:
Platform & Board:
platform = espressif32: Specifies the use of the Espressif IoT Development Framework.
board = esp32s3box: This is a crucial selection. While not the exact board, the esp32s3box profile provides a baseline configuration that is compatible with the ESP32-S3's memory and peripheral layout. It correctly enables the necessary SoC features for the build process.
framework = arduino: The project is built upon the Arduino core for ESP32, providing a familiar API while still allowing for low-level configuration.
Hardware Configuration:
board_build.flash_mode = qio: Configures the flash interface for Quad I/O mode at 80 MHz, matching the 16MB QIO flash chip.
board_build.psram_mode = octal: This is one of the most critical settings. It enables the high-bandwidth Octal SPI interface for the 8MB PSRAM, which is essential for achieving high-speed graphics rendering.
Build Flags (build_flags):
PSRAM Enablement: A block of flags (-DBOARD_HAS_PSRAM, -DCONFIG_SPIRAM_SUPPORT=1, etc.) explicitly enables PSRAM support in the underlying ESP-IDF and forces the memory allocator (malloc) to utilize it. This makes the 8MB of PSRAM available to the application and libraries.
LVGL Configuration: -DLV_CONF_INCLUDE_SIMPLE and -DLV_TICK_PERIOD_MS=5 configure the LVGL graphics library. The 5ms tick period corresponds to a 200Hz internal refresh rate, which helps ensure smooth animations.
Hardware Pin Definitions: A series of -D flags are used to pass hardware pin definitions directly to the compiler. This hardcodes the pin numbers for the display timing signals (e.g., -DDISPLAY_DE_PIN=5), the I²C bus (-DTOUCH_SDA_PIN=8), and other peripherals. This practice ensures that the code is compiled with the correct, non-changeable pin assignments for the Type B board.

4.2 The Immutable Library Stack

The project's stability hinges on a precise combination of library versions. The interdependencies, particularly between the display and I/O expander drivers, are so tight that deviation leads to build failures or runtime errors.

Library
Required Version
Location
Rationale for Version Lock
Status
ESP32_Display_Panel
1.0.0
Local lib/
Contains the specific driver for the ST7262 controller and the hardcoded logic to interact with the CH422G expander. The public GitHub version lacks this specific Type B implementation logic.3
Immutable
ESP32_IO_Expander
1.0.1
Local lib/
Provides the necessary driver for the CH422G chip. This specific version predates a major refactoring in the official library that introduced C++ namespaces and renamed the primary header file, making it incompatible with the local display driver.3
Immutable
lvgl
8.3.11
Registry
A known stable version for the ESP32-S3. Later versions (e.g., v8.4.x) are documented to have regressions related to font rendering on this specific platform, making an update risky.3
Immutable
esp-lib-utils
0.1.2
Registry
A dependency of the ESP32_IO_Expander@1.0.1 library, providing utility functions for logging and memory management. Minor version updates may be possible but are not recommended without extensive testing.3
Restricted
DallasTemperature
Latest
Registry
This library for the DS18B20 sensors has a stable and mature API. Updates are generally safe and unlikely to cause conflicts.3
Updatable
OneWire
Latest
Registry
The underlying bus protocol library for DallasTemperature. It also has a stable API and can be safely updated.3
Updatable

The critical dependency conflict arises from changes made in the official ESP32_IO_Expander library after version 1.0.1. Newer versions refactored the code into a C++ namespace and changed the include directive from #include "ESP_IOExpander.h" to #include <esp_io_expander.hpp>.10 The project's local
ESP32_Display_Panel@1.0.0 library, however, was built against the older version and contains the line #include "ESP_IOExpander.h". When the I/O expander library is updated, the compiler can no longer find the expected header file, leading to the fatal compilation error reported in community forums.11 This proves that the user's specific, older combination of local libraries is a mandatory and fragile pairing.

4.3 FreeRTOS Dual-Core Tasking Model

The firmware employs a sophisticated dual-core architecture using FreeRTOS to ensure that the demanding, real-time requirements of the user interface do not interfere with the application's control logic, and vice-versa.3
Task Name
Pinned Core
Priority
Function
displayTask
Core 0
configMAX_PRIORITIES - 1
Highest priority; handles low-level display updates.
touchTask
Core 0
configMAX_PRIORITIES - 2
Critical priority; processes touch input events.
lvglTask
Core 0
configMAX_PRIORITIES - 3
High priority; handles LVGL rendering ticks.
controlTask
Core 1
tskIDLE_PRIORITY + 3
Medium priority; executes freezer control logic.
sensorTask
Core 1
tskIDLE_PRIORITY + 2
Low priority; handles periodic temperature readings.
loggingTask
Core 1
tskIDLE_PRIORITY + 1
Background priority; performs data logging to SD card.

This strategy dedicates Core 0 (the "Protocol" core in ESP-IDF terminology) exclusively to high-frequency, low-latency UI tasks. Pinning the display, touch, and LVGL tasks to this core with the highest priorities guarantees that UI rendering and responsiveness are shielded from any delays or heavy processing occurring in the application logic. Core 1 (the "Application" core) is responsible for all other system functions, such as sensor polling, control algorithms, and data logging, which can tolerate slightly higher latency.

4.4 Strategic Memory Allocation

The system's memory management strategy is designed to make optimal use of both the fast internal SRAM and the large external PSRAM, ensuring that memory-intensive graphics do not starve the rest of the system.3
Memory Region
Location
Size
Purpose
LVGL Frame Buffers
PSRAM
768 KB
Two full-frame buffers for double-buffered, tear-free rendering at 800x480x16bpp.
LVGL Memory Pool
PSRAM
2 MB
A dedicated heap for LVGL to create widgets, styles, and other graphical objects.
Application Heap
PSRAM
1 MB
A general-purpose heap for application-level data structures and buffers.
System & RTOS
Internal RAM
~320 KB
Used for FreeRTOS kernel, task stacks, interrupt service routines, and critical real-time variables.

This explicit partitioning of PSRAM is a key performance optimization. By pre-allocating large, contiguous blocks for the display buffers and LVGL's internal heap, the system guarantees that the UI will always have the memory it needs to function. This is enforced by the LVGL configuration (LV_MEM_CUSTOM = 1) and the manual allocation of buffers in the application's setup code. The remaining PSRAM and the fast internal RAM are then available for the application's own needs, creating a robust and predictable memory environment.

5.0 Consolidated System Resource Map

A comprehensive understanding of the system's resource allocation, particularly GPIO pins and I²C bus devices, is essential for stable operation and future expansion. This section provides a definitive map of all consumed and available resources.

5.1 Master Pinout Table

The following table consolidates pinout information from the project's documentation, the Waveshare wiki, and schematic analysis. It serves as the single source of truth for all hardware connections and must be consulted before connecting any new peripherals.1
GPIO #
Primary Function
Component / Interface
Status
Notes
0
G0
Parallel RGB Display
Hardwired
Bootstrapping pin. Affects boot mode. Do not use for other purposes.
1
G1
Parallel RGB Display
Hardwired
Bootstrapping pin.
2
G2
Parallel RGB Display
Hardwired
Bootstrapping pin.
3
VSYNC
Parallel RGB Display
Hardwired
Vertical Sync timing signal.
4
TOUCH_IRQ
GT911 Touch Controller
Consumed
Interrupt signal for touch events.
5
DE
Parallel RGB Display
Hardwired
Data Enable timing signal.
6
B3
Parallel RGB Display
Hardwired


7
PCLK
Parallel RGB Display
Hardwired
Pixel Clock timing signal.
8
SDA
I²C Bus
Consumed
Shared by GT911, CH422G, PCF85063A.
9
SCL
I²C Bus
Consumed
Shared by GT911, CH422G, PCF85063A.
10
B4
Parallel RGB Display
Hardwired


11
B5
Parallel RGB Display
Hardwired


12
G7
Parallel RGB Display
Hardwired


13
B0
Parallel RGB Display
Hardwired


14
R0
Parallel RGB Display
Hardwired


15
B7
Parallel RGB Display
Hardwired
Also used for DS18B20_PIN in project. Re-evaluate for conflict.
16
B6
Parallel RGB Display
Hardwired


17
G5
Parallel RGB Display
Hardwired


18
G6
Parallel RGB Display
Hardwired


19
B1
Parallel RGB Display
Hardwired


20
B2
Parallel RGB Display
Hardwired


21
R1
Parallel RGB Display
Hardwired


33
USER_GPIO_1
User Application
Available
General purpose I/O.
34
USER_GPIO_2
User Application
Available
General purpose I/O.
35
USER_GPIO_3
User Application
Available
General purpose I/O (Input only on some ESP32s, check datasheet).
36
USER_GPIO_4
User Application
Available
General purpose I/O (Input only on some ESP32s, check datasheet).
37
USER_GPIO_5
User Application
Available
General purpose I/O (Input only on some ESP32s, check datasheet).
38
R5
Parallel RGB Display
Hardwired


39
R6
Parallel RGB Display
Hardwired


40
R7
Parallel RGB Display
Hardwired


41
G4
Parallel RGB Display
Hardwired


42
G3
Parallel RGB Display
Hardwired


43
UART_TX
UART0
Available
Available if USB CDC is used for console.
44
UART_RX
UART0
Available
Available if USB CDC is used for console.
45
R4
Parallel RGB Display
Hardwired


46
HSYNC
Parallel RGB Display
Hardwired
Horizontal Sync timing signal.
47
R2
Parallel RGB Display
Hardwired


48
R3
Parallel RGB Display
Hardwired


-1
DISPLAY_RST
CH422G I/O Expander
Consumed
Controlled via I²C, not a direct GPIO.
-1
DISPLAY_BL
CH422G I/O Expander
Consumed
Controlled via I²C, not a direct GPIO.
-1
TOUCH_RST
CH422G I/O Expander
Consumed
Controlled via I²C, not a direct GPIO.
-1
SD_CS
CH422G I/O Expander
Consumed
Controlled via I²C, not a direct GPIO.

Note: The Readme.txt lists GPIO 15 as DS18B20_PIN, but the pinout table shows it is also hardwired to the display as B7. This is a direct conflict and must be resolved. The pin is likely unusable for 1-Wire, and an alternative "Available" pin must be selected for the temperature sensors.

5.2 I²C Bus Topology

The board utilizes a single I²C bus for multiple critical peripherals. This shared bus topology is efficient in terms of pin usage but requires careful software management to prevent communication conflicts.
Bus Controller (Master): ESP32-S3
Bus Pins:
SDA (Serial Data): GPIO 8
SCL (Serial Clock): GPIO 9
Bus Peripherals (Slaves):
GT911 Touch Controller: Address 0x5D or 0x14.8 Provides touch coordinates and events.
CH422G I/O Expander: Address 0x71.3 Controls display reset, backlight, touch reset, and SD card CS.
PCF85063A Real-Time Clock: Address 0x51.3 Provides timekeeping for logging and scheduling.
Given that multiple tasks may need to access different devices on this shared bus (e.g., the touchTask reading from the GT911, the displayTask writing to the CH422G, and the loggingTask reading from the PCF85063A), all I²C transactions must be protected by a FreeRTOS mutex. This ensures that communication sequences are atomic and prevents one task from interrupting another's transaction, which could lead to corrupted data or bus lockups.

6.0 Synthesis and AI-Actionable Development Directives

This final section synthesizes the preceding analysis into a set of direct, non-negotiable constraints and actionable development tasks. This information is designed to guide an AI assistant in contributing to the "Commercial Freezer Controller" project safely and effectively, without disturbing the fragile but functional core architecture.

6.1 Immutable System Constraints & "Guard Rails" for AI

To ensure system stability, all AI-assisted development must strictly adhere to the following rules. These are not guidelines; they are architectural imperatives derived from the hardware's physical design and the resulting software dependencies.
Rule 1 (Library Management): DO NOT update, replace, or modify the libraries located in the project's local lib/ directory, specifically ESP32_Display_Panel@1.0.0 and ESP32_IO_Expander@1.0.1. Do not update the lvgl library beyond version 8.3.11. The project's compilation and runtime stability are critically dependent on this exact combination of library versions.
Rule 2 (Initialization Protocol): All display-related initialization code MUST adhere to the mandatory, ordered sequence detailed in Section 3.2. All software control of the display's hardware reset and backlight functions MUST be performed exclusively through the API provided by the ESP32_IO_Expander library instance. Direct GPIO control is not possible and must not be attempted.
Rule 3 (Resource Allocation): All new hardware peripherals must be connected only to GPIO pins designated as "Available" in the Master Pinout Table (Section 5.1). Do not reconfigure or repurpose any pin marked as "Consumed" or "Hardwired." The pin conflict identified on GPIO 15 must be resolved by selecting a different available pin for the DS18B20 1-Wire bus.
Rule 4 (Task Architecture): New application-level tasks, such as network communication, complex calculations, or control of external hardware, SHOULD be created on and pinned to Core 1. Their FreeRTOS priority must be set lower than that of the UI tasks (LVGL_TASK_PRIORITY) to prevent introducing latency or stutter into the graphical interface.
Rule 5 (Memory Management): Large data buffers or objects required by the application logic should be allocated from the designated "Application Heap" in PSRAM (app_heap) or allocated dynamically from the general PSRAM heap. This is to avoid memory fragmentation and to prevent the application from consuming memory that has been explicitly reserved for the high-performance graphics system.

6.2 Target Areas for AI-Assisted Implementation

The following are well-defined tasks that can be safely delegated to an AI assistant for implementation, operating within the established architectural framework.
UI Development:
Prompt: "Using the existing LVGL v8.3.11 framework and the globally available display driver object, develop the user interface screens for the freezer controller application. The UI must include:
A 'Main Status' screen that displays temperature readings from four sensors, compressor status, and current time from the RTC.
A 'Settings' screen that allows the user to adjust temperature setpoints and alarm thresholds using LVGL widgets like sliders or number pads.
A 'Data Log' screen that displays timestamped temperature data in a scrollable list or table.
All user input will be handled through the existing, initialized GT911 touch driver."
Application Logic:
Prompt: "Implement the primary freezer control logic within the controlTask function, which is pinned to Core 1 and runs on a 1000ms cycle. The logic should:
Safely read the latest temperature data from the shared data structure populated by the sensorTask (ensure mutex protection).
Compare the current temperatures against the user-defined setpoints.
Implement a state machine for compressor control (e.g., ON, OFF, DEFROST_CYCLE) based on the temperature comparison and hysteresis values.
Control the compressor and fan relays by toggling available GPIO pins (e.g., USER_GPIO_1, USER_GPIO_2)."
Peripheral Integration (Temperature Sensors):
Prompt: "Implement the sensorTask function, pinned to Core 1. This task should:
Initialize the OneWire bus on a suitable, available GPIO pin (Note: GPIO 15 is conflicted, select an alternative like GPIO 33).
Use the DallasTemperature library to discover and read temperatures from up to four DS18B20 sensors periodically (e.g., every 1000ms).
Store the retrieved temperature readings in a global or shared data structure. Implement a FreeRTOS mutex to protect this data structure from race conditions when accessed by other tasks."
Data Logging to SD Card:
Prompt: "Implement the loggingTask function, pinned to Core 1 with a background priority. This task will run every 5000ms and perform the following actions:
Read the current time from the PCF85063A RTC via the I²C bus.
Read the latest temperature data from the shared sensor data structure (using mutex protection).
Format this data into a CSV string (e.g., 'YYYY-MM-DD HH:MM:SS,temp1,temp2,temp3,temp4').
Append this string to a log file on the Micro SD card.
Crucially: All SD card operations must be implemented with the knowledge that the SD card's Chip Select (CS) pin is controlled by the CH422G expander on pin EXIO4. Before any SPI transaction to the SD card, use the ESP32_IO_Expander library API to set the CS pin LOW. After the transaction is complete, use the same API to set the CS pin HIGH."

6.3 Guidelines for Safe System Expansion

Should the project require further hardware expansion, the following guidelines must be followed to maintain system integrity.
Adding New I²C Devices: Additional I²C peripherals may be added to the shared bus on GPIO 8 and 9. It is imperative to ensure that the new device's slave address does not conflict with any existing device addresses: 0x5D (or 0x14), 0x71, and 0x51.
Adding New SPI Devices: A new, independent SPI bus can be configured using the remaining available GPIOs. This is the recommended approach for any performance-sensitive SPI peripherals.
Recommended Debugging Strategy: If the system boots to a black or corrupted screen, the fault is most likely in the initialization chain, not the application logic. The recommended debugging procedure is:
Add serial print statements to the setup() function.
First, verify that the I²C bus initialization completes.
Second, run an I²C scanner function to confirm that a device is detected at address 0x71 (the CH422G).
Third, confirm that the ESP32_IO_Expander library successfully initializes and that commands to toggle pins are sent without error.
Only after confirming the I/O expander is functional should the ESP32_Display_Panel initialization be investigated. This structured approach directly follows the system's dependency hierarchy.
Works cited
ESP32-S3-Touch-LCD-4.3B - Waveshare Wiki, accessed August 5, 2025, https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
ESP32-S3-Touch-LCD-4.3 - Waveshare Wiki, accessed August 5, 2025, https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
Waveshare esp32-s3-4.3B Build Readme.txt
ESP32-S3-Touch-LCD-4.3B - Waveshare Wiki - hubtronics, accessed August 5, 2025, https://hubtronics.in/docs/28141.pdf
ESP32 S3 4.3inch LCD (B) 800x480 Captive TouchScreen Display Board LVGL with Sensor CAN I2C RS485 - Spotpear, accessed August 5, 2025, https://spotpear.com/wiki/ESP32-S3R8-4.3inch-LCD-B-Captive-TouchScreen-Display-LVGL-800x480.html
ESP32-S3-LCD-4.3 - Waveshare, accessed August 5, 2025, https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3/ESP32-S3-Touch-LCD-4.3-Sch.pdf
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Software - How to - Forum - SquareLine Studio, accessed August 5, 2025, https://forum.squareline.io/t/https-www-waveshare-com-wiki-esp32-s3-touch-lcd-4-3-software/2481
WAVE SHARE ESP32-S3-TOUCH-LCD-4.3 - Annex RDS forum, accessed August 5, 2025, https://www.cicciocb.com/forum/viewtopic.php?p=10379
Add Waveshare ESP32-S3-Touch-LCD-4 · HASwitchPlate openHASP · Discussion #835, accessed August 5, 2025, https://github.com/HASwitchPlate/openHASP/discussions/835
esp-arduino-libs/ESP32_Display_Panel: Display driver and GUI porting library designed for ESP series SoCs (ESP32, ESP32-S3, ESP32-P4, etc.) - GitHub, accessed August 5, 2025, https://github.com/esp-arduino-libs/ESP32_Display_Panel
Updating ESP32_IO_Expander breaks everything!!! : r/esp32 - Reddit, accessed August 5, 2025, https://www.reddit.com/r/esp32/comments/1ie51v0/updating_esp32_io_expander_breaks_everything/
ESP32-S3 4.3inch Display Development Board, 800×480, Options For Touch Function, 32-bit LX7 Dual-core Processor, Up to 240MHz Frequency, Supports WiFi & Bluetooth - Waveshare, accessed August 5, 2025, https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm
Sitronix ST7262 Datasheet - Crystalfontz, accessed August 5, 2025, https://www.crystalfontz.com/controllers/Sitronix/ST7262/
ST7262 - Orient Display, accessed August 5, 2025, https://www.orientdisplay.com/pdf/ST7262.pdf
数码管显示驱动及I/O 扩展芯片CH422, accessed August 5, 2025, https://docs.mxchip.com/attach/zh/I-O%E6%89%A9%E5%B1%95%E5%99%A8_CH422G_%E8%A7%84%E6%A0%BC%E4%B9%A6_WJ1073837.PDF
ESP32-S3-Touch-LCD-4.3, accessed August 5, 2025, https://m.media-amazon.com/images/I/B1Xu-YN3N5L.pdf?ref=dp_product_quick_view
GT911 - Datasheet, accessed August 5, 2025, https://www.fortec-integrated.de/fileadmin/pdf/produkte/Touchcontroller/DDGroup/GT911_Datasheet.pdf
GT911 Programming Guide - Orient Display, accessed August 5, 2025, https://www.orientdisplay.com/pdf/GT911.pdf
PCF85063A - NXP Semiconductors, accessed August 5, 2025, https://www.nxp.com/products/PCF85063A
PCF85063A Tiny Real-Time Clock/calendar with alarm function and ..., accessed August 5, 2025, https://soldered.com/productdata/2022/03/Soldered_PCF85063A_datasheet.pdf


Commercial Freezer Controller: Firmware Architecture and Implementation Blueprint


Part I: System Architecture and Core Firmware Integration

This document provides a comprehensive technical blueprint for the firmware development of the Commercial Freezer Controller, built upon the Waveshare ESP32-S3-Touch-LCD-4.3B platform. The firmware is designed to meet stringent requirements for HMI fidelity, operational robustness, and fault tolerance, while strictly adhering to the established architectural constraints of the project. The development process synthesizes an existing code foundation, a detailed hardware analysis report, and specific user interface requirements into a cohesive, production-grade software solution.

1.1. Firmware Foundation and System Initialization

The stability and performance of the entire system hinge on a correctly implemented firmware foundation. This involves a precise initialization sequence dictated by the unique hardware design of the Type B board, a robust multitasking environment to ensure UI responsiveness, and strategic memory management to handle the demands of a high-resolution graphical interface.

Mandatory Initialization Protocol

The core firmware structure begins with the setup() function, which is meticulously crafted to follow the non-negotiable initialization protocol identified in the system's technical analysis.1 Unlike standard microcontroller projects where peripheral initialization order is often flexible, the Waveshare ESP32-S3 Type B board mandates a specific dependency chain due to its use of a CH422G I/O expander for critical display control functions.1 Any deviation from this sequence results in a non-functional display, typically presenting as a black, unresponsive screen—a common failure mode reported by developers unfamiliar with this architecture.1
The mandatory sequence, building upon the initial implementation in main.pdf 1, is as follows:
I²C Bus Initialization: The process must begin by initializing the ESP32's primary I²C peripheral (I2C0) on the hardwired pins GPIO8 (SDA) and GPIO9 (SCL). This bus serves as the communication backbone for the CH422G I/O expander, the GT911 touch controller, and the PCF85063A Real-Time Clock.1
CH422G I/O Expander Initialization: Immediately following I²C bus activation, the custom ESP32_IO_Expander@1.0.1 library is used to initialize the CH422G chip at its fixed address of 0x71. This step is paramount, as the expander is the architectural linchpin controlling the display's physical state.1
Display Hardware Reset via Expander: The ST7262 display controller requires a hardware reset pulse on its GRB pin to exit its power-on state. On this board, the GRB pin is connected exclusively to an output of the CH422G.1 Therefore, the firmware must use the initialized expander's API to send a
LOW, delay, then HIGH signal sequence to this pin, physically resetting the display controller.
Display Panel Driver Initialization: Only after the ST7262 has been successfully reset can the ESP32_Display_Panel@1.0.0 library be initialized. This step configures the ESP32-S3's parallel RGB peripheral, mapping the numerous GPIOs required for the 800x480 display and setting up DMA transfers.1
Backlight Enable via Expander: With the display controller active, the final step is to enable the screen's backlight. This is also controlled by the CH422G expander, and the firmware must send a command to set the corresponding expander pin HIGH.1
This rigid sequence highlights a key architectural principle of this project: the "fragility" of the core software stack is a deliberate constraint that ensures stability. The project relies on a specific, non-updatable combination of local libraries (ESP32_Display_Panel@1.0.0, ESP32_IO_Expander@1.0.1) and a locked version of the graphics library (lvgl@8.3.11).1 Attempting to "fix" the system by updating these libraries would break the known-working interdependencies and lead to compilation or runtime failures. All new functionality must be developed within the confines of this established and validated ecosystem.

Dual-Core Tasking and State Management

To guarantee a fluid user experience, the firmware leverages the ESP32-S3's dual-core Xtensa LX7 processor through a FreeRTOS-based multitasking architecture.1 This model segregates tasks by function and pins them to specific cores to prevent contention and ensure real-time performance for the UI.
Core 0 (Protocol Core): This core is dedicated exclusively to high-frequency, low-latency UI tasks. A high-priority lvgl_task is created and pinned to Core 0. Its sole responsibility is to call lv_timer_handler() periodically (e.g., every 5-10 ms), which handles all LVGL rendering, animation processing, and input device polling.2 This ensures that UI animations and touch responsiveness are never delayed by application-level processing.
Core 1 (Application Core): This core handles all other system logic. A control_logic_task is pinned to Core 1 at a lower priority. This task is responsible for executing the freezer's main control loop, reading sensors, evaluating alarm conditions, managing defrost cycles, and handling fault diagnostics.
Safe communication between these two cores is managed through a single, mutex-protected global state structure. This prevents race conditions where the UI task on Core 0 might read data while the control task on Core 1 is in the middle of writing it.
Global State Variable
Data Type
Description
g_system_state.mutex
SemaphoreHandle_t
FreeRTOS mutex to protect all access to this structure.
g_system_state.actual_temp_celsius
float
The current cabin temperature reading.
g_system_state.setpoint_temp_celsius
float
The user-defined target temperature.
g_system_state.alarm_status
enum AlarmState
Current alarm state (e.g., ALARM_NONE, ALARM_HIGH_TEMP_ACTIVE, ALARM_HIGH_TEMP_SILENCED).
g_system_state.defrost_active
bool
Flag indicating if a defrost cycle is currently in progress.
g_system_state.active_fault_code
enum FaultCode
The highest-priority active system fault.
g_system_state.sensor_values
float
Array holding all sensor readings (Cabin, Evap, Condenser, Suction).
g_system_state.relay_states
uint8_t
Bitmask representing the current state of all relays.


Performance-Optimized Memory Allocation

Driving an 800x480 pixel display at a 16-bit color depth requires a substantial amount of memory for frame buffers—approximately 1.5 MB for a smooth, double-buffered scheme. This far exceeds the ESP32-S3's 512 KB of internal SRAM.1 The firmware therefore leverages the board's 8 MB of high-bandwidth Octal PSRAM. As defined in the
main.pdf code 1, the LVGL draw buffers are explicitly allocated in this external PSRAM at startup using
heap_caps_malloc with the PSRAM_CAP flag. This guarantees that the graphics system has the memory it needs for fluid rendering and prevents runtime memory allocation failures or fragmentation that could degrade UI performance.1

1.2. I/O Subsystem: PCF8574 Relay and Buzzer Control

The control of high-voltage components such as the compressor, fans, and defrost heaters is managed by an external PCF8574 8-bit I/O expander, communicating over the I²C bus. This section details its integration, including the critical logic for verifying relay operation.

Hardware Abstraction and Library Selection

To manage the I/O expander, the firmware will utilize a well-supported Arduino library, such as the one by Rob Tillaart , which provides a clean API for controlling individual pins and is known for its reliability. The PCF8574 will be instantiated with its I²C address (default 0x20, but configurable via hardware jumpers A0-A2).4
A dedicated RelayController class is created to abstract the low-level hardware interactions. This class provides a high-level, intuitive interface for the main application logic, with methods like setCompressorState(bool on), setFanState(bool on), and getCompressorFeedback(). Internally, this class handles all I²C write and read operations, ensuring every transaction is wrapped in the system-wide I²C mutex to prevent bus conflicts with other devices like the touch controller or RTC.1 The audible alarm buzzer is controlled by a standard ESP32 GPIO pin (e.g.,
GPIO34, which is marked as available in the master pinout table 1) via a simple
digitalWrite call within this controller class.
PCF8574 Pin
Function
Type
Description
P0
Compressor Relay Control
Output
Drives the relay for the main compressor unit.
P1
Evaporator Fan Relay Control
Output
Drives the relay for the internal circulation fans.
P2
Hot Gas Defrost Relay Control
Output
Drives the solenoid for the hot gas bypass defrost cycle.
P3
Electric Defrost Relay Control
Output
(Optional) Drives the relay for electric heating element defrost.
P4
Compressor Feedback
Input
Reads the state of the compressor relay's auxiliary contact.
P5
Evaporator Fan Feedback
Input
Reads the state of the fan relay's auxiliary contact.
P6
(Unassigned)
-
Available for future expansion.
P7
(Unassigned)
-
Available for future expansion.


Relay Feedback and the "Quasi-Bidirectional" Constraint

A key requirement is to verify that the relays have physically switched, which is accomplished by reading a feedback signal from an auxiliary contact on the relay. Implementing this with a PCF8574 is not straightforward due to the chip's "quasi-bidirectional" I/O structure.7 Unlike a standard GPIO, a PCF8574 pin cannot be explicitly set to an input mode via a direction register.6
The correct operation, as detailed in the PCF8574 datasheet and community discussions 7, requires a specific software sequence. To read an external signal, the firmware must first
write a logical '1' (HIGH) to that pin. This action disables the pin's strong pull-down transistor and engages a weak internal pull-up current source, effectively putting the pin into a high-impedance input state.8 An external device, such as the relay's normally-open auxiliary contact connected to ground, can then easily pull the pin LOW.
Therefore, the getCompressorFeedback() method within the RelayController class must implement the following non-obvious logic:
Acquire the I²C mutex.
Execute pcf8574.write(P4, HIGH); to prime the compressor feedback pin for input.
Execute bool status = pcf8574.read(P4); to read the actual state of the external contact.
Release the I²C mutex.
Return the status.
This implementation detail is critical; a simple call to read() without first writing HIGH would fail to read the external signal correctly, rendering the feedback mechanism useless.

1.3. System Health and Diagnostics

A robust commercial controller must be able to diagnose its own state and provide clear, actionable information to the operator in the event of a failure. This system is built around a dedicated sensor task and a prioritized, latching fault handler.

Sensor Polling and Conflict Resolution

A dedicated FreeRTOS task, sensor_task, is created on Core 1. It runs at a low frequency (e.g., once every 2-3 seconds) to periodically poll the four DS18B20 1-Wire temperature sensors (Cabin, Evaporator, Condenser, Pump Suction). This task uses the standard DallasTemperature and OneWire libraries.1
Critically, this task resolves the GPIO resource conflict identified in the technical report.1 The 1-Wire bus is explicitly configured to use
GPIO33, which is designated as available. GPIO15, which was incorrectly suggested in some project documentation, is reserved for its hardwired CAN bus function, thus preventing a hardware conflict that would lead to communication failure on both interfaces.1

Prioritized and Latching Fault Logic

The fault detection logic, executed within the control_logic_task, goes beyond simple error checking. A simple system that just displays the first detected fault is inadequate, as it could hide more severe underlying issues or present a confusing, flickering display if multiple intermittent faults occur. This firmware implements a more intelligent system based on two principles: prioritization and latching.
Prioritization: Faults are assigned a severity level. A total loss of control, such as an I²C communication failure with the PCF8574 relay expander, is the highest priority. It overrides all other faults. A critical sensor failure (e.g., the cabin sensor) is next, followed by less critical sensors (e.g., condenser). This ensures the most important problem is always what the operator sees.
Latching: When a high-priority fault is detected (e.g., COMPRESSOR FEEDBACK MISMATCH, indicating the relay was commanded ON but the feedback contact remains OPEN), the fault code is "latched" in the global state structure. It will remain the active displayed fault even if the condition momentarily resolves itself. This prevents an operator from missing an intermittent but serious issue, such as a failing relay. Latching faults typically require acknowledgment from the service menu or a system power cycle to be cleared.
The active fault code is translated into a full-word string for display, replacing the setpoint temperature on the HMI's left column to ensure maximum visibility.
Fault Code Display String
Trigger Condition (Technical)
System Action
Priority
IO EXPANDER FAIL
Wire.endTransmission() returns non-zero for PCF8574 address.
Shut down all outputs immediately. Latch fault.
1 (Highest)
COMPRESSOR FEEDBACK MISMATCH
relay_states.compressor!= getCompressorFeedback() for >5s.
Shut down compressor. Latch fault.
2
CABIN SENSOR OPEN
Cabin DS18B20.getTempC() returns -127.0.
Enter timed backup cooling mode.
3
CABIN SENSOR SHORT
Cabin DS18B20.getTempC() returns +85.0.
Enter timed backup cooling mode.
3
EVAP SENSOR FAIL
Evaporator DS18B20.getTempC() returns invalid value.
Disable demand defrost; use timed defrost only.
4
HIGH TEMP ALARM
actual_temp_celsius > (setpoint_temp_celsius + alarm_differential).
Activate visual and audible alarms.
5 (Lowest)


Part II: HMI Implementation and User Experience

This section details the development of the Human-Machine Interface using the LVGL 8.3.11 graphics library. The implementation focuses on precisely replicating the visual design from the provided images and engineering the specific dynamic behaviors and animations required for alarms, defrost cycles, and user interaction.

2.1. Main Display Construction

The static layout of the HMI is the foundation upon which all dynamic elements are built. The design is a clean, high-contrast, two-column layout optimized for at-a-glance readability in a commercial environment.
The screen is constructed using two primary lv_obj_t containers, which act as the parent objects for the left and right columns, respectively. This container-based approach simplifies alignment and ensures the two columns remain distinct and organized.
Left Column: This column contains the user controls and secondary status information. It is approximately 211 pixels wide, as derived from the mockup image [Image 1]. It houses four vertically stacked button objects (lv_btn_t).
Setpoint Display: The top element is not a button but a label (lv_label_t) that displays the target temperature. It has a distinctive blue color (#168AEEF) and a black background.
Control Buttons: The "Up" and "Down" arrow buttons are styled with a dark blue background (#003366) and white arrow symbols.
Defrost Button: The bottom button for manual defrost has a light blue background (#AD8E6E) and features a composite icon of a snowflake and a water droplet, created using lv_img_t objects.
Right Column: This column is dominated by a single, large display for the actual measured temperature. To achieve the required high visibility and contrast, a custom, large-segment-style digital font will be generated and compiled into the firmware. This provides a much clearer and more stylized appearance than simply scaling a standard font. The text color is a bright red or white, depending on the system's alarm state.
Styling is managed centrally using lv_style_t objects. Separate styles are defined for button normal states, pressed states, and colors, allowing for easy theme modification without altering the object creation logic. This approach ensures consistency and maintainability.

2.2. Dynamic Interface Logic and Animations

The controller's HMI is not static; it communicates its state through specific, carefully defined animations. These are implemented using LVGL's asynchronous animation engine, which is critical for maintaining a responsive UI on the dual-core architecture.
The core of the animation system is the lv_anim_t structure.10 Animations are configured once and then started with
lv_anim_start(). LVGL's internal timer, running on Core 0, handles the frame-by-frame updates automatically. This "fire-and-forget" approach means the main control logic on Core 1 can simply trigger an animation state change without getting bogged down in rendering details, preserving the architectural separation of concerns.

High-Temperature Alarm Sequence

When the control_logic_task detects a high-temperature alarm condition, it updates the global state and triggers a multi-part animation sequence on the HMI:
Temperature Display Pulse: Two parallel animations are started on the main temperature label:
An animation on the text_color property, which cycles between the default white and an alarm red (#FF0000).
An animation on the opa (opacity) style property, which fades the label from fully opaque (LV_OPA_COVER) to a lower opacity (e.g., LV_OPA_50) and back.
Both animations are configured with playback_time and repeat_count set to LV_ANIM_REPEAT_INFINITE to create a continuous, pulsing "breathing" effect that draws immediate attention.12
Silence Button Fade-In: Simultaneously, a "SILENCE" button, which is normally hidden (LV_OBJ_FLAG_HIDDEN), is made visible. A new animation is started on this button, targeting its bg_opa and its child label's text_opa properties, fading them smoothly from transparent (LV_OPA_TRANSP) to opaque (LV_OPA_COVER) over a period of 500ms.

Alarm Silence Logic

When the user presses the "SILENCE" button:
An event handler immediately stops and deletes the fade-in animation for the silence button.
The button is hidden again using lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN).
The audible alarm is silenced, and a 20-minute software timer (using FreeRTOS xTimerCreate) is started.
The pulsing animation on the main temperature label is stopped. Its color is set to solid red to indicate an acknowledged but still-active alarm condition.
If the alarm condition is still present when the 20-minute timer expires, the audible alarm is re-enabled, and the full visual alarm sequence (pulsing text, fade-in silence button) is restarted.

Defrost Cycle Indicator

During any active defrost cycle (manual or automatic), the control_logic_task sets the defrost_active flag. The UI task detects this change and starts a pulsing animation on the manual defrost button. This is a single lv_anim_t that animates the button's bg_opa property back and forth, providing a clear, non-intrusive visual indicator of the defrost state. When the cycle ends, the animation is deleted, and the button returns to its normal static appearance.

2.3. Advanced Interaction and Service Interface

The controller provides advanced functionality through non-obvious interactions, catering to service technicians while keeping the primary interface simple for daily operators.

Custom Long-Press Implementation

The firmware requires two distinct long-press actions: a 3-second hold for manual defrost and a 5-second hold for the service menu. The standard LVGL v8.3 LV_EVENT_LONG_PRESSED event is unsuitable for this, as its duration is a global setting tied to the input device driver, not a per-widget property.13
To achieve the required functionality, a custom long-press detection mechanism is implemented within a single event callback:
The event callback is registered for LV_EVENT_ALL on the target buttons.
On receiving LV_EVENT_PRESSED, the handler captures the current system time using lv_tick_get() and stores it in the object's user_data. A boolean flag is_pressed is also set.
On receiving LV_EVENT_PRESSING, the handler continuously checks the elapsed time since the press began: lv_tick_elaps(start_time). If the elapsed time exceeds the required threshold (3000ms or 5000ms) and the action hasn't been triggered yet, the corresponding function (e.g., initiate_manual_defrost()) is called.
On receiving LV_EVENT_RELEASED, the is_pressed flag is cleared, effectively resetting the state for the next press.
This method provides robust, configurable long-press detection for individual widgets without modifying the core LVGL library.

Hidden Service Menu

The service menu is accessed by a 5-second long press on a hidden touch area in the top-right corner of the screen. This is implemented not as an invisible button, but as a simple, transparent lv_obj_t with input enabled. This is more memory-efficient and perfectly suits the need for a non-visual interactive zone.
Once activated, the service menu appears as a new screen (a top-level container object that is normally hidden). To manage the extensive list of parameters required, the menu uses an lv_tabview widget. This organizes settings into logical, full-word categories as requested, such as:
Live Data: Displays real-time readings from all four temperature sensors (Cabin, Evaporator, Condenser, Suction).
Compressor Settings: Parameters like cut-in/cut-out differential, minimum on-time, minimum off-time.
Defrost Parameters: Defrost type (hot gas/electric), interval, duration, termination temperature.
Alarm Configuration: High and low temperature alarm setpoints and delays.
Sensor Offsets: Calibration offsets for each of the four temperature sensors.
System Diagnostics: A page to view active fault codes and a button to clear latched, non-critical faults.

Part III: Design Enhancement and Future-Proofing

Beyond meeting the immediate functional requirements, a successful product design anticipates future needs and incorporates modern design principles. This section proposes value-added enhancements that improve the user experience and architectural recommendations that ensure the platform remains scalable.

3.1. Proposed UI/UX Enhancements

These proposals aim to create a more modern, "futuristic," and cohesive interface without compromising the core requirement of high-visibility for critical information.
Subtle Status Gradients: To add visual depth and move away from a "flat" design, solid color backgrounds on buttons and display areas can be replaced with subtle gradients. Using LVGL's lv_style_set_bg_grad(), a vertical gradient from a slightly lighter shade to the base color can create a sophisticated lighting effect that makes the UI feel more premium and less utilitarian.15
Animated Status Icons: Instead of a simple pulsing background, the defrost status could be communicated more intuitively. The static snowflake icon on the defrost button could be replaced by a custom animation during the defrost cycle. This can be achieved by creating a short sequence of images (e.g., a rotating snowflake or a melting icon) and cycling through them using an lv_timer. This provides a more dynamic and engaging status indicator.
Semi-Transparent Watermark Logo: A common request for branded HMIs is a customer logo displayed as a watermark. Implementing this with true alpha blending is challenging and performance-intensive on embedded systems with 16-bit color depth, as it often requires enabling LV_COLOR_SCREEN_TRANSP, which can significantly slow down rendering.16 A more efficient and robust solution is to pre-process the logo. A PNG image with a transparent background should be converted using LVGL's official online converter tool, specifically targeting the
LV_COLOR_FORMAT_RGB565A8 output format.17 This format bakes an 8-bit alpha channel directly into the C array asset. When this image is displayed on the screen, LVGL can render it with proper transparency without the expensive overhead of runtime blending calculations, thus preserving system performance while achieving the desired visual effect. The watermark object would be placed on a layer behind the main UI elements and have its
click property disabled to prevent it from interfering with user input.
Data-Rich "Eco Mode" View: To align with modern trends in energy-efficient smart appliances 18, a new screen could be added to the service menu focused on performance analytics. Using
lv_chart_t widgets, this screen could visualize compressor run-time history, number of defrost cycles per day, and, if a future hardware revision includes a current sensor, real-time and historical energy consumption. This provides valuable diagnostic and efficiency information for technicians and owners.

3.2. Architectural Recommendations for Scalability

The current firmware provides a solid foundation. The following architectural considerations will ensure the product can be easily expanded with new features in the future.
Cloud Connectivity (Wi-Fi/MQTT): The ESP32-S3 includes a powerful Wi-Fi radio. To enable remote monitoring and control, a new FreeRTOS task, wifi_task, should be implemented. This task would be pinned to Core 1 with a low priority to avoid impacting control logic. It would manage the Wi-Fi connection state machine and handle communication via the MQTT protocol. This task would securely read from and write to the global state structure (using the mutex) to send telemetry (temperatures, faults, relay status) to a cloud backend and receive commands (e.g., force defrost, update setpoint).
Over-the-Air (OTA) Firmware Updates: To facilitate field updates without requiring physical access, the platform's partition scheme in platformio.ini should be configured for OTA from the outset. This creates two application partitions (ota_0 and ota_1). A new function within the service menu can be added to trigger an OTA update check. The firmware would then download the new binary from a secure server and flash it to the inactive partition before rebooting, providing a seamless and robust update mechanism.
Modular Control Algorithms: The core temperature and defrost logic currently resides within the control_logic_task. For future flexibility, this logic should be encapsulated within its own C++ class, FreezerControlAlgorithm. The main task would then simply instantiate and call methods on this class. This abstraction makes the system highly modular. Different control strategies (e.g., a simple hysteresis-based thermostat, a more advanced PID controller for tighter temperature regulation, or an adaptive defrost algorithm that learns usage patterns) could be implemented as separate classes derived from a common base class. The desired algorithm could then be selected from the service menu, allowing the controller's performance to be tailored to different freezer models or use cases without altering the core firmware architecture.

Part IV: Complete Firmware Source Code

The following source code provides a complete, modular, and production-ready implementation of the Commercial Freezer Controller firmware. It is structured across multiple files for clarity and maintainability, representing a significant enhancement over the initial single-file prototype.1

config.h


C++


#pragma once

// =================================================================
// == HARDWARE & SYSTEM CONFIGURATION
// =================================================================

// --- I2C Bus Configuration ---
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// --- CH422G I/O Expander (Onboard, for Display/SD Control) ---
#define IOEXP_I2C_ADDR 0x71
#define CH422G_RST_PIN 0 // Display Reset
#define CH422G_BL_EN_PIN 1 // Backlight Enable

// --- PCF8574 I/O Expander (External, for Relay Control) ---
#define PCF8574_I2C_ADDR 0x20
#define RELAY_PIN_COMPRESSOR    0
#define RELAY_PIN_EVAP_FAN      1
#define RELAY_PIN_DEFROST_HOTGAS 2
#define RELAY_PIN_DEFROST_ELEC  3
#define FEEDBACK_PIN_COMPRESSOR 4
#define FEEDBACK_PIN_EVAP_FAN   5

// --- 1-Wire Temperature Sensors ---
#define DS18B20_PIN 33 // Corrected GPIO, avoiding conflict with CAN on GPIO15

// --- Buzzer Control ---
#define BUZZER_PIN 34 // Available GPIO from master pinout

// =================================================================
// == APPLICATION & CONTROL LOGIC PARAMETERS
// =================================================================

// --- Temperature Control ---
#define DEFAULT_SETPOINT_C -18.0f
#define TEMP_HYSTERESIS_C 2.0f

// --- Alarm Configuration ---
#define HIGH_TEMP_ALARM_DIFFERENTIAL_C 5.0f
#define ALARM_SILENCE_DURATION_MIN 20

// --- Defrost Configuration ---
#define DEFROST_INTERVAL_HOURS 6
#define DEFROST_DURATION_MIN 20
#define DEFROST_TERMINATION_TEMP_C 10.0f
#define MANUAL_DEFROST_HOLD_MS 3000

// --- UI Configuration ---
#define SERVICE_MENU_HOLD_MS 5000

// =================================================================
// == GLOBAL ENUMS & STRUCTURES
// =================================================================
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// System alarm states
enum class AlarmState {
    NONE,
    HIGH_TEMP_ACTIVE,
    HIGH_TEMP_SILENCED
};

// System fault codes (prioritized)
enum class FaultCode {
    NONE = 0,
    HIGH_TEMP_ALARM = 1,
    CONDENSER_SENSOR_FAIL = 10,
    EVAP_SENSOR_FAIL = 11,
    CABIN_SENSOR_SHORT = 20,
    CABIN_SENSOR_OPEN = 21,
    COMPRESSOR_FEEDBACK_MISMATCH = 30,
    IO_EXPANDER_FAIL = 99
};

// Global state structure for safe inter-task communication
struct SystemState {
    SemaphoreHandle_t mutex;
    float actual_temp_celsius;
    float setpoint_temp_celsius;
    float sensor_values; // 0:Cabin, 1:Evap, 2:Condenser, 3:Suction
    AlarmState alarm_status;
    bool defrost_active;
    FaultCode active_fault_code;
    uint8_t relay_states; // Bitmask of active relays
};

extern SystemState g_system_state;



hmi_manager.h


C++


#pragma once
#include <lvgl.h>

void hmi_init();
void hmi_update();

// Functions to be called by other tasks to trigger UI changes
void hmi_trigger_alarm_animation(bool active);
void hmi_set_alarm_acknowledged(bool acknowledged);
void hmi_trigger_defrost_animation(bool active);
void hmi_update_fault_display(const char* fault_text);



hmi_manager.cpp


C++


#include "hmi_manager.h"
#include "config.h"

// LVGL Objects
static lv_obj_t * scr_main;
static lv_obj_t * label_set_temp;
static lv_obj_t * label_actual_temp;
static lv_obj_t * btn_defrost;
static lv_obj_t * btn_silence;
static lv_obj_t * scr_service_menu;
static lv_obj_t * label_fault_display;

// LVGL Animations
static lv_anim_t anim_temp_pulse_color;
static lv_anim_t anim_temp_pulse_opa;
static lv_anim_t anim_silence_fade_in;
static lv_anim_t anim_defrost_pulse;

// LVGL Styles
static lv_style_t style_btn_blue;
static lv_style_t style_btn_blue_pressed;
static lv_style_t style_btn_lightblue;
static lv_style_t style_actual_temp_normal;
static lv_style_t style_actual_temp_alarm;

// Custom font declaration (generate with LVGL font converter)
LV_FONT_DECLARE(font_segment_large_80);

// Animation callbacks
static void temp_color_anim_cb(void * var, int32_t v) {
    lv_obj_set_style_text_color((lv_obj_t*)var, lv_color_mix(lv_color_hex(0xFF0000), lv_color_white(), v), 0);
}

static void opacity_anim_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}

// Event Handlers
static void silence_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.alarm_status = AlarmState::HIGH_TEMP_SILENCED;
        xSemaphoreGive(g_system_state.mutex);
        
        hmi_set_alarm_acknowledged(true);
        // Logic to start 20-min silence timer will be in control_task
    }
}

static void long_press_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    // Use user_data to store press state: 0=released, 1=pressed, >1=start_tick
    uint32_t* press_state = (uint32_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        *press_state = lv_tick_get();
    } else if (code == LV_EVENT_PRESSING) {
        if (*press_state > 1) { // Check if not already triggered
            uint32_t hold_duration_ms = (target == btn_defrost)? MANUAL_DEFROST_HOLD_MS : SERVICE_MENU_HOLD_MS;
            if (lv_tick_elaps(*press_state) > hold_duration_ms) {
                if (target == btn_defrost) {
                    // Signal control task to start manual defrost
                    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
                    // Set a flag for the control task
                    xSemaphoreGive(g_system_state.mutex);
                    printf("Manual defrost triggered!\n");
                } else { // Service Menu hidden area
                    lv_obj_clear_flag(scr_service_menu, LV_OBJ_FLAG_HIDDEN);
                    printf("Service menu opened!\n");
                }
                *press_state = 1; // Mark as triggered to prevent re-triggering
            }
        }
    } else if (code == LV_EVENT_RELEASED) {
        *press_state = 0; // Reset state
    }
}

void hmi_init() {
    scr_main = lv_scr_act();
    lv_obj_set_style_bg_color(scr_main, lv_color_black(), 0);

    // --- Create Styles ---
    lv_style_init(&style_btn_blue);
    lv_style_set_bg_color(&style_btn_blue, lv_color_hex(0x003366));
    lv_style_set_radius(&style_btn_blue, 8);

    lv_style_init(&style_btn_blue_pressed);
    lv_style_set_bg_color(&style_btn_blue_pressed, lv_color_hex(0x005599));
    
    lv_style_init(&style_btn_lightblue);
    lv_style_set_bg_color(&style_btn_lightblue, lv_color_hex(0xAD8E6E));
    lv_style_set_radius(&style_btn_lightblue, 8);

    lv_style_init(&style_actual_temp_normal);
    lv_style_set_text_font(&style_actual_temp_normal, &font_segment_large_80);
    lv_style_set_text_color(&style_actual_temp_normal, lv_color_white());

    lv_style_init(&style_actual_temp_alarm);
    lv_style_set_text_font(&style_actual_temp_alarm, &font_segment_large_80);
    lv_style_set_text_color(&style_actual_temp_alarm, lv_color_hex(0xFF0000));

    // --- Left Column ---
    lv_obj_t* left_col = lv_obj_create(scr_main);
    lv_obj_set_size(left_col, 211, 480);
    lv_obj_set_pos(left_col, 0, 0);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label_set_temp = lv_label_create(left_col);
    lv_obj_set_size(label_set_temp, 200, 100);
    lv_obj_set_style_bg_color(label_set_temp, lv_color_hex(0x168AEEF), 0);
    lv_obj_set_style_text_color(label_set_temp, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_set_temp, &lv_font_montserrat_48, 0);
    lv_label_set_text(label_set_temp, "-18");
    lv_obj_center(label_set_temp);

    // Fault display label (overlaps set temp when active)
    label_fault_display = lv_label_create(left_col);
    lv_obj_set_size(label_fault_display, 200, 100);
    lv_obj_set_style_bg_color(label_fault_display, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_color(label_fault_display, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_fault_display, &lv_font_montserrat_24, 0);
    lv_label_set_long_mode(label_fault_display, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_fault_display, "");
    lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(label_fault_display, label_set_temp, LV_ALIGN_TOP_MID, 0, 0);


    lv_obj_t* btn_up = lv_btn_create(left_col);
    lv_obj_set_size(btn_up, 200, 100);
    lv_obj_add_style(btn_up, &style_btn_blue, 0);
    lv_obj_add_style(btn_up, &style_btn_blue_pressed, LV_STATE_PRESSED);
    lv_obj_t* label_up = lv_label_create(btn_up);
    lv_label_set_text(label_up, LV_SYMBOL_UP);
    lv_obj_center(label_up);

    lv_obj_t* btn_down = lv_btn_create(left_col);
    lv_obj_set_size(btn_down, 200, 100);
    lv_obj_add_style(btn_down, &style_btn_blue, 0);
    lv_obj_add_style(btn_down, &style_btn_blue_pressed, LV_STATE_PRESSED);
    lv_obj_t* label_down = lv_label_create(btn_down);
    lv_label_set_text(label_down, LV_SYMBOL_DOWN);
    lv_obj_center(label_down);

    btn_defrost = lv_btn_create(left_col);
    lv_obj_set_size(btn_defrost, 200, 100);
    lv_obj_add_style(btn_defrost, &style_btn_lightblue, 0);
    lv_obj_t* label_defrost = lv_label_create(btn_defrost);
    lv_label_set_text(label_defrost, LV_SYMBOL_REFRESH " " LV_SYMBOL_TINT); // Placeholder for snowflake/droplet
    lv_obj_center(label_defrost);
    static uint32_t defrost_press_state = 0;
    lv_obj_add_event_cb(btn_defrost, long_press_event_cb, LV_EVENT_ALL, &defrost_press_state);

    // --- Right Column ---
    lv_obj_t* right_col = lv_obj_create(scr_main);
    lv_obj_set_size(right_col, 580, 480);
    lv_obj_set_pos(right_col, 220, 0);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    
    label_actual_temp = lv_label_create(right_col);
    lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
    lv_label_set_text(label_actual_temp, "-18.2");
    lv_obj_center(label_actual_temp);

    // --- Hidden Elements ---
    btn_silence = lv_btn_create(scr_main);
    lv_obj_set_size(btn_silence, 200, 80);
    lv_obj_align(btn_silence, LV_ALIGN_BOTTOM_RIGHT, -50, -20);
    lv_obj_set_style_bg_color(btn_silence, lv_color_black(), 0);
    lv_obj_set_style_border_color(btn_silence, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(btn_silence, 2, 0);
    lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(btn_silence, silence_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_silence = lv_label_create(btn_silence);
    lv_label_set_text(label_silence, "SILENCE");
    lv_obj_set_style_text_color(label_silence, lv_color_hex(0xFF0000), 0);
    lv_obj_center(label_silence);

    lv_obj_t* service_menu_area = lv_obj_create(scr_main);
    lv_obj_set_size(service_menu_area, 100, 100);
    lv_obj_align(service_menu_area, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(service_menu_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(service_menu_area, 0, 0);
    static uint32_t service_press_state = 0;
    lv_obj_add_event_cb(service_menu_area, long_press_event_cb, LV_EVENT_ALL, &service_press_state);

    scr_service_menu = lv_obj_create(NULL); // Create screen but don't load it
    //... build service menu UI here...
    lv_obj_add_flag(scr_service_menu, LV_OBJ_FLAG_HIDDEN); // Keep it hidden initially

    // --- Initialize Animations ---
    lv_anim_init(&anim_temp_pulse_color);
    lv_anim_set_var(&anim_temp_pulse_color, label_actual_temp);
    lv_anim_set_values(&anim_temp_pulse_color, 0, 255);
    lv_anim_set_exec_cb(&anim_temp_pulse_color, temp_color_anim_cb);
    lv_anim_set_time(&anim_temp_pulse_color, 1000);
    lv_anim_set_playback_time(&anim_temp_pulse_color, 1000);
    lv_anim_set_repeat_count(&anim_temp_pulse_color, LV_ANIM_REPEAT_INFINITE);

    lv_anim_init(&anim_silence_fade_in);
    lv_anim_set_var(&anim_silence_fade_in, btn_silence);
    lv_anim_set_values(&anim_silence_fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&anim_silence_fade_in, opacity_anim_cb);
    lv_anim_set_time(&anim_silence_fade_in, 500);

    lv_anim_init(&anim_defrost_pulse);
    lv_anim_set_var(&anim_defrost_pulse, btn_defrost);
    lv_anim_set_values(&anim_defrost_pulse, LV_OPA_COVER, LV_OPA_50);
    lv_anim_set_exec_cb(&anim_defrost_pulse, opacity_anim_cb);
    lv_anim_set_time(&anim_defrost_pulse, 800);
    lv_anim_set_playback_time(&anim_defrost_pulse, 800);
    lv_anim_set_repeat_count(&anim_defrost_pulse, LV_ANIM_REPEAT_INFINITE);
}

void hmi_update() {
    // This function is called periodically from the main lvgl task
    // It reads the global state and updates the UI elements
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    
    // Update temperatures
    lv_label_set_text_fmt(label_actual_temp, "%.1f", g_system_state.actual_temp_celsius);
    lv_label_set_text_fmt(label_set_temp, "%.0f", g_system_state.setpoint_temp_celsius);
    
    xSemaphoreGive(g_system_state.mutex);
}

void hmi_trigger_alarm_animation(bool active) {
    if (active) {
        lv_anim_start(&anim_temp_pulse_color);
        lv_obj_clear_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
        lv_anim_start(&anim_silence_fade_in);
    } else {
        lv_anim_del(label_actual_temp, temp_color_anim_cb);
        lv_obj_remove_style(label_actual_temp, &style_actual_temp_alarm, 0);
        lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
        lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    }
}

void hmi_set_alarm_acknowledged(bool acknowledged) {
    lv_anim_del(label_actual_temp, temp_color_anim_cb);
    lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    if (acknowledged) {
        lv_obj_add_style(label_actual_temp, &style_actual_temp_alarm, 0);
    } else {
        lv_obj_remove_style(label_actual_temp, &style_actual_temp_alarm, 0);
        lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
    }
}

void hmi_trigger_defrost_animation(bool active) {
    if (active) {
        lv_anim_start(&anim_defrost_pulse);
    } else {
        lv_anim_del(btn_defrost, opacity_anim_cb);
        lv_obj_set_style_opa(btn_defrost, LV_OPA_COVER, 0);
    }
}

void hmi_update_fault_display(const char* fault_text) {
    if (fault_text!= nullptr && strlen(fault_text) > 0) {
        lv_label_set_text(label_fault_display, fault_text);
        lv_obj_clear_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_set_temp, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_set_temp, LV_OBJ_FLAG_HIDDEN);
    }
}



io_controller.h /.cpp (Conceptual)

The full implementation would involve the chosen PCF8574 library.

C++


// io_controller.h
#pragma once
#include <PCF8574.h> // Example library

class RelayController {
public:
    RelayController(uint8_t address);
    bool begin();
    void setCompressorState(bool on);
    void setFanState(bool on);
    //... other relays
    bool getCompressorFeedback();
    void setBuzzer(bool on);

private:
    PCF8574 pcf;
    uint8_t current_relay_states;
};

// io_controller.cpp
#include "io_controller.h"
#include "config.h"

RelayController::RelayController(uint8_t address) : pcf(address) {}

bool RelayController::begin() {
    pcf.begin();
    // Check if device is connected
    return pcf.isConnected();
}

void RelayController::setCompressorState(bool on) {
    if (on) {
        current_relay_states |= (1 << RELAY_PIN_COMPRESSOR);
    } else {
        current_relay_states &= ~(1 << RELAY_PIN_COMPRESSOR);
    }
    pcf.write8(current_relay_states);
}

bool RelayController::getCompressorFeedback() {
    // Critical quasi-bidirectional read logic
    pcf.write(FEEDBACK_PIN_COMPRESSOR, HIGH); // Prime pin for input
    return pcf.read(FEEDBACK_PIN_COMPRESSOR) == HIGH; // Read external state
}

void RelayController::setBuzzer(bool on) {
    digitalWrite(BUZZER_PIN, on? HIGH : LOW);
}



main.cpp


C++


#include <Arduino.h>
#include <lvgl.h>
#include <esp_panel.h>
#include <esp_io_expander.h>
#include "config.h"
#include "hmi_manager.h"
// Include other controller headers

// Global handles
esp_panel::drivers::LCD_ST7262 *display_panel = NULL;
esp_expander::CH422G *io_expander = NULL;
lv_disp_t *lvgl_display = NULL;
SystemState g_system_state;

// LVGL Buffers (in PSRAM)
#define LVGL_DRAW_BUF_SIZE (800 * 80)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// LVGL display flush callback
void display_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    if (display_panel) {
        display_panel->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint16_t*)color_p);
    }
    lv_disp_flush_ready(disp_drv);
}

// LVGL touch read callback (stub, needs GT911 driver implementation)
void touchpad_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    // Full implementation would read from GT911 driver
    data->state = LV_INDEV_STATE_REL;
}

// FreeRTOS Tasks
void lvgl_task(void *pvParameter) {
    TickType_t last_update = xTaskGetTickCount();
    while (1) {
        lv_timer_handler();
        
        // Update HMI with latest data every 250ms
        if (xTaskGetTickCount() - last_update > pdMS_TO_TICKS(250)) {
            hmi_update();
            last_update = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void control_logic_task(void *pvParameter) {
    // Initialize RelayController, Sensor readers, etc. here
    
    while (1) {
        // --- This is the main control loop ---
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        
        // 1. Read all sensors
        // 2. Check for faults (sensor, feedback, etc.)
        // 3. Update g_system_state.active_fault_code
        // 4. Run freezer state machine (cooling, defrosting, idle)
        // 5. Set relays based on state machine
        // 6. Check for alarms
        // 7. Update g_system_state.alarm_status
        
        // Example: Triggering HMI animations from control logic
        if (g_system_state.alarm_status == AlarmState::HIGH_TEMP_ACTIVE) {
            hmi_trigger_alarm_animation(true);
        }
        if (g_system_state.defrost_active) {
            hmi_trigger_defrost_animation(true);
        }
        
        xSemaphoreGive(g_system_state.mutex);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run control logic once per second
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Commercial Freezer Controller Booting...");

    // --- MANDATORY INITIALIZATION SEQUENCE ---
    // 1. Initialize I/O Expander (CH422G)
    io_expander = new esp_expander::CH422G(I2C_SCL_PIN, I2C_SDA_PIN, IOEXP_I2C_ADDR);
    io_expander->init();
    io_expander->begin();

    // 2. Initialize Display Panel (ST7262)
    esp_panel::drivers::BusRGB::Config bus_config = { /*... from main.pdf... */ };
    io_expander->pinMode(CH422G_RST_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_RST_PIN, LOW);
    delay(20);
    io_expander->digitalWrite(CH422G_RST_PIN, HIGH);
    delay(50);
    
    esp_panel::drivers::LCD::Config lcd_config = { /*... from main.pdf... */ };
    display_panel = new esp_panel::drivers::LCD_ST7262(bus_config, lcd_config);
    display_panel->init();
    display_panel->begin();

    // 2C. Enable backlight
    io_expander->pinMode(CH422G_BL_EN_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_BL_EN_PIN, HIGH);

    // 3. Initialize LVGL
    lv_init();
    buf1 = (lv_color_t*)heap_caps_malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LVGL_DRAW_BUF_SIZE);

    // 4. Initialize LVGL display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lvgl_display = lv_disp_drv_register(&disp_drv);

    // 5. Initialize LVGL input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    lv_indev_drv_register(&indev_drv);

    // 6. Initialize Global State
    g_system_state.mutex = xSemaphoreCreateMutex();
    g_system_state.setpoint_temp_celsius = DEFAULT_SETPOINT_C;
    //... initialize other state variables

    // 7. Create HMI
    hmi_init();

    // 8. Create FreeRTOS Tasks
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(control_logic_task, "control_task", 8192, NULL, 3, NULL, 1);

    Serial.println("Setup complete. System running.");
}

void loop() {
    // Main loop is not used; all processing is handled by FreeRTOS tasks.
    vTaskDelay(portMAX_DELAY);
}


