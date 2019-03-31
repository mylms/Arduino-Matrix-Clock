# Arduino-Matrix-Clock v1.2

Simple Arduino Clock with LED matrix display 32 x 8 pix. In this sketch is LedControl library (https://github.com/wayoda/LedControl) used.

In v1.2 are add new features and deleted "graphic fonts".

In v1.2 is easier set the clock via computer (see code).


Menu:
- H: Hours
- M: Minutes

- y: year (new)
- m: month (new)
- d: day (new)

- /: 12/24h time format (00 = 12h time format, 01 = 24h time format) (new)
- f: Font (show nr of font in select font style)
- :: Dot style (0 - hide, 1 - show, 2 - blinking)
- b: Backlight intensity
- T1: Font turning 1 (turn each character) (new)
- T2: Font turning 2 (turn all diplay) (new)
- Strt: Reset second after press the button and show actual time

You need:
- Arduino Nano
- DS3231 module
- 4x LED matrix with MAX7219 driver (used LedControl library)
- 2x button


For use external power supply (no USB) you need:
- 1N4148 diode
- 5V power supply

![alt Arduino Matrix Clock](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-1.jpg)

![alt Arduino Matrix Clock - Schematics](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-9.png)



More info in Czech language: https://www.mylms.cz/text-arduino-hodiny-s-maticovym-displejem/ You can use forum there.

See video: https://www.youtube.com/watch?v=HScHKPGg8lE
