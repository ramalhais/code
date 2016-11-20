#define DEBUG 1
#define USE_EEPROM 0

#include <TinyPinChange.h>
#include <SoftSerial.h>

#include <EEPROM.h>
#include <avr/eeprom.h> // eeprom block read/write

#define EEPROM_BT_UPDATE_ADDR 0
#define EEPROM_BT_UPDATE_SIZE sizeof(byte)
#define EEPROM_BT_NAME_ADDR (EEPROM_BT_UPDATE_ADDR+EEPROM_BT_UPDATE_SIZE)
#define EEPROM_BT_NAME_SIZE 20
#define EEPROM_BT_PIN_ADDR (EEPROM_BT_NAME_ADDR+EEPROM_BT_NAME_SIZE)
#define EEPROM_BT_PIN_SIZE 4
#define EEPROM_BT_CURSPEED_ADDR (EEPROM_BT_PIN_ADDR+EEPROM_BT_PIN_SIZE)
#define EEPROM_BT_CURSPEED_SIZE sizeof(unsigned long)
#define EEPROM_BT_NEWSPEED_ADDR (EEPROM_BT_CURSPEED_ADDR+EEPROM_BT_CURSPEED_SIZE)
#define EEPROM_BT_NEWSPEED_SIZE sizeof(unsigned long)

typedef struct
{
  byte update;
  char name[EEPROM_BT_NAME_SIZE];
  char pin[EEPROM_BT_PIN_SIZE];
  unsigned long curSpeed;
  unsigned long newSpeed;
} 
tEepromBtUpdate;

#define PIN_SENSOR_HALL_SPEED 0
#define PIN_LED_ONBOARD 1
#define PIN_SOFTSERIAL_RX 2
#define PIN_SOFTSERIAL_TX 3

// Bluetooth HC-06 Module Default: Slave, 9600 baud rate, N, 8, 1. Pincode 1234
#define BT_DEFAULT_SERIAL_SPEED 9600
#define BT_SET_NAME "BlueSense XR400"
#define BT_SET_PIN "1234"

SoftSerial btSerial(PIN_SOFTSERIAL_RX, PIN_SOFTSERIAL_TX); // RX, TX

volatile double bikeSpeed = 0;
volatile unsigned long bikeSpeedLastTimeMs = 0;
volatile unsigned long bikeSpeedLastIntervalMs = 0;
volatile unsigned long bikeWheelTurnsTotal = 0;
volatile unsigned long bikeWheelTurnsTrip = 0;

#define MM_MS2KM_H 3.6

#define MM2INCH 25.4
#define WHEEL2ARC(rd,tw,thp) ((2 * PI * ( ( (rd * MM2INCH) / 2) + (tw * (thp / 100) ) ) ))
double bikeWheelArc = 0;

uint8_t VirtualPortNb;

void setup()
{
  pinMode(PIN_LED_ONBOARD, OUTPUT);

  //SensorHallSpeedConfig(18, 80, 100); // Front Rim Diameter(inches), Tire Width(milimeters), Tire Height Percent Relative to Width(percent)
  bikeWheelArc = WHEEL2ARC(18, 80, 100); // Front Rim Diameter(inches), Tire Width(milimeters), Tire Height Percent Relative to Width(percent)
  //SensorHallSpeedSetup();
  //attachInterrupt(0, ISRHallSpeed, CHANGE);

  BtSerialSetup();

  TinyPinChange_Init();
  VirtualPortNb=TinyPinChange_RegisterIsr(PIN_SENSOR_HALL_SPEED, HallSpeedPinChangeCallback);
  TinyPinChange_EnablePin(PIN_SENSOR_HALL_SPEED);
}

void HallSpeedPinChangeCallback(void)
{
  uint8_t PortChange;

  PortChange = TinyPinChange_GetPinEvent(VirtualPortNb);
  if(PortChange & TinyPinChange_PinToMsk(PIN_SENSOR_HALL_SPEED)) /* Check FIRST_INPUT has changed */
  {
    unsigned long timeMs = millis();
    unsigned long intervalMs = timeMs - bikeSpeedLastTimeMs;

    if (intervalMs > 200) {
      bikeSpeedLastTimeMs = timeMs;
      bikeSpeedLastIntervalMs = intervalMs;

      bikeWheelTurnsTotal++;
      bikeWheelTurnsTrip++;

      bikeSpeed = (bikeWheelArc / intervalMs) * MM_MS2KM_H;
    }
  }


}

void loop() {
#if DEBUG
  btSerial.println("\n# loop");
#endif

  while (btSerial.available() > 0) {
    byte b = btSerial.read();

#if USE_EEPROM
    if ('a' == b) {
      EepromBtSetPin("1111");
    }
    if ('b' == b) {
      EepromBtSetName("BlueSens XR400R");
    }
#endif

  }

#if DEBUG
  blink(1, 1000);
#endif

#if USE_EEPROM
  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
  btSerial.print("update=");
  btSerial.println((unsigned int)eepromBtUpdate.update);
  btSerial.print("name=");  
  btSerial.write((const uint8_t*)eepromBtUpdate.name, EEPROM_BT_NAME_SIZE);
  btSerial.println("");  
  btSerial.print("pin=");  
  btSerial.write((const uint8_t*)eepromBtUpdate.pin, EEPROM_BT_PIN_SIZE);
  btSerial.println("");  
  btSerial.print("curSpeed=");  
  btSerial.println(eepromBtUpdate.curSpeed);
  btSerial.print("newSpeed=");  
  btSerial.println(eepromBtUpdate.newSpeed);
#endif

#if DEBUG
  //HallSpeedCorrection();
  btSerial.print("C ");
  btSerial.println(bikeWheelTurnsTotal);
  btSerial.print("S ");
  btSerial.println(bikeSpeed);
  btSerial.print("LT ");
  btSerial.println(bikeSpeedLastTimeMs);
  btSerial.print("LI ");
  btSerial.println(bikeSpeedLastIntervalMs);
#endif

  delay(500);
}

