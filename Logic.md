## DIGITAL CONTROLLER WITH DEFROST AND

## FANS MANAGEMENT

# XR06CX

## 1 General warnings ....................................................................................................................................... 1

## 2 General description .................................................................................................................................... 1

## 3 Regulation .................................................................................................................................................. 1

## 4 Defrost ....................................................................................................................................................... 1

## 5 Fans ........................................................................................................................................................... 1

## 6 Front panel commands ............................................................................................................................... 1

## 7 Parameters ................................................................................................................................................. 2

## 8 Digital inputs (ONLY XR03CX) ................................................................................................................... 2

## 9 Installation and mounting ........................................................................................................................... 2

## 10 Electrical connections ................................................................................................................................. 2

## 11 How to use the hot key ............................................................................................................................... 2

## 12 Alarm signalling .......................................................................................................................................... 2

## 13 Technical data ............................................................................................................................................ 3

## 14 Connections ............................................................................................................................................... 3

## 15 Default setting values ................................................................................................................................. 3

## 1 GENERAL WARNINGS

### 1.1 PLEASE READ BEFORE USING THIS MANUAL

 This manual is part of the product and should be kept near the instrument for easy and quick
reference.
 The instrument shall not be used for purposes different from those described hereunder. It cannot be
used as a safety device.
 Check the application limits before proceeding.
 Dixell Srl reserves the right to change the composition of its products, even without notice, ensuring
the same and unchanged functionality.

### 1.2 SAFETY PRECAUTIONS

 Check the supply voltage is correct before connecting the instrument.
 Do not expose to water or moisture: use the controller only within the operating limits avoiding sudden
temperature changes with high atmospheric humidity to prevent formation of condensation
 Warning: disconnect all electrical connections before any kind of maintenance.
 Fit the probe where it is not accessible by the End User. The instrument must not be opened.
 In case of failure or faulty operation send the instrument back to the distributor or to “Dixell S.r.l.” (see
address) with a detailed description of the fault.
 Consider the maximum current which can be applied to each relay (see Technical Data).
 Ensure that the wires for probes, loads and the power supply are separated and far enough from each
other, without crossing or intertwining.
 In case of applications in industrial environments, the use of mains filters (our mod. FT1) in parallel with
inductive loads could be useful.

## 2 GENERAL DESCRIPTION

The **XR06CX** , **format 32 x 74 x 60 mm** , is microprocessor based controller, suitable for applications
on medium or low temperature ventilated refrigerating units. It has three relay outputs to control
compressor, fan, and defrost which can be either electrical or reverse cycle (hot gas). The device is
also provided with 2 NTC probe inputs, the first one for temperature control and the second one to be
located onto the evaporator, to control the defrost termination temperature and to managed the fan
and it’s provided with a configurable digital input. With the HOTKEY it’s possible to program the
instrument in a quick and easy way.

## 3 REGULATION

The regulation is performed according to
the temperature measured by the
thermostat probe with a positive
differential from the set point: if the
temperature increases and reaches set
point plus differential the compressor is
started and then turned off when the
temperature reaches the set point value
again.

In case of fault in the thermostat probe the start and stop of the compressor are timed through
parameters “ **Cy** ” and “ **Cn** ”.

## 4 DEFROST

Two defrost modes are available through the “ **td** ” parameter:
 **td=EL**  defrost through electrical heater (compressor OFF)
 **td=in**  hot gas defrost (compressor ON).
Other parameters are used to control the interval between defrost cycles ( **id** ) ), its maximum length
( **Md** ) and two defrost modes: timed or controlled by the evaporator’s probe. At the end of defrost
dripping time is started, its length is set in the **dt** parameter. With **dt=0** the dripping time is disabled.

## 5 FANS

With **FC** parameter it can be selected the fans functioning:
 **FC=cn**  will switch ON and OFF with the compressor and **not run** during defrost
 **FC=on**  fans will run even if the compressor is off, and not run during defrost

After defrost, there is a timed fan delay allowing for drip time, set by means of the “ **Fd** ” parameter.

 **FC=cy**  fans will switch ON and OFF with the compressor and **run** during defrost
 **FC=oY**  fans will run continuously also during defrost.

