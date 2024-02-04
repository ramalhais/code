// #define KMS_CLOCK_PIN 4
// #define KMS_DATAOUT_PIN 5
// #define KMS_DATAIN_PIN 6

// #define KMS_CLOCK_PIN 10
// #define KMS_DATAOUT_PIN 11
// #define KMS_DATAIN_PIN 12

// #ifndef RGB_BUILTIN
// // #define RGB_BUILTIN 48
// #define RGB_BUILTIN 2
// #endif

// #include <SPI.h>

#include <ESP32SPISlave.h>
ESP32SPISlave slave;
static constexpr uint32_t BUFFER_SIZE {5};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

// hw_timer_t *timer1;

// int clock_val = 0;
unsigned long clock_ts = millis();
// bool clock_changed = false;
// bool clock_updated = false;
// unsigned long clock_tsdiff = 0;

// void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_handler() {
  // int val = digitalRead(KMS_CLOCK_PIN);
  // unsigned long ts = micros();
  // if (val != clock_val) {
  //   clock_changed = true;
  //   clock_tsdiff = ts-clock_ts;
  //   clock_val = val;
  //   clock_ts = ts;
  // } else {
  //   clock_changed = false;
  // }
  // clock_updated=true;
// }

// #define TIMER1 1
// // #define DIVIDER 80
// #define DIVIDER 48

// #define VSPI_MISO   MISO
// #define VSPI_MOSI   MOSI
// #define VSPI_SCLK   SCK
// #define VSPI_SS     SS

// #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
// #define VSPI FSPI
// #endif

// SPIClass * vspi = NULL;
// static const int spiClk = 5000000; // 1 MHz

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing");

  // digitalWrite(RGB_BUILTIN, HIGH);   // Turn the RGB LED white
  // delay(1000);
  // digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off
  // delay(1000);

  // pinMode(KMS_CLOCK_PIN, INPUT);
  // pinMode(KMS_DATAIN_PIN, INPUT);
  // pinMode(KMS_DATAOUT_PIN, OUTPUT);

  // timer1 = timerBegin(TIMER1, DIVIDER, true);
  // timerSetAutoReload(timer1, true);
  // timerAttachInterrupt(timer1, &timer1_handler, false);
  // timerAlarmWrite(timer1, 4, true);
  // timerAlarmEnable(timer1);

  // attachInterrupt(KMS_CLOCK_PIN,  &timer1_handler, FALLING);

// Serial.printf("SCK=%d MOSI=%d MISO=%d SS=%d\n", SCK, MOSI, MISO, SS);
    
  // vspi = new SPIClass(VSPI);
  // SPI->begin();
  // pinMode(vspi->pinSS(), OUTPUT);


  // Serial.println("slave.setDataMode(SPI_MODE0)");
  slave.setDataMode(SPI_MODE0);
  // Serial.println("slave.begin()");
  slave.begin();
  // Serial.printf("VSPI MOSI=%d MISO=%d SCK=%d SS=%d\n", MOSI, MISO, SCK, SS);

  // // slave.begin(const uint8_t spi_bus, const int8_t sck, const int8_t miso, const int8_t mosi, const int8_t ss)
  // Serial.println("memset tx");
  memset(spi_slave_tx_buf, 0, BUFFER_SIZE);
  // Serial.println("memset rx");
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
  char sendTest[] = {0, 0, 0, 0, 0};
  Serial.println("Initialized");
}

const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(uint8_t byte)
{
    Serial.printf("%s%s.", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}

bool sent = false;

void loop() {
  // Serial.printf(".");
  // sleep(1);

  // if (clock_val) {
  //   neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0);
  // } else {
  //   neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0);
  // }

  // if (clock_changed) {
  //   if (clock_tsdiff > 0)
  //     // Serial.printf("%ld: %f MHz\n", micros(), (1.0/clock_tsdiff));
  //     Serial.printf("%d", clock_val);
  //   clock_changed = false;
  // }
  // if (clock_updated) {
  //   Serial.printf("%d", clock_val);
  //   clock_updated=false;
  // }
  // Serial.printf(".\n");
  // delay(1);

  // int val = digitalRead(KMS_CLOCK_PIN);
  // Serial.printf("%d\n", val);

  // unsigned long ts = micros();
  // int clock_tsdiff = ts-clock_ts;
  // clock_ts = ts;
  // Serial.printf("%f\n", ts/1000.0/1000.0);
  // Serial.printf("%d\n",micros());

  // SPI->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
//  digitalWrite(spi->pinSS(), LOW); //pull SS slow to prep other end for transfer
  // SPI->transfer(0b01010101);
  // digitalWrite(SPI->pinSS(), HIGH); //pull ss high to signify end of data transfer
  // SPI->endTransaction();

  unsigned long ts = millis();

#define KEY_APOSTROPHE 0x26 // 0b00100110
#define KD_KEYMASK	0x7f
#define KD_DIRECTION	0x80

#define KD_CNTL		0x01
#define KD_LSHIFT	0x02
#define KD_RSHIFT	0x04
#define KD_LCOMM	0x08
#define KD_RCOMM	0x10
#define KD_LALT		0x20
#define KD_RALT		0x40
#define KD_VALID	0x80

#define KD_FLAGKEYS	0x7f00

  char sendA[] = {0b11100011, 0b00001000, 0b00000000, 0b10000000, 0b10111001};
  char sendCmdApos[] = {0b11100011, 0b00001000, 0b00000000, KD_VALID|KD_RCOMM, KD_DIRECTION|KEY_APOSTROPHE};
  char sendCmdAposRelease[] = {0b11100011, 0b00001000, 0b00000000, KD_VALID, KEY_APOSTROPHE};
  char sendTestF[] = {0xff, 0xff, 0xff, 0xff, 0xff};
  char sendTest7[] = {0x7f, 0xff, 0xff, 0xff, 0xff};

  // if (ts-clock_ts > 250) {
    clock_ts=ts;
    // memcpy(spi_slave_tx_buf, sendTest7, 5);
    // if (!sent)
      memcpy(spi_slave_tx_buf, sendCmdApos, 5);
    // else
    //   memcpy(spi_slave_tx_buf, sendCmdAposRelease, 5);
    // memcpy(spi_slave_tx_buf, sendA, 5);
    // Serial.println("Sending bytes");
    // sent = !sent;
  // }

  int bytes = 0;
  slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
  while (slave.available()) {
    // show received data
    // Serial.print("Command Received: ");
    // Serial.println(spi_slave_rx_buf[0]);

    for (int i=0; i < slave.size(); i++) {
      char rxbyte = spi_slave_rx_buf[i];
      print_byte(rxbyte);
      // Serial.printf("0x%x ", rxbyte);
    }

    // Serial.print((spi_slave_rx_buf[0]&0x80) ? "1" : "0");
    slave.pop();
    Serial.print("|");
  }
  Serial.println();
  
  // if (ts-clock_ts > 250) {
    memset(spi_slave_tx_buf, 0, BUFFER_SIZE);
  // }
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
}
