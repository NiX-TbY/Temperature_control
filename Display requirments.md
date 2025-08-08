# Technical Implementation Guide:

# Commercial Freezer HMI on the

# ESP32-S3-Touch-LCD-4.3B with LVGL

## Section 1: System Foundation and Environment Setup

This section establishes the foundational knowledge of the hardware and software stack
required to implement the specified Human-Machine Interface (HMI). It provides a detailed
analysis of the target hardware, the Waveshare ESP32-S3-Touch-LCD-4.3B, maps its physical
interfaces to the necessary software drivers, and details the configuration of a high-performance
development environment. Addressing these critical, non-obvious dependencies upfront is
essential for a successful and efficient development cycle.

### 1.1 Hardware Platform Analysis: The ESP32-S3-Touch-LCD-4.3B

The selected hardware platform is the Waveshare ESP32-S3-Touch-LCD-4.3B, a development
board specifically engineered for HMI applications. A thorough understanding of its key
components is paramount for leveraging its full capabilities.
The core of the board is an ESP32-S3-WROOM-1-N16R8 module, which features a powerful
dual-core 240MHz Xtensa LX7 processor. This module is equipped with 16MB of Quad SPI
Flash for program storage and, critically, 8MB of Octal PSRAM. This high-capacity PSRAM is a
vital resource for managing the graphical assets and frame buffers required by a high-resolution
display, though its performance characteristics relative to internal SRAM will heavily influence
the memory allocation strategy for the application.
The display itself is a 4.3-inch In-Plane Switching (IPS) panel with a resolution of 800x480 pixels
and a 16-bit "65K" color depth. It provides excellent viewing angles of up to 160 degrees, a
feature beneficial for industrial environments where the operator may not be directly in front of
the screen. The display connects to the ESP32-S3 via a parallel RGB interface, which allows for
high-speed data transfer necessary for achieving smooth animations and fast refresh rates at
this resolution.
User input is handled by a 5-point capacitive touch panel overlaid on the display. This panel
communicates with the ESP32-S3 processor over an I2C bus and supports interrupts for
responsive touch detection. The use of a shared I2C bus necessitates careful management of
device addresses, especially when integrating other I2C peripherals.
The selection of the "Type B" variant of this board is particularly well-suited for the commercial
freezer application outlined in the HMI design document. Unlike the standard model, the Type B
board includes features designed for industrial and commercial deployment, such as a
wide-range 7-36V DC power input, optically isolated digital I/O, and onboard CAN and RS
transceivers. This hardware choice directly validates and supports the HMI design's stringent
focus on safety, reliability, and robust operation in challenging commercial environments. The
physical hardware and the software's design philosophy are, therefore, perfectly aligned from
the project's inception.


### 1.2 Definitive Pinout and Interface Mapping

To prevent errors during development, it is crucial to establish a single, verified source of truth
for the board's pinout, consolidating data from product pages, wikis, and schematics.
● **RGB Display Interface:** The parallel RGB interface is the highest-bandwidth connection
on the board. Based on an analysis of forum discussions and board schematics, the
primary control signals are mapped to specific GPIOs. These pins are dedicated to the
esp_lcd peripheral within the ESP32-S3.
● **I2C Touch Interface:** The documentation consistently identifies the primary I2C bus
(I2C0) for the touch controller and the external I2C header. This bus utilizes GPIO8 for
SDA (Data) and GPIO9 for SCL (Clock).
● **I/O Expander Interface:** The board uses a CH422G I/O expander chip to manage signals
that could not be directly mapped to the ESP32-S3 due to the large number of pins
consumed by the RGB display. This expander is also an I2C device, residing on the same
bus as the touch controller. The wiki explicitly warns that the CH422G and touch panel
use specific, fixed I2C slave addresses, and developers must avoid connecting any other
I2C devices with conflicting addresses. This is a critical constraint for any hardware
expansion.
The following table provides a consolidated and verified pinout for all critical hardware
interfaces.

#### Table 1: Verified Hardware Interface Pinout for ESP32-S3-Touch-LCD-4.3B

```
Interface Signal ESP32-S3 GPIO On-Board
Connector
```
```
Notes
```
```
RGB Display PCLK GPIO7 40-pin FPC Pixel Clock for the
parallel RGB bus.
DE GPIO5 40-pin FPC Data Enable
signal.
VSYNC GPIO3 40-pin FPC Vertical
Synchronization
signal.
HSYNC GPIO46 40-pin FPC Horizontal
Synchronization
signal.
DATA[0..15] GPIO4, 45, 48, 47,
21, 14, 13, 12, 11,
10, 9, 8, 6, 1, 2, 42
```
```
40-pin FPC 16 parallel data
lines for RGB
color.
I2C Bus 0 SDA GPIO8 I2C Terminal /
Internal
```
```
Shared bus for
Touch, I/O
Expander, and
external port.
SCL GPIO9 I2C Terminal /
Internal
```
```
Shared bus for
Touch, I/O
Expander, and
external port.
Touch Panel INT Via CH422G 6-pin FPC Touch interrupt
```

