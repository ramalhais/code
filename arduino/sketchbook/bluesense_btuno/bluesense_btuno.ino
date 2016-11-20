#define BS_DEBUG 1
#define BS_USE_EEPROM 0

#define PIN_LED_ONBOARD 13
#define PIN_SENSOR_HALL_SPEED 4
#define PIN_SENSOR_HALL_RPM 5

// Bluetooth HC-06 Module Default: Slave, 9600 baud rate, N, 8, 1. Pincode 1234
#define BT_DEFAULT_SERIAL_SPEED 9600
#define BT_SET_NAME "BlueSense XR400"
#define BT_SET_PIN "1111"

#include <PinChangeInt.h>
#include <MeetAndroid.h>

#include <EEPROM.h>
#include <avr/eeprom.h> // eeprom block read/write

#define EEPROM_BT_NAME_SIZE 20
#define EEPROM_BT_PIN_SIZE 4

typedef struct
{
  byte update;
  char name[EEPROM_BT_NAME_SIZE];
  char pin[EEPROM_BT_PIN_SIZE];
  unsigned long curSpeed;
  unsigned long newSpeed;
} 
tEepromBt;

#define EEPROM_BT_UPDATE_ADDR 0
#define EEPROM_BT_UPDATE_SIZE sizeof(tEepromBt.update)
#define EEPROM_BT_NAME_ADDR (EEPROM_BT_UPDATE_ADDR+EEPROM_BT_UPDATE_SIZE)
#define EEPROM_BT_PIN_ADDR (EEPROM_BT_NAME_ADDR+EEPROM_BT_NAME_SIZE)
#define EEPROM_BT_CURSPEED_ADDR (EEPROM_BT_PIN_ADDR+EEPROM_BT_PIN_SIZE)
#define EEPROM_BT_CURSPEED_SIZE sizeof(tEepromBt.curSpeed)
#define EEPROM_BT_NEWSPEED_ADDR (EEPROM_BT_CURSPEED_ADDR+EEPROM_BT_CURSPEED_SIZE)
#define EEPROM_BT_NEWSPEED_SIZE sizeof(tEepromBt.newSpeed)

HardwareSerial btSerial = Serial;

volatile unsigned long bikeSpeedLastTimeMs = millis();
volatile unsigned long bikeSpeedLastIntervalMs = 0;
volatile unsigned long bikeWheelTurnsTotal = 0;
volatile unsigned long bikeWheelTurnsTripStart = 0;

volatile unsigned long bikeRPMLastTimeMs = millis();
volatile unsigned long bikeRPMLastIntervalMs = 0;
volatile unsigned long bikeRPMTotal = 0;

#define MM_MS2KM_H 3.6
#define MM2INCH 25.4
#define WHEEL2DIAMETER(rd,tw,thp) ((2 * PI * ( ( (rd * MM2INCH) / 2) + (tw * (thp / 100) ) ) ))

double bikeWheelDiameter = 0;

#define BIKE_DEFAULT_RIM_DIAMETER_INCHES 21
#define BIKE_DEFAULT_TIRE_WIDTH 80
#define BIKE_DEFAULT_TIRE_HEIGHT_PERCENT 100

MeetAndroid meetAndroid;

void setup() {
  pinMode(PIN_LED_ONBOARD, OUTPUT);

  pinMode(PIN_SENSOR_HALL_SPEED, INPUT);
  digitalWrite(PIN_SENSOR_HALL_SPEED, HIGH); // Enable internal pull-up resistor
  PCintPort::attachInterrupt(PIN_SENSOR_HALL_SPEED, &PinChangeCallbackSpeed, FALLING); // (RISING, FALLING or CHANGE)

  pinMode(PIN_SENSOR_HALL_RPM, INPUT);
  digitalWrite(PIN_SENSOR_HALL_RPM, HIGH); // Enable internal pull-up resistor
  PCintPort::attachInterrupt(PIN_SENSOR_HALL_RPM, &PinChangeCallbackRPM, FALLING); // (RISING, FALLING or CHANGE)

  BtSerialSetup();

  bikeWheelDiameter = WHEEL2DIAMETER(BIKE_DEFAULT_RIM_DIAMETER_INCHES, BIKE_DEFAULT_TIRE_WIDTH, BIKE_DEFAULT_TIRE_HEIGHT_PERCENT); // Front Rim Diameter(inches), Tire Width(milimeters), Tire Height Percent Relative to Width(percent)
}

