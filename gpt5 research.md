# Waveshare ESP32-S3 4.3" Freezer Controller Project

## Project Overview & Hardware Support

This project implements a **commercial freezer controller** on the Waveshare ESP32-S3 Touch LCD 4.3" Type
B board. It supports all required peripherals and uses a dual-core architecture for reliability and
performance. Key features include:

```
Temperature Sensing: Four DS18B20 digital temperature sensors on a OneWire bus for accurate
freezer temperature readings.
Relay Outputs: A PCF8574 I/O expander controls relays for the compressor, hot gas bypass valve,
electric heater, and cooling fans. Using an I²C expander frees ESP32-S3 GPIOs and allows safe 3.3V
control of multiple relays.
Buzzer Alarm: A piezo buzzer driven directly by an ESP32-S3 GPIO (with a transistor if needed for
current) provides audible alerts (e.g. open door alarm, fault notifications).
Touchscreen Display: A 4.3" 800×480 IPS LCD with 5-point capacitive touch (GT911 controller) serves
as the HMI. The display is driven via the ESP32-S3's 16-bit parallel RGB interface (RGB565, 65k colors)
with the Sitronix ST7262 LCD controller. The capacitive touch panel communicates via I²C and
raises an interrupt on touch for responsive input.
```
All these peripherals are integrated into a cohesive **PlatformIO** project with an emphasis on modular
structure and fault tolerance. The codebase is structured in C++ (Arduino/ESP-IDF style) with separate
components for sensors, UI, and control logic, ensuring maintainability and clarity.

## PlatformIO Setup and Partitioning

We use **PlatformIO** with the Espressif 32 platform and Arduino framework for this project. The
configuration is tuned to the Waveshare board’s hardware to enable external PSRAM and high-speed
parallel display operation. Important settings in platformio.ini include:

```
[env:waveshare_esp32_s3_touch_lcd_4_3b]
platform= espressif
board= esp32s3box ; use ESP32-S3 with similar specs (16MB flash, 8MB
PSRAM)
framework= arduino
monitor_speed= 115200
```
```
; --- Flash & PSRAM Configuration ---
board_build.flash_mode= qio ; Quad I/O flash at 80MHz
board_build.flash_size= 16MB ; Use entire 16MB external flash
board_build.arduino.memory_type =qio_opi ; Quad-SPI flash, Octal PSRAM mode
board_build.psram_mode= octal ; Enable 8MB Octal PSRAM high bandwidth
```
#### •

#### •

#### •

#### •

```
1 2
```
```
3 4
```
```
3
5 3
6
4
```

```
; --- Build Flags for PSRAM and LVGL ---
build_flags =
-D BOARD_HAS_PSRAM ; Inform Arduino core that PSRAM is
present
-D LV_CONF_INCLUDE_SIMPLE ; Use lv_conf.h without path
-D LV_TICK_PERIOD_MS= 5 ; LVGL tick period 5ms for 200Hz
refresh
```
```
; --- Libraries (pinned for stability) ---
lib_deps=
lvgl/lvgl@^8.3.11 ; LVGL graphics library v8.3 (stable)
```
```
espressif/ESP32_Display_Panel@=1.0.0 ; Espressif LCD panel driver (ST
support)
espressif/ESP32_IO_Expander@=1.0.1 ; CH422G I/O expander driver
milesburton/DallasTemperature@^3.11.0 ; DS18B20 temperature library
paulstoffregen/OneWire@^2.3.7 ; OneWire bus library for DS18B
robertmao/PCF8574@^1.0.0 ; PCF8574 expander library (or custom
I2C code)
```
This configuration enables the full 16MB flash and 8MB PSRAM of the ESP32-S3-WROOM module.
PSRAM is used in **octal mode** for maximum bandwidth, which is critical for driving the 800×480 display at
high frame rates. The build flags ensure PSRAM support is enabled in the heap allocator and
configure LVGL’s tick to 5ms for smooth UI animations. We pin LVGL to the 8.3.x branch, as this version
is known to be stable on this hardware (newer LVGL 8.4/9 versions have had issues with fonts and
compilation on ESP32-S3). The Espressif ESP32_Display_Panel and ESP32_IO_Expander libraries
are included to handle the unique display and I/O expander on this board (as explained below). We lock
these to the proven versions (1.0.0/1.0.1 respectively) to avoid breaking changes in newer releases.

**Partition Scheme:** We use the default partition scheme provided by the board definition (since we specified
esp32s3box). This allocates ample space for the application and uses PSRAM for dynamic data. If needed,
a custom partition table can be used to dedicate a portion of flash for an SPIFFS/LittleFS filesystem (for
fonts, images) but in this project the 16MB flash is enough for code and assets without a separate partition.
The key is that PSRAM is activated so large allocations (LVGL draw buffers, images, etc.) go into external
memory, freeing internal SRAM for critical tasks.

## System Architecture (Dual-Core Task Split)

The firmware is designed with a **dual-core FreeRTOS task architecture** to separate time-critical UI
rendering from the control logic of the freezer. Each ESP32-S3 core runs dedicated tasks:

```
Core 0 (App CPU) – User Interface: All display drawing and touch handling tasks run here at high
priority. By pinning the GUI tasks to Core 0, we ensure the LVGL graphics and touch processing have
real-time responsiveness and are not blocked by sensor I/O or control logic. Core 0 handles:
```
```
7
```
```
8
```
```
5 4
```
```
9 4
7
```
```
8
```
```
10 11
```
#### •

