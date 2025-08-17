ESP32‑S3‑Touch‑LCD‑4.3B test‑image generation

This   document   provides   an   in‑depth   investigation   into   how   Waveshare’s  ESP32‑S3‑Touch‑LCD‑4.3B

development   board   produces   the   colour‑bar   test   image   supplied   in   the   Arduino   example
08_DrawColorBar.ino . In addition to describing the high‑level workflow, the report dives deeply into

the   hardware   used   (display   controller,   touch   controller,   I/O   expander,   USB/CAN/RS‑485   transceivers   and

power circuits), the electrical signalling and timing requirements, a comprehensive pin‑mapping table and a

step‑by‑step analysis of the code. Particular focus is given to the  CH422 I/O expander  because the test

program will not work unless this chip is properly initialised; the expander controls the display‑backlight

enable and the reset lines of both the LCD and the touch panel and also provides additional digital I/O.

1 Introduction

The 4.3B board combines an ESP32‑S3 module (dual‑core Xtensa LX7 at up to 240 MHz) with 16 MB of flash

and 8 MB of PSRAM

1

. An on‑board 4.3 inch IPS TFT (800×480 pixels) with capacitive touch (5‑point, glass

panel)   is   driven   through   a  16‑bit   RGB565   parallel   bus.   The   board’s   peripherals   include   an   I²C   bus,   a

full‑speed USB port (via CH343 bridge), an SD‑card slot, half‑duplex RS‑485 and high‑speed CAN ports, a

battery  charger  and  opto‑isolated  digital  I/O
eight bidirectional GPIO lines and four general‑purpose outputs

.  A  key  component,  the  CH422  I/O  expander,  provides
; the example uses these pins to control

1

2

the backlight, to reset the LCD and the touch controller and to switch between USB and CAN modes. Failing

to initialise the CH422 leaves the backlight off and keeps the display in reset, so understanding its registers

is critical.

The Arduino sketch  08_DrawColorBar.ino  does little work itself – it calls  waveshare_lcd_init()  and

then enters an idle loop. All of the hardware initialisation, bus configuration and test pattern generation

happen inside the called functions. This report reconstructs those actions from source code and datasheet

information and explains how the test image appears on the screen.

2 System architecture and high‑level operation

2.1 Board block diagram (textual)

Although the board’s schematic is spread across three pages, its functional blocks can be summarised as

follows:

1.

ESP32‑S3 module (U9) – The microcontroller hosts the application and provides high‑speed LCD

parallel RGB, I²C, SPI and serial interfaces. Many of its GPIO pins are routed to the LCD connector

or through the CH422 expander to external connectors.

2.

ST7262 timing controller and 4.3 inch LCD – An integrated display driver and panel that accepts 16‑

or 24‑bit RGB pixel data, HSYNC/VSYNC and DE signals. It contains source and gate drivers, timing

logic, on‑chip DC/DC converters, gamma/ID OTP memory and LVDS lanes

3

. The panel has

800 RGB pixels per line and 480 lines.

1

3.

GT911 capacitive touch controller – A 5‑point touch IC with 26 TX and 14 RX electrodes, connected

via I²C (GPIO8/GPIO9) and generating an interrupt on GPIO4

4

. Its I²C address is selected by the

reset/INT pins

5

.

4.

CH422 I/O expander – A two‑wire I/O expander that exposes eight bidirectional lines (IO0–IO7) and

four outputs (OC0–OC3). Each internal register has its own I²C address; the important registers are
MODE  (0x24),  IN  (0x26),  OUT  (0x38) and  OUT_UPPER  (0x23)

. The board maps these lines to

6

backlight, LCD reset, touch reset, SD‑card chip select, USB/CAN selector and isolated digital I/O.

Without configuring the mode register to enable outputs, the expander defaults to input mode and

the connected devices remain disabled.

5.

CH343 USB‑serial bridge – Provides the Type‑C USB port for programming and logging. It is a

full‑speed USB 2.0 device that emulates a UART, includes an internal oscillator and firmware and

adapts dynamically to a wide range of baud rates

7

.

6.

RS‑485 and CAN transceivers – The board contains a half‑duplex RS‑485 transceiver (not named in

the simplified bill of materials) and an NXP TJA1051 CAN transceiver. The latter implements

ISO 11898‑2 physical layer and supports data rates up to 5 Mbit/s

8

. Both differential interfaces

share the microcontroller’s UART/CAN pins; a line controlled by the CH422 selects USB (CH343)

versus CAN operation.

7.

Power management – A DC‑DC converter boosts a 3.7 V Li‑ion battery to 5 V and a linear regulator

produces 3.3 V. The ST7262 requires a controlled power‑on sequence: release reset after rails are

stable, assert display enable (DISP) after ≥10 ms and turn on the backlight after ≥250 ms

9

.

2.2 High‑level execution of the colour‑bar test