void PinChangeCallbackSpeed(void) {
  unsigned long timeMs = millis();
  unsigned long intervalMs = timeMs - bikeSpeedLastTimeMs;

  bikeSpeedLastTimeMs = timeMs;
  bikeSpeedLastIntervalMs = intervalMs;

  bikeWheelTurnsTotal++;
}

void PinChangeCallbackRPM(void) {
  unsigned long timeMs = millis();
  unsigned long intervalMs = timeMs - bikeRPMLastTimeMs;

  bikeRPMLastTimeMs = timeMs;
  bikeRPMLastIntervalMs = intervalMs;

  bikeRPMTotal++;
}

void loop() {
#if BS_DEBUG
  btSerial.println("\n# loop");
#endif

  meetAndroid.receive(); // you need to keep this in your loop() to receive events
  meetAndroid.send(GetBikeSpeed()); // Read input pin and send result to Android


#if BS_USE_EEPROM
    while (btSerial.available() > 0) {
    byte b = btSerial.read();

    if ('a' == b) {
      EepromBtSetPin("1111");
    }
    if ('b' == b) {
      EepromBtSetName("BlueSens XR400R");
    }
  }
#endif

#if BS_DEBUG
  blink(30, 15);
#endif

#if BS_USE_EEPROM
  tEepromBt eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));
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

#if BS_DEBUG
  btSerial.print("Turns ");
  btSerial.println(bikeWheelTurnsTotal);
  btSerial.print("Speed ");
  double mBikeSpeed = GetBikeSpeed();
  btSerial.println(mBikeSpeed);
  btSerial.print("SLTime ");
  btSerial.println(bikeSpeedLastTimeMs);
  btSerial.print("SLInterval ");
  btSerial.println(bikeSpeedLastIntervalMs);

  btSerial.print("RPM Total ");
  btSerial.println(bikeRPMTotal);
  btSerial.print("RPM ");
  double mBikeRPM = GetBikeRPM();
  btSerial.println(mBikeRPM);
  btSerial.print("RLTime ");
  btSerial.println(bikeRPMLastTimeMs);
  btSerial.print("RLInterval ");
  btSerial.println(bikeRPMLastIntervalMs);

  btSerial.print("Vcc ");
  btSerial.print(GetVoltage()/1000);
  btSerial.print(".");
  btSerial.println(GetVoltage()%1000);

  btSerial.print("Temp ");
  btSerial.println(GetTemperature());
#endif

  delay(500);
}

void BtSerialSetup() {
#if BS_USE_EEPROM
  EepromBtSerialSetup();
#else
#if BS_DEBUG
  int blinkStep = 0;
#endif

  btSerial.begin(BT_DEFAULT_SERIAL_SPEED);

  //  btSerial.write("AT+NAME");
  //  btSerial.write(BT_SET_NAME); // Set bluetooth device name.
  //  btSerial.flush();

#if BS_DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

  //  btSerial.write("AT+PIN");
  //  btSerial.write(BT_SET_PIN); // Set bluetooth pairing PIN.
  //  btSerial.flush();

#if BS_DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

  //  btSerial.write("AT+BAUD4"); // Set bluetooth serial speed.
  //  btSerial.flush();
  //  btSerial.begin(newSpeed);

#if BS_DEBUG
  blink(++blinkStep, 200);
#else
  //  delay(1000);
#endif

#endif
}

#if BS_USE_EEPROM
void EepromBtSerialSetup() {
#if BS_DEBUG
  int blinkStep = 0;
#endif

  tEepromBt eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));

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
#if BS_DEBUG
    blink(++blinkStep, 200);
#else
    delay(1000);
