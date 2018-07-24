# Arduino-Matrix-Clock v1.1

Simple Arduino Clock with LED matrix display 32 x 8 pix. In this sketch is LedControl library (https://github.com/wayoda/LedControl) used. In v1.1 was add few new fonts and possibility hide/show dots.

Menu:
- H: Hours
- M: Minutes
- F: Font (show nr of font in select font style)
- :: Dot style (0 - hide, 1 - show, 2 - blinking)
- B: Backlight intensity
- Strt: Reset second after press the button and show actual time

- Arduino Nano
- DS3231 module
- 4x LED matrix with MAX7219 driver (used LedControl library)
- 2x button

For use external power supply (no USB) you need
- 1N4148 diode
- 5V power supply

![alt Arduino Matrix Clock](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-1.jpg)

![alt Arduino Matrix Clock - Schematics](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-9.png)



More info in Czech language: https://www.mylms.cz/text-arduino-hodiny-s-maticovym-displejem/

See video: https://www.youtube.com/watch?v=HScHKPGg8lE