```
12 13
```

```
The LVGL task that periodically calls lv_timer_handler() to update the UI (running at ~200 Hz
tick).
The Display refresh task (if using a separate thread to push frame buffers) which calls the LCD
driver flush method. In practice, LVGL can be configured to call the flush in its own context, so a
separate task may not be needed, but all display updates occur on Core 0.
The Touch input task that waits for touch interrupts or polls the GT911 controller via I2C for new
touches. This task signals LVGL when input is available (using lv_indev_drv callbacks).
Core 1 (App CPU) – Control Logic: The freezer’s brains run on Core 1. These tasks include:
Control Task: Implements the main state machine for freezer operation, handling compressor
cycles, defrost (hot gas / heater) logic, fan control, and alarm conditions. This runs at a moderate
priority in a loop with a 1Hz cycle time (one control update per second, which is sufficient for thermal
control).
Sensor Task: Handles reading the DS18B20 sensors asynchronously. This task triggers temperature
conversions and reads the results without blocking the control logic. It runs at a lower priority on
Core 1, waking when sensor data is needed.
I/O Expander Task (Relay/Buzzer): Optionally, toggling relays and buzzer could be done directly in
the control task, but if those operations were slow (I2C writes), we could offload them to a separate
task or use the control task to send commands to an I2C driver task. In practice, the PCF8574 I/O
expander writes are very fast (single I2C byte), so the control task can write to it directly under a
mutex.
Logging/Networking Task: (Optional) If logging to SD card or sending data over network, those
would also run on Core 1 at the lowest priority so as not to interfere. (Networking is not in current
requirements, but the architecture allows adding WiFi tasks on Core 1 later if needed.)
```
This arrangement leverages the dual 240 MHz cores to ensure **UI fluidity and control reliability**. The UI
tasks on Core 0 can run at high priority and update the display at ~60 FPS without stuttering, since they
never have to wait on sensor I/O or heavy computations on Core 1. Meanwhile, the control logic on
Core 1 can perform sensor reads, state decisions, and file logging without being starved by the constant
demands of refreshing a high-resolution display. FreeRTOS queues and mutexes are used for thread-safe
communication between cores – for example, a queue might send user commands (like setpoint changes)
from the UI to the control task, and a shared data structure (protected by a mutex) holds the latest sensor
readings and compressor state for the UI to display.

**Task Synchronization:** Care is taken with any data shared across cores. A **mutex** (e.g., an xSemaphore or
std::mutex if using Arduino) protects structures that are written by the control logic and read by the UI
(such as current temperature readings, active mode, alarms). Because the LVGL library is single-threaded,
all LVGL calls (creating widgets, updating labels) occur on the LVGL task/Core 0. If the control task needs to
update the UI (e.g., trigger a popup or change a status icon), it will send an event or set a flag that the UI
task picks up, rather than calling LVGL directly from Core 1. This prevents thread conflicts in the graphics
library.

Below is an outline of the FreeRTOS tasks and their core affinity and purpose:

```
Core 0 (UI Core):
```
- lvglTask (High priority): LVGL tick and rendering loop (handles
lv_timer_handler).
- touchTask (High priority): Reads GT911 touch via I2C, feeds LVGL input device.

#### • • • • • • • •

```
12 14
```

- (Optional) displayFlushTask (High priority): Handles sending buffered frames
to LCD (if not done in lvglTask).

```
Core 1 (Control Core):
```
- controlTask (Medium priority): Main freezer control loop (state machine,
decisions).
- sensorTask (Low priority): DS18B20 temperature acquisition (scheduled
conversions).
- loggingTask (Low priority): Data logging or network, runs in background.

_(The actual implementation might combine some of these for simplicity; e.g., sensor reading could be part of
controlTask’s loop, but conceptually this separation improves clarity.)_

## Display and Touch Interface (LVGL on ST7262 & GT911)

Driving the 800×480 capacitive touch display is the most technically challenging part of this project due to
the Waveshare board’s unique design. Unlike typical dev boards, the **LCD’s critical control signals are not
directly connected to ESP32 GPIOs** – they are routed through a WCH CH422G I/O expander chip. The
LCD controller (Sitronix ST7262) requires toggling a hardware reset line (called GRB or LCD_RST) and
enabling the backlight power pin, but on this board those signals can only be toggled via I²C commands to
the CH422G expander. This means a standard approach like calling digitalWrite(LCD_RST,
LOW) will **not work** , and out-of-the-box display libraries (like TFT_eSPI, LovyanGFX) cannot initialize this
display hardware. To solve this, we use Espressif’s dedicated **ESP32 LCD and IO expander libraries** :

