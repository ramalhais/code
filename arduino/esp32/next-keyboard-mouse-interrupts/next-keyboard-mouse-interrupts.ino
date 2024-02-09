#define PIN_TO_KBD 32    // Input to this keyboard
#define PIN_FROM_KBD 33  // Output from this keyboard

#define TIMER1 1
// // #define DIVIDER 80
#define DIVIDER 1052

hw_timer_t *timer1;

enum {
  STATE_WAITING,
  STATE_READY,
  STATE_READING,
  STATE_SHORT_READ,
  STATE_LONG_READ
};

uint32_t read_data = 0;
bool read_data_ready = false;

uint32_t write_data = 0;
bool write_data_ready = false;
uint32_t write_data_idle = 0b1100000000011000000000;
bool write_data_idle_ready = false;

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_write_handler();

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_read_handler() {
  static int state = STATE_WAITING;
  static uint32_t data = 0;
  static uint8_t all_high_counter = 0;
  static uint8_t bits;

  bool val = digitalRead(PIN_TO_KBD);
  if (!read_data_ready)
    data = data | val<<bits;

  if (state == STATE_WAITING) {
    all_high_counter = (all_high_counter*val) + val;
    if (all_high_counter == 16+1) { // sense 16 continuous bits high
      all_high_counter = 0;
      bits = 0;
      data = 0;
      read_data_ready = false;
      state = STATE_READY;
    } else {
    }
  } else if (state == STATE_READY) {
    if (val == 0) {
      data = 0;
      bits++;
      state = STATE_READING;
    }
  } else if (state == STATE_READING) {
    bits++;
    if (bits == 6) { // data bit 5 (starting from 0, not counting start bit)
      if (val) {
        state = STATE_SHORT_READ;
      } else {
        state = STATE_LONG_READ;
      }
    }
  } else {
    bits++;
    if (state == STATE_SHORT_READ && bits == 11) { // 11 bits including start and stop bit
      // Serial.printf("%X\n", data&0b11111111111);
      state = STATE_WAITING;
      read_data = data;
      read_data_ready = true;
      timerDetachInterrupt(timer1);
      timer1 = timerBegin(TIMER1, DIVIDER, true);
      timerSetAutoReload(timer1, true);
      timerAttachInterrupt(timer1, &timer1_write_handler, false);
      timerAlarmWrite(timer1, 4, true);
      timerAlarmEnable(timer1);
    } else if (state == STATE_LONG_READ && bits == 23) {
      // Serial.printf("%X\n", data&0b11111111111111111111111);
      state = STATE_WAITING;
      read_data = data;
      read_data_ready = true;
      timerDetachInterrupt(timer1);
      timer1 = timerBegin(TIMER1, DIVIDER, true);
      timerSetAutoReload(timer1, true);
      timerAttachInterrupt(timer1, &timer1_write_handler, false);
      timerAlarmWrite(timer1, 4, true);
      timerAlarmEnable(timer1);
    }
  }
}

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_write_handler() {
  static uint8_t bits = 0;
  uint32_t data = 0;

  if (write_data_ready && !write_data_idle_ready) {
    data = write_data;
  } else {
    write_data_idle_ready = true;
    data = write_data_idle;
  }

  digitalWrite(PIN_FROM_KBD, data & 1<<bits);
  bits++;
  if (bits == 22) {
    if (write_data_ready && !write_data_idle_ready)
      write_data_ready = false;
    if (write_data_idle_ready)
      write_data_idle_ready = false;
    bits = 0;
    timerDetachInterrupt(timer1);
    timer1 = timerBegin(TIMER1, DIVIDER, true);
    timerSetAutoReload(timer1, true);
    timerAttachInterrupt(timer1, &timer1_read_handler, false);
    timerAlarmWrite(timer1, 4, true);
    timerAlarmEnable(timer1);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initialized");
  pinMode(PIN_TO_KBD, INPUT_PULLUP);
  pinMode(PIN_FROM_KBD, OUTPUT);

  timer1 = timerBegin(TIMER1, DIVIDER, true);
  timerSetAutoReload(timer1, true);
  timerAttachInterrupt(timer1, &timer1_read_handler, false);
  timerAlarmWrite(timer1, 4, true);
  timerAlarmEnable(timer1);

  if (digitalPinToInterrupt(PIN_TO_KBD) == -1)
    Serial.printf("Unable to use PIN_TO_KBD (%d) as interrupt\n", PIN_TO_KBD);
}

void loop() {
  if (read_data_ready) {
    // printf("0x%X\n", read_data);
    if ((read_data & 0x1FF) == 0b000100000) {
      Serial.printf("K");
    } else if ((read_data & 0x1FF) == 0b000100010) {
      Serial.printf("M");
    } else if ((read_data & 0x3FFFFF) == 0b0000000000111111011110) {
      Serial.printf("\nRESET\n");
    } else if ((read_data & 0x1FFF) == 0b0111000000000) {
      Serial.println("");

      if (read_data & 0x2000)
        Serial.printf("L");
      else
        Serial.printf("l");

      if (read_data & 0x4000)
        Serial.printf("R");
      else
        Serial.printf("r");

      Serial.println("");
    } else {
      Serial.printf("?");
    }
    read_data_ready = false;
  }
}