An additional parameter “ **FS** ” provides the setting of temperature, detected by the evaporator probe,
above which the fans are always OFF. This is used to make sure circulation of air only if his
temperature is lower than set in “ **FS** ”

### 5.1 FANS AND DIGITAL INPUT

```
When the digital input is configured as door switch iF=do , fans and compressor status depends on the
dC parameter value:
 dC=no  normal regulation;
 dC=Fn  fans OFF;
 dC=cP  compressor OFF;
 dC=Fc  compressor and fans OFF.
When rd=y , the regulation restart with door open alarm.
```
## 6 FRONT PANEL COMMANDS

```
To display target set point, in
programming mode it selects a
parameter or confirm an
operation
```
```
To start a manual defrost
```
```
In programming mode it browses
the parameter codes or increases
the displayed value
```
```
AUX
```
```
In programming mode it browses
the parameter codes or
decreases the displayed value
```
```
KEYS COMBINATION
```
## +^

```
To lock or unlock the keyboard
```
## +^ To enter in programming mode^

## +^ To return to room temperature display^

```
LED MODE SIGNIFICATO
On Compressor enabled
Flashing Anti short cycle delay enabled (AC parameter)
On Defrost in progress
Flashing Dripping in progress
On Fans output enabled
Flashing Fans delay after defrost
On Measurement unit
Flashing Programming mode
On Measurement unit
Flashing Programming mode
```
### 6.1 HOW TO SEE THE SET POINT

1. Push and immediately release the **SET** key, the set point will be showed;
2. Push and immediately release the **SET** key or wait about 5s to return to normal visualisation.

### 6.2 HOW TO CHANGE THE SETPOINT

1. Push the SET key for more than 2 seconds to change the Set point value;
2. The value of the set point will be displayed and the “°C” or “°F” LED starts blinking;
3. To change the Set value push the or arrows.
4. To memorise the new set point value push the SET key again or wait 10s.

### 6.3 HOW TO START A MANUAL DEFROST

```
Push the DEF key for more than 2 seconds and a manual defrost will start
```
### 6.4 HOW TO CHANGE A PARAMETER VALUE

```
To change the parameter’s value operate as follows:
```
1. Enter the Programming mode by pressing the **SET** + keys for 3s (“ **°C” or “°F”** LED starts
    blinking).
2. Select the required parameter. Press the “ **SET** ” key to display its value
3. Use or to change its value.
4. Press “ **SET** ” to store the new value and move to the following parameter.
**To exit** : Press **SET** + or wait 15s without pressing a key.
**NOTE** : the set value is stored even when the procedure is exited by waiting the time-out to expire.

### 6.5 HIDDEN MENU

```
The hidden menu includes all the parameters of the instrument.
HOW TO ENTER THE HIDDEN MENU
```
1. Enter the Programming mode by pressing the **SET** + keys for 3s (“ **°C” or “°F”** LED starts
    blinking).
2. Released the keys, then push again the **SET** + keys for more than 7s. The L2 label will be
    displayed immediately followed from the Hy parameter.
    **NOW YOU ARE IN THE HIDDEN MENU.**
3. Select the required parameter.
4. Press the “ **SET** ” key to display its value
5. Use or to change its value.
6. Press “ **SET** ” to store the new value and move to the following parameter.
**To exit** : Press **SET** + or wait 15s without pressing a key.
**NOTE1:** if there aren’t any parameter in L1, after 3s the “nP” message is displayed. Keep the keys
pushed till the L2 message is displayed.
**NOTE2** : the set value is stored even when the procedure is exited by waiting the time-out to expire.
HOW TO MOVE A PARAMETER FROM THE HIDDEN MENU TO THE FIRST
LEVEL AND VICEVERSA.
Each parameter present in the HIDDEN MENU can be removed or put into “THE FIRST LEVEL” (user
level) by pressing **SET** +. In HIDDEN MENU when a parameter is present in First Level the decimal
point is on.

### 6.6 TO LOCK THE KEYBOARD

