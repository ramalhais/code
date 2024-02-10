#define PIN_TO_KBD 32    // Input to this keyboard
#define PIN_FROM_KBD 33  // Output from this keyboard

// https://github.com/tmk/tmk_keyboard/issues/704
#define NEXT_KBD_TIMING_US 52 // *microseconds
//#define NEXT_KBD_TIMING_100NS 527 // *100nanoseconds. should be 52.75 for NeXT KMS, but our KMS is 53microseconds
#define NEXT_KBD_TIMING_100NS 530 // *100nanoseconds. should be 52.75 for NeXT KMS, but our KMS is 53microseconds

#define TIMER1 1
#define DIVIDER_US 80 // 80Mhz / 80 = 1Mhz. 1/f = 1/1000000 = 1 microsecond. Timer ticks every microsecond.
#define DIVIDER_100NS 8 // 80Mhz / 8 = 10Mhz. 1/f = 1/10000000 = 100 nanosecond. Timer ticks every 100 nanoseconds.

#define KMS_QUERY_KEYBOARD 0b000100000
#define KMS_QUERY_MOUSE 0b000100010
#define KMS_RESET 0b0000000000111111011110
#define KMS_LED_PREFIX 0b0111000000000
#define KMS_LED_LEFT_MASK 0x2000
#define KMS_LED_RIGHT_MASK 0x4000

// Not apostrophe, it's tilde or backtick or grave
#define KEY_APOSTROPHE 0x26  // 0b00100110
#define KD_RCOMM 0x10
#define NEXT_KEY_A 0x39

hw_timer_t *timer1;

enum handler_state_e {
  STATE_WAITING,
  STATE_READY,
  STATE_READING,
  STATE_SHORT_READ,
  STATE_LONG_READ
};

enum query_type_e {
  QUERY_NONE,
  QUERY_KEYBOARD,
  QUERY_MOUSE
};

uint32_t read_data = 0;
bool read_data_ready = false;

uint32_t write_data = 0;
bool write_data_ready = false;

uint32_t mouse_data = 0;
bool mouse_data_ready = false;

const uint32_t write_data_idle = 0b1100000000011000000000;
bool write_data_idle_ready = false;

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_write_handler();

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_read_handler() {
  static int state = STATE_WAITING;
  static uint32_t data = 0;
  static uint8_t all_high_counter = 0;
  static uint8_t bits;

  bool val = digitalRead(PIN_TO_KBD);
  if (!read_data_ready)
    data = data | val << bits;

  if (state == STATE_WAITING) {
    all_high_counter = (all_high_counter * val) + val;
    if (all_high_counter == 6) {  // sense 8 continuous bits high
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
      // digitalWrite(PIN_FROM_KBD, 1);
    }
  } else if (state == STATE_READING) {
    bits++;
    if (bits == 6) {  // data bit 5 (starting from 0, not counting start bit)
      if (val) {
        state = STATE_SHORT_READ;
      } else {
        state = STATE_LONG_READ;
      }
    }
  } else {
    bits++;
    if (state == STATE_SHORT_READ && bits == 11) {  // 11 bits including start and stop bit
      state = STATE_WAITING;
      read_data = data;
      read_data_ready = true;
      timerDetachInterrupt(timer1);
      timerAttachInterrupt(timer1, &timer1_write_handler, false);
    } else if (state == STATE_LONG_READ && bits == 23) {
      state = STATE_WAITING;
      read_data = data;
      read_data_ready = true;
      timerDetachInterrupt(timer1);
      timerAttachInterrupt(timer1, &timer1_write_handler, false);
    }
  }
}

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR timer1_write_handler() {
  static uint8_t bits = 0;
  uint32_t data = 0;
  bool is_keyboard_query = (read_data & 0x1FF) == KMS_QUERY_KEYBOARD;
  bool is_mouse_query = (read_data & 0x1FF) == KMS_QUERY_MOUSE;

  if (is_keyboard_query && write_data_ready && !write_data_idle_ready) {
    data = write_data;
  } else if (is_mouse_query && mouse_data_ready && !write_data_idle_ready) {
    data = mouse_data;
  } else {
    write_data_idle_ready = true;
    data = write_data_idle;
  }

  digitalWrite(PIN_FROM_KBD, (bool)(data&(1<<bits)));
  bits++;
  if (bits == 22) {
    if (is_keyboard_query && write_data_ready && !write_data_idle_ready)
      write_data_ready = false;
    if (is_mouse_query && mouse_data_ready && !write_data_idle_ready)
      mouse_data_ready = false;
    if (write_data_idle_ready)
      write_data_idle_ready = false;
    bits = 0;
    timerDetachInterrupt(timer1);
    timerAttachInterrupt(timer1, &timer1_read_handler, false);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initialized");
  pinMode(PIN_TO_KBD, INPUT_PULLUP);
  pinMode(PIN_FROM_KBD, OUTPUT);

  timer1 = timerBegin(TIMER1, DIVIDER_100NS, true);
  timerAttachInterrupt(timer1, &timer1_read_handler, false);
  timerAlarmWrite(timer1, NEXT_KBD_TIMING_100NS, true);
  timerSetAutoReload(timer1, true);
  timerAlarmEnable(timer1);

  if (digitalPinToInterrupt(PIN_TO_KBD) == -1)
    Serial.printf("Unable to use PIN_TO_KBD (%d) as interrupt\n", PIN_TO_KBD);
}

char c = 0;
char modifiers = 0;
bool pressed = false;

void loop() {

  if (c == 0 && Serial.available()) {
    char tmpc = Serial.read();
    //    if (tmpc != 0x0A) {
    c = tmpc;
    pressed = true;
    //    }

    if (c == 'Q') {
      c = KEY_APOSTROPHE;
      modifiers = KD_RCOMM;
    } else if (c == 0x0A) {
      c = 0x2A;  // ENTER/RETURN
    } else {
      c = c - ('a' - NEXT_KEY_A);
    }
  }

  if (c != 0 && !write_data_ready) {
    // sendKey(c, pressed, modifiers);
    write_data = 0b1000000000010000000000 | ((c&0x7F) << 1) | (c && !pressed) << 8 | ((modifiers&0x7F) << 12) | ((c&0x7F) || (modifiers&0x7F)) << 19;
    write_data_ready = true;

    if (pressed) {
      pressed = false;
    } else {
      c = 0;
      modifiers = 0;
    }
    Serial.printf("\nSent 0x%x (0x%x). pressed=%d modifiers=0x%x\n", (c&0x7F), c, pressed, modifiers);
  }

  if (read_data_ready) {
    // printf("0x%X\n", read_data);
    if ((read_data & 0x1FF) == KMS_QUERY_KEYBOARD) {
      Serial.printf("K");
    } else if ((read_data & 0x1FF) == KMS_QUERY_MOUSE) {
      Serial.printf("M");
    } else if ((read_data & 0x3FFFFF) == KMS_RESET) {
      Serial.printf("\nRESET\n");
    } else if ((read_data & 0x1FFF) == KMS_LED_PREFIX) {
      Serial.println("");

      if (read_data & KMS_LED_LEFT_MASK)
        Serial.printf("L");
      else
        Serial.printf("l");

      if (read_data & KMS_LED_RIGHT_MASK)
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
