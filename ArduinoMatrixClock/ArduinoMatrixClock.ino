/*
Name:		ArduinoMatrixClock.ino
Created:	16.01.2018 20:56:49
Last rev.:	21.04.2019
Version:	1.3
Author:		Petan (www.mylms.cz)
*/

/*
https://www.mylms.cz/text-arduino-hodiny-s-maticovym-displejem/

https://github.com/mylms/Arduino-Matrix-Clock

D2 - BTN 1 (set internal_pullup)
D3 – BTN 2 (set internal_pullup)
D4 – matrix display, pin DIN
D5 – matrix display, pin CLK
D6 – matrix display, pin CS
A4 – RTC module, pin SDA
A5 – RTC module, pin SCL
GND – common for all modules
5V – common for all modules, 5V supply connected via 1N4148


ISSUES:
If clock shows 45 hours, check your RTC module (check address, change battery, check wires, or at last try to change RTC module).


HOW TO USE
Press both buttons at the same time, then release. Now, you are in menu mode.
By pressing BTN1 you change menu item, By pressing BTN2 change value of selected item.
H = hour, M = minute,
y = year, m = month, d = day,
AM/PM = time format 12/24 h, 
F = font, : = dots, B = brightness, RI = rotate font 1, RII = rotate font 2, U = rotate font upside down
Strt = start (second are set to 0 after press the button)


SERIAL COMMUNICATION (9600b)
You have to send three chars. 1st is function, other two are digits
XNN -> X = function; NN = number 00 to 99 (two digits are nessesary)
Command is case sensitive!! R01 and r01 are different commands!

y = year (00 - 99)
m = month (1 - 12)
d = day (1 - 31)
w = day of week (1 - 7)

H = hour (0 - 23)
M = minute (0 - 59)
S = second (0 - 59)

D = show date (how many second per minute is date shown 00 = no, 60 = always; date is shown last xx second)
t = show time (how many second per minute is temperature shown 00 = no, 60 = always; temperature is shown last xx second)
R = rotate font 1
r = rotate font 2
U = rotate font UpsideDown
b = brightness (0 - 15)
f = font (1 - 5)
/ = 12/24 hour format (/00 = 12h; /01 = 24h)
: = dot style (:00 = not shown; :01 = always lit; :02 = blinking)

*/

#include <EEPROM.h>
#include <Wire.h>
#include <LedControl.h>

//OTHERS
const byte versionMajor = 1;
const byte versionMinor = 3;

//you can change this value to change shown temperature - it's only temperature offset
//Example: Real temperature is 23°C. Clock shows 26°C. Difference is -3°C. You have to change value to 100 - 3 = 97
const byte temperatureOffset = 97;	//99 = -1, 100 = 0, 101 = 1


//MATRIX DISPLAY
byte devices = 4;	//count of displays
LedControl lc = LedControl(4, 5, 6, devices);	//DIN, CLK, CS, count of displays

//RTC DS3231
//How to read time from RTC without library, see https://www.mylms.cz/text-kusy-kodu-k-arduinu/#ds3231
#define DS3231_I2C_ADDRESS 0x68 //address of DS3231 module
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; //global variables for time
byte currentTemperatureH = 0;	//temperature in degC
byte currentTemperatureL = 0;	//255 = 0.75, 128 = 0.5, 64 = 0,25, other = 0.0

//TIMMING
unsigned long presentTime;
unsigned long displayTime;	//drawing
unsigned long temperatureTime;	//gat temp

//IO
#define BTN1 2
#define BTN2 3
bool lastInput1; //last state of #1 input 
bool lastInput2; //last state of #2 input 
bool presentInput1; //actual state of input #1
bool presentInput2; //actual state of input #2

//SYSTEM STATE
byte systemState;	//0 = show time/date/temp, other = menu
byte showMode = 0;	//0 = time, 1 = date, 2 = temperature
bool showDots;	//dots are shown
bool pmDotEnable = false;	//pm dot is shown