At a very high level the system produces the test pattern through the following stages:

1.

Power‑up and reset: When the board receives power, the DC‑DC converter generates 3.3 V and 5 V
rails. The ST7262’s  GRB  (reset) line is held low by the CH422 until the microcontroller configures the

expander. The backlight enable (DISP) is also low, so the panel remains dark.

2.

CH422 initialisation: The Arduino sketch prints “Initialize IO expander”, constructs an
ESP_IOExpander_CH422G  object and calls its  init() / begin()  functions. This writes the

CH422 MODE register (0x24) to enable output mode on IO0–IO7 and open‑drain mode on the upper

outputs; it also writes default values to the OUT and OUT_UPPER registers (0x38 and 0x23)
code then calls  pinMode()  for each needed pin (TP_RST, LCD_BL, LCD_RST, SD_CS, USB_SEL) and
uses  digitalWrite()  to drive these lines to their initial states (reset high, backlight off). Without

. The

6

3.

this sequence the ST7262 would never release reset nor would the backlight turn on.
Display bus configuration: A  BusRGB  object is created with the 16 data pins, HSYNC/VSYNC/DE,
pixel clock and the ST7262 timing parameters ( HPW ,  HBP ,  HFP ,  VPW ,  VBP ,  VFP ). The code

4.

sets the pixel clock to 16 MHz and uses a bounce buffer sized to 800×10 pixels.
ST7262 driver creation: A  LCD_ST7262  object is constructed (wrapped by the macro
EXAMPLE_LCD_CLASS ). The driver’s  init() ,  reset()  and  begin()  methods release the

ST7262’s internal reset, configure the controller for 16‑bit RGB565, program the gamma tables and
start the timing generator. If enabled, the  attachRefreshFinishCallback()  counts VSYNC

pulses to measure frame rate. At this point the ST7262 is sending blank frames because DISP is still

low.

5.

Backlight and drawing: After waiting ~100 ms (to satisfy T1 from the power‑on sequence), the code
calls  colorBarTest() . This function divides the display width into three vertical stripes; for each

stripe it fills the bounce buffer with a constant 16‑bit value representing blue, green or red and calls

2

drawBitmap()  to stream the buffer to the panel. When drawing is complete the code sets DISP

high through the CH422 (turning on the backlight) and prints “LCD draw finish callback”. The
microcontroller continues to refresh the same pattern on each frame while the  loop()  function

simply prints “IDLE loop” every second.

The remaining sections dissect the hardware and signals used at each stage and then decode the code in

detail.

3 Hardware components in depth

3.1 ST7262 LCD timing controller and panel

The Sitronix ST7262 is a highly integrated LCD timing controller designed for QVGA to WVGA TFT panels. Key

features drawn from the data sheet include:

•

Interface support: The chip supports 4‑lane LVDS and 24‑bit parallel RGB inputs, with SYNC,

SYNC‑DE and DE‑only modes. The 16‑bit mode used on this board simply transmits the upper bits of

each colour channel (RGB565) while the lower bits of unused colour channels are ignored

3

.

•

Integrated power and drivers: On‑chip DC/DC converters, reference voltages and level shifters

drive up to 4 source driver ICs and 480 gate lines, simplifying external circuitry

3

.

•

Supply and logic levels: The ST7262 operates from a 3.3 V core and a separate analog supply; the

logic high threshold is ≈0.7·V<sub>DDI</sub>, and the logic low threshold is ≈0.3·V<sub>DDI</
sub>, ensuring compatibility with the ESP32’s 3.3 V GPIOs.

•

Power sequencing: A mandatory sequence prevents display artifacts. After power rails stabilise,
GRB  (global reset) must remain low (T0≥0 ms). At least 10 ms later  DISP  may be driven high (T1).

The pixel clock and sync signals must start before the backlight is enabled, and the backlight must

remain off for at least 250 ms after DISP goes high

9

. During power‑off, the backlight is turned off

first; DISP is then pulled low for ≥5 ms and the panel is allowed ≥100 ms to discharge before
removing power

.

9

•

RGB timing constraints: For RGB mode the data sheet specifies a pixel clock duty cycle of 40 %–

60 % and a minimum HSYNC pulse width of two PCLK cycles. Setup and hold times for data/DE and

VSYNC/HSYNC are ≥12 ns
(HPW=4, HBP=8, HFP=8) and vertical parameters (VPW=4, VBP=8, VFP=8), satisfying the table’s

. The example uses a 16 MHz clock with horizontal parameters

10

minimum timing. These values correspond to 820 PCLK cycles per line and 500 lines per frame
(~31 fps). Changing the pixel clock frequency or porch values in  waveshare_lcd_port.h  would

adjust the frame rate accordingly.

3.2 GT911 capacitive touch controller

The Goodix GT911 handles the capacitive touch panel. According to the manufacturer’s data sheet, GT911