void BtSerialSetup() {
  int blinkStep = 0;

#if USE_EEPROM
  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));

  if (0 == eepromBtUpdate.update) { // Empty EEPROM? Set defaults
    eepromBtUpdate.update = 2;
    memcpy(eepromBtUpdate.name, BT_SET_NAME, sizeof(BT_SET_NAME)-1);
    memcpy(eepromBtUpdate.pin, BT_SET_PIN, EEPROM_BT_PIN_SIZE);
    eepromBtUpdate.curSpeed = BT_DEFAULT_SERIAL_SPEED;
    eepromBtUpdate.newSpeed = BT_DEFAULT_SERIAL_SPEED;
  } 
#endif

#if USE_EEPROM
  btSerial.begin(eepromBtUpdate.curSpeed);
#else
  btSerial.begin(BT_DEFAULT_SERIAL_SPEED);
#endif

#if USE_EEPROM
  if (2 == eepromBtUpdate.update) {
#endif
    btSerial.print("AT+NAME");
#if USE_EEPROM
    btSerial.write((const uint8_t*)eepromBtUpdate.name, sizeof(eepromBtUpdate.name)); // Set bluetooth device name.
#else
    btSerial.write(BT_SET_NAME); // Set bluetooth device name.
#endif
    btSerial.flush();

#if DEBUG
    blink(++blinkStep, 200);
#endif

    btSerial.print("AT+PIN");
#if USE_EEPROM
    btSerial.write((const uint8_t*)eepromBtUpdate.pin, EEPROM_BT_PIN_SIZE); // Set bluetooth pairing PIN.
#else
    btSerial.write(BT_SET_PIN); // Set bluetooth pairing PIN.
#endif
    btSerial.flush();

#if DEBUG
    blink(++blinkStep, 200);
#endif

#if USE_EEPROM
    char newSpeedCode;

    switch (eepromBtUpdate.newSpeed) {
    default:
      newSpeedCode = '4';
      break;
    case 9600:
      newSpeedCode = '4';
      break;
    case 38400:
      newSpeedCode = '6';
      break;
    case 115200:
      newSpeedCode = '8';
      break;
    }

    btSerial.print("AT+BAUD"); // Set bluetooth serial speed.
    btSerial.print(newSpeedCode);
    btSerial.flush();
#endif

#if DEBUG
    blink(++blinkStep, 200);
#endif

#if USE_EEPROM
    btSerial.begin(eepromBtUpdate.newSpeed);

    eepromBtUpdate.update = 1;
    eepromBtUpdate.curSpeed = eepromBtUpdate.newSpeed;
    eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
#endif

#if DEBUG
    blink(++blinkStep, 200);
#endif

#if USE_EEPROM
  }
#endif
}

void EepromBtSetPin(const char *pin) {
  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));

  eepromBtUpdate.update = 2;
  memcpy(eepromBtUpdate.pin, pin, EEPROM_BT_PIN_SIZE);

  eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
  delay(15000);
  Reboot();
}

void EepromBtSetName(const char *name) {
  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));

  eepromBtUpdate.update = 2;
  memcpy(eepromBtUpdate.name, name, strlen(name));
  memset(eepromBtUpdate.name+strlen(name), 0, EEPROM_BT_NAME_SIZE-strlen(name));

  eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));
  delay(15000);
  Reboot();
}

void Reboot(void) {
  noInterrupts(); // disable interrupts which could mess with changing prescaler
  CLKPR = 0b10000000; // enable prescaler speed change
  CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  void (*ptrToFunction)(); // allocate a function pointer
  ptrToFunction = 0x0000; // set function pointer to bootloader reset vector
  (*ptrToFunction)(); // jump to reset, which bounces in to bootloader
}

void blink(unsigned int times, unsigned int speed) {
  while (times--) {
    digitalWrite(PIN_LED_ONBOARD, HIGH);
    delay(speed);
    digitalWrite(PIN_LED_ONBOARD, LOW);
    delay(speed);
  }
  delay(1000);
}

void SensorHallSpeedConfig(int bikeRimDiameterInches, int bikeTireWidth, int bikeTireHeightPercent) {
  bikeWheelArc = (2 * PI * ( ( (bikeRimDiameterInches * MM2INCH) / 2) + (bikeTireWidth * (bikeTireHeightPercent / 100) ) ) );
}

void ISRHallSpeed() {
  unsigned long timeMs = millis();

  unsigned long intervalMs = timeMs - bikeSpeedLastTimeMs;
  bikeSpeedLastTimeMs = timeMs;
  bikeSpeedLastIntervalMs = intervalMs;

  bikeWheelTurnsTotal++;
  bikeWheelTurnsTrip++;

  bikeSpeed = (bikeWheelArc / intervalMs) * MM_MS2KM_H;
}

void HallSpeedCorrection() {
  int intervalMs = millis() - bikeSpeedLastTimeMs;

  if (bikeSpeedLastIntervalMs < intervalMs) {
    bikeSpeed = (bikeWheelArc / intervalMs) / (1000000/3600);
  }
}

int getBikeKmTotal() {
  return bikeWheelTurnsTotal * bikeWheelArc / 1000000;
}

int getBikeKmTrip() {
  return bikeWheelTurnsTrip * bikeWheelArc / 1000000;
}

void SensorHallSpeedSetup() {
  attachInterrupt(PIN_SENSOR_HALL_SPEED, ISRHallSpeed, FALLING);
}