//chars
const uint64_t symbols[] = {
	0x0000000000000000,	//space
	0x0018180000181800,	//: - dots (in menu)
	0x003b66663e060607,	//b - backlight
	0x000f06060f06361c,	//f - font
	0x006666667e666666,	//H - hour
	0x007f66460606060f,	//L - "LMS"
	0x0063636b7f7f7763,	//M - minute
	0x003c66701c0e663c,	//S - "Strt"
	0x00182c0c0c3e0c08,	//t - temperature
	0x000f06666e3b0000,	//r - Rotate II, "Strt"
	0x006e33333e303038,	//d (10) - day
	0x006766363e66663f,	//R - Rotate I
	0x003c66030303663c,	//C - celsius
	0x01f204c813204180,	//12/24 - "/" - time mode
	0x000c1e3333330000,	//v - version
	0x003e776363636363,	//U - upside down
	0x1f303e3333330000,	//y - year
	0x00636b7f7f330000,	//m - month
	0x001f36666666361f,	//D - show date
	0x000f06161e16467f,	//F - reserve...maybe for Fahrenheit
	0x003e676f7b73633e,	//font #1 - 0
	0x007e181818181c18,
	0x007e660c3860663c,
	0x003c66603860663c,
	0x0078307f33363c38,
	0x003c6660603e067e,
	0x003c66663e060c38,
	0x001818183060667e,
	0x003c66663c66663c,
	0x001c30607c66663c,
	0x003c66666e76663c,	//font #2 - 0
	0x007e1818181c1818,
	0x007e060c3060663c,
	0x003c66603860663c,
	0x0030307e32343830,
	0x003c6660603e067e,
	0x003c66663e06663c,
	0x001818183030667e,
	0x003c66663c66663c,
	0x003c66607c66663c,
	0x1c2222222222221c,	//font #3 - 0
	0x1c08080808080c08,
	0x3e0408102020221c,
	0x1c2220201820221c,
	0x20203e2224283020,
	0x1c2220201e02023e,
	0x1c2222221e02221c,
	0x040404081020203e,
	0x1c2222221c22221c,
	0x1c22203c2222221c,
	0x001c22262a32221c,	//font #4 - 0
	0x003e080808080c08,
	0x003e04081020221c,
	0x001c22201008103e,
	0x0010103e12141810,
	0x001c2220201e023e,
	0x001c22221e020418,
	0x000404040810203e,
	0x001c22221c22221c,
	0x000c10203c22221c,
	0x003c24242424243c,	//font #5 - 0
	0x0020202020202020,
	0x003c04043c20203c,
	0x003c20203c20203c,
	0x002020203c242424,
	0x003c20203c04043c,
	0x003c24243c04043c,
	0x002020202020203c,
	0x003c24243c24243c,
	0x003c20203c24243c
};

byte symbolsLenght = sizeof(symbols) / 80;	//count numbers of symbols
byte fontOffset = 20;	//count of symbols before 1st number (only 10, 20, 30, ...)

//default values...but they are load from EEPROM
byte bright = 7;
byte font = 1;	//do not set less than 1. symbols 0-19 are used for letters etc.
byte dotStyle = 2;	//0 - off, 1 - on, 2 - blinking
byte timeMode1224 = 1;	//12/24 hour mode 12 = 0, 24 = 1
byte rotateFont1 = 0;	//font rotateing verticaly (one char)
byte rotateFont2 = 0;	//font rotateing verticaly (all display)
byte showDate = 0;	//how many second in one minute cycle is date shown
byte showTemperature = 0;	//how many second in one minute cycle is temperature shown
byte upsideDown = 0;	//us font Upside down