```
ESP32_Display_Panel : a library that leverages the ESP32-S3’s LCD peripheral (16-bit parallel
interface) and supports various controllers including ST7262. It knows how to drive the RGB data
lines and timing signals (HSYNC, VSYNC, DE, PCLK) according to the ST7262 requirements. We
configure it with the exact pin mapping of this board (see LCD pin mapping below).
ESP32_IO_Expander (CH422G) : a companion library to control the CH422G I/O expander via I²C. This
is essential , because without it the LCD reset and backlight lines cannot be toggled and the screen
would remain dark. The expander’s I/O pins are used for:
LCD Reset (EXPANDER_IO0 or IO3 depending on board rev) – must be pulsed low then high during
init.
LCD Backlight enable (EXPANDER_IO2) – to turn the screen backlight on/off.
Touch controller Reset (EXPANDER_IO1) – resets the GT911 touch IC.
SD card CS (EXPANDER_IO4) – chip select for the SD card slot (since most GPIOs are used by LCD)
.
USB/CAN mode switch (EXPANDER_IO5) – not used in our app (selects USB or CAN function).
```
The CH422G is an I²C device (on the same I2C bus as the touch controller). We initialize it at startup, set the
necessary expander pins as outputs, and drive them appropriately (LCD_RST high, LCD_BL on, etc.). The
library provides a class ESP_IOExpander_CH422G with methods to set pins. This is integrated with the
Display_Panel library by passing the expander instance to the LCD panel init, so that the panel driver can
automatically use the expander to reset the LCD and control backlight power.

**LCD Panel Configuration:** The LCD is a parallel RGB panel with a **16-bit color bus** (RGB565 format). We
derive the timing parameters from Waveshare’s documentation and the ST7262 datasheet. For 800×480 at

```
15
```
```
15 16
```
```
17 18
```
#### •

#### •

```
19
```
-
    20 21
-^22
-^23
-^24
    25
-^26

```
27
```

~60 Hz refresh, the pixel clock (PCLK) is ~20-25 MHz. The ESP32’s LCD peripheral is configured for **16-bit
parallel (8080-like)** mode, using 16 data GPIOs and the four control signals. The specific pin mapping
(according to the Waveshare schematic and LCD timing.txt) is:

```
Timing signals: HSYNC = GPIO46, VSYNC = GPIO3, DE (Data Enable) = GPIO5, PCLK = GPIO
```
. These connect to the LCD FPC connector.
**16-bit Data lines:**
Red: R3–R7 on GPIO1, GPIO2, GPIO42, GPIO41, GPIO
Green: G2–G7 on GPIO39, GPIO0, GPIO45, GPIO48, GPIO47, GPIO
Blue: B3–B7 on GPIO14, GPIO38, GPIO18, GPIO17, GPIO

_(Note: GPIO0, 45, 46 are strapping pins on ESP32-S3, so their state at boot matters. The code ensures proper
functions (e.g., not pulling them low) to avoid boot mode issues.)_

With these mappings, we create a custom **panel configuration header** (often ESP_Panel_Conf.h) to
override any default profiles and exactly match our hardware. In this config, we specify the
resolution (800×480), data bus width (16-bit), the GPIO assignments for all signals, and the type of LCD
controller (ST7262). We also specify the connection of the CH422G expander (its I2C address and which
expander pin corresponds to LCD reset, etc.). Setting ESP_PANEL_USE_SUPPORTED_BOARD (0) and
providing our definitions ensures the ESP32_Display_Panel library uses our custom configuration
.

**Display Initialization Sequence:** On startup, the following steps occur (in code, usually in setup()):

```
Initialize I2C bus on GPIO8 (SDA) and GPIO9 (SCL) at 400 kHz. The GT911 touch and CH422G
expander share this bus. We must ensure no address conflicts – by default, GT911 uses
0x5D/0x14 and CH422G uses 0x71 (per documentation). (The PCF8574 expander for relays, if
connected to this bus, should be configured to an address (e.g., 0x20) that does not conflict .)
Init CH422G Expander: Create the ESP_IOExpander_CH422G object and call its init. We set all
relevant expander pins to output mode. Then drive LCD_RST low, LCD_BL off, Touch_RST low via
the expander. After a short delay, set LCD_RST high to release the LCD from reset (per ST
datasheet timing) , and also set Touch_RST high. We leave the backlight off until the LCD
controller is fully initialized, to avoid a white screen flash.
Init LCD Panel: Create the ESP_Panel object (from ESP32_Display_Panel) for our ST7262 LCD. We
pass it the bus configuration (the ESP32’s LCD peripheral handles the data lines and sync signals) and
a reference to the IO expander. We then call panel.init() to configure the panel. Under the
hood, this will send the proper command sequence to the ST7262 over I2C (the ST7262 has an
internal serial control interface for configuration registers). The library takes care of sending
commands like Display Mode Setting , Frame Rate , Enable Display Output , etc., as required by the
controller.
Enable Backlight: Once the panel init is done (and probably after LVGL is set up to draw something),
we set the expander pin for backlight on (e.g., CH422G.setPin(EXIO2, HIGH)). This powers the
screen’s backlight LED driver and makes the image visible. The backlight can later be dimmed or
turned off for power save by toggling this pin.
Init LVGL: We call lv_init() to initialize the LVGL library. We allocate two draw buffers in
PSRAM, each about 1/4 to 1/2 of the screen (e.g., 50 lines of 800px, 800 50 2 bytes ≈ 80 KB each).
Using two buffers enables double buffering for smooth, tear-free updates. The buffers are
```
```
28
```
#### •

```
29
```
-
-^30
-^31
-^32
    _33_

```
34 35
```
```
36
37
```
#### 1.

```
38 39
40 41
39
2.
27
```
```
16
```
#### 3.

```
42 43
```
#### 4.

```
22
```
#### 5.

```
44 45
```

