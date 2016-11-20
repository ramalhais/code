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

#define PIN_SOFTSERIAL_RX 0
#define PIN_LED_ONBOARD 1
#define PIN_SOFTSERIAL_TX 2
#define PIN_SENSOR_HALL_SPEED 5
//#define PIN_SENSOR_HALL_RPM 5

// Bluetooth HC-06 Module Default: Slave, 9600 baud rate, N, 8, 1. Pincode 1234
#define BT_DEFAULT_SERIAL_SPEED 9600
#define BT_SET_NAME "BlueSense XR400"
#define BT_SET_PIN "1111"

SoftSerial btSerial(PIN_SOFTSERIAL_RX, PIN_SOFTSERIAL_TX); // RX, TX

volatile unsigned long bikeSpeedLastTimeMs = millis();
volatile unsigned long bikeSpeedLastIntervalMs = 0;
volatile unsigned long bikeWheelTurnsTotal = 0;
volatile unsigned long bikeWheelTurnsTrip = 0;

volatile unsigned long bikeRPMLastTimeMs = millis();
volatile unsigned long bikeRPMLastIntervalMs = 0;

#define MM_MS2KM_H 3.6
#define MM2INCH 25.4
#define WHEEL2ARC(rd,tw,thp) ((2 * PI * ( ( (rd * MM2INCH) / 2) + (tw * (thp / 100) ) ) ))

double bikeWheelArc = 0;

#define BIKE_DEFAULT_RIM_DIAMETER_INCHES 18
#define BIKE_DEFAULT_TIRE_WIDTH 80
#define BIKE_DEFAULT_TIRE_HEIGHT_PERCENT 100

uint8_t VirtualPortNb;

void setup()
{
  pinMode(PIN_LED_ONBOARD, OUTPUT);

  pinMode(PIN_SENSOR_HALL_SPEED, INPUT);
  digitalWrite(PIN_SENSOR_HALL_SPEED, HIGH);

  //  pinMode(PIN_SENSOR_HALL_RPM, INPUT);
  //  digitalWrite(PIN_SENSOR_HALL_RPM, HIGH);

  BtSerialSetup();

  bikeWheelArc = WHEEL2ARC(BIKE_DEFAULT_RIM_DIAMETER_INCHES, BIKE_DEFAULT_TIRE_WIDTH, BIKE_DEFAULT_TIRE_HEIGHT_PERCENT); // Front Rim Diameter(inches), Tire Width(milimeters), Tire Height Percent Relative to Width(percent)

  TinyPinChange_Init();

  VirtualPortNb=TinyPinChange_RegisterIsr(PIN_SENSOR_HALL_SPEED, PinChangeCallback);
  TinyPinChange_EnablePin(PIN_SENSOR_HALL_SPEED);

  //  VirtualPortNb=TinyPinChange_RegisterIsr(PIN_SENSOR_HALL_RPM, PinChangeCallback);
  //  TinyPinChange_EnablePin(PIN_SENSOR_HALL_RPM);
}

void PinChangeCallback(void) {
  uint8_t PortChange;

  PortChange = TinyPinChange_GetPinEvent(VirtualPortNb);
  if(PortChange & TinyPinChange_PinToMsk(PIN_SENSOR_HALL_SPEED)) {  // Check PIN_SENSOR_HALL_SPEED has changed.
    static uint8_t curHallSpeedPinValue = HIGH;

    unsigned long timeMs = millis();
    unsigned long intervalMs = timeMs - bikeSpeedLastTimeMs;

    uint8_t val = digitalRead(PIN_SENSOR_HALL_SPEED);
    if (curHallSpeedPinValue == HIGH && val == LOW && intervalMs > 0) {
      bikeSpeedLastTimeMs = timeMs;
      bikeSpeedLastIntervalMs = intervalMs;

      bikeWheelTurnsTotal++;
      bikeWheelTurnsTrip++;
    }

    curHallSpeedPinValue = val;
  }

  //  if(PortChange & TinyPinChange_PinToMsk(PIN_SENSOR_HALL_RPM)) {  // Check PIN_SENSOR_HALL_RPM has changed.
  //    static uint8_t curHallRPMPinValue = HIGH;
  //
  //    unsigned long timeMs = millis();
  //    unsigned long intervalMs = timeMs - bikeRPMLastTimeMs;
  //
  //    uint8_t val = digitalRead(PIN_SENSOR_HALL_SPEED);
  //    if (curHallRPMPinValue == HIGH && val == LOW && intervalMs > 0) {
  //      bikeRPMLastTimeMs = timeMs;
  //      bikeRPMLastIntervalMs = intervalMs;
  //    }
  //
  //    curHallRPMPinValue = val;
  //  }
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
  blink(30, 15);
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
  btSerial.print("Turns ");
  btSerial.println(bikeWheelTurnsTotal);
  btSerial.print("Speed ");
  double mBikeSpeed = GetBikeSpeed();
  btSerial.println(mBikeSpeed);
  btSerial.print("SLTime ");
  btSerial.println(bikeSpeedLastTimeMs);
  btSerial.print("SLInterval ");
  btSerial.println(bikeSpeedLastIntervalMs);
  //  btSerial.print("RPM ");
  //  double mBikeRPM = GetBikeRPM();
  //  btSerial.println(mBikeRPM);
  //  btSerial.print("RLTime ");
  //  btSerial.println(bikeRPMLastTimeMs);
  //  btSerial.print("RLInterval ");
  //  btSerial.println(bikeRPMLastIntervalMs);
#endif

  delay(500);
}

