# Arduino-Matrix-Clock v1.2

Simple Arduino Clock with LED matrix display 32 x 8 pix. In this sketch is LedControl library (https://github.com/wayoda/LedControl) used.
In v1.2 are add new features and deleted "graphic fonts".
In v1.2 is easier set the clock via computer.

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

- Arduino Nano
- DS3231 module
- 4x LED matrix with MAX7219 driver (used LedControl library)
- 2x button

SERIAL COMMUNICATION (9600b)
You have to send three chars. 1st is function, other two are digits
XDD -> X = function; DD = number 00 to 99 (two digits are nessesary)
Command is case sensitive!! M01 and m01 are different commands.

y = year (0 - 99)
m = month (1 - 12)
d = day (1 - 31)
w = day of week (1 - 7)

H = hour (0 - 23)
M = minute (0 - 59)
S = second (0 - 59)

T = turn font 1
t = turn font 2
b = brightness (0 - 15)
f = font (1 - 5)
/ = 12/24 hour format (/00 = 12h; /01 = 24h)
: = dot style (:00 = not shown; :01 = always shining; :02 = blinking)


For use external power supply (no USB) you need
- 1N4148 diode
- 5V power supply

![alt Arduino Matrix Clock](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-1.jpg)

![alt Arduino Matrix Clock - Schematics](https://www.mylms.cz/obrazky/elektronika/arduino-matrix-clock-9.png)



More info in Czech language: https://www.mylms.cz/text-arduino-hodiny-s-maticovym-displejem/ You can use forum there.

See video: https://www.youtube.com/watch?v=HScHKPGg8lE