1. Keep pressed for more than 3s the and keys.
2. The “ **OF** ” message will be displayed and the keyboard will be locked. If a key is pressed more
    than 3s the “OF” message will be displayed.


### 6.7 TO UNLOCK THE KEYBOARD

Keep pressed together for more than 3s the and keys till the **“on”** message will be displayed.

## 7 PARAMETERS

#### REGULATION

**Hy Differential:** (0,1°C  25°C / 1°F ÷ 45°F) Intervention differential for set point. Compressor Cut
IN is SET POINT + differential (Hy). Compressor Cut OUT is when the temperature reaches the
set point.
**LS Minimum SET POINT:** (-55°C÷SET/- 67 °F÷SET): Sets the minimum value for the set point..
**US Maximum SET POINT:** (SET÷99°C/ SET÷99°F). Set the maximum value for set point.
**ot First probe calibration:** (-9.9÷9.9°C / - 17 °F ÷ 1 7 °F) allows to adjust possible offset of the first
probe.
**P2 Evaporator probe presence: n=** not present; **y=** the defrost stops by temperature.
**oE Second probe calibration:** (-9.9÷9.9°C / - 17 °F ÷ 1 7 °F) allows to adjust possible offset of the
second probe.
**od Outputs activation delay at start up:** (0÷99min) This function is enabled at the initial start up of
the instrument and inhibits any output activation for the period of time set in the parameter.
**AC Anti-short cycle delay** : (0÷50 min) minimum interval between the compressor stop and the
following restart.
**Cy Compressor ON time with faulty probe:** (0÷ 99 min) time during which the compressor is active
in case of faulty thermostat probe. With Cy=0 compressor is always OFF.
**Cn Compressor OFF time with faulty probe:** (0÷99 min) time during which the compressor is OFF
in case of faulty thermostat probe. With Cn=0 compressor is always active.

**DISPLAY**

**CF Measurement unit:** (°C÷°F) **°C** =Celsius; **°F** =Fahrenheit. **WARNING** : When the measurement
unit is changed the SET point and the values of the parameters Hy, LS, US, oE, o1, AU, AL
have to be checked and modified if necessary.
**rE Resolution (only for °C):** (dE ÷ in) **dE** = decimal between - 9.9 and 9.9°C; **in** = integer
**Ld Default display:** (P1 ÷ P2) P1= thermostat probe; P2= evaporator probe. SP=Set point (only
XR04CX)
**dy Display delay:** (015 min.) when the temperature increases, the display is updated of 1 °C/1°F
after this time.

**DEFROST**

**td Defrost type:** (EL – in) **EL=** electrical heater, compressor OFF; **in=** hot gas, compressor ON;
**dE Defrost termination temperature:** (- 55 ÷50°C / -67÷99°F) if **P2=Y** it sets the temperature
measured by the evaporator probe, which causes the end of defrost.
**id Interval between defrost cycles:** (0÷99 minutes) Determines the time interval between the
beginning of two defrost cycles.
**Md Maximum length for defrost:** (0÷99 min. with 0 no defrost) when **P2=n** , (not evaporator probe:
timed defrost) it sets the defrost duration, when **P2 = y** (defrost end based on temperature) it sets
the maximum length for defrost.
**dd Start defrost delay:** ( 0÷99min) This is useful when different defrost start times are necessary to
avoid overloading the plant.
**dF Display during defrost:** (rt / it / SP / dF) **rt=** real temperature; **it=** start defrost temperature; **SP=**
SET-POINT; **dF=** label **dF.
dt Drip time:** (0÷99 min) time interval between reaching defrost termination temperature and the
restoring of the control’s normal operation. This time allows the evaporator to eliminate water
drops that might have formed due to defrost.
**dP Defrost at power –on:** (y÷n) **y=** at power on defrost starts; **n=** defrost doesn’t start at power-on

**FANS**

