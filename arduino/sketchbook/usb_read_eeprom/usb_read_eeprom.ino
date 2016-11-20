#define USB_CFG_DEVICE_NAME     'U','S','B',' ','E','E','P','R','O'
#define USB_CFG_DEVICE_NAME_LEN 9
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
  DigiUSB.delay(10);

}

void loop() {
  int lastRead;

  while (DigiUSB.available()  > 0) {
    DigiUSB.read();
    DigiUSB.delay(10);
  }

  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
  DigiUSB.print("update=");
  DigiUSB.delay(10);
  DigiUSB.println(eepromBtUpdate.update);
  DigiUSB.delay(10);
  DigiUSB.print("name=");  
  DigiUSB.delay(10);
  DigiUSB.println(eepromBtUpdate.name);
  DigiUSB.delay(10);
  DigiUSB.print("pin=");  
  DigiUSB.delay(10);
  DigiUSB.println(eepromBtUpdate.pin);
  DigiUSB.delay(10);
  DigiUSB.print("curSpeed=");  
  DigiUSB.delay(10);
  DigiUSB.println(eepromBtUpdate.curSpeed);
  DigiUSB.delay(10);
  DigiUSB.print("newSpeed=");  
  DigiUSB.delay(10);
  DigiUSB.println(eepromBtUpdate.newSpeed);
  DigiUSB.delay(10);
}




