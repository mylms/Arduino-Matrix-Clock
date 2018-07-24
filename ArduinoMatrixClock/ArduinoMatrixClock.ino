/*
Name:		PixelClock.ino
Created:	16.01.2018 20:56:49
Last rev.:	24.07.2018 
Version:	1.1
Author:	Petr
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
*/

#include <EEPROM.h>
#include <Wire.h>
#include <LedControl.h>

//MATRIX DISPLAY
byte devices = 4;
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
	0x0000000000000000,
	0x0018180000181800,
	0x003f66663e66663f,
	0x000f06161e16467f,
	0x006666667e666666,
	0x007f66460606060f,
	0x0063636b7f7f7763,
	0x003c66701c0e663c,
	0x00182c0c0c3e0c08,
	0x000f06666e3b0000,
	0x003e676f7b73633e,
	0x007e181818181c18,
	0x007e660c3860663c,
	0x003c66603860663c,
	0x0078307f33363c38,
	0x003c6660603e067e,
	0x003c66663e060c38,
	0x001818183060667e,
	0x003c66663c66663c,
	0x001c30607c66663c,
	0x003c66666e76663c,
	0x007e1818181c1818,
	0x007e060c3060663c,
	0x003c66603860663c,
	0x0030307e32343830,
	0x003c6660603e067e,
	0x003c66663e06663c,
	0x001818183030667e,
	0x003c66663c66663c,
	0x003c66607c66663c,
	0x1c2222222222221c,
	0x1c08080808080c08,
	0x3e0408102020221c,
	0x1c2220201820221c,
	0x20203e2224283020,
	0x1c2220201e02023e,
	0x1c2222221e02221c,
	0x040404081020203e,
	0x1c2222221c22221c,
	0x1c22203c2222221c,
	0x001c22262a32221c,
	0x003e080808080c08,
	0x003e04081020221c,
	0x001c22201008103e,
	0x0010103e12141810,
	0x001c2220201e023e,
	0x001c22221e020418,
	0x000404040810203e,
	0x001c22221c22221c,
	0x000c10203c22221c,
	0x003c24242424243c,
	0x0020202020202020,
	0x003c04043c20203c,
	0x003c20203c20203c,
	0x002020203c242424,
	0x003c20203c04043c,
	0x003c24243c04043c,
	0x002020202020203c,
	0x003c24243c24243c,
	0x003c20203c24243c,
	0x0000ff0000ff0000,
	0x00003fc0c03f0000,
	0x0000cf3030cf0000,
	0x00000ff0f00f0000,
	0x0000f30c0cf30000,
	0x000033cccc330000,
	0x0000c33c3cc30000,
	0x000003fcfc030000,
	0x0000fc0303fc0000,
	0x00003cc3c33c0000,
	0x0000000000000000,
	0x1818000000000000,
	0x0000181800000000,
	0x1818181800000000,
	0x0000000018180000,
	0x1818000018180000,
	0x0000181818180000,
	0x1818181818180000,
	0x0000000000001818,
	0x1818000000001818,
	0x0000000000000000,
	0x0000001818000000,
	0x030300000000c0c0,
	0x030300181800c0c0,
	0xc3c300000000c3c3,
	0xc3c300181800c3c3,
	0xdbdb00000000dbdb,
	0xdbdb00181800dbdb,
	0xdbdb00c3c300dbdb,
	0xdbdb00dbdb00dbdb
};

byte symbolsLenght = sizeof(symbols) / 80;	//count numbers of symbols

//default values...but they are load from EEPROM
byte bright = 0;
byte font = 1;
byte dotStyle = 2;
bool showDots;

void setup() {
	//COMMUNICATION
	Wire.begin(); //start I2C communication
	Serial.begin(9600);

	//IO
	pinMode(BTN1, INPUT_PULLUP);
	pinMode(BTN2, INPUT_PULLUP);

	bright = EEPROM.read(0);	//load light intensity from EEPROM
	if (bright < 0 || bright > 15) {
		//in case variable out of range
		bright = 7;
	}

	font = EEPROM.read(1);	//load font style from EEPROM
	if (font < 0 || font > symbolsLenght - 1) {
		//in case variable out of range
		font = 1;
	}

	dotStyle = EEPROM.read(2);	//load dot style from EEPROM
	if (dotStyle < 0 || dotStyle > 2) {
		//in case variable out of range
		dotStyle = 2;
	}


	delay(10);

	//SET ALL DISPLAYS
	for (byte address = 0; address<devices; address++) {
		lc.shutdown(address, false);	//powersaving
		lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
		lc.clearDisplay(address);		//clear display
	}

	//INIT TIME SETTING
	//SetRtc(15, 41, 8, 6, 30, 3, 18);	//sec, min, hour, dayOfWeek, dayOfMonth, month, year

	Intro();	//show LMS!
}