**FC Fans operating mode:** (cn, on, cY, oY) **cn=** in runs with the compressor, OFF during defrost;
**on=** continuous mode, OFF during defrost;; **cY=** runs with the compressor, ON during defrost;
**oY=** continuous mode, ON during defrost.
**Fd Fans delay after defrost:** (0÷99 min) Interval between end of defrost and evaporator fans start.
**FS Fans stop temperature:** (- 55 ÷50°C / -67°F ÷ 99°F) setting of temperature, detected by
evaporator probe, above which fans are always OFF.

**ALARMS**

**AU Maximum temperature alarm:** (AL÷99°C/99°F) when this temperature is reached the alarm is
enabled, after the “ **Ad** ” delay time.
**AL Minimum temperature alarm:** (-55÷AU°C /- 67 ÷AU°F) when this temperature is reached the
alarm is enabled, after the “ **Ad** ” delay time.
**Ad Temperature alarm delay:** (0÷99 min) time interval between the detection of an alarm condition
and alarm signalling.
**dA Exclusion of temperature alarm at startup:** (0÷9 9 min) time interval between the detection of
the temperature alarm condition after instrument power on and alarm signalling.

**DIGITAL INPUT**

**iP Digital input polarity:** (oP ÷ cL) **oP** = activated by closing the contact; **cL** = activated by opening
the contact;
**iF Digital input configuration:** (EA/bA/do/dF/Au/Hc) **EA=** external alarm: “EA” message is
displayed; **bA** = serious alarm “CA” message is displayed; **do** = door switch function; **dF** = defrost
activation; **Au** =not used; **Hc=** inversion of the kind of action;
**di Digital input delay:** (0÷99 min) with **iF=EA** or **bA** delay between the detection of the external
alarm condition and its signalling.. With **iF=do** it represents the delay to activate the door open
alarm.
**dC Compressor and fan status when open door** : (no/Fn/cP/Fc): **no** = normal; **Fn** = Fans OFF; **cP**
=Compressor OFF; **Fc** = Compressor and fans OFF;
**rd Regulation with door open** : (n÷y) **n** = no regulation if door is opened; **Y** = when di is elapsed
regulation restarts even if door open alarm is present;

**OTHER**

**d1 Thermostat probe display (read only)
d2 Evaporator probe display (read only)
Pt Parameter code table
rL Software release**

## 8 DIGITAL INPUTS (ONLY XR03CX)

```
The free voltage digital input is programmable in different configurations by the “ i1F ” parameter.
```
### 8.1 DOOR SWITCH (IF=DO)

```
It signals the door status and the corresponding relay output status through the “ dC ” parameter: no =
normal (any change); Fn = Fan OFF; CP = Compressor OFF; FC = Compressor and fan OFF.
Since the door is opened, after the delay time set through parameter “ di ”, the door alarm is enabled,
the display shows the message “ dA ” and the regulation restarts if rd = y. The alarm stops as soon
as the external digital input is disabled again. With the door open, the high and low temperature alarms
are disabled.
```
### 8.2 EXTERNAL ALARM (IF=EA)

```
As soon as the digital input is activated the unit will wait for “ di ” time delay before signalling the “ EA ”
alarm message. The outputs status don’t change. The alarm stops just after the digital input is de-
activated.
```
### 8.3 SERIOUS ALARM (IF=BA)

```
When the digital input is activated, the unit will wait for “ di ” delay before signalling the “ CA ” alarm
message. The relay outputs are switched OFF. The alarm will stop as soon as the digital input is de-
activated.
```
### 8.4 START DEFROST (IF=DF)

```
It starts a defrost if there are the right conditions. After the defrost is finished, the normal regulation will
restart only if the digital input is disabled otherwise the instrument will wait until the “ Md ” safety time is
expired.
```
### 8.5 INVERSION OF THE KIND OF ACTION: HEATING - COOLING (IF=HC)

```
This function allows to invert the regulation of the controller: from cooling to heating and viceversa.
```
## 9 INSTALLATION AND MOUNTING

```
Instrument XR 06 CX shall be mounted on vertical panel, in a
29x71 mm hole, and fixed using the special bracket supplied.
The temperature range allowed for correct operation is 060 °C.
Avoid places subject to strong vibrations, corrosive gases,
excessive dirt or humidity. The same recommendations apply to
probes. Let air circulate by the cooling holes.
```
## 10 ELECTRICAL CONNECTIONS