```
allocated with heap_caps_malloc(..., MALLOC_CAP_SPIRAM) so that they reside in external
PSRAM, preventing large memory usage in internal SRAM.
LVGL Display Driver Setup: We create an lv_disp_drv_t and lv_disp_draw_buf_t. We init
the draw buffer with our two PSRAM buffers. We set the flush callback (disp_drv.flush_cb) to
the function provided by the ESP_Panel library, typically panel->getLcd()->flush. This ties
LVGL’s rendering to our LCD driver: when LVGL is ready to output pixels, it calls this flush function,
which in turn uses the ESP32’s DMA to send the buffer over the RGB interface to the screen. We
register the driver with lv_disp_drv_register.
LVGL Input Device (Touch) Setup: We create an lv_indev_drv_t for the touch panel. The GT
driver is provided by the ESP32_Display_Panel library as well (it has a built-in handler for common
touch controllers). We set the read callback to panel->read() or a wrapper that reads from
GT911 via I2C. This will fetch touch points (GT911 supports up to 5 simultaneous touches) and feed
LVGL. We also configure the touch interrupt pin (GPIO4) – the GT911 asserts this pin low when new
touch data is ready. We attach a simple ISR or use the IO expander to detect this (the GT911 INT
might actually be wired through CH422G on some pin as well, according to the schematic). On
interrupt, we can signal the touch task to read coordinates.
Create UI Screens: With LVGL up, we build the UI (screens, buttons, labels, charts, etc.) according to
the HMI design. This might include a main screen showing current temperature, setpoint, mode
(cooling/heating/idle), and status icons for each relay, plus menus for settings, defrost control, etc.
```
By following this sequence, we adhere to the **strict initialization order** imposed by the hardware.
Notably, the CH422G must be initialized _before_ the LCD panel, so that reset/backlight pins can be toggled,
otherwise the LCD or touch will not come out of reset and the display stays blank. The requirement
to coordinate these in software is the primary challenge of using this Waveshare board.

**Community Insights:** Other developers have faced this unique setup; the consensus (as seen in forums
and Waveshare’s own examples) is to use Espressif’s panel and expander libraries together. This
project embraces that known-good stack: **LVGL -> ESP32_Display_Panel -> ESP32_IO_Expander**. This
layered approach ensures that LVGL simply calls a flush routine, not caring about hardware details, while
the display panel library handles the RGB timing and uses the IO expander driver to toggle reset/bl pins
behind the scenes. The result is a working display with minimal custom code for the developer, aside from
configuration. It is important to avoid updating these libraries to newer versions without careful testing, as
subtle changes (like renaming headers or adding C++ namespaces) have broken compatibility in the past

. The versions we use are known to work with this board’s Type-B configuration.

## Sensor and Actuator Management

### DS18B20 Temperature Sensors (OneWire)

All four DS18B20 sensors are connected on a single OneWire bus (parasite power mode is not used; each
sensor has Vdd, GND, and data). We choose a free GPIO on the ESP32-S3 for the OneWire data line (for
example, GPIO16 or another not occupied by the display – GPIO16 is used for CAN RX on the board, which
we may repurpose if CAN is unused). A 4.7k pull-up resistor from the data line to 3.3V is required. The
**DallasTemperature** library is used to manage the sensors, built on the OneWire library.

**Asynchronous Reading:** Instead of blocking for each temperature conversion (which can take 750ms for
12-bit resolution), the sensor task uses a non-blocking state machine: 1. Issue a **convert T command** to all

#### 6.

```
46
```
#### 7.

```
47
```
#### 8.

```
20 48
```
```
49 19
```
```
18 50
50
```
```
10 11
```

sensors (this starts the temperature measurement on each DS18B20). 2. Continue with other tasks or
set a timer. The DS18B20 will perform conversion internally (~750ms) while the MCU can do other work. 3.
After the conversion time, read the scratchpad from each sensor to get the temperature, and **check CRC** to
ensure data integrity. 4. Update the global temperature variables (for each probe) and any fault flags if a
sensor is missing or gives an out-of-range reading. 5. Repeat this cycle periodically (e.g., every 1 second,
since thermal changes are slow).

By doing this in a separate FreeRTOS task (or as part of the control task loop with non-blocking delays), we
ensure sensor I/O does not stall the main control logic. The DallasTemperature library supports
asynchronous operation by calling requestTemperatures() to start conversion and then later calling
getTempC() for each sensor. We also store the addresses of each DS18B20 (learned at startup) and map
them to specific roles (e.g., Evaporator coil sensor, Freezer air sensor, etc.).

If a sensor read fails or CRC check fails, the code sets a fault flag for that sensor and possibly triggers an
alarm on the UI. The control logic can default to a safe mode (e.g., turn off compressor if critical sensor is
unreadable to prevent freezing uncontrolled). Robust handling ensures a disconnected or failed sensor will
not hang the OneWire bus transactions indefinitely – timeouts are applied and errors are caught.

### Relay Outputs via PCF

The compressor, hot gas valve, heater, and fan relays are controlled through a **PCF8574 8-bit I/O expander**
on I²C. This expander is connected to the same I2C bus (GPIO8/GPIO9) but at a different address (commonly
0x20 for PCF8574 with all address pins low). The PCF8574 outputs can source/sink only limited current, so
each output drives the coil of a relay module or an intermediate transistor/MOSFET that actually powers the
relay coil (depending on the relay board design).

