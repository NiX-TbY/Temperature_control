# HMI Design and Specification for a

# Commercial Freezer Touch Screen

# Controller

## Section 1: Foundational Principles of HMI Design for

## Industrial & Commercial Environments

The design of a Human-Machine Interface (HMI) for a commercial freezer controller requires a
disciplined approach that balances modern user expectations with the stringent demands of
safety, reliability, and operational efficiency. The following principles form the foundation of the
proposed design, ensuring the final interface is not only functional but also intuitive and robust.

### 1.1. The User-Centric Mandate in HMI Design

The guiding methodology for this HMI design is User-Centered Design (UCD), a philosophy that
places the operator at the core of the entire process. This approach prioritizes the user's tasks,
environment, and mental model to create an interface that is effective, efficient, and satisfying to
use. The key objectives derived from UCD are:
● **Efficacy:** The interface must be oriented toward the operator's primary tasks, such as
monitoring temperature and responding to alarms, with minimal cognitive load.
● **Efficiency:** The design must be easy to learn for new users and easy to remember for
infrequent users. It should be structured to reduce the potential for human error and
ensure safe operation.
● **Satisfaction:** The final product must be compatible with the operator's needs and
expectations, fostering confidence in the system.
To achieve these objectives, the design adheres to fundamental principles of interaction. It
provides clear **visibility** of all available functions, establishes a logical **mapping** between
controls and their effects, and delivers immediate and unambiguous **feedback** for every action
the operator takes. This ensures that the operator always understands the system's current
state and the consequences of their inputs.

### 1.2. The "Pro-sumer" Interface: Bridging Consumer UI and Industrial

### HMI

A critical analysis of the current market reveals a divergence between consumer-grade smart
device interfaces and traditional industrial control panels. Consumer products, such as smart
home thermostats and refrigerators, often prioritize rich aesthetics, complex animations, and
dense feature sets. In contrast, industrial HMIs have historically prioritized raw functionality,
clarity, and extreme reliability, sometimes at the expense of intuitive design.
The request for a modern touch screen interface with animated feedback (a "pulsing" light)
suggests an expectation for a more contemporary user experience, akin to consumer
electronics. However, the application—a commercial freezer—is a critical piece of equipment


where failure can lead to significant financial loss from spoiled goods. Operators may be
working in challenging environments, potentially wearing gloves or in suboptimal lighting
conditions, which necessitates a design that is far more robust than a typical consumer app.
Simply replicating a consumer-style interface would be inappropriate, as it could compromise
safety and usability in a commercial context. Conversely, a purely utilitarian industrial design
might feel dated and unintuitive to operators accustomed to modern smartphones and tablets.
Therefore, the optimal design strategy is a "Professional-Consumer" or "Pro-sumer" approach.
This model synthesizes the best of both worlds: it leverages the clean aesthetics and intuitive
interaction patterns of modern User Interface (UI) design while strictly adhering to the safety,
clarity, and ergonomic principles of high-performance HMI design. The resulting interface will be
immediately understandable to a novice operator but robust and reliable enough for an expert
technician, ensuring it is both user-friendly and mission-critical.

### 1.3. Safety, Clarity, and Error Prevention

In any industrial or commercial control system, preventing operator error is paramount. The
design incorporates the principle of "Idiot Proofing," more formally known as error-tolerant
design, which aims to make the system resilient to mistakes and guide the user toward correct
actions.
A core component of this is establishing a clear **hierarchy of information**. The most critical
piece of data—the actual internal temperature of the freezer—must be the most prominent
element on the screen, instantly readable from a distance. Secondary information, such as the
commanded temperature (setpoint), and interactive controls are visually subordinate to prevent
distraction from the primary data.
**Color and contrast** are used with strict discipline. Following established HMI standards, the
color palette is restrained. Bright, saturated colors, particularly red, are reserved exclusively for
indicating abnormal conditions, such as an active alarm. During normal operation, the screen
will use a muted, high-contrast color scheme. This makes the interface visually "quiet," ensuring
that when an alarm does occur, the change is immediate and impossible to ignore. This practice
prevents "alarm fatigue," where operators become desensitized to warning colors used for
non-critical functions.
Finally, **font legibility** is a non-negotiable requirement. The design specifies clear, easily
readable fonts with sufficient contrast against their background to ensure text stands out under
various ambient lighting conditions.

## Section 2: Analysis and Expert Refinement of the

## Proposed Interface

This section deconstructs the initial user request, applying the foundational principles to refine
the layout, controls, and indicators into a cohesive and professional HMI design tailored for the
specified 800x480 pixel display.