```
The instrument is provided with screw terminal block to connect cables with a cross section up to 2,
mm^2. Before connecting cables make sure the power supply complies with the instrument’s
requirements. Separate the probe cables from the power supply cables, from the outputs and the
power connections. Do not exceed the maximum current allowed on each relay, in case of heavier
loads use a suitable external relay.
```
### 10.1 PROBES

```
The probes shall be mounted with the bulb upwards to prevent damages due to casual liquid
infiltration. It is recommended to place the thermostat probe away from air streams to correctly
measure the average room temperature. Place the defrost termination probe among the evaporator
fins in the coldest place, where most ice is formed, far from heaters or from the warmest place during
defrost, to prevent premature defrost termination.
```
## 11 HOW TO USE THE HOT KEY

### 11.1 HOW TO PROGRAM THE HOT KEY FROM THE INSTRUMENT (UPLOAD)

1. Program one controller with the front keypad.
2. When the controller is ON, insert the “Hot key” and push key; the " **uP** " message appears
    followed a by flashing “ **Ed** ”
3. Push “ **SET** ” key and the “ **Ed** ” will stop flashing.
4. Turn OFF the instrument remove the “Hot Key”, then turn it ON again.

```
NOTE : the “ Er ” message is displayed for failed programming. In this case push again o key if you want
to restart the upload again or remove the “Hot key” to abort the operation.
```
### 11.2 HOW TO PROGRAM AN INSTRUMENT USING HOT KEY (DOWNLOAD)

1. Turn OFF the instrument.
2. Insert a **programmed “Hot Key” into the 5 PIN receptacle** and then turn the Controller ON.
3. Automatically the parameter list of the “Hot Key” is downloaded into the Controller memory, the
    “ **do** ” message is blinking followed a by flashing “ **Ed** ”.
4. After 10 seconds the instrument will restart working with the new parameters.
5. Remove the “Hot Key”..

```
NOTE : the “ Er ” message is displayed for failed programming. In this case push again o key if you want
to restart the upload again or remove the “Hot key” to abort the operation.
```
## 12 ALARM SIGNALLING

```
Mess. Cause Outputs
"P1" Room probe failure Compressor output according to “Cy” e “Cn”
"P2" Evaporator probe failure Defrost end is timed
"HA" Maximum temperature alarm Outputs unchanged
"LA" Minimum temperature alarm Outputs unchanged
“EA” External alarm Outputs unchanged
“CA” Serious external alarm All outputs OFF
“dA” Door Open Compressor and fans restarts
```
### 12.1 ALARM RECOVERY

```
Probe alarms P1 ” and “ P2” start some seconds after the fault in the related probe; they automatically
stop some seconds after the probe restarts normal operation. Check connections before replacing the
```

probe. Temperature alarms “ **HA** ” and “ **LA** ” automatically stop as soon as the temperature returns to
normal values.
Alarms **“EA”** and **“CA”** (with iF=bL) recover as soon as the digital input is disabled.

## 13 TECHNICAL DATA

**Housing:** self extinguishing ABS.
**Case:** frontal 32x74 mm; depth 60mm;
**Mounting:** panel mounting in a 71x29mm panel cut-out
**Protection:** IP20; **Frontal protection:** IP
**Connections:** Screw terminal block  2,5 mm^2 wiring.
**Power supply:** according to the model 230Vac 10%, 50/60Hz --- 110Vac 10%, 50/60Hz
**Power absorption:** 3 .5VA max
**Display** : 2 digits, red LED, 14,2 mm high; **Inputs** : Up to 2 NTC.
**Digital input:** free voltage contact
**Relay outputs: compressor** SPST 8(3) A, 250Vac; SPST 16(6)A 250Vac or 20(8)A 250Vac
**defrost:** SPDT 8(3) A, 250Vac
**fan:** SPST 8(3) A, 250Vac or SPST 5(2) A
**Data storing** : on the non-volatile memory (EEPROM).
**Kind of action:** 1B; **Pollution degree:** 2; **Software class:** A.;
**Rated impulsive voltage** : 2500V; **Overvoltage Category** : II
**Operating temperature:** 0÷60 °C; **Storage temperature:** - 25 ÷ 60 °C.
**Relative humidity:** 20 85% (no condensing)
**Measuring and regulation range: NTC** - 40÷110°C;
**Resolution:** 0,1 °C or 1°C or 1 °F (selectable); **Accuracy (ambient temp. 25°C)** : ±0, 1 °C ±1 digit