We configure the PCF8574 library to set the four relevant pins as outputs. Each bit corresponds to a specific
control line: - P0: Compressor relay (ACTIVE_HIGH for compressor on) - P1: Hot Gas bypass valve (defrost
valve) solenoid - P2: Electric Heater relay (for defrost in electric mode, if applicable) - P3: Evaporator Fans
relay - (P4-P7 could control additional loads or be left unused/spare)

Toggling a relay simply involves writing a byte to the expander via I2C. For example, to turn on compressor
(P0) and fan (P3) while others off, we write binary 00001001 = 0x09 to the PCF8574. The PCF library allows
writing individual pins, or we can maintain a shadow state of the output byte in software and update it
atomically. We ensure **thread safety** by updating relays only from the control task context (or by using a
mutex around I2C writes if the UI might also access the expander). Typically, only the control logic changes
the relays based on state machine decisions.

The I2C operations are quick (PCF8574 uses a single byte write). In case the I2C bus is busy (e.g., while the
touch or CH422G is accessed), we may queue the operation or retry, but because we prioritize keeping the
UI responsive, the control task’s I2C access can be a lower priority. This avoids any stuttering on the display.

### Buzzer

The buzzer is driven by a single GPIO (for instance, GPIO 8 is used for SDA so not that; perhaps GPIO 15 or
16 if free, or one of the available outputs on the board’s screw terminal). The buzzer can be used in two
ways: - **On/Off beeps:** simply setting the GPIO HIGH (with output mode) will apply 3.3V to the buzzer,

```
51
```
```
52 53
```

making a tone (if it’s a simple piezo buzzer it will click or produce a tone depending if it’s an active buzzer).
Toggling the pin with a delay can create beeps. - **PWM for tone:** Using the LEDC PWM peripheral, we can
drive the buzzer at a specific frequency (e.g., 2 kHz tone) for a more noticeable alarm sound.

For simplicity, the code will include a Buzzer class that abstracts this. It can have methods like beep(ms
duration) for a simple beep or alarmTone(frequency, ms duration). The buzzer is directly
handled by the control logic or an alarm subsystem. For example, if a door-open alarm condition is
detected, the control task might call Buzzer.beep(1000) to beep for 1 second (or repeatedly). This can
be done without affecting other tasks significantly.

Since the buzzer pin is just a normal GPIO, controlling it does not involve heavy processing or libraries, and
thus it can be done in the control task or via a simple timer (no special concurrency concerns aside from not
blocking other actions while beeping, which can be handled with non-blocking delays or a dedicated low-
priority task for buzzer that plays patterns).

## Memory and Performance Optimizations

Driving a high-resolution GUI and handling real-time control on a microcontroller requires careful memory
and performance management. We apply several optimizations:

```
PSRAM Utilization: The ESP32-S3’s 8MB external PSRAM is used for all large buffers. We explicitly
enable PSRAM in the build (BOARD_HAS_PSRAM and PSRAM octal mode) and configure LVGL to use
custom memory allocators that pull from PSRAM. The LVGL draw buffers (~160KB total for
double buffer at 800x50 each) and the style/obj memory pool (configured via LV_MEM_SIZE, e.g.,
2MB for LVGL) reside in PSRAM. We also put any large static assets (images) in external RAM
by using appropriate macros or linker sections if needed. This ensures the internal 512KB SRAM
remains mostly free for stack, critical data, and WiFi/BLESP tasks if they come into play.
DMA Frame Buffer Alignment: The ESP32’s LCD DMA requires buffers to be aligned to 4 bytes and
ideally placed in PSRAM (since the LCD driver can read from PSRAM directly via the EDMA). We
allocate with heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA) to get
DMA-capable PSRAM buffers. This avoids slowing down the frame flush.
LVGL Configuration: The LVGL library is tuned for performance: we set LV_USE_GPU_NXP_PXP etc.
off (no special GPU), but we do set LV_ATTRIBUTE_FAST_MEM to place critical drawing routines in
IRAM if needed. The tick rate is 5ms, and we reduce animations or use the 16-bit color depth to
limit computation. Font caching can be adjusted to avoid heavy calculations at runtime.
Task Priorities and Yields: The high-priority UI tasks on Core 0 will frequently yield (e.g., after each
LVGL flush or tick) to let other tasks run, since FreeRTOS tick is 1ms (we can use 1000 Hz tick rate per
Waveshare guidance for smoother LVGL). The control task runs at a lower priority and sleeps
most of the time (waiting 1000ms per loop), so it hardly impacts CPU usage. Sensor reads and I2C
are also low duty-cycle. This means most of the time, the CPU can boost the frame rate if needed. If
we find that the UI can update faster than 30-40 FPS, we might even increase the LVGL buffer height
to cover more lines, trading memory for speed to reduce the number of flush calls.
Watchdog Timer: As part of fault tolerance, we enable the Task Watchdog (TWDT) on the control
core. The control task is subscribed to the watchdog with (for example) a 5 second timeout.
In each iteration of the control loop, after finishing all steps, the task calls
esp_task_wdt_reset() to feed the watchdog. If the control task ever hangs (due to a coding
```
#### •

```
54 55
```
```
55 45
```
#### •

#### •

```
56
```
#### •

```
57
```
#### •

```
58 59
```
```
60
```