void setup() {
	//COMMUNICATION
	Wire.begin(); //start I2C communication
	Serial.begin(9600);	//for communication with PC

	//IO
	//there is internal pull-up used - read variables are negated
	pinMode(BTN1, INPUT_PULLUP);
	pinMode(BTN2, INPUT_PULLUP);

	bright = EEPROM.read(0);	//load light intensity from EEPROM
	if (bright < 0 || bright > 15) {
		//in case variable out of range
		bright = 7;
	}

	font = EEPROM.read(1);	//load font style from EEPROM
	if (font < 1 || font > symbolsLenght - (fontOffset/10)) {
		//in case variable out of range
		font = 1;
	}

	dotStyle = EEPROM.read(2);	//load dot style from EEPROM
	if (dotStyle < 0 || dotStyle > 2) {
		//in case variable out of range
		dotStyle = 2;
	}

	showTemperature = EEPROM.read(3);	//load temperature time EEPROM
	if (showTemperature < 0 || showTemperature > 60) {
		//in case variable out of range
		showTemperature = 0;
	}

	rotateFont1 = EEPROM.read(4);	//load rotate font 1 from EEPROM
	if (rotateFont1 < 0 || rotateFont1 > 1) {
		//in case variable out of range
		rotateFont1 = 0;
	}

	rotateFont2 = EEPROM.read(5);	//load rotate font 2 from EEPROM
	if (rotateFont2 < 0 || rotateFont2 > 1) {
		//in case variable out of range
		rotateFont2 = 0;
	}

	timeMode1224 = EEPROM.read(6);	//load 12/24 hour mode from EEPROM
	if (timeMode1224 < 0 || timeMode1224 > 1) {
		//in case variable out of range
		timeMode1224 = 1;
	}

	showDate = EEPROM.read(7);	//load date time
	if (showDate < 0 || showDate > 60) {
		//in case variable out of range
		showDate = 0;
	}

	upsideDown = EEPROM.read(8);	//load upsideDown state from EEPROM
	if (upsideDown < 0 || upsideDown > 1) {
		//in case variable out of range
		upsideDown = 0;
	}

	delay(10);	//just small delay...I thing I have had add it for correct function of display

	//SET ALL DISPLAYS
	for (byte address = 0; address<devices; address++) {
		lc.shutdown(address, false);	//powersaving
		lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
		lc.clearDisplay(address);		//clear display
	}

	/*
	//INIT TIME SETTING
	//You can use this for first setup
	//Do not forget deactivate it!
	SetRtc(15, 41, 8, 6, 30, 3, 18);	//sec, min, hour, dayOfWeek, dayOfMonth, month, year
	*/

	Intro();	//show LMS and version
	Serial.println("Send '?' for help");

	GetTemperature();	//just for start
}