```
Interface Signal ESP32-S3 GPIO On-Board
Connector
```
```
Notes
```
```
signal, managed
by the I/O
expander.
RST Via CH422G 6-pin FPC Touch reset signal,
managed by the
I/O expander.
USB-UART TX GPIO43 USB-C (UART) For programming
and serial logging.
RX GPIO44 USB-C (UART) For programming
and serial logging.
Power 5V / 7-36V N/A USB-C / DC
Terminal
```
```
Board can be
powered via USB
or external DC
input.
```
### 1.3 Development Environment Configuration (PlatformIO)

For professional embedded development, PlatformIO provides a superior alternative to the
standard Arduino IDE, offering better dependency management and build configuration control.
The following platformio.ini configuration is optimized for the ESP32-S3-Touch-LCD-4.3B.
[env:waveshare_esp32_s3_touch_lcd_4_3b]
platform = espressif
board = esp32s3box
framework = arduino
monitor_speed = 115200

; --- Board Build Flags ---
; Enable Octal PSRAM and set Flash speed/mode for performance
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L
board_build.flash_mode = qio

; --- Global Build Flags for Libraries ---
build_flags =
-D BOARD_HAS_PSRAM
-D LV_CONF_INCLUDE_SIMPLE
-D LV_TICK_PERIOD_MS=

; --- Library Dependencies ---
lib_deps =
lvgl/lvgl@~8.3.
espressif/ESP32_Display_Panel
espressif/ESP32_IO_Expander

This configuration sets several critical parameters. The board = esp32s3box profile serves as a


robust starting point for ESP32-S3 devices with Octal PSRAM and RGB displays. The
board_build.arduino.memory_type = qio_opi directive is essential for enabling the 8MB of Octal
PSRAM. Build flags explicitly enable PSRAM awareness in the code and set a 5ms LVGL tick
period for improved UI responsiveness. The LVGL library version is pinned to 8.3.x, as
community reports indicate potential compilation issues with newer versions on this specific
hardware and its associated drivers.

### 1.4 Core Library Architecture: The Mandatory Driver Stack

An analysis of the hardware and available software reveals a mandatory three-layer driver
architecture. A developer attempting to use a generic SPI-based graphics library like TFT_eSPI
or LovyanGFX will encounter immediate failure. This is because the board's display utilizes a
parallel RGB interface with an ST7262 controller, a combination not supported by these
common libraries. The correct and officially supported solution is the ESP_Display_Panel library
from Espressif, which is specifically designed for the ESP32-S3's LCD peripheral and various
display controllers, including the ST7262.
Furthermore, the board's design offloads critical functions, such as backlight control and SD
card chip select, to the CH422G I/O expander chip. This makes the ESP32_IO_Expander library
an indispensable low-level dependency. Without it, the ESP_Display_Panel library cannot
control the backlight, and the screen will remain dark.
Consequently, the only viable software stack is a three-tiered system:

1. **Top Layer (Application):** LVGL is used for creating all UI widgets, handling styling, and
    managing application-level logic.
2. **Mid Layer (Hardware Abstraction):** ESP_Display_Panel serves as the crucial bridge. It
    abstracts the complexities of the RGB bus, the ST7262 LCD controller, and the I2C touch
    panel, providing simple flush() and read() functions for LVGL.
3. **Low Layer (Dependency):** ESP32_IO_Expander provides the necessary functions to
    communicate with the CH422G chip. The ESP_Display_Panel library depends on this
    layer to perform essential operations like enabling the display's backlight.
Failure to understand and implement this specific hierarchical architecture will result in a
non-functional display and significant development delays.

## Section 2: Driver Initialization and LVGL Porting

This section provides the code-level blueprint for initializing the hardware subsystems and
integrating them with the LVGL graphics library. The strategy outlined here emphasizes a
robust, self-documenting configuration method that ensures portability and clarity, minimizing
reliance on pre-defined library configurations that may not perfectly match the target hardware.

### 2.1 Initializing the Display and Touch Subsystems

The ESP_Display_Panel library offers two configuration methods: selecting a pre-defined board
profile or defining a custom board by setting specific hardware macros. While a "Waveshare"
profile may exist, it might not perfectly match the 4.3B variant's unique pinout or I/O expander
configuration. Therefore, the most reliable and self-documenting method is to treat the board as
a custom configuration. This approach makes the code independent of future library updates to
pre-defined profiles and serves as clear, centralized documentation of the hardware interface


within the project itself.
This is achieved by creating a global ESP_Panel_Conf.h file in the Arduino libraries directory
(one level above the ESP32_Display_Panel library folder) and setting #define
ESP_PANEL_USE_SUPPORTED_BOARD (0). Within this file, all hardware parameters are
explicitly defined.
The initialization sequence in the main application code then follows a clear, logical progression:

1. **Instantiate Core Objects:** Create global pointers for the ESP_Panel object and the
    CH422G I/O expander object.
2. **Initialize I2C:** Begin the I2C communication on the correct pins (GPIO8, GPIO9) as this
    bus is shared by the touch controller and the I/O expander.
3. **Initialize I/O Expander:** Create an instance of the CH422G class from the
    ESP32_IO_Expander library, passing the I2C bus handle and the device's I2C address.
4. **Initialize Panel:** Create an instance of the ESP_Panel class.
5. **Configure and Start Panel:** Call panel->init() followed by panel->begin(). The init()
    function configures the panel using the settings from ESP_Panel_Conf.h. Critically, it also
    takes the handle to the previously initialized I/O expander object, allowing it to control the
    backlight. The begin() function completes the initialization and turns on the display.

