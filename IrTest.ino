 /*
 You can use this sketch to get codes from your IR remote control.
 Code are send to Serial Port. IR receiver is connect to pin D7
 External library IRremote is required (https://github.com/z3t0/Arduino-IRremote)
 */

#include <IRremote.h>

//IR REMOTE CONTROL
int receirerPin = 7;  //Arduino pin connected to IR receiver
IRrecv irReceiver(receirerPin);
decode_results irResults;

void setup()
{
  Serial.begin(9600); //start serial communication
  irReceiver.enableIRIn(); // start the IR receiver
}

void loop() {
  if (irReceiver.decode(&irResults)) {
    Serial.println(irResults.value, HEX); //send data to PC
    irReceiver.resume(); // Receive next value
  }
  delay(100); //some small delay
}