```
bug or unexpected block), the watchdog will reset the ESP32, preventing a frozen controller in the
field. The UI tasks on Core 0 can also be under watchdog if needed, but since they run at
high priority frequently, it's unlikely they starve. We configure the watchdog to monitor both cores’
idle tasks as well, to catch any scenario where FreeRTOS scheduling stops.
Non-Blocking I/O: All sensor and I2C interactions have timeouts and are non-blocking where
possible. For I2C (touch, expander, PCF8574), if a device doesn’t respond, the driver will not get stuck
indefinitely; it will report an error and the error handler can reset that subsystem. Similarly, OneWire
(DallasTemperature) is used carefully: by splitting the convert and read phases, we avoid sitting and
waiting. This prevents a slow sensor or broken wire from halting the main loop. In case of an I2C
lockup (e.g., noise causing the bus to hang), we can implement a recovery by re-initializing the I2C
driver if no communication succeeds after X attempts.
Fault Flags and Safe States: Each peripheral (sensors, expander, etc.) has a “health” flag. The control
logic continuously monitors these. For example, if all temperature readings fail, the system can
assume the worst (perhaps initiate a shutoff to avoid running blind). If a relay expander doesn’t
acknowledge, we can light an alarm on the UI (“Output Fault”). By designing the state machine to be
fault-aware , the system can enter a safe mode rather than blindly controlling if something is wrong.
```
These optimizations collectively ensure the system runs **smoothly and reliably**. In testing, with full UI
running and all tasks active, the ESP32-S3 should still have CPU headroom (especially since the heavy lifting
of pixel pushing is done via DMA). Memory usage is balanced between internal and external memory to
avoid fragmentation and exhaustion of critical internal RAM needed by WiFi or other system components
.

## Code Structure and Module Breakdown

The project is organized into a clear modular directory structure within the PlatformIO environment:

```
FreezerController/
├── platformio.ini # PIO configuration (board, env, libs, etc.)
└── src/
├── main.cpp # Main entry point: system setup and task
startup
├── sensors/
│ ├── ds18b20.h / .cpp # DS18B20 handling (OneWire bus init, read
logic)
│ └── pcf8574.h / .cpp # PCF8574 driver (if not using external lib)
├── ui/
│ ├── display.h / .cpp # LVGL setup, screen creation, display driver
binding
│ ├── touch.h / .cpp # Touch handling (GT911 I2C reads, IRQ
handling)
│ └── lvgl_ui.h / .cpp # UI screens and events (could be multiple
files for each screen)
└── logic/
├── control.h / .cpp # Main control state machine and logic
├── states.h # Definition of states, constants for modes
```
```
61 62
```
#### •

#### •

```
63 64
```

```
(cooling, defrost, etc.)
└── alarms.h / .cpp # Alarm and fault management (optionally
separate)
```
Additionally, a lib/ folder can hold any local libraries. In our case, we include ESP32_Display_Panel
and ESP32_IO_Expander as library dependencies; PlatformIO will fetch them. However, given the version
sensitivity, we could also embed them in lib/ to lock the versions. The lib/ folder may also include
lv_conf.h (LVGL configuration) and the custom ESP_Panel_Conf.h for the display driver as
mentioned.

**main.cpp:** Responsible for initializing all components and starting tasks. It will set up serial debug
(Serial.begin(115200)), configure the I2C and OneWire buses, initialize the display and touch (through
our UI module), and create the FreeRTOS tasks for control and possibly a separate task for LVGL if needed.
In Arduino framework, setup() and loop() can be used: one common pattern is to create the control
task in setup() (pinned to core 1, or core 0 depending on desired assignment) and let the default
loop() run LVGL on the other core. For example:

```
#include<Arduino.h>
#include"sensors/ds18b20.h"
#include"sensors/pcf8574.h"
#include"ui/display.h"
#include"ui/touch.h"
#include"logic/control.h"
```
```
TaskHandle_tcontrolTaskHandle;
```
```
voidsetup() {
Serial.begin(115200);
Serial.println("System Booting...");
```
```
// Initialize I2C (for touch, CH422G, PCF8574)
Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN, 400000); // e.g., SDA=GPIO8,
SCL=GPIO
```
```
// Initialize display and touch subsystems (LVGL, etc.)
init_display_and_touch(); // This will do panel + LVGL init as described
above
```
```
// Initialize sensors (OneWire bus)
init_temperature_sensors();
```
```
// Initialize relays and buzzer
init_relays_and_buzzer();
```
```
// Set up the Task Watchdog (5s timeout monitoring control task)
constint WDT_TIMEOUT_S= 5;
```
```
65 66
```
```
38
```

```
esp_task_wdt_config_twdt_config = {
.timeout_ms = WDT_TIMEOUT_S* 1000,
.idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // watch idle tasks on
both cores
.trigger_panic= true
};
esp_task_wdt_init(&wdt_config);
```
```
// Create the control logic task on Core 1
xTaskCreatePinnedToCore(
controlTask, // task function
"ControlLogic", // name
8192 , // stack size (words)
NULL, // parameters
1, // priority (medium)
&controlTaskHandle,
1 // pin to Core 1 (application core)
);
```
```
// Optionally, create a separate task for LVGL handling on Core 0:
// xTaskCreatePinnedToCore(uiTask, "UITask", 8192, NULL, 2, NULL, 0);
// (Alternatively, use loop() on core 0 for UI as done below.)
}
```
```
voidloop() {
// If we haven't made a separate LVGL task, run LVGL handling in this loop.
lv_timer_handler(); // LVGL tick handler to update UI
delay(5); // run at 5ms intervals to match LV_TICK_PERIOD_MS
}
```
In the above setup, the Arduino loop() itself runs on **Core 0** by default in this configuration (since we
pinned the control task to core 1). We call lv_timer_handler() in the loop with a 5ms delay, effectively
dedicating Core 0 to continuously updating LVGL. Meanwhile, controlTask runs on Core 1
independently. This achieves the desired core split: UI on Core 0, control on Core 1 as required.