### 2.1. Overall Layout and Dimensional Analysis

The proposed layout consists of a narrow control column on the left and a large primary display
area on the right. This is a logical and efficient arrangement that separates interactive controls
from the primary status display. The design is precisely mapped to the 800x480 pixel resolution


of the ESP32-S3-Touch-LCD-4.3B display.
The dimensions of all touch-interactive elements are designed to be large enough for accurate
activation, even by an operator wearing gloves, a common scenario in commercial kitchens.
The pixel dimensions for each button exceed the minimum recommendations from industrial
standards, ensuring high usability and minimizing the chance of accidental presses.

### 2.2. The Control Column (Left): Setpoint, Adjustment, and Defrost

This column contains the user's primary means of interacting with and configuring the freezer's
operation.
● **Setpoint Display (Top-Left):** The commanded temperature is displayed in a dedicated
zone. The use of blue text for the numerical value is acceptable, as it is a cool,
non-alarming color that provides good contrast against a dark background and is
thematically appropriate for a freezer.
● **Temperature Adjustment Buttons:** The initial request for a red "Up" button, while based
on a common "hot/cold" consumer metaphor, presents a significant safety conflict. In HMI
design, the color red is universally reserved for alarm, danger, or stop conditions. Using
red for a routine operational control would violate this critical safety principle and could
diminish the operator's response to a genuine red-colored alarm.To resolve this, the
design is modified for enhanced safety and consistency. Both the "Up" and "Down"
buttons will use the same neutral, deep blue background color. Functionality is clearly and
unambiguously differentiated by universally recognized icons: a solid up arrow (▲) for
increasing temperature and a solid down arrow (▼) for decreasing it. This change aligns
the interface with industrial safety standards without compromising usability.
● **Manual Defrost Button:** A simple snowflake icon is often used in climate control systems
to indicate the activation of the cooling compressor. A defrost cycle, however, is a
temporary heating process. To avoid confusion, the design uses a more precise
composite icon featuring a stylized snowflake within a circle, with a water droplet beneath
it. This icon more accurately communicates the concept of "thawing" or "melting". The
user's suggestion to have the light blue background pulse slowly during an active defrost
cycle is an excellent form of persistent visual feedback and is incorporated into the final
specification.

### 2.3. The Primary Display (Right): Actual Temperature

This large area is the most important part of the interface, conveying the freezer's current
operational status. The design prioritizes at-a-glance readability above all else. A black
background with large white numbers provides the maximum possible contrast and is ideal for
legibility from a distance and in varying light conditions.
To maximize visibility, the font size for the actual temperature has been significantly increased to
be the dominant element on the screen. For the font, the request for "blocky" numbers points
toward a style that is clear and unambiguous. Suitable options include digital-style fonts that
mimic 7-segment displays, such as "Digit Tech," or geometric sans-serif fonts like "Blocky".
These fonts are highly legible for numerical data and are often available with licenses that
permit free commercial use.

### 2.4. The Integrated Alarm System


The alarm system is designed to be unmissable and intuitive, using the main temperature
display as the primary indicator.

1. **Normal State:** The actual temperature digits (DISP_ACTUAL) are displayed in
    high-contrast white. The bottom-right corner of the screen, designated for alarm
    interaction, remains dormant and black.
2. **Alarm Active State:** When an alarm condition is triggered, the color of the large
    temperature digits changes from white to a bright, urgent red and begins to pulse,
    providing a visual cue directly linked to the out-of-spec data. Simultaneously, the audible
    buzzer sounds, and the "SILENCE" interface appears in the bottom-right corner.
3. **Alarm Interaction:** The "SILENCE" button is designed for clarity and to avoid a cluttered
    look. Instead of a solid red box, the interface consists of the word "SILENCE" in red text,
    framed above and below by two stylized, tapered red lines. This creates a clear touch
    target that is only visible and interactive during an active alarm, preventing accidental
    activation.
4. **Alarm Silenced State:** Once the operator presses the "SILENCE" area, the audible
    buzzer stops, and the temperature digits cease pulsing but remain red to indicate the
    alarm condition is still present. The "SILENCE" interface is replaced by a countdown timer
    (e.g., "15:00"), informing the operator how much time remains before the buzzer
    reactivates if the condition is not resolved.

## Section 3: The Final GUI Design: Visuals and

## Functional States

This section presents the complete visual design and specifications, translating the analysis and
refinements into a concrete, implementable blueprint for the 800x480 pixel display.

### 3.1. High-Fidelity Master Mockup (Normal Operating State)

