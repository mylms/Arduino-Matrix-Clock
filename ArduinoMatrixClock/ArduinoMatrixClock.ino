/*
Name:		ArduinoMatrixClock.ino
Created:	16.01.2018 20:56:49
Last rev.:	31.03.2019
Version:	1.2
Author:		Petan (www.mylms.cz)
*/

/*
https://www.mylms.cz/text-arduino-hodiny-s-maticovym-displejem/

https://github.com/mylms/Arduino-Matrix-Clock

D2 - BTN 1 (set internal_pullup)
D3 � BTN 2 (set internal_pullup)
D4 � matrix display, pin DIN
D5 � matrix display, pin CLK
D6 � matrix display, pin CS
A4 � RTC module, pin SDA
A5 � RTC module, pin SCL
GND � common for all modules
5V � common for all modules, 5V supply connected via 1N4148


NOTE I: Daylight saving currently is not used (maybe in future).
NOTE II: Calendar (year, month, day, ...) currently are not used. You can just set it.

ISSUES:
If clock shows 45 hours, check your RTC module (check address, change battery, check wires, or at last try to change RTC module).

HOW TO USE
Press both buttons at the same time, then release. Now, you are in menu mode.
By pressing BTN1 you change menu item, By pressing BTN2 change value of item.
H = hour, M = minute,
y = year, m = month, d = day,
AM/PM = time format, 
F = font, : = dots, B = brightness, TI = turn font 1, TII = turn font 2
Strt = start (second are set to 0 after press the button)


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

*/

#include <EEPROM.h>
#include <Wire.h>
#include <LedControl.h>

//MATRIX DISPLAY
byte devices = 4;	//count of displays
LedControl lc = LedControl(4, 5, 6, devices);	//DIN, CLK, CS, count of displays

//RTC DS3231
#define DS3231_I2C_ADDRESS 0x68 //address of DS3231 module
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; //global variables

//TIMMING
unsigned long presentTime;
unsigned long displayTime;	//drawing

//IO
#define BTN1 2
#define BTN2 3
bool lastInput1; //last state of #1 input 
bool lastInput2; //last state of #2 input 
bool presentInput1; //actual state of input #1
bool presentInput2; //actual state of input #2

//SYSTEM STATE
byte systemState;