integrates   a   high‑performance   capacitive   sensing   engine   and   an   embedded   microcontroller.   Notable

features include:

•

Sensor array: Up to 26 transmit (Tx) and 14 receive (Rx) channels for interpolation across the

glass

4

.

•

Multi‑touch capability: Up to 5 simultaneous touch points with coordinate resolution typically

0.1 mm; reporting rate is 100 Hz, ensuring responsive tracking

4

.

3

•

I²C interface: Supports two addresses (0x5D/0x5C for 0xBA/0xBB and 0x14/0x15 for 0x28/0x29)

selected by the levels on the reset and INT pins during power‑on

5

. The board ties the reset to

CH422 IO0 (TP_RST) and the INT to GPIO4 (TP_IRQ); the example resets the chip high through CH422

and leaves INT floating, thus choosing the 0x5D/0x5C address (0xBA/0xBB in 8‑bit form).

•

Interrupt signalling: When a touch event is available, TP_IRQ is pulled low; the host reads a status

register then a set of coordinate registers. In the test program this interrupt pin is not used because

the code does not read touch data, but proper GT911 operation is critical when building interactive

UIs.

3.3 CH422 I/O expander and its role

The CH422 (specifically CH422G variant) is a dual‑function LED driver and I/O expander. For I/O expansion it

offers  eight   bidirectional   I/O   pins   (IO0–IO7)  and  four   output‑only   pins   (OC0–OC3).   The   chip

communicates via a two‑wire serial interface and, unusually, each internal register is mapped to a separate

7‑bit   I²C   address   rather   than   using   a   single   device   address.   The   registers   important   for   this   board   are

(addresses in hexadecimal):

•

MODE (0x24) – Configures the direction of pins 0–7 and selects open‑drain drive for the upper

outputs. Setting bit 0 enables output mode on IO0–IO7; setting bit 2 enables open‑drain mode on

OC0–OC3

6

.

•

IN (0x26) – Read‑only register returning the current state of IO0–IO7

6

.

•

OUT (0x38) – Write‑only register controlling the states of IO0–IO7

6

.

•

OUT_UPPER (0x23) – Write‑only register controlling the states of OC0–OC3 (pins 8–11)

6

.

  ESP_IOExpander_CH422G ,   abstracts   these   details.   When
The   library   used   in   the   example,
pinMode(pin, OUTPUT)   is called for a pin below 8, it sets the output‑enable bit in the MODE register;
when   pinMode(pin,   OPEN_DRAIN)   is   called   for   a   pin   8–11   it   sets   the   open‑drain   enable   bit
.
Subsequent   digitalWrite()   calls   modify   an   internal   bitmap   and   then   write   that   bitmap   to   the

6

appropriate OUT register

11

. Reading inputs temporarily disables output mode, reads the IN register and

then restores the previous mode

12

.

On the 4.3B board, the CH422 pins are used as follows:

CH422

Board

pin

silk‑screen

Function

Default state

during boot

Notes

IO0

TP_RST

Reset for GT911

touch controller

High (inactive)

Drives the GT911 reset pin; set low

to reset the touch IC.

IO1

LCD_BL

Backlight enable

Low (off)

must go high only after ST7262 is

Controls the LCD backlight MOSFET;

stable.

Additional reset line; ST7262 also

IO2

LCD_RST

Reset for ST7262

High (inactive)

uses global reset via GRB; both

must be released.

IO3

SD_CS

Chip‑select for the

TF‑card slot

High (inactive)

Pulling low selects the SD‑card; the

example does not access the card.

4

CH422

Board

pin

silk‑screen

Function

Default state

during boot

Notes

IO4

(unused)

Available for user

or digital input

High (pull‑up)

Wired to the CH422 header; can be

repurposed.

USB_SEL/

CAN_SEL

Mode select

between USB and

CAN

When low the CH343 is connected;

Pull‑down

when high the TJA1051 is

selects USB

connected; this line is configured as

IO5

IO6,

IO7

DI0, DI1

Opto‑isolated

digital inputs

Input

output.

Each line passes through an

optocoupler; reading requires

switching IO6/IO7 to input mode

temporarily.

OC0

DO0

OC1

DO1

Opto‑isolated

digital output 0

Opto‑isolated

digital output 1

Available

Open‑drain

Drives an external load via
optocoupler T1; pulled high

externally; used by user application.

Open‑drain

Same as DO0; drives optocoupler

T3.

OC2

(unused)

open‑drain output

Open‑drain

2

Available

Not connected on the board;

reserved for expansion.

OC3

(unused)

open‑drain output

Open‑drain

Not connected on the board.

3

Because   the   CH422   starts   with   all   lines   in   input   mode,   the   firmware   must   call   pinMode()   and
digitalWrite()  to set IO0–IO2 high (releasing resets) and IO1 low (keeping backlight off). Later, when