The master mockup represents the GUI during standard, non-alarm operation.
● **Left Column:** A vertical stack of four distinct zones against a black background.
○ Top: The commanded temperature is displayed in blue digits (e.g., "-18").
○ Middle: Two square buttons with deep blue backgrounds contain black up and down
arrow icons.
○ Bottom: A rectangular button with a light blue background contains the white
composite defrost icon.
● **Right Display:** A large, unified area with a black background.
○ The actual freezer temperature is displayed prominently in the center using a very
large, white, blocky digital-style font (e.g., "-18.2").
○ The bottom-right corner is dormant and black.

### 3.2. Component Specification Summary Table

This table provides precise pixel-based specifications for each UI element, serving as a
definitive guide for software developers. The coordinate system origin (0,0) is the top-left corner
of the display.


```
Element ID Description Position (X, Y) &
Size (W, H) in
Pixels
```
```
Default State
Appearance
(Color, Font/Icon,
Text)
```
```
Primary Function
```
```
DISP_SET Commanded Temp
Display
```
```
(0, 0), 168x131 Background:
#000000, Text:
Blue (#00AEEF)
```
```
Displays the target
temperature set by
the user.
BTN_UP Temp Increase
Button
```
```
(0, 131), 168x131 Background: Deep
Blue (#003366),
Icon: Black Up
Arrow
```
```
Increases the
commanded
temperature.
```
```
BTN_DOWN Temp Decrease
Button
```
```
(0, 262), 168x131 Background: Deep
Blue (#003366),
Icon: Black Down
Arrow
```
```
Decreases the
commanded
temperature.
```
```
BTN_DEFROST Manual Defrost
Button
```
```
(0, 393), 168x87 Background: Light
Blue (#ADD8E6),
Icon: White
Defrost Symbol
```
```
Initiates a manual
defrost cycle.
```
```
DISP_ACTUAL Actual Temp
Display
```
```
(168, 0), 632x480 Background:
#000000, Text:
Large White
(#FFFFFF) Blocky
Numbers
```
```
Displays current
temperature. Text
turns red during
alarm.
```
```
ALARM_ZONE Alarm Interaction
Area
```
##### (589, 349),

```
211x
```
```
Background:
#
(dormant). No
icon/text.
```
```
Displays the
"SILENCE"
interface during an
active alarm.
```
### 3.3. Visual Specification of Interactive States

```
● State 1: Temperature Adjustment: When BTN_UP or BTN_DOWN is pressed, its
background color momentarily brightens to provide tactile feedback. The value in
DISP_SET updates immediately.
● State 2: Manual Defrost Active: After BTN_DEFROST is pressed, its light blue
background begins a slow, rhythmic pulse. This provides persistent visual confirmation
that the defrost cycle is in progress.
● State 3: Alarm Condition Active: The text within DISP_ACTUAL changes from white to
red (#FF0000) and begins to pulse slowly. The audible buzzer sounds. In the
ALARM_ZONE, the "SILENCE" interface appears: red text framed by two tapered red
horizontal lines.
● State 4: Alarm Buzzer Silenced: Upon pressing the "SILENCE" interface, the audible
buzzer stops. The text in DISP_ACTUAL stops pulsing but remains static red. The
"SILENCE" interface is replaced by a white countdown timer (e.g., "15:00") on a black
background.
```
## Section 4: Technical Specifications and


## Implementation Assets

This section provides the specific color, font, and icon assets required for development.

### 4.1. Color Palette Specification

```
Functional Name Swatch HEX Code RGB Value Usage Notes
BG_Primary ⚫ #000000 0, 0, 0 Primary
background for all
display areas.
Text_Primary ⚪ #FFFFFF 255, 255, 255 Primary text color
for actual
temperature and
icons.
Text_Setpoint 🔵 #00AEEF 0, 174, 239 Text color for
commanded
temperature
display.
Button_Control_B
G
```
```
🟦 #003366 0, 51, 102 Background for
standard control
buttons
(Up/Down).
Button_Defrost_B
G
```
```
💧 #ADD8E6 173, 216, 230 Background for
Defrost button
(normal state).
Icon_Primary ⚫ #000000 0, 0, 0 Icon color for
standard control
buttons.
Alarm_Active 🔴 #FF0000 255, 0, 0 Color for alarm
text and indicators.
Reserved for
active alarm states
only.
```
### 4.2. Typographic and Iconographic Assets

