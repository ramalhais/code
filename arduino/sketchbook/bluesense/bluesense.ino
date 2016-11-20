// ATMEL ATTINY45 / ARDUINO
//
//                           +-\/-+
//  Ain0       (D  5)  PB5  1|    |8   VCC
//  Ain3       (D  3)  PB3  2|    |7   PB2  (D  2)  INT0  Ain1
//  Ain2       (D  4)  PB4  3|    |6   PB1  (D  1)        pwm1
//                     GND  4|    |5   PB0  (D  0)        pwm0
//                           +----+

#include <SoftSerial.h>
#include <TinyPinChange.h>

#include <MeetAndroid.h>

// Bluetooth HC-06 Module Default: Slave, 9600 baud rate, N, 8, 1. Pincode 1234
#define BT_INITIAL_SERIAL_SPEED 9600
#define BT_SERIAL_SPEED 115200

#define PIN_ONBOARD_LED 1

// Bluetooth serial comm
#define PIN_SOFTSERIAL_RX 3
#define PIN_SOFTSERIAL_TX 4

// Sensors
#define USE_INT_TEMP yes
#define PIN_TEMPERATURE_ENGINE 5
#define PIN_HALL_RPM 6
#define PIN_HALL_SPEED 7

SoftSerial bluetoothSerial(PIN_SOFTSERIAL_RX, PIN_SOFTSERIAL_TX); // RX, TX
MeetAndroid meetAndroid(bluetoothSerial);

void setup()
{
  // Setup serial connection to Bluetooth.
  bluetoothSerial.begin(BT_INITIAL_SERIAL_SPEED);
  bluetoothSerial.println("AT+BAUD8"); // Change speed to 115200bps.
  bluetoothSerial.begin(BT_SERIAL_SPEED);
//  delay(1000*8/BT_SERIAL_SPEED);
//  bluetoothSerial.flush();

  bluetoothSerial.println("AT+NAMEBlueSense XR400R"); // Set bluetooth device name.
//  delay(1000*6/BT_SERIAL_SPEED);
//  bluetoothSerial.flush();

  bluetoothSerial.println("AT+PIN7777"); // Set bluetooth pairing PIN.
  delay(1000*8/BT_SERIAL_SPEED);
  bluetoothSerial.flush();
}

void loop()
{
  meetAndroid.receive(); // you need to keep this in your loop() to receive events

  meetAndroid.send(getBikeKPH()); // Read input pin and send result to Android

  //Serial.println(updateBikeKPH());

  // add a little delay otherwise the phone is pretty busy
  delay(1000);
}