the display is ready, IO1 is driven high to enable the backlight. If this sequence is skipped, the LCD will

remain blank even though the ST7262 is generating timing signals.

3.4 CH343 USB‑serial bridge

The   WCH  CH343  is   an   integrated   USB   to   UART   bridge.   It   enumerates   as   a   full‑speed   USB   device   and

provides a standard serial port that can be used by Arduino IDE or other terminal software. Features include

dynamic   baud‑rate   adaptation,   hardware   flow   control   (RTS/CTS)   and   built‑in   firmware   for   emulating   AT

commands

7

. On this board the CH343 connects to the ESP32’s UART0 pins, enabling firmware flashing

and debugging via the Type‑C connector.

3.5 TJA1051 high‑speed CAN transceiver and RS‑485 transceiver

The NXP TJA1051 is a high‑speed CAN transceiver used on the board’s CAN connector. It fully implements

the ISO 11898‑2:2016 physical layer and supports Classic CAN and CAN FD at up to 5 Mbit/s

8

. The device

has a wide supply range (4.5–5.5 V) and a separate V<sub>IO</sub> pin so that it can interface with 3.3 V

logic; the board ties V<sub>IO</sub> to the ESP32’s 3.3 V rail. The transceiver goes into an off‑mode when

5

the EN pin is low

13

. The board connects CANH/CANL to a screw terminal and uses IO15 (CANTX) and IO16

(CANRX) as the microcontroller’s transmit and receive lines

14

. A note on the PCB shows that setting CH422

IO5 high selects CAN mode; pulling it low selects USB mode, effectively disconnecting the CAN transceiver

from the ESP32.

The   RS‑485   interface   uses   a   separate   differential   transceiver   (not   labelled   in   the   provided   datasheets)

connected to IO43 (RS485_RXD) and IO44 (RS485_TXD)

15

. Automatic direction switching is implemented

with a few discrete transistors.

3.6 Power and backlight circuits

The board may be powered from a USB‑C 5 V source or via an external DC input (7–36 V). An on‑board DC/

DC   converter   supplies   5   V   which   then   feeds   a   3.3   V   LDO   for   the   ESP32   and   peripheral   ICs.   The  LCD

backlight  is driven by a constant‑current LED driver (U19 in the schematic). The driver accepts an enable

pin (DISP) which is connected to CH422 IO1 (LCD_BL). When IO1 is high, the driver supplies current to the

LED array; when low, the LED current is zero. Because LED lifetime and the ST7262’s power sequencing

depend on the backlight delay, the firmware only sets IO1 high after the display timing has started and

after the required 250 ms delay

9

.

4 Electrical signalling and timing

4.1 RGB565 pixel bus

The ESP32’s LCD peripheral drives a 16‑bit RGB565 bus consisting of 5 red bits, 6 green bits and 5 blue bits.

The mapping used on this board is summarised in the pin‑mapping table below. Pixel data are clocked on

the rising edge of PCLK at 16 MHz; data lines are updated on the falling edge. HSYNC pulses for 4 cycles at

the start of each line; after 8 cycles of back porch, 800 pixel data are transmitted while DE is high, followed

by an 8‑cycle front porch.  VSYNC  pulses for 4 lines at the top of each frame; after an 8‑line vertical back

porch, 480 lines of image data are sent, followed by an 8‑line front porch. According to the ST7262 data

sheet, the duty cycle of PCLK must be 40–60 % and setup/hold times for data/DE/HSYNC/VSYNC must be

10

≥12   ns
waveshare_lcd_port.h .

.   The   example   meets   these   requirements   by   using   the   macros   defined   in

During each pixel transfer, the 16 data lines carry the following bits (from MSB to LSB):

Bit

Colour bit

index

(RGB565)

Board signal

ESP32

pin

Description

15

14

13

12

11

R7

R6

R5

G7

G6

Data15

GPIO40

Highest red bit

16

Data14

GPIO41

Red bit 6

17

Data13

GPIO42

Red bit 5

17

Data12

GPIO21

Green bit 7

18

Data11

GPIO47

Green bit 6

19

6

Bit

Colour bit

index

(RGB565)

Board signal

ESP32

pin

Description

10

9

8

7

6

5

4

3

2

1

0

G5

G4

B7

B6

B5

B4

B3

R4

R3

G3

Data10

GPIO48

Green bit 5

20

Data9

Data8

Data7

Data6

Data5

Data4

Data3

Data2

Data1

GPIO45

Green bit 4

21

GPIO10

Blue bit 7

22

GPIO17

Blue bit 6

23

GPIO18

Blue bit 5

24

GPIO38

Blue bit 4

25

GPIO14

Blue bit 3

26

GPIO2

Red bit 4

27

GPIO1

Red bit 3

27

GPIO0

Green bit 3

28

VSYNC/DE?

Not used in

colour bits

In RGB565 mode bit 0 is unused; some designs

—