void loop() {
	//store input to var
	presentInput1 = digitalRead(BTN1);
	presentInput2 = digitalRead(BTN2);

	switch (systemState) {
	case 0:
		//SHOW ACTUAL TIME
		presentTime = millis();

		//"multasking"
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
				DrawSymbol(3, 3);	//F
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (font / 10) + (font * 10));	//actual font
				DrawSymbol(0, (font % 10) + (font * 10));	//actual font
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add hour
				minute++;
				if (minute > 59) {
					minute = 0;
				}
			}
		}
		break;

	case 4:
		//menu 4
		//set FONT
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 1);	//:
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dotStyle / 10) + 10);	//actual dot style
				DrawSymbol(0, (dotStyle % 10) + 10);	//actual dot style
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add hour
				font++;
				if (font > symbolsLenght - 1) {
					font = 1;
				}

				DrawSymbol(3, 3);	//F
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (font / 10) + (font * 10));	//actual font
				DrawSymbol(0, (font % 10) + (font * 10));	//actual font

				delay(25);
			}
		}
		break;

	case 5:
		//menu 5
		//set DOT STYLE
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected

				//NEXT
				systemState++;
				DrawSymbol(3, 2);	//B
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (bright / 10) + 10);	//actual light intensity
				DrawSymbol(0, (bright % 10) + 10);	//actual light intensity
			}
		}

		if (presentInput2 != lastInput2) {
			//change detected BTN2
			if (presentInput2) {
				//rising edge detected
				//add hour
				dotStyle++;
				if (dotStyle > 2) {
					dotStyle = 0;
				}

				DrawSymbol(3, 1);	//:
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (dotStyle / 10) + 10);	//actual dot style
				DrawSymbol(0, (dotStyle % 10) + 10);	//actual dot style

				delay(25);
			}
		}
		break;

	case 6:
		//menu 5
		//set BRIGHTNES
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
				//add hour
				bright++;
				if (bright > 15) {
					bright = 0;			
				}
				
				DrawSymbol(3, 2);	//B
				DrawSymbol(2, 0);	//space
				DrawSymbol(1, (bright / 10) + 10);	//actual light intensity
				DrawSymbol(0, (bright % 10) + 10);	//actual light intensity

				for (byte address = 0; address<devices; address++) {
					lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
				}

				delay(25);
			}
		}
		break;



	case 7:
		//menu 6
		//EXIT
		if (presentInput1 != lastInput1) {
			//change detected BTN1
			if (presentInput1) {
				//rising edge detected
				SetRtc(0, minute, hour, dayOfWeek, dayOfMonth, month, year);	//set time and zero second
				EEPROM.write(0, bright);	//store actual light intensity to addr 0
				EEPROM.write(1, font);	//store actual font to addr 1
				EEPROM.write(2, dotStyle);	//store actual font to addr 1
				systemState = 0;
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
	if (systemState == 0 || systemState == 2) {
		DrawSymbol(2, (hour % 10) + (font * 10));
		DrawSymbol(3, (hour / 10) + (font * 10));
	}

	if (systemState == 0 || systemState == 3) {
		DrawSymbol(0, (minute % 10) + (font * 10));
		DrawSymbol(1, (minute / 10) + (font * 10));
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
	}

	font = storedFont;
}

void Intro() {
	DrawSymbol(3, 5);	//L
	DrawSymbol(2, 6);	//M
	DrawSymbol(1, 7);	//S
	DrawSymbol(0, 0);	//!

	//DrawSymbol(0, (symbolsLenght % 10) + 10);	//actual light intensity
	//DrawSymbol(1, (symbolsLenght / 10) + 10);	//actual light intensity
	delay(5000);
}

void DrawSymbol(byte adr, byte symbol) {
	//draw symbol
	//offset move the symbol to right side

	for (int i = 0; i < 8; i++) {
		byte row = (symbols[symbol] >> i * 8) & 0xFF;
		lc.setRow(adr, i, ByteRevers(row));

		//blinking dots on display
		if (adr == 2 && dotStyle > 0){
			if (i == 1) lc.setLed(2, 1, 7, showDots);  //addr, row, column
			if (i == 2) lc.setLed(2, 2, 7, showDots);
			if (i == 5) lc.setLed(2, 5, 7, showDots);
			if (i == 6) lc.setLed(2, 6, 7, showDots);
		}
	}
}

byte ByteRevers(byte in) {
	//font turning
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
	//second letter - data (space = 0, ! = 1, etc. see ASCII table)

	if (Serial.available() > 0) {
		byte receivedCommand;
		receivedCommand = Serial.read();	//read first letter

		if (receivedCommand < 90) {
			//received data is less than 90 (letter Z)
			delay(10);	//wait for second letter

			byte receivedData;
			receivedData = Serial.read();
			receivedData -= 32;	//shift - 32 -> 32 = space

			switch (receivedCommand) {
			case 65:
				//year 65 = A
				year = receivedData;
				lc.setLed(3, 7, 0, true);	//show setting dot
				break;
			case 66:
				//month 66 = B
				month = receivedData;
				lc.setLed(3, 7, 1, true);	//show setting dot
				break;
			case 67:
				//dayOfMonth 67 = C
				dayOfMonth = receivedData;
				lc.setLed(3, 7, 2, true);	//show setting dot
				break;
			case 68:
				//hour 68 = D
				hour = receivedData;
				lc.setLed(3, 7, 3, true);	//show setting dot
				break;
			case 69:
				//minute 69 = E
				minute = receivedData;
				lc.setLed(3, 7, 4, true);	//show setting dot
				break;
			case 70:
				//second 70 = F
				second = receivedData;
				lc.setLed(3, 7, 5, true);	//show setting dot
				break;
			case 71:
				//dayofWeek 71 = G
				dayOfWeek = receivedData;
				lc.setLed(3, 7, 6, true);	//show setting dot
				break;
			}
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
		}

		//flush serial data
		Serial.flush();
	}
}