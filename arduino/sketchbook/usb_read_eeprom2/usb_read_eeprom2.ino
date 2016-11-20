#include <DigiUSB.h>

#include <EEPROM.h>
#include <avr/eeprom.h>

#define EEPROM_BT_UPDATE_ADDR 0
#define EEPROM_BT_NAME_SIZE 20

typedef struct
{
  byte update;
  char name[EEPROM_BT_NAME_SIZE];
  unsigned int pin;
  unsigned long curSpeed;
  unsigned long newSpeed;
} 
tEepromBtUpdate;

void setup() {
  DigiUSB.begin();
}

void get_input() {
  int lastRead;
  // when there are no characters to read, or the character isn't a newline
  while (true) { // loop forever
    if (DigiUSB.available()) {
      // something to read
      lastRead = DigiUSB.read();
      DigiUSB.write(lastRead);

      if (lastRead == '\n') {
        break; // when we get a newline, break out of loop
      }
    }

    // refresh the usb port for 10 milliseconds
    DigiUSB.delay(10);
  }
}

void loop() {
  // print output
  DigiUSB.println("Waiting for input...");
  // get input
  get_input();
  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
//  DigiUSB.print("update=");
//  DigiUSB.println(eepromBtUpdate.update);
  DigiUSB.print("name=");
  for (int i = 0; i<EEPROM_BT_NAME_SIZE; i++)
    DigiUSB.write(eepromBtUpdate.name[i]);
  DigiUSB.print("pin=");  
//  DigiUSB.println(eepromBtUpdate.pin);
  DigiUSB.print("curSpeed=");  
//  DigiUSB.println(eepromBtUpdate.curSpeed);
  DigiUSB.print("newSpeed=");  
//  DigiUSB.println(eepromBtUpdate.newSpeed);
}