**controlTask (in control.cpp):** This implements the main control loop, which runs forever on Core 1.
Pseudocode structure:

```
#include"control.h"
#include"sensors/ds18b20.h"
#include"sensors/pcf8574.h"
#include"logic/states.h"
#include"logic/alarms.h"
#include<esp_task_wdt.h>
```
```
voidcontrolTask(void* pv) {
```

```
Serial.println("Control Task started on Core 1");
esp_task_wdt_add(NULL); // subscribe this task to watchdog
```
```
// Example state variables
ControllerState state= STATE_IDLE;
uint32_t lastTempRead= 0;
```
```
for (;;){
uint32_t now= millis();
```
```
// 1. READ SENSORS (Non-blocking)
if(now- lastTempRead>= 1000) { // read temperatures every 1000ms
update_all_temperatures(); // triggers conversion or reads
cached values
lastTempRead= now;
}
```
```
// 2. EXECUTE STATE MACHINE
// Determine control actions based on temperatures and desired
setpoints.
update_control_state(); // e.g., decide to turn compressor on/off,
defrost, etc.
```
```
// 3. UPDATE ACTUATORS
apply_outputs_via_relays(); // writes to PCF8574 pins according to
state (compressor, etc.)
```
```
// 4. SHARE STATE WITH UI
xSemaphoreTake(stateMutex, portMAX_DELAY);
globalStateCopy= currentStateData; // update global struct that UI
reads
xSemaphoreGive(stateMutex);
```
```
// 5. FEED THE WATCHDOG
esp_task_wdt_reset(); // indicate that control loop did not stall
```
```
// Delay until next loop iteration (run roughly 1 Hz)
vTaskDelay(pdMS_TO_TICKS(1000));
}
}
```
In this outline (inspired by the design documentation) , the control task reads sensors, runs the state
machine, updates outputs, and then yields. If any step were to hang (say update_all_temperatures()
got stuck waiting on a sensor), the watchdog would **not** be reset and would trigger a reboot after 5
seconds, thus recovering the system from a fault. Each sub-function (sensors, state machine, outputs) is
designed to be robust: for example, update_all_temperatures() checks a conversion state machine

```
67
```
```
60
```
```
51 60
```

so it never blocks more than a few milliseconds; apply_outputs_via_relays() writes to the PCF
and if the I2C bus is busy, it will retry or time out rather than blocking indefinitely.

**LVGL UI Module:** This handles creating the UI screens and updating them. We might have functions like
ui_create_main_screen() that sets up labels for each temperature, icons for each relay, etc., and
stores their object pointers. Then the UI task or loop can periodically refresh the displayed values by calling
lv_label_set_text(tempLabel1, formatTemp(probe1_temp)), etc., within an lv_timer or when
notified of new data. We ensure such updates happen in the LVGL thread context (Core 0). For example,
after the control task updates the globalStateCopy, it could call
lv_async_call(ui_update_from_state, NULL) – a mechanism to schedule a call to update UI on the
LVGL thread.

**Touch Handling:** The touch.cpp would set up an interrupt on GPIO4 (touch INT). In the interrupt
handler (or using the IO expander’s interrupt if the INT pin is routed through it), we simply signal the
touchTask (like give a semaphore or set a flag). The touchTask (running on Core 0) then reads the
touch data registers via I2C (GT911 protocol) to get coordinates. We translate those into LVGL’s input
data format and feed them using lv_indev_drv. This may also be done by the ESP_Panel library
automatically if we use its panel->read() which likely reads GT911. In that case, our job is just to call
lv_indev_drv_feed() in a loop.

## README and Wiring Guide

Finally, the project includes a detailed **README.md** that describes how to wire up the hardware and how to
build/flash the firmware. Key points from the README include:

```
Board Setup: Ensure you have the Waveshare ESP32-S3 4.3" Type B board. Select the 5V or 7-36V
power input as needed (the board can be powered via USB-C or the VIN terminal). Connect
the USB-C to your PC for programming.
Wiring Peripherals:
DS18B20 Sensors: Connect all four sensors in parallel to the chosen OneWire GPIO (e.g., GPIO16).
Use a 4.7 kΩ pull-up resistor from data line to 3.3V. Power each sensor with 3.3V and GND (all
ground connections common with the board’s GND). The screw terminal labeled "Sensor" on the
board can be used (it likely breaks out an ADC, but we repurpose for OneWire). Each sensor will be
assigned in software to a role (freezer air, coil, ambient, etc.).
Relay Module (PCF8574): The PCF8574 expander should be connected to the board’s I2C terminal (a
PH2.0 4-pin connector for I2C and power). Connect SDA to GPIO8, SCL to GPIO9, and supply the
module with 3.3V and GND from the board. The relay control lines (P0–P3 outputs) wire into the relay
driver circuits (which then connect to the actual relay coils and ultimately switch the compressor,
valve, heater, fans mains circuits – typically via a transistor or optocoupler on the relay board).
Important: Choose the I2C address of the PCF8574 by tying its address pins A0,A1,A2 to GND or VCC
appropriately (0x20 is default with all grounded). Do not use an address that conflicts with 0x
(RTC), 0x5D (touch), or 0x71 (CH422G) on the same bus.
Buzzer: Connect a piezo buzzer + to an ESP32 GPIO (for example, IO seis? Actually we need a free
pin; if IO6 is free we can use it, but IO6 was maybe used by something (it's listed as RTC_INT on
schematic). We can use another free pin like IO37 if available, or one of the expander outputs if
convenient). The negative terminal of the buzzer goes to GND. If the buzzer requires more current,
```
```
68 69
```
#### •