void BtSerialSetup() {
#if USE_EEPROM
  EepromBtSerialSetup();
#else
#if DEBUG
  int blinkStep = 0;
#endif

  btSerial.begin(BT_DEFAULT_SERIAL_SPEED);

  //  btSerial.write("AT+NAME");
  //  btSerial.write(BT_SET_NAME); // Set bluetooth device name.
  //  btSerial.flush();

#if DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

  //  btSerial.write("AT+PIN");
  //  btSerial.write(BT_SET_PIN); // Set bluetooth pairing PIN.
  //  btSerial.flush();

#if DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

  //  btSerial.write("AT+BAUD4"); // Set bluetooth serial speed.
  //  btSerial.flush();
  //  btSerial.begin(newSpeed);

#if DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

#endif
}

void EepromBtSerialSetup() {
#if DEBUG
  int blinkStep = 0;
#endif

  tEepromBtUpdate eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));

  if (0 == eepromBtUpdate.update) { // Empty EEPROM? Set defaults
    eepromBtUpdate.update = 2;
    memcpy(eepromBtUpdate.name, BT_SET_NAME, sizeof(BT_SET_NAME)-1);
    memcpy(eepromBtUpdate.pin, BT_SET_PIN, EEPROM_BT_PIN_SIZE);
    eepromBtUpdate.curSpeed = BT_DEFAULT_SERIAL_SPEED;
    eepromBtUpdate.newSpeed = BT_DEFAULT_SERIAL_SPEED;
  } 

  btSerial.begin(eepromBtUpdate.curSpeed);

  if (2 == eepromBtUpdate.update) {
    btSerial.print("AT+NAME");
    btSerial.write((const uint8_t*)eepromBtUpdate.name, sizeof(eepromBtUpdate.name)); // Set bluetooth device name.
    btSerial.flush();
#if DEBUG
    blink(++blinkStep, 200);
#else
    delay(1000);
#endif

    btSerial.print("AT+PIN");
    btSerial.write((const uint8_t*)eepromBtUpdate.pin, EEPROM_BT_PIN_SIZE); // Set bluetooth pairing PIN.
    btSerial.flush();
#if DEBUG
    blink(++blinkStep, 200);
#else
    delay(1000);
#endif

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
#if DEBUG
    blink(++blinkStep, 200);
#else
    delay(1000);
#endif

    btSerial.begin(eepromBtUpdate.newSpeed);

    eepromBtUpdate.update = 1;
    eepromBtUpdate.curSpeed = eepromBtUpdate.newSpeed;
    eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBtUpdate));

#if DEBUG
    blink(++blinkStep, 200);
#endif
  }
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
  //  bikeWheelArc = (2 * PI * ( ( (bikeRimDiameterInches * MM2INCH) / 2) + (bikeTireWidth * (bikeTireHeightPercent / 100) ) ) );
  bikeWheelArc = WHEEL2ARC(bikeRimDiameterInches, bikeTireWidth, bikeTireHeightPercent);
}

double GetBikeSpeed() {
  static double sBikeSpeed = 0;
  unsigned int intervalMs = millis() - bikeSpeedLastTimeMs;

  if (bikeSpeedLastIntervalMs > intervalMs) {
    sBikeSpeed = (bikeWheelArc / bikeSpeedLastIntervalMs) * MM_MS2KM_H;
  } 
  else {
    sBikeSpeed = (bikeWheelArc / (intervalMs*2)) * MM_MS2KM_H;
  }

  return sBikeSpeed;
}

double GetBikeRPM() {
  static double sBikeRPM = 0;
  unsigned int intervalMs = millis() - bikeRPMLastTimeMs;

  if (bikeRPMLastIntervalMs > intervalMs) {
    sBikeRPM = (1000 / bikeRPMLastIntervalMs);
  } 
  else {
    sBikeRPM = (1000 / (intervalMs*2));  
  }

  return sBikeRPM;
}

int getBikeKmTotal() {
  return bikeWheelTurnsTotal * bikeWheelArc / 1000000;
}

int getBikeKmTrip() {
  return bikeWheelTurnsTrip * bikeWheelArc / 1000000;
}