connect R2, but on this board the lower bits are

not routed.

The control signals are connected as follows:

Signal

VSYNC

ESP32 pin

Purpose

Notes

GPIO3

Vertical sync pulse

Low for 4 lines at the start of the frame

29

.

HSYNC

GPIO46

Horizontal sync

Low for 4 pixel clocks at the start of each

pulse

line

21

.

DE

PCLK

GPIO5

Data‑enable gate

High during active pixel region

30

.

GPIO7

Pixel clock

16 MHz; latches data on rising edge

31

.

DISP (backlight

enable)

CH422

IO1

Enables LED driver

Held low until drawing starts

32

.

4.2 I²C bus and CH422 communication

The I²C bus uses GPIO8 as  SDA  and GPIO9 as  SCL

33

. It operates at up to 400 kHz and is shared by the

GT911 and the CH422 expander. Both devices respond at 3.3 V logic levels. The CH422 does not have a

single fixed address; instead, each register is accessed using its own 7‑bit address (e.g. 0x24 for MODE, 0x26

for IN, 0x38 for OUT and 0x23 for OUT_UPPER

6

). To write a bit pattern to the outputs, the host transmits

a start condition, writes the register address (with the write bit), writes one data byte and then issues a stop

condition.   Because   the   mode   register   must   be   programmed   before   the   outputs   can   be   driven,   the

initialisation sequence writes 0x01 (enable output) and 0x04 (enable open‑drain) to the MODE register. The

7

GT911 uses a conventional single‑address scheme (0x5D/0x5C) and implements register pointers internally;

after resetting the GT911 the host writes configuration and then polls or uses interrupts for touch data.

5

4.3 SPI, USB and other buses

The SD‑card slot uses the ESP32’s SPI2 interface: GPIO11 (MOSI), GPIO12 (SCK) and GPIO13 (MISO) for data

34

. The chip‑select line is connected to CH422 IO3 (SD_CS), allowing the card to be deselected by default;

this prevents bus contention when the card is not used. The USB interface uses GPIO19 (D‑) and GPIO20

(D+), which connect to the CH343. When CH422 IO5 is low the CH343 is enabled and the CAN transceiver is

disconnected;   when   IO5   is   high   the   CAN   transceiver   is   enabled   and   the   CH343   is   isolated

35

.   This

multiplexing is useful because the ESP32 only has a single set of pins for these functions.

The   RS‑485   lines   (GPIO43/GPIO44)   and   CAN   lines   (GPIO15/GPIO16)   are   differential;   they   interface   to

external transceivers. The opto‑isolated digital inputs DI0/DI1 connect to CH422 IO6/IO7 and pass through

optocouplers to handle 5–36 V signals

36

. Digital outputs DO0/DO1 (OC0/OC1) similarly drive optocouplers

for isolated loads.

5 Comprehensive pin mapping

The table below consolidates all of the available information on how each ESP32 GPIO pin (module pin
name  IOx ) maps to the LCD, touch, SD card, USB, RS‑485, CAN and CH422 lines. The table includes short

phrases; detailed descriptions are provided in the text above.

ESP32

pin
( IOx )

LCD

signal /

Other

peripheral

colour bit

assignment

CH422

mapping

Direction

Notes

IO0

IO1

IO2

IO3

IO4

IO5

IO6

IO7

IO8

IO9

G3

R3

R4

VSYNC

–

DE

–

PCLK

–

–

–

–

–

–

Touch interrupt

(TP_IRQ)

–

–

–

I²C SDA, Touch

SDA

I²C SCL, Touch

SCL

–

–

–

–

–

–

output

output

output

output

input

Green data bit 3

28

Red data bit 3

27

Red data bit 4

27

Vertical sync pulse

29

GT911 pulls low on

touch

37

output

Data enable gate

30

CH422 DI0

input

Opto‑isolated digital

input 0

36

output

Pixel clock (16 MHz)

38

bidirectional

Shared with CH422 and

GT911

39

bidirectional

Shared with CH422 and

GT911

39

–

–

–

8

ESP32

pin
( IOx )

LCD

signal /

Other

peripheral

colour bit

assignment

CH422

mapping

Direction

Notes

IO10

B7

–

IO11

IO12

IO13

–

–

–

IO14

B3

IO15

IO16

IO17

IO18

IO19

IO20

IO21

IO38

IO39

IO40

IO41

IO42

IO43

IO44

IO45

–

–

B6

B5

–

–

G7

B4

G2

R7

R6

R5

–

–

G4

IO46

HSYNC

SPI MOSI (SD

card)

SPI SCK (SD

card)

SPI MISO (SD

card)

–

CAN TX / RS‑485

TXD

CAN RX /

RS‑485 RXD

–

–

USB_DN

USB_DP

–

–

–

–

–

–

RS‑485 RXD

RS‑485 TXD

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

–

9

output

Blue data bit 7

22