void loop() {
	//store input to variarbe
	presentInput1 = digitalRead(BTN1);
	presentInput2 = digitalRead(BTN2);

	switch (systemState) {
	case 0:
		//SHOW ACTUAL TIME
		presentTime = millis();

		//30 second timer
		if (presentTime - temperatureTime >= 30000) {
			temperatureTime = presentTime;
			GetTemperature();
		}

		//0.5 second trigger
		if (presentTime - displayTime >= 500) {
			displayTime = presentTime;

			GetRtc();		//get actual time
			WriteTime();	//write actual time (etc) to matrix display
		}

		if (!presentInput1 && !presentInput2) {
			systemState = 1;
			//go to "pre"menu
		}
		break;

	case 1:
		if (presentInput1 && presentInput2) {
			//NEXT
			GetRtc();		//get actual time (read time in 24h format - in menu is always 24h time format)
			systemState++; //Go to menu
			DrawSymbol(3, 0);	//space
			DrawSymbol(2, 0);	//space
			DrawSymbol(1, 0);	//space
			DrawSymbol(0, 4);	//H
		}
		break;

	case 2:
		//menu 1
		//set HOURS
		WriteTime();

		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 6);	//M
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, 0);	//space
				DrawSymbol(0, 0);	//space
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add hour
				hour++;
				if (hour > 23) {
					hour = 0;
				}
			}
		}
		break;

	case 3:
		//menu 3
		//set MINUTES
		WriteTime();

		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 16);	//y
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (year / 10) + fontOffset);	//actual font
				DrawSymbol(0, (year % 10) + fontOffset);	//actual font
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add minutes
				minute++;
				if (minute > 59) {
					minute = 0;
				}
			}
		}
		break;

	case 4:
		//menu 4
		//set YEAR
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 17);	//m
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (month / 10) +  fontOffset);	//actual font
				DrawSymbol(0, (month % 10) +  fontOffset);	//actual font
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add year
				year++;
				if (year > 50) {
					year = 19;
				}

				DrawSymbol(3, 16);	//y
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (year / 10) + fontOffset);	//actual font
				DrawSymbol(0, (year % 10) + fontOffset);	//actual font

				delay(25);
			}
		}
		break;

	case 5:
		//menu 5
		//set MONTH
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 10);	//d
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dayOfMonth / 10) + fontOffset);	//actual font
				DrawSymbol(0, (dayOfMonth % 10) + fontOffset);	//actual font
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add month
				month++;
				if (month > 12) {
					month = 1;
				}

				DrawSymbol(3, 6);	//M
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (month / 10) + fontOffset);	//actual font
				DrawSymbol(0, (month % 10) + fontOffset);	//actual font

				delay(25);
			}
		}
		break;

	case 6:
		//menu 6
		//set DAY
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 13);	//12/24
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual time mode
				DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual time mode
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add day of month
				dayOfMonth++;

				if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
					if (dayOfMonth > 31) {
						dayOfMonth = 1;
					}
				}

				if (month == 4 || month == 6|| month == 9 || month == 11) {
					if (dayOfMonth > 30) {
						dayOfMonth = 1;
					}
				}

				if (month == 2) {
					if (CheckLeapYear(year)) {
						//leap year
						if (dayOfMonth > 29) {
							dayOfMonth = 1;
						}
					}
					else {
						if (dayOfMonth > 28) {
							dayOfMonth = 1;
						}
					}
				}

				DrawSymbol(3, 10);	//d
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dayOfMonth / 10) + fontOffset);	//actual day
				DrawSymbol(0, (dayOfMonth % 10) + fontOffset);	//actual day

				delay(25);
			}
		}
		break;

	case 7:
		//menu 7
		//set 12/24 MODE
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 18);	//date
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (showDate / 10) + fontOffset);	//actual show date time
				DrawSymbol(0, (showDate % 10) + fontOffset);	//actual show date time
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set 12/24 mode
				timeMode1224++;
				if (timeMode1224 > 1) {
					timeMode1224 = 0;
				}

				DrawSymbol(3, 13);	//12/24
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual time mode
				DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual time mode

				delay(25);
			}
		}
		break;

	case 8:
		//menu 8
		//show show DATE
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 8);	//t - temperature
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (showTemperature / 10) + fontOffset);	//actual show temp time
				DrawSymbol(0, (showTemperature % 10) + fontOffset);	//actual show temp time
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set show date
				showDate++;
				if (showDate > 60) {
					showDate = 0;
				}

				DrawSymbol(3, 18);	//date
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (showDate / 10) + fontOffset);	//actual show date time
				DrawSymbol(0, (showDate % 10) + fontOffset);	//actual show date time

				delay(25);
			}
		}
		break;

	case 9:
		//menu 9
		//show TEMPERATURE
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 3);	//F
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (font / 10) + (font * 10) + fontOffset - 10);	//actual font
				DrawSymbol(0, (font % 10) + (font * 10) + fontOffset - 10);	//actual font
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set show temperature
				showTemperature++;
				if (showTemperature > 60) {
					showTemperature = 0;
				}

				DrawSymbol(3, 8);	//t - temperature
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (showTemperature / 10) + fontOffset);	//actual show temp time
				DrawSymbol(0, (showTemperature % 10) + fontOffset);	//actual show temp time

				delay(25);
			}
		}
		break;

	case 10:
		//menu 10
		//set FONT
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 1);	//:
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dotStyle / 10) + fontOffset);	//actual dot style
				DrawSymbol(0, (dotStyle % 10) + fontOffset);	//actual dot style
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set font
				font++;
				if (font > symbolsLenght - (fontOffset / 10)) {
					font = 1;
				}

				DrawSymbol(3, 3);	//F
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (font / 10) + (font * 10) + fontOffset - 10);	//actual font
				DrawSymbol(0, (font % 10) + (font * 10) + fontOffset - 10);	//actual font

				delay(25);
			}
		}
		break;

	case 11:
		//menu 11
		//set DOT STYLE
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 2);	//B
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (bright / 10) + fontOffset);	//actual light intensity
				DrawSymbol(0, (bright % 10) + fontOffset);	//actual light intensity
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set dot style
				dotStyle++;
				if (dotStyle > 2) {
					dotStyle = 0;
				}

				DrawSymbol(3, 1);	//:
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dotStyle / 10) + fontOffset);	//actual dot style
				DrawSymbol(0, (dotStyle % 10) + fontOffset);	//actual dot style

				delay(25);
			}
		}
		break;

	case 12:
		//menu 12
		//set BRIGHTNES
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 11);	//R
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (rotateFont1 / 10) + fontOffset);	//actual rotate font 1
				DrawSymbol(0, (rotateFont1 % 10) + fontOffset);	//actual rotate font 1
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add brightness
				bright++;
				if (bright > 15) {
					bright = 0;			
				}
				
				DrawSymbol(3, 2);	//B
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (bright / 10) + fontOffset);	//actual light intensity
				DrawSymbol(0, (bright % 10) + fontOffset);	//actual light intensity

				for (byte address = 0; address<devices; address++) {
					lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
				}

				delay(25);
			}
		}
		break;

	case 13:
		//menu 13
		//set rotate FONT 1
		//Rotate each symbol separately (vertical)
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 9);	//r
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (rotateFont2 / 10) + fontOffset);	//actual rotate font 2
				DrawSymbol(0, (rotateFont2 % 10) + fontOffset);	//actual rotate font 2
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set font rotateing
				rotateFont1++;
				if (rotateFont1 > 1) {
					rotateFont1 = 0;
				}

				DrawSymbol(3, 11);	//R
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (rotateFont1 / 10) + fontOffset);	//actual rotate font 1
				DrawSymbol(0, (rotateFont1 % 10) + fontOffset);	//actual rotate font 1

				delay(25);
			}
		}
		break;

	case 14:
		//menu 14
		//set rotate FONT 2
		//Rotate all display (verticaly)
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 15);	//U
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (upsideDown / 10) + fontOffset);	//actual rotate font 2
				DrawSymbol(0, (upsideDown % 10) + fontOffset);	//actual rotate font 2
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set font rotateing
				rotateFont2++;
				if (rotateFont2 > 1) {
					rotateFont2 = 0;
				}

				DrawSymbol(3, 9);	//r
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (rotateFont2 / 10) + fontOffset);	//actual rotate font 2
				DrawSymbol(0, (rotateFont2 % 10) + fontOffset);	//actual rotate font 2

				delay(25);
			}
		}
		break;

	case 15:
		//menu 15
		//set rotate upsidedown
		//Rotate all display (horizontaly)
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 7);	//S
				DrawSymbol(2, 8);	//t
				DrawSymbol(1, 9);	//r
				DrawSymbol(0, 8);	//t
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set font rotateing
				upsideDown++;
				if (upsideDown > 1) {
					upsideDown = 0;
				}

				DrawSymbol(3, 15);	//U
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (upsideDown / 10) + fontOffset);	//actual rotate font 2
				DrawSymbol(0, (upsideDown % 10) + fontOffset);	//actual rotate font 2

				delay(25);
			}
		}
		break;

	case 16:
		//menu 16
		//EXIT
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected
				//because this clock do not have date (calendar) this clock do not set days, monts etc.
				//but you can set these variables via serial port
				SetRtc(0, minute, hour, dayOfWeek, dayOfMonth, month, year);	//set time and zero second

				EEPROM.write(0, bright);	//store actual light intensity to addr 0
				EEPROM.write(1, font);	//store actual font to addr 1
				EEPROM.write(2, dotStyle);	//store actual font to addr 2
				EEPROM.write(3, showTemperature);	//store temperature time to addr 3
				EEPROM.write(4, rotateFont1);	//store rotate font1 to addr 4
				EEPROM.write(5, rotateFont2);	//store rotate font2 to addr 5
				EEPROM.write(6, timeMode1224);	//store 12/24 mode to addr 6
				EEPROM.write(7, showDate);	//store date time to eeprom
				EEPROM.write(8, upsideDown);	//store upsideDown rotate to EEPROM

				systemState = 0;	//show actual time
			}
		}
		break;
	}

	lastInput1 = presentInput1; //save current state to last state
	lastInput2 = presentInput2; //save current state to last state

	SerialComm();	//read data from PC
}