```
Asset Type Recommended
Asset Name
```
```
Source / Link (if
available)
```
```
License Type Notes
```
```
Font (Display) Digit Tech
(7-Segment)
```
```
fontesk.com/digit-t
ech-typeface/
```
```
Free for
Commercial Use
(OFL)
```
```
Provides excellent
clarity for
numerical
displays.
Font (UI Text) Roboto or System
Default
```
```
fonts.google.com/s
pecimen/Roboto
```
```
Apache License
2.
```
```
A clean, legible
sans-serif font for
secondary text
labels (e.g.,
"SILENCE").
```

```
Asset Type Recommended
Asset Name
```
```
Source / Link (if
available)
```
```
License Type Notes
```
```
Icon (Up/Down) Font Awesome
Solid: caret-up,
caret-down
```
```
fontawesome.com/
icons/categories/ar
rows
```
```
Free (CC BY 4.0) Universally
understood for
increment/decrem
ent actions.
Icon (Defrost) Custom
Composite
( ❄ + 💧 )
```
```
To be created
based on defrost
symbol
conventions
```
```
N/A A custom vector
asset: stylized
snowflake in a
circle with a water
droplet below.
```
## Section 5: Conclusion and Recommendations for

## Future Iterations

This report has detailed a comprehensive HMI design for a commercial freezer controller,
translating a functional concept into a professional, safe, and ergonomically sound interface.

### 5.1. Summary of Design Enhancements

The final design successfully interprets all functional requirements while elevating the interface
through the application of rigorous HMI principles. Key enhancements include:
● **Pixel-Perfect Layout:** A grid-based system with ergonomically sized touch targets,
precisely mapped to the 800x480 display, improves usability and reduces operator error.
● **Safety-Compliant Color Scheme:** The color palette reserves red exclusively for alarms,
enhancing the visibility and impact of critical alerts.
● **Unambiguous Iconography:** The defrost icon has been refined to provide clear, intuitive
communication of its function.
● **Refined Alarm System:** The alarm indicator is now integrated directly into the main
temperature display, and the "Silence" button has been redesigned for a cleaner, more
modern aesthetic while maintaining clear functionality.

### 5.2. Recommendations for Future Product Enhancement

The current design provides a robust foundation. To ensure the product remains competitive, a
strategic roadmap for future enhancements is recommended. The commercial appliance market
is increasingly adopting "smart" features, and users expect connectivity. Controllers with remote
monitoring, data logging, and wireless programmability represent the current state-of-the-art.
A phased approach is advised:
● **Phase 2 (Connectivity):** Integrate the ESP32-S3's built-in Wi-Fi module to enable remote
monitoring via a simple, built-in web server interface, providing at-a-glance status without
requiring a dedicated app.
● **Phase 3 (Data & Compliance):** Implement onboard memory for data logging of
temperature history and alarm events. This is a critical feature for businesses that must
comply with HACCP (Hazard Analysis and Critical Control Points) regulations.
● **Phase 4 (Advanced Features):** Leverage collected data to offer advanced functionality,
such as programmable operating schedules for energy savings, intelligent defrost cycles,


```
and predictive maintenance alerts.
```
### 5.3. The Next Step: Usability Testing

Before committing to mass production, it is strongly recommended that this HMI design be
implemented on a functional prototype and subjected to formal usability testing. Testing with a
representative group of end-users in a realistic setting will provide invaluable feedback, validate
the design choices, and ensure the final product is highly effective in the real world.

#### Works cited