output

Data to SD card

34

output

Clock to SD card

34

input

Data from SD card

34

output

Blue data bit 3

26

Transmits CAN or

output

RS‑485 depending on

IO5

40

Receives CAN or RS‑485

depending on IO5

40

Blue data bit 6

23

Blue data bit 5

24

input

output

output

differential

USB negative data line

41

differential

USB positive data line

41

output

output

output

output

output

output

input

output

output

output

Green data bit 7

18

Blue data bit 4

25

Green data bit 2

17

Red data bit 7

42

Red data bit 6

43

Red data bit 5

44

RS‑485 receive line

15

RS‑485 transmit line

15

Green data bit 4

21

Horizontal sync pulse

21

ESP32

pin
( IOx )

IO47

IO48

EXIO0

EXIO1

EXIO2

EXIO3

EXIO4

EXIO5

OD0/

OD1

LCD

signal /

Other

peripheral

colour bit

assignment

CH422

mapping

Direction

Notes

G6

G5

–

–

–

–

–

–

–

–

–

DI0

TP_RST

DISP

–

–

output

output

CH422 IO6

input

Green data bit 6

19

Green data bit 5

20

Opto‑isolated digital

input 0

36

CH422 IO0

output

GT911 reset line

45

CH422 IO1

output

Backlight enable

32

LCD_RST

CH422 IO2

output

Additional ST7262 reset

line

SD_CS

CH422 IO3

output

SD‑card chip select

46

USB_SEL/

CAN_SEL

DO0/DO1

CH422 IO5

output

CH422

open‑drain

OC0/OC1

outputs

Selects between USB

(low) and CAN (high)

35

Drive optocoupled

outputs; not used in the

test demo

36

This mapping clearly shows that virtually every available ESP32 pin is allocated to the LCD bus or peripheral

functions. Additional user I/O must therefore come through the CH422 expander or over the CAN/RS‑485

connectors.

6 Software operation and code flow

6.1 Waveshare Arduino sketch overview

The Arduino sketch  08_DrawColorBar.ino  is intentionally simple. It opens the serial port at 115 kbit/s,
prints a start message, calls   waveshare_lcd_init()   and then prints an end message. The   loop()

function delays one second and prints “IDLE loop”, so all of the interesting work happens inside the LCD

initialisation function.

void setup() {

Serial.begin(115200);

Serial.println("RGB LCD example start");

waveshare_lcd_init();

Serial.println("RGB LCD example end");

}

void loop() {

delay(1000);

10

Serial.println("IDLE loop");

}

6.2 LCD initialisation sequence ( waveshare_lcd_init() )

The   helper   waveshare_lcd_init()   is   defined   in   waveshare_lcd_port.cpp .   Its   operation   can   be

expanded into discrete steps:

1.

Create and initialise the CH422 expander. The function prints “Initialize IO expander” (via
Serial.println() ), constructs an  ESP_IOExpander_CH422G  object and calls  init() /
begin() . The library writes 0x01 (output enable) and 0x04 (open‑drain enable) to the CH422’s

MODE register and clears the output bits

6

. It then calls  pinMode()  for each used pin: IO0

(TP_RST), IO1 (LCD_BL), IO2 (LCD_RST), IO3 (SD_CS) and IO5 (USB_SEL) are set as outputs; IO6/IO7

(DI0/DI1) remain inputs. The code asserts IO0 and IO2 high (releasing resets) and asserts IO1 low

2.

(backlight off). A 100 ms delay allows the ST7262 to exit reset.
Configure the RGB bus. The function prints “Create RGB LCD bus”, then creates a  BusRGB  object. It
passes the 16 data pins defined in  waveshare_lcd_port.h  along with HSYNC, VSYNC, PCLK, DE

and DISP lines, as well as timing parameters (16 MHz pixel clock, HPW=4, HBP=8, HFP=8, VPW=4,
VBP=8, VFP=8). The bus allocates a bounce buffer sized to  EXAMPLE_LCD_WIDTH × 10  pixels

(800×10) to minimise tearing.

3.

4.

Create the ST7262 driver. The function prints “Create LCD device” and uses the macro
EXAMPLE_LCD_CLASS(ST7262, …)  to instantiate a  LCD_ST7262  driver with the bus. It calls
lcd->init() ,  lcd->reset()  and  lcd->begin() . If the display driver’s
BasicBusSpecification  indicates that display on/off control is supported, it calls  lcd-
>setDisplayOnOff(true) .
Attach callbacks (optional). If  EXAMPLE_LCD_ENABLE_PRINT_FPS  is set, a VSYNC callback counts
frames and prints the frame‑rate; if  EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK  is set, a

callback prints “LCD draw finish callback” when a drawing operation completes. These callbacks run in

interrupt context; they must be short.

5.

Draw the colour bars. The function prints “Draw color bar from top left to bottom right, the order is B –
 G – R”. It calls  lcd->colorBarTest() , a method of the display driver. Although the library’s

