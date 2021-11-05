// BikeMon for ESP32

#include "BluetoothSerial.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

#define LED_ON LOW
#define LED_OFF HIGH

BluetoothSerial SerialBT;
typedef struct {
  int cycle_ms;
  int last_cycle_ms;
  int type;
  char *name;
  bool enabled;
} observations;

void setup() {
  Serial.begin(115200);
//  Serial.println("Serial Txd is on pin: "+String(TX));
//  Serial.println("Serial Rxd is on pin: "+String(RX));

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  while(!SerialBT.begin("BikeMon", false)) {
    delay(500);
  }

  // Wait for Bluetooth client
  while(!SerialBT.hasClient()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
  }
  digitalWrite(LED_BUILTIN, LED_OFF);
}

int rpm = 1300;
float afr = 14.7;
void loop() {
  if(SerialBT.available()) {
    digitalWrite(LED_BUILTIN, LED_ON);
    String line = SerialBT.readStringUntil(';');
    Serial.println("Received: " + line);
    if(line == "CMD=RESET") {
      SerialBT.println("RESET=1;");
      Serial.println("RESET=1;");
      ESP.restart();
    } else if(line == "CMD=RPM") {
      SerialBT.printf("RPM=%d;\n",rpm);
      Serial.printf("RPM=%d;\n", rpm);
    } else if(line == "CMD=AFR") {
      SerialBT.printf("AFR=%.2f;\n",afr);
      Serial.printf("AFR=%.2f;\n", afr);
    }
  } else {
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  

}