void WriteTime() {
	byte storedFont = font;	//store actual seting during MENU

	if (systemState > 0) {
		//reserve font for menu
		font = 1;
	}

	if (second == 0) {
		//show time in 0 second
		showMode = 0;
	}

	if (second == showDate && showDate > 0) {
		//showdate is now & showdate is enabled 
		showMode = 1;
	}

	if (second == showTemperature && showTemperature > 0) {
		//showtemp is now & showtemp is enabled 
		showMode = 2;
	}

	if (showTemperature == 60) {
		//salways howtemp
		showMode = 2;
	}

	if (showDate == 60) {
		//always show date
		//date has priority
		showMode = 1;
	}

	//write time to matrix display in menu
	if (systemState == 2) {
		//show hours in 24h format in menu (set hours)
		DrawSymbol(2, (hour % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(3, (hour / 10) + (font * 10) + fontOffset - 10);

		showDots = false;	//hide dots (colon)
		pmDotEnable = false;	//hide PM dot
	}

	//write time to matrix display in menu
	if (systemState == 3) {
		//show minute in menu
		DrawSymbol(0, (minute % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(1, (minute / 10) + (font * 10) + fontOffset - 10);
	}

	if (systemState == 0) {
		//show data in normal state
		switch (showMode) {
		case 0:
			//time
			if (timeMode1224 == 0) {
				//12h format
				if (hour >= 0 && hour <= 11) {
					pmDotEnable = false;
				}

				if (hour == 12) {
					pmDotEnable = true;
				}

				if (hour == 0) {
					hour = 12;
				}

				if (hour > 12) {
					hour -= 12;
					pmDotEnable = true;
				}
			}
			else {
				//24h format
				pmDotEnable = false;
			}

			//show dots (colon)
			if (systemState == 0) {
				//TIME
				switch (dotStyle) {
				case 0:
					showDots = false;	//hide dots
					break;
				case 1:
					showDots = true;	//show dots
					break;
				case 2:
					showDots = !showDots;	//blinking
					break;
				}
			}

			//hour
			DrawSymbol(2, (hour % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(3, (hour / 10) + (font * 10) + fontOffset - 10);

			//minute
			DrawSymbol(0, (minute % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(1, (minute / 10) + (font * 10) + fontOffset - 10);
			break;
		case 1:
			//date
			//day (instead of hour)
			DrawSymbol(2, (dayOfMonth % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(3, (dayOfMonth / 10) + (font * 10) + fontOffset - 10);

			//month (instead of minute)
			DrawSymbol(0, (month % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(1, (month / 10) + (font * 10) + fontOffset - 10);

			showDots = false;	//hide dots (colon)
			pmDotEnable = false;	//hide PM dot
			break;
		case 2:
			//temperature
			DrawSymbol(2, (currentTemperatureH % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(3, (currentTemperatureH / 10) + (font * 10) + fontOffset - 10);

			switch (currentTemperatureL) {
			case 255:
				DrawSymbol(1, 8 + (font * 10) + fontOffset - 10);
				break;
			case 128:
				DrawSymbol(1, 5 + (font * 10) + fontOffset - 10);
				break;
			case 64:
				DrawSymbol(1, 3 + (font * 10) + fontOffset - 10);
				break;
			default:
				DrawSymbol(1, 0 + (font * 10) + fontOffset - 10);
				break;
			}

			DrawSymbol(0, 12);	//draw "C" symbol
			showDots = false;	//hide dots (colon)
			pmDotEnable = false;	//hide PM dot
			break;
		}

	}

	font = storedFont;
}

void Intro() {
	DrawSymbol(3, 5);	//L
	DrawSymbol(2, 6);	//M
	DrawSymbol(1, 7);	//S
	DrawSymbol(0, 0);	//space

	delay(2000);

	//version of fw
	DrawSymbol(3, 14);	//v
	DrawSymbol(2, versionMajor + fontOffset);	//1
	DrawSymbol(1, 1);	//:
	DrawSymbol(0, versionMinor + fontOffset);	//3

	delay(2000);
}

void DrawSymbol(byte adr, byte symbol) {
	//draw symbol
	//adr - used part of display
	if (rotateFont2 == 1) {
		adr = 3 - adr;
	}

	byte j = 0;	//variable for upsidedown font turning

	for (byte i = 0; i < 8; i++) {
		j = i;
		if (upsideDown == 1) {
			//turn font upside down
			j = 7 - i;
		}
		
		byte row = (symbols[symbol] >> i * 8) & 0xFF;	//just some magic
		lc.setRow(adr, j, ByteRevers(row));

		//blinking dots on display
		//I have to draw "dots" during draw symbol. In other case it's blinking.
		//Better variant would update symbol before draw - before FOR structure. Maybe in next version :)
		if (adr == 2 && dotStyle > 0){
			//colon
			if (i == 1) lc.setLed(adr, 1, 7, showDots);  //addr, row, column
			if (i == 2) lc.setLed(adr, 2, 7, showDots);
			if (i == 5) lc.setLed(adr, 5, 7, showDots);
			if (i == 6) lc.setLed(adr, 6, 7, showDots);
		}
		
		if (adr == 2 && systemState == 0 && (showMode == 1 || showMode == 2)) {
			//date and temperature point
			if (i == 5) lc.setLed(2, 5, 7, true);
			if (i == 6) lc.setLed(2, 6, 7, true);
		}

		if (adr == 0) {
			//PM point
			lc.setLed(0, 7, 7, pmDotEnable);
		}
	}
}

byte ByteRevers(byte in) {
	//font rotateing
	if (rotateFont1 == 1) {
		//do not rotate
		return(in);
	}

	byte out;
	out = 0;
	if (in & 0x01) out |= 0x80;
	if (in & 0x02) out |= 0x40;
	if (in & 0x04) out |= 0x20;
	if (in & 0x08) out |= 0x10;
	if (in & 0x10) out |= 0x08;
	if (in & 0x20) out |= 0x04;
	if (in & 0x40) out |= 0x02;
	if (in & 0x80) out |= 0x01;
	return(out);
}

//Set RTC
void SetRtc(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {	
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); //set 0 to first register

	Wire.write(decToBcd(second)); //set second
	Wire.write(decToBcd(minute)); //set minutes 
	Wire.write(decToBcd(hour)); //set hours
	Wire.write(decToBcd(dayOfWeek)); //set day of week (1=su, 2=mo, 3=tu) 
	Wire.write(decToBcd(dayOfMonth)); //set day of month
	Wire.write(decToBcd(month)); //set month
	Wire.write(decToBcd(year)); //set year
	Wire.endTransmission();
}

//read RTC
void GetRtc() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); //write "0"
	Wire.endTransmission();

	Wire.requestFrom(DS3231_I2C_ADDRESS, 7);	//request - 7 bytes from RTC
	second = bcdToDec(Wire.read() & 0x7f);
	minute = bcdToDec(Wire.read());
	hour = bcdToDec(Wire.read() & 0x3f);
	dayOfWeek = bcdToDec(Wire.read());
	dayOfMonth = bcdToDec(Wire.read());
	month = bcdToDec(Wire.read());
	year = bcdToDec(Wire.read());
}

void GetTemperature() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0x11);
	Wire.endTransmission();

	Wire.requestFrom(DS3231_I2C_ADDRESS, 2);	//request - 2 bytes from RTC
	if (Wire.available()) {
		currentTemperatureH = Wire.read();	//temperature
		currentTemperatureL = Wire.read();	//decimals

		currentTemperatureH = currentTemperatureH + temperatureOffset - 100;
	}
	else {
		currentTemperatureH = 99; //error value
		currentTemperatureL = 0;
	}
}

//conversion Dec to BCD 
byte decToBcd(byte val) {
	return((val / 10 * 16) + (val % 10));
}

//conversion BCD to Dec 
byte bcdToDec(byte val) {
	return((val / 16 * 10) + (val % 16));
}

//serial communication with PC
void SerialComm() {
	//first char - type of data
	//second and third char - data
	//there are used only "printable" characters

	if (Serial.available() > 0) {
		byte receivedCommand;
		receivedCommand = Serial.read();	//read first char

		delay(10);	//wait for other char

		byte receivedDataTens;
		receivedDataTens = Serial.read();
		receivedDataTens -= 48;	// ASCII code for "0" is 48

		byte receivedDataOnes;
		receivedDataOnes = Serial.read();
		receivedDataOnes -= 48;	// ASCII code for "0" is 48

		byte receivedData;
		receivedData = (receivedDataTens * 10) + receivedDataOnes;
		if (receivedData > 99) {
			//maximal value is 99
			receivedData = 0;	//value is out of range
		}

		switch (receivedCommand) {
		case 47:
			//12/24 47 = /
			if (receivedData > 1) {
				receivedData = 1;
			}
			timeMode1224 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(6, timeMode1224);	//save
			break;
		case 58:
			//dot style 58 = :
			if (receivedData > 2) {
				receivedData = 2;
			}
			dotStyle = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(2, dotStyle);	//save
			break;
		case 68:
			//show date 68 = D
			if (receivedData > 60) {
				receivedData = 0;
			}
			showDate = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(7, showDate);	//save
			break;
		case 72:
			//hour 72 = H
			if (receivedData > 23) {
				receivedData = 23;
			}
			hour = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 77:
			//minute 77 = M
			if (receivedData > 59) {
				receivedData = 59;
			}
			minute = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 83:
			//second 83 = S
			if (receivedData > 59) {
				receivedData = 59;
			}
			second = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 82:
			//Rotate font 1 82 = R
			if (receivedData > 1) {
				receivedData = 0;
			}
			rotateFont1 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(4, rotateFont1);	//save
			break;
		case 85:
			//UpsideDown 1 85 = U
			if (receivedData > 1) {
				receivedData = 0;
			}
			upsideDown = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(8, upsideDown);	//save
			break;
		case 98:
			//brightness 98 = b
			if (receivedData > 15) {
				receivedData = 7;
			}
			bright = receivedData;

			for (byte address = 0; address<devices; address++) {
				lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
			}

			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(0, bright);	//save
			break;
		case 100:
			//dayOfMonth 100 = d
			if (receivedData > 31) {
				receivedData = 1;
			}
			dayOfMonth = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//save
			break;
		case 102:
			//font 102 = f
			if (receivedData > 5) {
				receivedData = 1;
			}

			if (receivedData == 0) {
				receivedData = 1;
			}
			font = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(1, font);	//save
			break;
		case 109:
			//month 109 = m
			if (receivedData > 12) {
				receivedData = 1;
			}
			month = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 114:
			//rotate 2 114 = r
			if (receivedData > 1) {
				receivedData = 0;
			}
			rotateFont2 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(5, rotateFont2);	//save
			break;
		case 116:
			//temperature time 116 = t
			if (receivedData > 60) {
				receivedData = 0;
			}
			showTemperature = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(5, showTemperature);	//save
			break;
		case 119:
			//dayofWeek 119 = w
			if (receivedData > 6) {
				receivedData = 1;
			}
			dayOfWeek = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 121:
			//year 121 = y
			year = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 63:
			//get data ? = 63
			Serial.println("");
			Serial.println("https://github.com/mylms/Arduino-Matrix-Clock");
			Serial.print("v");
			Serial.print(versionMajor);
			Serial.print(".");
			Serial.print(versionMinor);
			Serial.println("");
			Serial.println("");
			Serial.println("Date (y/m/d): ");
			Serial.println(year);
			Serial.println(month);
			Serial.println(dayOfMonth);
			Serial.println(dayOfWeek);

			Serial.println("");
			Serial.println("Time (H/M/S): ");
			Serial.println(hour);
			Serial.println(minute);
			Serial.println(second);

			Serial.println("");
			Serial.println("Temperature (read only): ");
			Serial.print(currentTemperatureH);
			switch (currentTemperatureL) {
			case 255:
				Serial.println(",75");
				break;
			case 128:
				Serial.println(",5");
				break;
			case 64:
				Serial.println(",25");
				break;
			default:
				Serial.println(",0");
				break;
			}

			Serial.println("");
			Serial.println("Show date (D 00-60): ");
			Serial.println(showDate);

			Serial.println("");
			Serial.println("Show temperature (t 00-60): ");
			Serial.println(showTemperature);

			Serial.println("");
			Serial.println("Font rotate (R 00-01): ");
			Serial.println(rotateFont1);

			Serial.println("");
			Serial.println("Display rotate (r 00-01): ");
			Serial.println(rotateFont2);

			Serial.println("");
			Serial.println("UpsideDown rotate (U 00-01): ");
			Serial.println(upsideDown);

			Serial.println("");
			Serial.println("Brightness (b 00-15): ");
			Serial.println(bright);

			Serial.println("");
			Serial.println("Font (f 00-05): ");
			Serial.println(font);

			Serial.println("");
			Serial.println("12/24h mode (/ 00-01): ");
			Serial.println(timeMode1224);

			Serial.println("");
			Serial.println("Dot style (: 00-02): ");
			Serial.println(dotStyle);
			break;
		}
		//flush serial data
		Serial.flush();
	}
}

bool CheckLeapYear(int _year) {
	//return true if leap year
	if (_year % 4 != 0) return false;
	if (_year % 100 == 0 && _year % 400 != 0) return false;
	if (_year % 400 == 0) return true;
	return true;
}