### 2.2 LVGL Core Integration

With the hardware drivers initialized, the next step is to create the "glue" layer that connects
them to LVGL. This involves configuring LVGL's display and input driver structures to use the
functions provided by the ESP_Panel object.
First, the LVGL library itself is initialized with a call to lv_init(). Next, two draw buffers must be
allocated. The memory for these buffers is of critical importance for performance and will be
discussed in Section 5.
The **display driver binding** is accomplished by initializing an lv_disp_drv_t structure. The most
crucial field in this structure is the flush_cb (flush callback). This function pointer must be set to
the flush method provided by the ESP_Panel object's LCD interface. This can be retrieved with
panel->getLcd()->flush(). When LVGL has finished rendering a portion of the screen into a draw
buffer, it calls this function to transfer the pixel data to the physical display.
Similarly, the **input driver binding** is handled by initializing an lv_indev_drv_t structure for a
pointer-type device. Its read_cb (read callback) function pointer is set to the read method of the
ESP_Panel object's touch interface, retrieved via panel->getTouch()->read(). LVGL periodically
calls this function to poll the touch controller for new touch coordinates and state (pressed or
released).

### 2.3 Asset Management: Fonts and Symbols without Image Files

The HMI design document specifies the use of custom fonts and icons, and the user query
emphasizes creating these without relying on external image files like BMP or PNG. Storing
graphical assets as images consumes significant flash memory and adds runtime overhead for
decoding and rendering. LVGL's integrated font system provides a vastly more efficient and
professional solution.
This approach treats icons as characters within a custom font file. The process is
straightforward:

1. **Acquire Font Assets:** Download the TrueType Font (.ttf) or OpenType Font (.otf) files for
    the required typefaces. For this project, this includes the "Digit Tech" font for the main


```
temperature display and the "Font Awesome" font, which contains a comprehensive
library of icons, including the required up and down arrows (caret-up, caret-down).
```
2. **Generate Fonts with LVGL Converter:** Use LVGL's official online font converter tool.
    ○ **Main Display Font:** First, upload the "Digit Tech" font file. Set the desired height
       (e.g., 120 pixels) and a bits-per-pixel (BPP) value of 4 for anti-aliasing. The tool will
       generate a C source file (.c) containing the font data.
    ○ **UI Symbol Font:** Next, upload the Font Awesome font file. Set a suitable height for
       the icons (e.g., 48 pixels) and BPP. In the "range" input field, enter the Unicode
       values for the specific icons needed, separated by commas. For example, the
       caret-up arrow is 0xf077 and caret-down is 0xf078. This ensures that only the data
       for the required glyphs are included in the output file, minimizing its size.
3. **Integrate Custom Defrost Symbol:** The unique defrost symbol (snowflake with a water
    droplet) must be created as a vector graphic (SVG) using a tool like Inkscape. This SVG
    can then be imported into a font editor like FontForge and exported as a single-character
    TTF file. This new font file is then processed through the LVGL converter just like the
    others.
4. **Incorporate into Project:** Add the generated C files to the PlatformIO project's src or lib
    directory so they are compiled and linked into the final firmware.
5. **Use in Code:** In the application code, declare the fonts using the LV_FONT_DECLARE()
    macro. They can then be applied to any label or button text using LVGL's style system.
    The icons are referenced using their predefined LVGL symbol names (e.g.,
    LV_SYMBOL_UP, LV_SYMBOL_DOWN) or custom defines.
This method is superior to image-based assets because it is highly memory-efficient and allows
icons to be scaled, colored, and styled using the same mechanisms as regular text, providing
maximum flexibility and performance.

## Section 3: HMI Construction with LVGL

This section details the process of translating the static visual design specified in the HMI
document into a dynamic and interactive interface using LVGL's object and styling systems. It
covers the mapping of UI components to LVGL widgets, the application of precise visual styles,
and the implementation of the specified animations for visual feedback.

### 3.1 Translating the HMI Specification to LVGL Objects

The foundation of the GUI is a hierarchy of LVGL objects parented to the main screen, which is
accessed via lv_scr_act(). Each visual element from the HMI specification is mapped to a
specific LVGL widget type, creating a structured and manageable UI tree.
● **Backgrounds and Displays:** The main display areas (DISP_ACTUAL and the left control
column) are created using basic lv_obj widgets, which serve as colored panels. The
actual and setpoint temperature values (DISP_ACTUAL, DISP_SET) are implemented as
lv_label objects placed on top of these panels.
● **Buttons:** The interactive elements (BTN_UP, BTN_DOWN, BTN_DEFROST) are lv_btn
objects. Each button will contain a child lv_label object to display its respective icon (e.g.,
the up arrow symbol).
● **Alarm Interface:** The ALARM_ZONE is an initially hidden container object (lv_obj). It will
parent the "SILENCE" lv_label and two lv_line objects used to draw the stylized horizontal


lines above and below the text, as specified in the design document.
This hierarchical structure is crucial for managing layout and visibility. For example, hiding the
ALARM_ZONE container will automatically hide all of its children (the text and lines).

### 3.2 Styling and Theming with Precision

