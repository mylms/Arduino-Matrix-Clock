# Arduino-Matrix-Clock v1.5

Simple Arduino Clock with LED matrix display 32 x 8 pix.

In this sketch are used some external libraries. You have to download and instal them:
LED CONTROL: https://github.com/wayoda/LedControl
IR REMOTE CONTROL: https://github.com/z3t0/Arduino-IRremote

For more info and discussion see https://www.mylms.cz/arduino-hodiny-s-maticovym-displejem/ You can use forum there. Please write in english.

See video: https://youtu.be/HDweqN9cDNA

In v1.5 it's possible to invert display and control clock by a IR remote control. You have to change IR Remote control codes in sketch to your codes. You can use...

You can control clock by the buttons, Serial port and/or IR remote control.

Menu:
- H: Hour
- M: Minute

- y: year
- m: month
- d: day

- /: 12/24h time format (00 = 12h time format, 01 = 24h time format)
- f: Font (show # of font in select font style 01 - 05)
- :: Dot style (0 - hide, 1 - show, 2 - blinking)
- b: Backlight intensity (00 - 15)
- D: Show date (what second is date shown; 00 = newer, 60 = always)
- t: Show temperature (what second is temperature shown 00 = newer, 60 = always)
- R: Font turning 1 (turn each character; 00 = off, 01 = on)
- r: Font turning 2 (turn all diplay; 00 = off, 01 = on)
- U: Turn font UpsideDown (00 = off, 01 = on)
- v: Vertical mode (00 - standard horizontal, 01 - vertical mode)
- i: Invert display (00 - no invert, 01 - invert display)
- Strt: Reset second (set to 0) after release the button and show actual time

You need:
- Arduino Nano
- DS3231 module
- 4x LED matrix with MAX7219 driver (used LedControl library)
- 2x button (


For use external power supply (no USB) you need:
- 1N4148 diode
- 5V power supply

![alt Arduino Matrix Clock](https://www.mylms.cz/wp-content/uploads/2018/06/arduino-matrix-clock-1.jpg)

![alt Arduino Matrix Clock - Schematics](https://www.mylms.cz/wp-content/uploads/2020/01/arduino-matrix-clock-v1-5.png)