//chars
const uint64_t symbols[] = {
	0x0000000000000000,	//space
	0x0018180000181800,	//:
	0x003b66663e060607,	//b
	0x000f06060f06361c,	//f
	0x006666667e666666,	//H
	0x007f66460606060f,	//L
	0x0063636b7f7f7763,	//M
	0x003c66701c0e663c,	//S
	0x00182c0c0c3e0c08,	//t
	0x000f06666e3b0000,	//r
	0x006e33333e303038,	//d (10)
	0x005e4c4c0c0c2d3f,	//T1
	0x00beacac0c0c2d3f,	//T2
	0x01f204c813204180,	//12/24
	0x000c1e3333330000,	//v
	0x0063954525956300,	//daylight saving
	0x1f303e3333330000,	//y
	0x00636b7f7f330000,	//m
	0x0002020600000600,	//reserve
	0x0002020600000e00,	//reserve
	0x003e676f7b73633e,	//font 1 - 0
	0x007e181818181c18,
	0x007e660c3860663c,
	0x003c66603860663c,
	0x0078307f33363c38,
	0x003c6660603e067e,
	0x003c66663e060c38,
	0x001818183060667e,
	0x003c66663c66663c,
	0x001c30607c66663c,
	0x003c66666e76663c,	//font 2 - 0
	0x007e1818181c1818,
	0x007e060c3060663c,
	0x003c66603860663c,
	0x0030307e32343830,
	0x003c6660603e067e,
	0x003c66663e06663c,
	0x001818183030667e,
	0x003c66663c66663c,
	0x003c66607c66663c,
	0x1c2222222222221c,	//font 3 - 0
	0x1c08080808080c08,
	0x3e0408102020221c,
	0x1c2220201820221c,
	0x20203e2224283020,
	0x1c2220201e02023e,
	0x1c2222221e02221c,
	0x040404081020203e,
	0x1c2222221c22221c,
	0x1c22203c2222221c,
	0x001c22262a32221c,	//font 4 - 0
	0x003e080808080c08,
	0x003e04081020221c,
	0x001c22201008103e,
	0x0010103e12141810,
	0x001c2220201e023e,
	0x001c22221e020418,
	0x000404040810203e,
	0x001c22221c22221c,
	0x000c10203c22221c,
	0x003c24242424243c,	//font 5 - 0
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
byte fontOffset = 20;	//count of symbols before 1st number (10, 20, 30, ...)

//default values...but they are load from EEPROM
byte bright = 7;
byte font = 1;	//do not set less than 1. symbols 0-19 are used for letters etc.
byte dotStyle = 2;	//0 - off, 1 - on, 2 - blinking
byte daylightSaving = 0;	//daylight saving is enable from 31.3. to 27.10.
byte timeMode1224 = 1;	//12/24 hour mode 12 = 0, 24 = 1
byte turnFont1 = 0;	//font turning verticaly (one char)
byte turnFont2 = 0;	//font turning verticaly (all display)

bool showDots;	//dots are shown
bool pmDotEnable = false;	//pm dot is shown
bool dayightTimeEnable = false;	//is daylight time

void setup() {
	//COMMUNICATION
	Wire.begin(); //start I2C communication
	Serial.begin(9600);	//just for receive settings from PC

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

	daylightSaving = EEPROM.read(3);	//load daylight saving from EEPROM
	if (daylightSaving < 0 || daylightSaving > 1) {
		//in case variable out of range
		daylightSaving = 1;
	}

	turnFont1 = EEPROM.read(4);	//load turn font 1 from EEPROM
	if (turnFont1 < 0 || turnFont1 > 1) {
		//in case variable out of range
		turnFont1 = 0;
	}

	turnFont2 = EEPROM.read(5);	//load turn font 2 from EEPROM
	if (turnFont2 < 0 || turnFont2 > 1) {
		//in case variable out of range
		turnFont2 = 0;
	}

	timeMode1224 = EEPROM.read(6);	//load 12/24 hour mode from EEPROM
	if (timeMode1224 < 0 || timeMode1224 > 1) {
		//in case variable out of range
		timeMode1224 = 1;
	}

	delay(10);	//just delay...I thing I have had add it for correct function of display

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
}

void loop() {
	//store input to variarbe
	presentInput1 = digitalRead(BTN1);
	presentInput2 = digitalRead(BTN2);

	switch (systemState) {
	case 0:
		//SHOW ACTUAL TIME
		presentTime = millis();

		//0.5 second trigger
		//do not afraid millis rollover
		if (presentTime - displayTime >= 500) {
			displayTime = presentTime;

			GetRtc();		//get actual time
			WriteTime();	//write actual time to matrix display
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
				if (year > 99) {
					year = 0;
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
				DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual turn font 2
				DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual turn font 2
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
				//set 12/24 mode
				timeMode1224++;
				if (timeMode1224 > 1) {
					timeMode1224 = 0;
				}

				DrawSymbol(3, 13);	//12/24
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual turn font 2
				DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual turn font 2

				delay(25);
			}
		}
		break;

	case 8:
		//menu 8
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

	case 9:
		//menu 9
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

	case 10:
		//menu 10
		//set BRIGHTNES
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 11);	//T1
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (turnFont1 / 10) + fontOffset);	//actual turn font 1
				DrawSymbol(0, (turnFont1 % 10) + fontOffset);	//actual turn font 1
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

	case 11:
		//menu 11
		//set TURN FONT 1
		//Rotate each symbol separately (vertical)
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 12);	//T2
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (turnFont2 / 10) + fontOffset);	//actual turn font 2
				DrawSymbol(0, (turnFont2 % 10) + fontOffset);	//actual turn font 2
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//set font turning
				turnFont1++;
				if (turnFont1 > 1) {
					turnFont1 = 0;
				}

				DrawSymbol(3, 11);	//T1
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (turnFont1 / 10) + fontOffset);	//actual turn font 1
				DrawSymbol(0, (turnFont1 % 10) + fontOffset);	//actual turn font 1

				delay(25);
			}
		}
		break;

	case 12:
		//menu 12
		//set TURN FONT 2
		//Rotate all display (verticaly)
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
				//set font turning
				turnFont2++;
				if (turnFont2 > 1) {
					turnFont2 = 0;
				}

				DrawSymbol(3, 12);	//T2
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (turnFont2 / 10) + fontOffset);	//actual turn font 2
				DrawSymbol(0, (turnFont2 % 10) + fontOffset);	//actual turn font 2

				delay(25);
			}
		}
		break;

	case 13:
		//menu 13
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
				EEPROM.write(3, daylightSaving);	//store daylight mode to addr 3
				EEPROM.write(4, turnFont1);	//store turn font1 to addr 4
				EEPROM.write(5, turnFont2);	//store turn font2 to addr 5
				EEPROM.write(6, timeMode1224);	//store 12/24 mode to addr 6

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
		//reserve font in menu
		font = 1;
	}

	//write time to matrix display
	if (systemState == 2) {
		//show hours in 24h format in menu (set hours)
		DrawSymbol(2, (hour % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(3, (hour / 10) + (font * 10) + fontOffset - 10);
	}

	if (systemState == 0) {
		//show hours in normal state
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
			pmDotEnable = false;
		}

		DrawSymbol(2, (hour % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(3, (hour / 10) + (font * 10) + fontOffset - 10);
	}

	if (systemState == 0 || systemState == 3) {
		//show minutes in normal state and "set minutes" state
		DrawSymbol(0, (minute % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(1, (minute / 10) + (font * 10) + fontOffset - 10);
	}

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
	else{
		//MENU
		showDots = false;	//hide dots
		pmDotEnable = false;	//hide PM Dot
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
	DrawSymbol(2, 21);	//1
	DrawSymbol(1, 1);	//:
	DrawSymbol(0, 22);	//2

	delay(2000);
}

void DrawSymbol(byte adr, byte symbol) {
	//draw symbol
	//adr - used part of display
	if (turnFont2 == 1) {
		adr = 3 - adr;
	}

	for (int i = 0; i < 8; i++) {
		byte row = (symbols[symbol] >> i * 8) & 0xFF;	//just some magic
		lc.setRow(adr, i, ByteRevers(row));

		//blinking dots on display
		//I have to draw "dots" during draw symbol. In other case it's blinking.
		//Better variant would update symbol before draw - before FOR structure. Maybe in next version :)
		if (adr == 2 && dotStyle > 0){
			if (i == 1) lc.setLed(2, 1, 7, showDots);  //addr, row, column
			if (i == 2) lc.setLed(2, 2, 7, showDots);
			if (i == 5) lc.setLed(2, 5, 7, showDots);
			if (i == 6) lc.setLed(2, 6, 7, showDots);
		}

		if (adr == 0) {
			lc.setLed(0, 7, 7, pmDotEnable);
		}
	}
}

byte ByteRevers(byte in) {
	//font turning
	if (turnFont1 == 1) {
		//do not turn
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

//conversion Dec to BCD 
byte decToBcd(byte val) {
	return((val / 10 * 16) + (val % 10));
}

//conversion BCD to Dec 
byte bcdToDec(byte val) {
	return((val / 16 * 10) + (val % 16));
}

//serial communication with PC
//set time via PC
void SerialComm() {
	//first letter - type of data
	//second and third letter - data
	//there are used only "printable" characters

	if (Serial.available() > 0) {
		byte receivedCommand;
		receivedCommand = Serial.read();	//read first letter

		delay(10);	//wait for other letters

		byte receivedDataTens;
		receivedDataTens = Serial.read();
		receivedDataTens -= 48;	// ASCII code for '0' is 48

		byte receivedDataOnes;
		receivedDataOnes = Serial.read();
		receivedDataOnes -= 48;	// ASCII code for '0' is 48

		byte receivedData;
		receivedData = (receivedDataTens * 10) + receivedDataOnes;
		if (receivedData > 99) {
			receivedData = 99;
		}

		switch (receivedCommand) {
		case 47:
			//12/24 47 = /
			if (receivedData > 1) {
				receivedData = 1;
			}
			timeMode1224 = receivedData;
			lc.setLed(2, 7, 6, true);	//show setting dot
			EEPROM.write(6, timeMode1224);	//store 12/24 mode to addr 6
			break;
		case 58:
			//dot style 58 = :
			if (receivedData > 2) {
				receivedData = 2;
			}
			dotStyle = receivedData;
			lc.setLed(2, 7, 3, true);	//show setting dot
			EEPROM.write(2, dotStyle);	//store actual font to addr 2
			break;
		case 72:
			//hour 72 = H
			if (receivedData > 23) {
				receivedData = 23;
			}
			hour = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 3, true);	//show setting dot
			break;
		case 77:
			//minute 77 = M
			if (receivedData > 59) {
				receivedData = 59;
			}
			minute = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 4, true);	//show setting dot
			break;
		case 83:
			//second 83 = S
			if (receivedData > 59) {
				receivedData = 59;
			}
			second = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(2, 7, 0, true);	//show setting dot
			break;
		case 84:
			//turn 1 84 = T
			if (receivedData > 1) {
				receivedData = 0;
			}
			turnFont1 = receivedData;
			lc.setLed(2, 7, 4, true);	//show setting dot
			EEPROM.write(4, turnFont1);	//store turn font1 to addr 4
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

			lc.setLed(2, 7, 1, true);	//show setting dot
			EEPROM.write(0, bright);	//store actual light intensity to addr 0
			break;
		case 100:
			//dayOfMonth 100 = d
			if (receivedData > 31) {
				receivedData = 1;
			}
			dayOfMonth = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 2, true);	//show setting dot
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
			lc.setLed(2, 7, 2, true);	//show setting dot
			EEPROM.write(1, font);	//store actual font to addr 1
			break;
		case 109:
			//month 109 = m
			if (receivedData > 12) {
				receivedData = 1;
			}
			month = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 1, true);	//show setting dot
			break;
		case 116:
			//turn 2 116 = t
			if (receivedData > 1) {
				receivedData = 0;
			}
			turnFont2 = receivedData;
			lc.setLed(2, 7, 5, true);	//show setting dot
			EEPROM.write(5, turnFont2);	//store turn font2 to addr 5
			break;
		case 119:
			//dayofWeek 119 = w
			if (receivedData > 6) {
				receivedData = 1;
			}
			dayOfWeek = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 6, true);	//show setting dot
			break;
		case 121:
			//year 121 = y
			year = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
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