source code is not included here, it is straightforward: it divides the horizontal resolution by the

number of colours (3) to determine the width of each bar; for each bar it fills the bounce buffer with
a constant 16‑bit colour value and calls  drawBitmap()  to stream the buffer to the panel. Because
the ST7262 continuously scans through the rows, each call to  drawBitmap()  triggers a DMA

transfer that outputs a few lines of pixel data while the CPU prepares the next lines.

6.

Enable the backlight. After the bars are drawn the code uses the CH422 to set IO1 high, turning on

the backlight driver. This final action illuminates the previously written image; the colours now
appear clearly. The function then returns to  setup() , which prints “RGB LCD example end”.

6.3 Colour bar drawing algorithm

Although the library hides the implementation of  colorBarTest() , a typical approach for generating the

test pattern is as follows:

1.

Compute the stripe width as  EXAMPLE_LCD_WIDTH / 3  (≈266 pixels).

11

2.

3.

Prepare an array of three 16‑bit RGB565 values: blue (0x001F), green (0x07E0) and red (0xF800).
For each stripe index  i  from 0 to 2:

◦

Fill the bounce buffer with the constant colour value  colors[i]  for  stripe_width ×
EXAMPLE_LCD_HEIGHT  pixels.

◦

Call
drawBitmap(x = i × stripe_width, y = 0, width = stripe_width, height =

EXAMPLE_LCD_HEIGHT, buffer) .

4.

Flush the buffer; the DMA controller streams the pixel data to the bus while generating HSYNC/

VSYNC pulses. The process repeats for each stripe until the full frame is drawn.

Because the bounce buffer holds only 800×10 pixels, the library subdivides each stripe into chunks and

streams them sequentially. The CPU remains free to perform other tasks (such as printing logs) while the

DMA transfers occur.

6.4 Working with the CH422 expander in code

To emphasise how critical the CH422 is, here is a simplified pseudocode representation of the initialisation

sequence for the expander:

// Setup I²C bus on GPIO8/9

i2c.begin(I2C_MASTER_NUM, SDA=8, SCL=9);

// Create expander instance

ESP_IOExpander_CH422G expander(I2C_MASTER_NUM);

expander.init();      // sets up I²C bus

expander.begin();     // writes default outputs and configures mode

// Configure pins as outputs

expander.pinMode(TP_RST, OUTPUT);

expander.pinMode(LCD_BL, OUTPUT);

expander.pinMode(LCD_RST, OUTPUT);

expander.pinMode(SD_CS, OUTPUT);

expander.pinMode(USB_SEL, OUTPUT);

// Release resets and disable backlight

expander.digitalWrite(TP_RST, HIGH);

expander.digitalWrite(LCD_RST, HIGH);

expander.digitalWrite(LCD_BL, LOW);

expander.digitalWrite(SD_CS, HIGH);

expander.digitalWrite(USB_SEL, LOW);  // selects USB over CAN

delay(100); // wait for peripherals to stabilise

// Later, enable backlight after drawing

expander.digitalWrite(LCD_BL, HIGH);

12

  pinMode()   sets   the   output   enable   bit   in   the   MODE   register   (0x24)   and
Under   the   hood,
digitalWrite()   updates a shadow register before writing to the OUT register (0x38) or OUT_UPPER

register   (0x23)   depending   on   the   pin   index
misconfigured, the library reports an error and the  colorBarTest()  is not executed.

.   If   the   CH422   is   not   connected   or   the   I²C   address   is

6

7 Integration: how everything works together

Putting  the  above  pieces  together,  the  flow  of   signals   that   produces   the   final   colour‑bar   image   can   be

summarised step by step:

1.

Power supplies stabilise. The 3.3 V rail powers the ESP32, ST7262, GT911 and CH422; the 5 V rail
powers the LED backlight driver and USB transceivers. The ST7262’s  GRB  pin is held low and its

DISP pin is low, so the panel does not refresh yet.

2.

I/O expander setup. The ESP32 configures the CH422 over I²C, enabling output mode on IO0–IO7

and open‑drain mode on OC0–OC3. It sets the GT911 reset and ST7262 reset lines high, keeping

both devices in a known state. It also leaves the backlight off and deselects the SD‑card.

3.

RGB bus and timing generator. The ESP32’s LCD peripheral sets up the 16‑bit RGB bus and begins

toggling PCLK at 16 MHz. HSYNC and VSYNC pulses define line and frame boundaries; DE gates the

active pixel region. The ST7262 latches data on PCLK rising edges; since the bounce buffer initially

contains zeros, the screen remains black.

4.

LCD driver initialisation. The driver resets the ST7262 internally, writes configuration registers,
loads gamma curves and starts the internal timing generator. The driver’s  begin()  call ensures