## 14 CONNECTIONS

### 14.1 XR06CX – 20+8+5A OR 16+8+5A – 1 10VAC OR 230VAC

**NOTE:** The compressor relay is 20(8)A or 16(6)A depending on the model.
**NOTE:** Connect the 120Vac power supply to 4 - 5

### 14.2 XR06CX -- 8+8+8A -- 110VAC OR 230VAC

**NOTE:** Connect the 120Vac power supply to 6 - 7

## 15 DEFAULT SETTING VALUES

```
LABEL DESCRIPTION RANGE DEFAULT
```
REGULATION

```
Hy Differential 0.1 ÷ 25°C/1 ÷ 45°F 2.0°C / 4 °F
```
```
LS Minimum Set Point - 55°C÷SET/-67°F÷SET - 55 °C /-
55°F
US Maximum Set Point SET÷99°C/ SET÷ 99 °F
99 °C /
99°F
ot First probe calibration - 9.9÷9.9°C/- 17 ÷1 7 °F 0.
```
```
P2 Second probe presence n – Y y
```
```
oE Second probe calibration - 9.9÷9.9°C/- 17 ÷1 7 °F 0.
```
```
od Outputs activation delay at start up 0 ÷ 99 min 0
```
```
AC Anti-short cycle delay 0 ÷ 50 min 1
```
Cy (^) Compressor ON time faulty probe 0 ÷ 99 min 15
Cn Compressor OFF time faulty probe 0 ÷ 99 min 30
DISPLAY
CF Measurement units °C - °F °C / °F
rE Resolution (only for °C) dE – in dE
Ld Default Display P1 - P2 - SP P
dy Display delay 0 ÷ 15 min 0
DEFROST
td (^) Defrost type EL – in EL
dE Defrost termination temperature - 55 ÷50°C/- 67 ÷ 99 °F
8.0 °C / 46
°F
id Interval between defrost cycles 0 ÷ 99 hours 6
Md Maximum length for defrost 0 ÷ 99 min. 30
dd Start defrost delay 0 ÷ 99 min. 0
dF Display during defrost rt – in – SP – dF it
dt Drip time 0 ÷ 99 min 0
dP Defrost at power-on y - n n
FANS
FC Fans operating mode cn – on – cY – oY on
Fd Fans delay after defrost 0 ÷ 99 min 10
FS (^) Fans stop temperature - 55 ÷50°C/- 67 ÷ 99 °F 2.0 °C / 36
°F
ALARMS
AU Maximum temperature alarm ALL÷99°C / ALL÷ 99 °F

#### 99 °C / 99

#### °F

```
AL Minimum temperature alarm
```
#### - 55°C÷ALU/-

#### 67°F÷ALU

#### - 55 °C / -

#### 55 °F

```
Ad Temperature alarm delay 0 ÷ 99 min 15
```
```
dA
Exclusion of temperature alarm at
startup
0 ÷ 9 9 min 90
```
```
DIGITAL INPUT
```
```
iP Digital input polarity cL – oP cL
```
```
iF Digital input configuration EA^ –^ bA^ –^ do^ –^ dF –^ Au^
```
- Hc
    EA

```
di Digital input delay 0 ÷ 99 min 5
```
```
dC Compressor and fan status when open
door
no /Fn / cP / Fc FC
```
```
rd Regulation with door open n - Y y
```
```
OTHER
```
```
d1 Thermostat probe display Read Only - - -
```
```
d2 Evaporator probe display Read Only - - -
```
```
Pt Parameter code table Read Only - - -
```
```
rL Firmware release Read Only - - -
```