#endif

    btSerial.print("AT+PIN");
    btSerial.write((const uint8_t*)eepromBtUpdate.pin, EEPROM_BT_PIN_SIZE); // Set bluetooth pairing PIN.
    btSerial.flush();
#if BS_DEBUG
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
#if BS_DEBUG
    blink(++blinkStep, 200);
#else
    delay(1000);
#endif

    btSerial.begin(eepromBtUpdate.newSpeed);

    eepromBtUpdate.update = 1;
    eepromBtUpdate.curSpeed = eepromBtUpdate.newSpeed;
    eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));

#if BS_DEBUG
    blink(++blinkStep, 200);
#endif
  }
}
#endif

#if BS_USE_EEPROM
void EepromBtSetPin(const char *pin) {
  tEepromBt eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));

  eepromBtUpdate.update = 2;
  memcpy(eepromBtUpdate.pin, pin, EEPROM_BT_PIN_SIZE);

  eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));
  delay(15000);
  Reboot();
}

void EepromBtSetName(const char *name) {
  tEepromBt eepromBtUpdate;
  eeprom_read_block((void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));

  eepromBtUpdate.update = 2;
  memcpy(eepromBtUpdate.name, name, strlen(name));
  memset(eepromBtUpdate.name+strlen(name), 0, EEPROM_BT_NAME_SIZE-strlen(name));

  eeprom_write_block((const void*)&eepromBtUpdate, (void*)EEPROM_BT_UPDATE_ADDR, sizeof(tEepromBt));
  delay(15000);
  Reboot();
}
#endif

void Reboot(void) {
  noInterrupts(); // disable interrupts which could mess with changing prescaler
  CLKPR = 0b10000000; // enable prescaler speed change
  CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  void (*ptrToFunction)(); // allocate a function pointer
  ptrToFunction = 0x0000; // set function pointer to bootloader reset vector
  (*ptrToFunction)(); // jump to reset, which bounces in to bootloader
}

// https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
int GetVoltage() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion

  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8 | low);

  //  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  //  result = 1126400L / result; // Back-calculate AVcc in mV (1.1*1024*1000)
  result = 1156300L / result; // Measure VCC and back-calculate 1.1V voltage
  //result = 1184665L / result;
  return result;
}

// https://code.google.com/p/tinkerit/wiki/SecretThermometer
long GetTemperature() {
  long result;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(20); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = ((result - 310) / 1.17);
//  result = (result - 125) * 1075;
  return result;
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
  //  bikeWheelDiameter = (2 * PI * ( ( (bikeRimDiameterInches * MM2INCH) / 2) + (bikeTireWidth * (bikeTireHeightPercent / 100) ) ) );
  bikeWheelDiameter = WHEEL2DIAMETER(bikeRimDiameterInches, bikeTireWidth, bikeTireHeightPercent);
}

double GetBikeSpeed() {
  static double sBikeSpeed = 0;
  unsigned int intervalMs = millis() - bikeSpeedLastTimeMs;

  if (bikeSpeedLastIntervalMs > intervalMs) {
    sBikeSpeed = (bikeWheelDiameter / bikeSpeedLastIntervalMs) * MM_MS2KM_H;
  } 
  else {
    sBikeSpeed = (bikeWheelDiameter / (intervalMs*1)) * MM_MS2KM_H;
  }

  return sBikeSpeed;
}

double GetBikeRPM() {
  if (bikeRPMLastIntervalMs == 0)
    return 0;

  static double sBikeRPM = 0;
  unsigned int intervalMs = millis() - bikeRPMLastTimeMs;

  if (bikeRPMLastIntervalMs > intervalMs) {
    sBikeRPM = (1000 / bikeRPMLastIntervalMs);
  } 
  else {
    sBikeRPM = (1000 / (intervalMs*1));  
  }

  return sBikeRPM;
}

int getBikeKmTotal() {
  return bikeWheelTurnsTotal * bikeWheelDiameter / 1000000;
}

int getBikeKmTrip() {
  return (bikeWheelTurnsTotal - bikeWheelTurnsTripStart) * bikeWheelDiameter / 1000000;
}