LVGL's style system is used to achieve the exact visual appearance detailed in the HMI
specification's color palette and component table. Instead of applying properties to each object
individually, reusable lv_style_t objects are created. This approach promotes consistency and
simplifies future design changes.
Separate styles will be defined for each distinct component type:
● style_up_down_btn: Sets the deep blue background (#003366), black text/icon color
(#000000), and zero radius for sharp corners.
● style_defrost_btn: Sets the light blue background (#ADD8E6) and white icon color
(#FFFFFF).
● style_actual_temp_label: Sets the large "Digit Tech" font and white text color (#FFFFFF).
● style_setpoint_label: Sets the blue text color (#00AEEF) and the appropriate font.
● style_alarm_text: Sets the red text color (#FF0000).
These styles are initialized once and then applied to the corresponding LVGL objects using
lv_obj_add_style(object, &style, 0). For interactive feedback, such as a button brightening when
pressed, a separate style for the pressed state (LV_STATE_PRESSED) can be created and
added to the object.

### 3.3 Implementing Dynamic Visual Feedback and Animations

The HMI specification requires "slow, rhythmic pulsing" effects for the active defrost button and
the active alarm state. A simple on/off blinking effect implemented with a timer would appear
jarring and unprofessional. The specified "pro-sumer" aesthetic demands a smooth, continuous
transition, which is the exact purpose of LVGL's powerful animation engine, lv_anim.
The most flexible method for creating this custom pulse is to animate a style property. A generic
animation can be configured to drive the effect for both the defrost button and the alarm text.

1. **Define Animation Template:** An lv_anim_t structure is initialized.
2. **Set Animation Parameters:**
    ○ The animation is configured to run from a start value of 0 to an end value of 255.
    ○ The duration is set to a relatively slow value, such as 1500 ms, to create a gentle
       pulse.
    ○ lv_anim_set_playback_time() is set to the same duration, causing the animation to
       automatically reverse.
    ○ lv_anim_set_repeat_count() is set to LV_ANIM_REPEAT_INFINITE to make the
       pulse continuous.
    ○ The animation path is set to lv_anim_path_ease_in_out to create a natural
       acceleration and deceleration at the ends of the pulse.
3. **Implement a Custom Executor Callback:** The animation's exec_cb is set to a custom
    function. This callback receives the animated object and the current animation value (from
    0 to 255).
4. **Apply the Effect:** Inside the callback, the incoming value is used as an alpha level to mix
    the object's base color with a highlight color using lv_color_mix().
       ○ For the **defrost button** , its base light blue (#ADD8E6) is mixed with white


##### (#FFFFFF).

```
○ For the alarm text , its base red (#FF0000) is mixed with a slightly brighter red or
white to create a "throbbing" effect.
```
5. **Set the New Color:** The resulting mixed color is applied directly to the object's local style
    property for background color or text color.
This technique produces a high-quality, visually appealing effect that is highly configurable and
efficient, directly fulfilling the nuanced requirements of the HMI design.

#### Table 2: HMI Element to LVGL Implementation Map

```
Element ID Description LVGL Widget(s) Key Style
Properties
```
```
Associated
Font/Symbol
DISP_SET Commanded Temp
Display
```
```
lv_obj
(background),
lv_label
```
```
bg_color:
#000000,
text_color:
#00AEEF
```
```
font_digit_tech_me
dium
```
```
BTN_UP Temp Increase
Button
```
```
lv_btn, lv_label bg_color:
#003366,
text_color:
#000000, radius: 0
```
```
font_awesome_sy
mbols
(LV_SYMBOL_UP
)
BTN_DOWN Temp Decrease
Button
```
```
lv_btn, lv_label bg_color:
#003366,
text_color:
#000000, radius: 0
```
```
font_awesome_sy
mbols
(LV_SYMBOL_DO
WN)
BTN_DEFROST Manual Defrost
Button
```
```
lv_btn, lv_label bg_color:
#ADD8E6,
text_color:
#FFFFFF, radius:
0
```
```
font_custom_defro
st
(DEFROST_SYM
BOL)
```
```
DISP_ACTUAL Actual Temp
Display
```
```
lv_obj
(background),
lv_label
```
```
bg_color:
#000000,
text_color:
#FFFFFF
```
```
font_digit_tech_lar
ge
```
```
ALARM_ZONE Alarm Interaction
Area
```
```
lv_obj (container),
lv_label, lv_line
(x2)
```
```
bg_color:
#000000,
text_color:
#FF0000,
line_color:
#FF
```
```
font_roboto_ui
```
## Section 4: Implementing Advanced Interaction Logic

This section details the implementation of the stateful, event-driven logic that defines the core
user experience. It focuses on translating the HMI specification's requirements for button
interactions—specifically the distinction between a single tap and a continuous hold—into robust
and maintainable code.


### 4.1 Event-Driven Control for User Input

The LVGL event system is the cornerstone of the application's interactivity. A centralized event
callback function, static void main_event_handler(lv_event_t * e), will be registered to handle
inputs from all interactive UI elements. This approach centralizes control logic, making the code
easier to debug and manage.
When registering the callback using lv_obj_add_event_cb(), the user_data parameter will be
leveraged to pass a pointer to a global structure containing handles to all major UI elements.
This gives the event handler immediate access to any object it needs to modify, without relying
on global variables. Inside the handler, lv_event_get_code(e) determines the type of event that
occurred (e.g., click, press, release), and lv_event_get_target(e) identifies which object
triggered it.

### 4.2 State Management for Continuous Operations (Tap vs. Hold)

A key functional requirement is that a brief tap on the temperature adjustment buttons changes
the setpoint by a single increment, while pressing and holding the button causes the value to
change continuously. While LVGL provides a LV_EVENT_LONG_PRESSED_REPEAT event, its
repeat rate is a global setting for the input device, which lacks the flexibility needed for a
polished interface.
A superior, more professional implementation uses a hybrid approach combining LVGL events
with a dedicated LVGL timer (lv_timer_t). This decouples the repeat logic from the input driver
and provides per-button control over repeat rate and even acceleration.

1. **Tap Logic:** The handler for the LV_EVENT_CLICKED event is kept simple. When this
    event is received from BTN_UP or BTN_DOWN, the code performs a single increment or
    decrement of the setpoint variable and updates the text of the DISP_SET label.
2. **Hold Logic:** This is managed through a state machine controlled by press and release
    events.
       ○ A global lv_timer_t * variable, adjustment_timer, is created and initialized to NULL.
       ○ When the event handler receives an LV_EVENT_PRESSED event from an
          adjustment button, it creates a new lv_timer. This timer is configured to call a
          specific callback function, adjustment_timer_cb, periodically (e.g., every 200 ms). A
          static variable is used to store the direction of adjustment (up or down).
       ○ The adjustment_timer_cb function contains the logic to increment or decrement the
          setpoint value. This function can also implement acceleration by modifying its own
          repeat period after a certain number of ticks.
       ○ When the event handler receives an LV_EVENT_RELEASED or
          LV_EVENT_PRESS_LOST event, it checks if adjustment_timer is active (not
          NULL). If it is, the timer is deleted using lv_timer_del(), and the pointer is reset to
          NULL. This immediately stops the continuous adjustment.
This hybrid model cleanly separates the single-tap and continuous-hold functionalities, prevents
the CLICKED event from firing after a long press is released, and creates robust, decoupled
code that is easy to tune and maintain.

### 4.3 Alarm System Logic

The alarm system is a state machine as described in the HMI document. Its state transitions will


be managed by a central function, void update_alarm_state(alarm_state_t new_state).
● **State 1: Normal:** All alarm indicators are hidden. The DISP_ACTUAL text is white.
● **State 2: Alarm Active:** When the system logic detects an out-of-bounds temperature, it
calls update_alarm_state(ALARM_ACTIVE). This function will:
○ Change the DISP_ACTUAL label's text color to red (#FF0000).
○ Start the pulsing animation on the DISP_ACTUAL label.
○ Make the ALARM_ZONE container object visible.
○ Activate the physical audible buzzer via a GPIO pin.
● **State 3: Alarm Silenced:** The ALARM_ZONE object (containing the "SILENCE" button)
is registered to listen for the LV_EVENT_CLICKED event. When tapped, its handler calls
update_alarm_state(ALARM_SILENCED). This function will:
○ Deactivate the audible buzzer.
○ Stop the pulsing animation on the DISP_ACTUAL label (using lv_anim_del()), but
leave its text color red to indicate the underlying fault condition persists.
○ Hide the "SILENCE" text and lines.
○ Create and display a countdown timer label within the ALARM_ZONE.
○ Create a new lv_timer that decrements the countdown label every second. If this
timer reaches zero before the alarm condition is resolved, it will call
update_alarm_state(ALARM_ACTIVE) again, re-sounding the buzzer.
● **Return to Normal:** If the system detects the temperature has returned to the normal
range, it calls update_alarm_state(ALARM_NORMAL), which resets all UI elements to
their default state and deletes any active alarm timers.

## Section 5: Performance Optimization and Finalization

This section provides a checklist of critical configurations to ensure the application is fluid,
responsive, and professionally executed. Achieving high performance on an embedded system
with a high-resolution display requires careful management of memory, CPU resources, and
rendering pipelines.

### 5.1 Strategies for Maximizing Display Fluidity (FPS)

The central optimization challenge for this hardware is the trade-off between memory usage and
rendering performance. The 800x480 pixel display requires 768 KB for a full-screen, 16-bpp
(RGB565) frame buffer (800 * 480 * 2 bytes). The ESP32-S3's fast internal SRAM is limited to
512 KB, making it impossible to store a full frame buffer there. While the board has 8MB of
external PSRAM, accessing it is significantly slower than internal SRAM and would severely
degrade rendering performance, leading to low frame rates and a sluggish user experience.
Consequently, the only viable high-performance strategy is to use smaller, partial-refresh buffers
that are allocated exclusively in the fast, internal SRAM. The following checklist outlines the
essential steps to achieve this and maximize performance:

1. **Buffer Configuration:** In lv_conf.h, enable double buffering
    (LV_USE_GPU_STM32_DMA2D_DOUBLE_BUFFER 1 or equivalent setting for full-page
    buffering if not using a dedicated GPU setting). Create two draw buffers. A size of at least
    1/10th of the screen height is recommended for good performance. A size of
    LV_HOR_RES_MAX * 80 (1/6th of the screen height) is an excellent choice, balancing
    memory usage and rendering efficiency. This results in two buffers of 800 * 80 * 2 = 128


```
KB each, for a total of 256 KB, which fits comfortably within the internal SRAM.
```
2. **Memory Allocation:** It is imperative that these draw buffers are allocated in the correct
    memory region. Use the ESP-IDF function heap_caps_malloc(buffer_size,
    MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA). The MALLOC_CAP_INTERNAL flag
    ensures allocation in the fast internal SRAM, and MALLOC_CAP_DMA ensures the
    memory is accessible by the LCD peripheral's DMA controller for high-speed transfers.
3. **Compiler and CPU Settings:** In the project's build configuration (platformio.ini or
    menuconfig), set the Compiler Optimization Level to "Performance" (-O2 or -O3) and the
    CPU Frequency to the maximum of 240MHz. These settings ensure the code itself
    executes as quickly as possible.
4. **Task Affinity (Dual-Core Optimization):** The ESP32-S3's dual-core architecture should
    be exploited to prevent the UI from stuttering. Use the FreeRTOS API
    (xTaskCreatePinnedToCore) to pin the LVGL task handler (lv_timer_handler in the main
    loop) to one core (e.g., Core 1). All other application logic, such as sensor reading,
    network communication, or control algorithms, should be pinned to the other core (e.g.,
    Core 0). This isolation guarantees that the UI rendering loop has dedicated CPU
    resources and remains responsive even when the system is busy.
5. **LVGL Library Settings:** In lv_conf.h or menuconfig, ensure that
    CONFIG_LV_MEMCPY_MEMSET_STD is enabled. This directs LVGL to use the
    ESP-IDF's highly optimized memcpy and memset functions instead of its own generic
    implementations, yielding a small but measurable performance increase.

### 5.2 Ensuring Instantaneous Touch Responsiveness

A responsive touch interface is critical for user satisfaction. The ESP_Display_Panel library
handles the underlying I2C communication with the touch controller. To ensure low latency, two
key areas must be addressed:

1. **LVGL Polling Frequency:** The lv_timer_handler() function is responsible for calling the
    input device's read_cb. The frequency of this call is determined by the
    LV_TICK_PERIOD_MS setting in lv_conf.h. To minimize perceived lag, this value should
    be set to a low number, such as 5 or even 2 milliseconds. This increases the rate at which
    LVGL polls for touch input.
2. **Hardware Interrupts:** For the lowest possible latency, the touch controller's hardware
    interrupt pin should be utilized if the driver supports it. This allows a physical touch to
    trigger an immediate read of the I2C data, rather than waiting for the next polling cycle.
    The ESP_Display_Panel library and its underlying drivers should be configured to use the
    touch interrupt pin, which is connected to the ESP32-S3 via the CH422G I/O expander.
By implementing these software and hardware strategies, the HMI will feel immediate and fluid,
meeting the expectations for a modern touch-based interface.

#### Table 3: Performance Optimization Checklist

```
Parameter Recommended Setting Location Rationale
Draw Buffer Size LV_HOR_RES_MAX *
80
```
```
Application Code Balances large buffer
size for efficient
rendering with limited
internal SRAM.
```

```
Parameter Recommended Setting Location Rationale
Buffer Location Internal DMA-capable
RAM
```
```
Application Code
(heap_caps_malloc)
```
```
Maximizes memory
access speed for
rendering; PSRAM is
too slow for draw
buffers.
Double Buffering Enabled lv_conf.h Allows the CPU to
render to one buffer
while the DMA
transfers the other,
preventing screen
tearing.
CPU Frequency 240 MHz platformio.ini /
menuconfig
```
```
Maximizes the
processing speed for
LVGL's rendering
calculations.
Compiler
Optimization
```
```
Performance (-O2) platformio.ini /
menuconfig
```
```
Generates
faster-executing
machine code at the
cost of a larger binary
size.
Task Affinity UI on Core 1, App on
Core 0
```
```
Application Code
(xTaskCreatePinnedTo
Core)
```
```
Isolates the UI thread
from other system
tasks, preventing
stutter and lag.
LVGL Tick Period 5 ms or less lv_conf.h Increases the polling
frequency for touch
input and the refresh
rate of animations.
```
## Section 6: Consolidated Implementation Blueprint

## (Code Skeletons)

This final section consolidates the strategies and techniques from the preceding analysis into a
series of well-commented C++ code skeletons. These blueprints provide a direct,
copy-adaptable foundation for the final application, significantly reducing development time and
the potential for implementation errors.

### main.cpp Structure

#include <Arduino.h>
#include <lvgl.h>
#include <ESP_Panel_Library.h>
#include "ui_assets.h" // Assumed header for custom fonts

// --- Global Object Handles ---
struct UIHandles {


lv_obj_t* disp_actual_label;
lv_obj_t* disp_set_label;
lv_obj_t* btn_up;
lv_obj_t* btn_down;
lv_obj_t* btn_defrost;
lv_obj_t* alarm_zone;
lv_obj_t* alarm_silence_label;
lv_obj_t* alarm_countdown_label;
};
UIHandles ui;

// --- Global State Variables ---
float actual_temp = -18.2f;
float setpoint_temp = -18.0f;
lv_timer_t* adjustment_timer = NULL;

// --- Function Prototypes ---
void setup_drivers_and_lvgl();
void create_gui_layout();
void register_event_handlers();
void main_event_handler(lv_event_t* e);
void adjustment_timer_cb(lv_timer_t* timer);
void app_logic_task(void* pvParameters);

// --- Main Setup and Loop ---
void setup() {
Serial.begin(115200);

setup_drivers_and_lvgl();
create_gui_layout();
register_event_handlers();

// Create a dedicated task for non-GUI logic on Core 0
xTaskCreatePinnedToCore(
app_logic_task, // Task function
"AppLogic", // Task name
4096, // Stack size
NULL, // Task parameters
1, // Priority
NULL, // Task handle
0 // Core ID
);
}

void loop() {
// LVGL's task handler should run continuously in the main loop
(pinned to Core 1 by default in Arduino)
lv_timer_handler();


delay(5); // A small delay is crucial
}

### Driver and LVGL Initialization

void setup_drivers_and_lvgl() {
// 1. Initialize LVGL
lv_init();

// 2. Initialize the display panel
// This assumes ESP_Panel_Conf.h is set up for a custom board
ESP_Panel* panel = new ESP_Panel();
panel->init();
panel->begin();

// 3. Allocate draw buffers in internal, DMA-capable memory
// Buffer size is 1/6th of the screen height for optimal
performance
size_t buf_size = ESP_PANEL_LCD_H_RES * 80;
void* buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t),
MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
void* buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t),
MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

// 4. Initialize LVGL display driver
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = ESP_PANEL_LCD_H_RES;
disp_drv.ver_res = ESP_PANEL_LCD_V_RES;
disp_drv.flush_cb = (void (*)(lv_disp_drv_t*, const lv_area_t*,
lv_color_t*))panel->getLcd()->flush;
disp_drv.draw_buf = new lv_disp_draw_buf_t();
lv_disp_draw_buf_init(disp_drv.draw_buf, buf1, buf2, buf_size);
lv_disp_drv_register(&disp_drv);

// 5. Initialize LVGL input device driver (touch)
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = (void (*)(lv_indev_drv_t*,
lv_indev_data_t*))panel->getTouch()->read;
lv_indev_drv_register(&indev_drv);
}

### GUI Layout Creation


void create_gui_layout() {
// Use the HMI document for pixel-perfect positions and sizes
lv_obj_t* screen = lv_scr_act();
lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

// --- Create Left Column Elements ---
ui.btn_up = lv_btn_create(screen);
lv_obj_set_pos(ui.btn_up, 0, 131);
lv_obj_set_size(ui.btn_up, 168, 131);
//... apply styles for background color, radius, etc....
lv_obj_t* up_arrow_label = lv_label_create(ui.btn_up);
//... apply font style with Font Awesome symbol...
lv_label_set_text(up_arrow_label, LV_SYMBOL_UP);
lv_obj_center(up_arrow_label);

//... create BTN_DOWN, BTN_DEFROST, and DISP_SET similarly...

// --- Create Right Display Elements ---
ui.disp_actual_label = lv_label_create(screen);
lv_obj_align(ui.disp_actual_label, LV_ALIGN_CENTER, (168/2), 0);
// Center in right panel
//... apply style with large "Digit Tech" font and white color...
lv_label_set_text_fmt(ui.disp_actual_label, "%.1f", actual_temp);

// --- Create Hidden Alarm Zone ---
ui.alarm_zone = lv_obj_create(screen);
lv_obj_set_pos(ui.alarm_zone, 589, 349);
lv_obj_set_size(ui.alarm_zone, 211, 131);
lv_obj_add_flag(ui.alarm_zone, LV_OBJ_FLAG_HIDDEN); // Initially
hidden
//... create "SILENCE" label and lines as children of
alarm_zone...
}

### Event Handler Registration and Logic

void register_event_handlers() {
// Pass the 'ui' struct as user data to have access to all handles
lv_obj_add_event_cb(ui.btn_up, main_event_handler, LV_EVENT_ALL,
&ui);
lv_obj_add_event_cb(ui.btn_down, main_event_handler, LV_EVENT_ALL,
&ui);
lv_obj_add_event_cb(ui.btn_defrost, main_event_handler,
LV_EVENT_ALL, &ui);
lv_obj_add_event_cb(ui.alarm_zone, main_event_handler,
LV_EVENT_ALL, &ui);
}


void main_event_handler(lv_event_t* e) {
lv_event_code_t code = lv_event_get_code(e);
lv_obj_t* target = lv_event_get_target(e);
UIHandles* p_ui = (UIHandles*)lv_event_get_user_data(e);

// --- Temperature Adjustment Logic ---
if (target == p_ui->btn_up |

| target == p_ui->btn_down) {
if (code == LV_EVENT_CLICKED) {
setpoint_temp += (target == p_ui->btn_up)? 0.1 : -0.1;
lv_label_set_text_fmt(p_ui->disp_set_label, "%.1f",
setpoint_temp);
}
else if (code == LV_EVENT_PRESSED) {
if (adjustment_timer == NULL) {
// Pass the target button to the timer's user data
adjustment_timer =
lv_timer_create(adjustment_timer_cb, 200, target);
}
}
else if (code == LV_EVENT_RELEASED |

| code == LV_EVENT_PRESS_LOST) {
if (adjustment_timer!= NULL) {
lv_timer_del(adjustment_timer);
adjustment_timer = NULL;
}
}
}
//... other event logic for defrost, alarm silence, etc....
}

void adjustment_timer_cb(lv_timer_t* timer) {
lv_obj_t* target_btn = (lv_obj_t*)timer->user_data;
UIHandles* p_ui = &ui; // Access global ui struct

setpoint_temp += (target_btn == p_ui->btn_up)? 0.1 : -0.1;
// Optional: Add logic to accelerate change by modifying timer
period
// if (lv_timer_get_run_cnt(timer) > 10) {
lv_timer_set_period(timer, 50); }
lv_label_set_text_fmt(p_ui->disp_set_label, "%.1f",
setpoint_temp);
}


### Animation and Application Logic Skeletons

// --- Animation Callback for Pulsing Effect ---
void pulse_anim_cb(void* obj, int32_t v) {
lv_obj_t* target_obj = (lv_obj_t*)obj;
// Example for defrost button
lv_color_t base_color = lv_color_hex(0xADD8E6); // Light Blue
lv_color_t pulse_color = lv_color_hex(0xFFFFFF); // White

// Use 'v' (0-255) as the mix ratio
lv_obj_set_style_bg_color(target_obj, lv_color_mix(pulse_color,
base_color, v), 0);
}

// --- Main Application Logic Task (runs on Core 0) ---
void app_logic_task(void* pvParameters) {
for (;;) {
// --- Read sensors ---
// actual_temp = read_temperature_sensor();

// --- Update UI (thread-safe using LVGL's mechanisms) ---
lv_label_set_text_fmt(ui.disp_actual_label, "%.1f",
actual_temp);

// --- Check for alarm conditions ---
// if (actual_temp > ALARM_THRESHOLD) {
// update_alarm_state(ALARM_ACTIVE);
// } else {
// update_alarm_state(ALARM_NORMAL);
// }

vTaskDelay(pdMS_TO_TICKS(1000)); // Run once per second
}
}

#### Works cited

1. ESP32-S3-Touch-LCD-4.3 - Waveshare Wiki,
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3 2. ESP32-S3-Touch-LCD-4.3B -
Waveshare Wiki, https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B 3. Waveshare
ESP32-S3 4.3in Touch LCD Display Development Board 800×480 32-bit LX7,
https://www.ebay.com/itm/256574974170 4. ESP32-S3 4.3 inch Touch LCD Development Board
Type B, 800×480, 5-point Touch, 32-bit LX7 Dual-core Processor - Waveshare,
https://www.waveshare.com/esp32-s3-touch-lcd-4.3b.htm 5. ESP32-S3-Touch-LCD-4.3B运行
PlatformIO LVGL8.4 – 走着的小站 - OpenPilot, https://www.openpilot.cc/archives/4466 6. New
Output - Waveshare,


https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B/ESP32-S3-Touch-LCD-4.3B-Sch.p
df 7. ESP32-S3-Touch-LCD-4.3B - Waveshare Wiki - hubtronics,
https://hubtronics.in/docs/28141.pdf 8. Waveshare ESP32-S3-Touch-LCD-4.3 with Squareline
Studio and PlatformIO - GitHub,
https://github.com/istvank/Waveshare-ESP32-S3-Touch-LCD-4.3 9.
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3 - How-to - LVGL Forum,
https://forum.lvgl.io/t/https-www-waveshare-com-wiki-esp32-s3-touch-lcd-4-3/14462 10.
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Software - How to - Forum -
SquareLine Studio,
https://forum.squareline.io/t/https-www-waveshare-com-wiki-esp32-s3-touch-lcd-4-3-software/
81 11. ESP32_Display_Panel - Arduino Documentation,
https://docs.arduino.cc/libraries/esp32_display_panel/ 12. Lzw655/ESP32_Display_Panel:
Arduino library for driving display panel using the ESP32,
https://github.com/Lzw655/ESP32_Display_Panel 13. ESP32_IO_Expander - Arduino
Documentation, https://docs.arduino.cc/libraries/esp32_io_expander/ 14.
esp-arduino-libs/ESP32_IO_Expander: Arduino library of driving IO expander chips for the ESP
SoCs - GitHub, https://github.com/esp-arduino-libs/ESP32_IO_Expander 15.
esp32beans/ESP32_Display_Panel_Backlight_Off: Arduino library of driving display panel for
the ESP SoCs (Backlight Off) - GitHub,
https://github.com/esp32beans/ESP32_Display_Panel_Backlight_Off 16. Font (lv_font) - LVGL
9.4 documentation, https://docs.lvgl.io/master/details/main-modules/font.html 17. Fonts — LVGL
documentation, https://docs.lvgl.io/9.0/overview/font.html 18. Animations — LVGL
documentation, https://docs.lvgl.io/8.0/overview/animation.html 19. Animation (lv_anim) - LVGL
9.4 documentation, https://docs.lvgl.io/master/details/main-modules/animation.html 20. Events
— LVGL documentation, https://docs.lvgl.io/9.1/overview/event.html 21. How to speed up
drawing with lvgl 8? · Issue #3921 - GitHub, https://github.com/lvgl/lvgl/issues/3921 22. LCD &
LVGL Performance - Waveshare,
https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7/Performance.pdf