1. Guidelines for designing touch-screen user ... - Esa Automation,
https://www.esa-automation.com/wp-content/uploads/2017/10/04_Guidelines-for-designing-touc
h-screen-user-interfaces-.pdf 2. Smart home with Samsung SmartThings | Samsung US,
https://www.samsung.com/us/smartthings/ 3. Samsung Family Hub™ | Samsung US | Samsung
USA, https://www.samsung.com/us/explore/family-hub-refrigerator/overview/ 4. Thermostat
designs, themes, templates and downloadable graphic elements on Dribbble,
https://dribbble.com/tags/thermostat 5. Best Smart Thermostats of 2025: Tested in Our Homes -
CNET, https://www.cnet.com/home/energy-and-utilities/best-smart-thermostats/ 6. HMI Panels,
Terminals & Industrial PCs | Schneider Electric USA,
https://www.se.com/us/en/product-category/2100-hmi-terminals-and-industrial-pc/ 7. Touch
Panel HMI - Human Machine Interfaces - Delta Electronics,
https://www.deltaww.com/en-US/products/Touch-Panel-HMI-Human-Machine-Interfaces/ALL/ 8.
Q-SYS Touch Screen Controllers,
https://www.qsys.com/products-solutions/q-sys/control-io-controllers/q-sys-touch-screen-controll
ers/ 9. 10+ Hundred Defrost Logo Royalty-Free Images, Stock Photos & Pictures | Shutterstock,
https://www.shutterstock.com/search/defrost-logo 10. HMI System Design – more than just a
touch., https://www.mouser.com/pdfdocs/EAO_TA_HMI_System_Design_EN.pdf 11.
FridgeAlert™ | Temperature Monitoring With Touch Screen - ControlByWeb,
https://controlbyweb.com/fridgealert/ 12. Product Section Touchscreens - Elan Industries,
https://www.elanindustries.com/simplified-product-page/touchscreens 13. Top 10 HMI Design
Best Practices: An Ultimate Guide - Aufait UX,
https://www.aufaitux.com/blog/hmi-design-best-practices/ 14. HMI Design Best Practices: The
Complete Guide - dataPARC,
https://www.dataparc.com/blog/hmi-design-best-practices-complete-guide/ 15. HMI Design -
Best Practices for Effective HMI Screens - SolisPLC,
https://www.solisplc.com/tutorials/hmi-design 16. UCM 1443 - Foodservice Equipment
Touchscreen Display Module - Renau Corporation,
https://renau.com/commercial/ucm-1443-flush-mount-foodservice-equipment-touchscreen-contr
ol/ 17. User Interface Design for Touch Screen Displays,
https://www.newvisiondisplay.com/ui-design-touch-screen-displays/ 18. 233536 Ui Arrows Stock
Vectors and Vector Art - Shutterstock,
https://www.shutterstock.com/search/ui-arrows?image_type=vector 19. Arrows Icons | Font
Awesome, https://fontawesome.com/icons/categories/arrows 20. 92 FREE arrow SVG icons -
Untitled UI, https://www.untitledui.com/free-icons/arrows 21. Browse thousands of Arrow UI
images for design inspiration - Dribbble, https://dribbble.com/search/arrow-ui 22. Thin Arrow Ui
Icons stock illustrations - iStock, https://www.istockphoto.com/photos/thin-arrow-ui-icons 23.
Arrow Ui Images - Free Download on Freepik,


https://www.freepik.com/free-photos-vectors/arrow-ui 24. ELI5: What difference does using the
"snowflake" make on a cars air conditioning system? : r/explainlikeimfive - Reddit,
https://www.reddit.com/r/explainlikeimfive/comments/4045zu/eli5_what_difference_does_using_
the_snowflake/ 25. 870+ Defrost Icon Stock Illustrations, Royalty-Free Vector Graphics & Clip
Art - iStock, https://www.istockphoto.com/illustrations/defrost-icon 26. 426 Defrosting Snowflake
Stock Illustrations, Vectors & Clipart - Dreamstime,
https://www.dreamstime.com/illustration/defrosting-snowflake.html 27. Defrosting Defrost
Snowflake #1165166 Vector Icon | VectorIcons,
https://vectoricons.net/icon/1165166/defrosting-defrost-snowflake-icon 28. Blocky – A Free
Display Font - Logos By Nick, https://logosbynick.com/blocky-font/ 29. Digit Tech Font Family ›
Fontesk, https://fontesk.com/digit-tech-typeface/ 30. 3+ Hundred 7 Segment Fonts Royalty-Free
Images, Stock Photos & Pictures | Shutterstock,
https://www.shutterstock.com/search/7-segment-fonts 31. 7-Segment display font - Torinak,
https://torinak.com/font/7-segment 32. Seven Segment Font - FONT Repo Free TTF Fonts,
https://www.fontrepo.com/font/21332/seven-segment 33. 400+ Free Block Letter Fonts ›
Fontesk, https://fontesk.com/tag/block/ 34. Seven Segment Regular font | Fonts2u.com,
https://sk.fonts2u.com/seven-segment-regular.font 35. Blocky font Fonts - MyFonts,
https://www.myfonts.com/pages/tags/blocky%20font-fonts 36. Blocky Font - Font Bundles,
https://fontbundles.net/dmletter-studio/4037778-blocky-font 37. Seven Segment Font Family
Download for Desktop & WebFont | CDNFonts.com,
https://www.cdnfonts.com/seven-segment.font 38. 1 Free Blocky, Digital, Variable Font - 1001
Fonts, https://www.1001fonts.com/blocky+digital+variable-fonts.html 39. 7 Free Blocky, Digital,
Wide Fonts, https://www.1001fonts.com/blocky+digital+wide-fonts.html 40. Web Thermostat -
ControlByWeb, https://controlbyweb.com/blog/web-thermostat/