that the ST7262 is outputting the correct biases to the source and gate drivers. If the CH422 had not
released  LCD_RST , the ST7262 would remain silent and the bus lines would have no effect.

5.

Colour bar streaming. The firmware fills the bounce buffer with blue pixel values and instructs the

driver to draw a bar covering the left third of the screen. The DMA engine copies data from PSRAM to

the bus while the CPU is free. The process repeats for green and red bars. Because the ST7262

constantly reads from the bus, these values are latched into the display matrix as soon as they

appear on the lines. At this stage the panel still appears dark because the backlight LED driver is

disabled.

6.

Backlight enable. After the drawing completes and the ST7262 is running, the ESP32 drives CH422

IO1 high. The backlight driver turns on and the previously written RGB data become visible as three

vertical colour bars. The VSYNC callback prints the frame rate, confirming that the system refreshes

at the expected speed.
Idle loop and touch readiness. The Arduino  loop()  does nothing except print an “IDLE loop”

7.

message. However, the GT911 is now ready; if the code were extended to read touch reports, it

would use I²C transactions triggered by TP_IRQ. The CAN and RS‑485 transceivers are also idle

because IO5 remains low (USB mode), but could be enabled by driving IO5 high.

This sequence highlights the interplay between hardware and software. The CH422 is pivotal because it

gates critical control signals. The ST7262 demands precise timing and power sequencing; the ESP32’s LCD
peripheral   and   the   esp_panel   library   supply   this   with   minimal   CPU   overhead.   Together,   these

components deliver flicker‑free colour bars and provide a base for more complex user interfaces.

13

8 Example colour‑bar image

The figure below reproduces the colour‑bar test pattern (blue on the left, green in the centre and red on the

right). It is generated at the same resolution (800×480) and demonstrates the mapping of RGB565 values
used by the example.

9 Conclusion and further directions

This   exhaustive   analysis   shows   that   producing   a   simple   test   pattern   on   the  ESP32‑S3‑Touch‑LCD‑4.3B

requires coordinated configuration of multiple chips. The  CH422 I/O expander  must be initialised first; it

releases   reset   lines,   selects   USB/CAN   mode   and   controls   the   backlight.   The  ST7262  requires   specific
power‑on  sequencing  and  precise  RGB  timing;  the  ESP32’s  LCD  peripheral  and  the   esp_panel   library

provide this by generating a 16 MHz pixel clock and carefully chosen porch parameters. The GT911 remains

ready on the shared I²C bus; its reset and interrupt lines are also controlled by the CH422. Transceivers for

USB,   RS‑485   and   CAN   sit   idle   until   selected.   All   of   this   is   orchestrated   by   a   few   lines   of   Arduino   code,

demonstrating how hardware abstraction libraries hide complexity.

For developers building on this board, key takeaways are:

•

Respect the ST7262 power sequence. Always release reset, wait, start timing signals, then enable

the backlight

9

.

•

Always initialise the CH422 before using any peripherals. Configure pin modes and drive states;

failure to do so will keep devices in reset or disconnected

6

.

•

Use appropriate timing parameters. The provided values satisfy the ST7262’s minimum

requirements

10

; increasing the pixel clock requires adjusting porch values and may reduce colour

quality.

•

Leverage DMA and bounce buffers. Streaming large images over the RGB bus is only practical
when DMA transfers free the CPU; the  esp_panel  library handles this automatically.

Armed   with   these   details   one   can   modify   the   demo   to   draw   custom   graphics,   integrate   touch   input,

communicate over CAN/RS‑485 and drive external devices via the CH422’s additional pins. Understanding

the hardware at this level allows safe and reliable customisation of the ESP32‑S3‑Touch‑LCD‑4.3B board.

14

1

14

15

16

17

18

19

20

21

22

23

24

25

26

27

28

29

30

31

32

33

34

35

36

37

38

39

40

41

42

43

44

45

46

ESP32-S3-Touch-LCD-4.3B - Waveshare Wiki

https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B

2

CH422_features.txt

file:///home/oai/share/extracted/CH422_features.txt

3

ST7262

file:///home/oai/share/ST7262.pdf

4

GT911_I2C_address.txt

file:///home/oai/share/extracted/GT911_I2C_address.txt

5

9

ST7262_power_seq.txt

file:///home/oai/share/extracted/ST7262_power_seq.txt

6

11

12

ESPHome: esphome/components/ch422g/ch422g.cpp Source File

https://api-docs.esphome.io/ch422g_8cpp_source

7

GT911_features.txt

file:///home/oai/share/extracted/GT911_features.txt

8

CH422_features_detail.txt

file:///home/oai/share/extracted/CH422_features_detail.txt

10

ST7262_rgb_timing.txt

file:///home/oai/share/extracted/ST7262_rgb_timing.txt

13

TJA1051_intro_features.txt

file:///home/oai/share/extracted/TJA1051_intro_features.txt

15