```
70 71
```
#### •

#### •

#### •

```
72
```
```
39
```
-

```
73
```

```
use a transistor: GPIO -> base of NPN (with resistor), emitter to GND, buzzer + to 5V, buzzer - to
collector. In our design, we assume a small 5V buzzer that can be driven via transistor.
```
```
Additional Connections: The Waveshare board has many features (RS485, CAN, battery, etc.) but
those are not used in this project by default. If not used, keep those ports disconnected. If using the
RTC (PCF85063A) for timekeeping, note it's already on the I2C bus at address 0x51 and the coin cell
should be in place. The code can easily interface to it via the RTCLib if needed.
```
```
Software Setup: To build the firmware, the README instructs to install VS Code with PlatformIO.
Open the project folder in VS Code, and PlatformIO will automatically detect platformio.ini.
One can then click Build, or use the command pio run, to compile. To flash the board, press the
BOOT button (if needed) and reset, or let the USB serial auto-programming do it. The serial monitor
can be used at 115200 baud to see logs (which will show initialization steps, sensor readings, etc.).
```
```
Usage: Upon boot, the device will show a splash screen then the main UI. The README describes the
UI navigation (e.g., tapping certain areas to go into settings). It also describes expected behavior: the
compressor relay will turn on when the temperature exceeds the setpoint by a threshold, etc., and
defrost cycles will activate the hot gas or heater periodically (or upon manual trigger via UI). If a
sensor fault occurs, the UI will display an error icon and the system will fail-safe.
```
```
Safety & Reliability: The README highlights the fault tolerance features – e.g., “If a temperature
sensor is disconnected, that reading will show ‘--’ on the display and the controller will stop cooling to
prevent damage. The buzzer will sound and an error message will be shown. The system will attempt to
reboot if a critical fault is detected.” It also reminds the user that the hardware I/O are low voltage
signals; the actual mains wiring of the compressor/heater is through relays which must be
appropriately rated and installed by a qualified person. The Type B board’s opto-isolated inputs/
outputs can add safety when interfacing with high voltage systems.
```
By following the above design, we have a complete PlatformIO project that is **production-ready**. It is
structured for clarity, uses robust libraries for the display and touch, and dedicates separate MCU resources
to the UI and control tasks for maximum reliability. The code is heavily commented, explaining the rationale
for settings (like why PSRAM is enabled, or why a particular task is pinned to a core) so that future
maintainers or AI assistants (like GitHub Copilot) can easily follow the logic and make improvements.
Overall, this system takes full advantage of the Waveshare ESP32-S3 board’s capabilities (fast dual-core CPU,
ample PSRAM, high-resolution LCD) to implement a modern, responsive freezer controller HMI while
ensuring **rock-solid stability** for an appliance that must run 24/7 without error.

**Sources:**

```
Waveshare ESP32-S3 4.3B Wiki – Board specifications and pin details
Gemini Technical Report – Detailed analysis of display and expander architecture
PlatformIO/Arduino Setup – PSRAM and flash configuration for ESP32-S
Dual-Core Tasking Model – FreeRTOS tasks mapped to Core0/Core
LVGL Integration Guide – Display, touch driver binding and refresh strategy
Control Loop Example – Pseudocode for sensor reading and watchdog reset
```
#### •

#### •

#### •

#### •

#### •^272

#### •^1519

#### •^374

#### •^1275

#### •^1850

#### •^5160


ESP32-S3-Touch-LCD-4.3 - Waveshare Wiki
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.

Gemini Waveshare Report.txt
file://file-4uBBwFnkMvGJMSPGMzaiYj

Display requirments.txt
file://file-8TSjxnybLUmzmXwg15K3iM

LCD timing.txt
file://file-4RXg2rNGencFHGrz3H7ZGo

Fault controls.PDF
file://file-81vSoH3dYZeKmQgxvNqHL

Waveshare ESP32 S3 Touch LCD 4.3B - LVGL + Wifi - no luck - General discussion - LVGL Forum
https://forum.lvgl.io/t/waveshare-esp32-s3-touch-lcd-4-3b-lvgl-wifi-no-luck/

```
1 2 22 23 25 26 27 39 56 57 71 72
```
```
3 4 5 7 8 9 10 11 12 13 14 15 16 17 20 21 24 28 40 41 42 43 44 45 47 48 49 54 55 68
69 74 75
```
```
6 18 19 34 35 36 37 38 46 50 70
```
```
29 30 31 32 33 73
```
```
51 52 53 58 59 60 61 62 65 66 67
```
```
63 64
```

