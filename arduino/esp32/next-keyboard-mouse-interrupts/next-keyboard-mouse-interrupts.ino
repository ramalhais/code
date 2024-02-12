// Needs Arduino board esp32 <= 2.0.11 selected in boards manager, for ESP32-USB-Soft-Host to work
//#define FORCE_TEMPLATED_NOPS
#include <ESP32-USB-Soft-Host.h>

#include <ADebouncer.h>

#define PIN_TO_KBD 32    // Input to this keyboard
#define PIN_FROM_KBD 33  // Output from this keyboard

// USB Soft Host Pins
#define DP_P0  16  // D+
#define DM_P0  17  // D-
#define DP_P1  -1 // 22. -1 to disable
#define DM_P1  -1 // 23. -1 to disable
#define DP_P2  -1 // 18. -1 to disable
#define DM_P2  -1 // 19. -1 to disable
#define DP_P3  -1 // 13. -1 to disable
#define DM_P3  -1 // 15. -1 to disable

// NeXT Mouse mini-DIN 8pin: https://deskthority.net/wiki/Bus_mouse#NeXT_.28non-ADB.29
// 8pin mini-DIN. mouse pin1 = 5V. mouse pin8 = GND
// PS2 to NeXT Mouse Arduino code: http://www.asterontech.com/Asterontech/next_ps2_mouse.html
#define NEXT_MOUSE_XA_PIN 18 // mouse pin 2
#define NEXT_MOUSE_XB_PIN 19 // mouse pin 3
#define NEXT_MOUSE_YA_PIN 21 // mouse pin 4
#define NEXT_MOUSE_YB_PIN 25 // mouse pin 5
#define NEXT_MOUSE_RIGHT_PIN 26 // mouse pin 6
#define NEXT_MOUSE_LEFT_PIN 27 // mouse pin 7

typedef struct {
  unsigned int xa : 1;
  unsigned int xb : 1;
  unsigned int ya : 1;
  unsigned int yb : 1;
  unsigned int right : 1;
  unsigned int left : 1;
} next_mouse_data;

// #define DIVIDER_US 80 // 80Mhz / 80 = 1Mhz. 1/f = 1/1000000 = 1 microsecond. Timer ticks every microsecond.
#define DIVIDER_100NS 8 // 80Mhz / 8 = 10Mhz. 1/f = 1/10000000 = 100 nanosecond. Timer ticks every 100 nanoseconds.

#define NEXT_MOUSE_TIMER_NR 2
#define NEXT_MOUSE_TIMING_100NS 530 // *100nanoseconds. use same as keyboard timing to read at least 2 samples. mouse data is sent by keyboard every 2 queries.

next_mouse_data data;
next_mouse_data data_old;
bool next_mouse_changed = false;
#define NEXT_MOUSE_DEBOUNCE_PERIOD_MS 20   // Define the debounce period in milliseconds
ADebouncer next_mouse_left_debouncer, next_mouse_right_debouncer;

inline void next_mouse_read_pins() {
  data_old = data;

  // We can ignore the signal on these pins, because what matters is the quadratic encoding changes
  data.xa = digitalRead(NEXT_MOUSE_XA_PIN);
  data.xb = digitalRead(NEXT_MOUSE_XB_PIN);
  data.ya = digitalRead(NEXT_MOUSE_YA_PIN);
  data.yb = digitalRead(NEXT_MOUSE_YB_PIN);

  data.right = next_mouse_right_debouncer.debounce(!digitalRead(NEXT_MOUSE_RIGHT_PIN));
  data.left = next_mouse_left_debouncer.debounce(!digitalRead(NEXT_MOUSE_LEFT_PIN));
}

inline int next_mouse_diff() {
  return memcmp(&data, &data_old, sizeof(next_mouse_data));
}

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR next_mouse_read_interrupt() {
  next_mouse_read_pins();
  if (next_mouse_diff()) {
    // Update data to be sent via keyboard
    next_mouse_handle_changes();
    next_mouse_changed = true; // Used for printing changes
  }
}

// int quadratic_decoder[16] = {
// // | xaold | xbold | xa | xb | Output |
// // [0b0001] = 1,
// // [0b0111] = 1,
// // [0b1110] = 1,
// // [0b1000] = 1

// // [1] = 1,
// // [7] = 1,
// // [14] = 1,
// // [8] = 1

// 0,1,0,0,0,0,0,1,1,0,0,0,0,0,1,0

// // |   0   |   0   |  1 |  0 |   0    |
// // |   1   |   0   |  1 |  1 |   0    |
// // |   1   |   1   |  0 |  1 |   0    |
// // |   0   |   1   |  0 |  0 |   0    |
// };

#define QUADRATIC_DECODER_BITS 0b0100000110000010;

inline void next_mouse_handle_changes() {
  int dx = 0, dy = 0;
  bool button_left = 0, button_right = 0;

  if (data.xa != data_old.xa || data.xb != data_old.xb) {
    int index = (data_old.xa<<3) | (data_old.xb<<2) | (data.xa<<1) | data.xb;
    int left = (1<<index) & QUADRATIC_DECODER_BITS; // should be right when true, but somehow it's left? even tried reading inverted signal
    dx = left ? -1 : 1;
  }

  if (data.ya != data_old.ya || data.yb != data_old.yb) {
    int index = (data_old.ya<<3) | (data_old.yb<<2) | (data.ya<<1) | data.yb;
    int up = (1<<index) & QUADRATIC_DECODER_BITS; // should be down when true, but somehow it's up? even tried reading inverted signal
    dy = up ? -1 : 1;
  }

  if (data.right != data_old.right) {
    button_right = data.right;
  }

  if (data.left != data_old.left) {
    button_left = data.left;
  }

  queue_mouse_from_interrupt(dx, dy, button_left, button_right);
}

void next_mouse_print_changes() {
  if (!next_mouse_changed) {
    return;
  }

  if (data.xa != data_old.xa || data.xb != data_old.xb) {
    int index = (data_old.xa<<3) | (data_old.xb<<2) | (data.xa<<1) | data.xb;
    int left = (1<<index) & QUADRATIC_DECODER_BITS;
    // printf("\nGoing %s. index=%d. %d %d %d %d\n", left ? "left" : "right", index, data_old.xa, data_old.xb, data.xa, data.xb);
    Serial.printf("\nNeXT Mouse: %s\n", left ? "left" : "right");
  }

  if (data.ya != data_old.ya || data.yb != data_old.yb) {
    int index = (data_old.ya<<3) | (data_old.yb<<2) | (data.ya<<1) | data.yb;
    int up = (1<<index) & QUADRATIC_DECODER_BITS;
    // printf("\nGoing %s. index=%d. %d %d %d %d\n", up ? "up" : "down", index, data_old.ya, data_old.yb, data.ya, data.yb);
    Serial.printf("\nNeXT Mouse: %s\n", up ? "up" : "down");
  }

  if (data.right != data_old.right) {
    Serial.printf("\nNeXT Mouse: right button %s\n", data.right ? "pressed" : "released");
  }

  if (data.left != data_old.left) {
    Serial.printf("\nNeXT Mouse: left button %s\n", data.left ? "pressed" : "released");
  }

  next_mouse_changed = false;
}

void next_mouse_init(hw_timer_t **next_mouse_timer) {
  pinMode(NEXT_MOUSE_XA_PIN, INPUT_PULLUP);
  pinMode(NEXT_MOUSE_XB_PIN, INPUT_PULLUP);
  pinMode(NEXT_MOUSE_YA_PIN, INPUT_PULLUP);
  pinMode(NEXT_MOUSE_YB_PIN, INPUT_PULLUP);
  pinMode(NEXT_MOUSE_RIGHT_PIN, INPUT_PULLUP);
  pinMode(NEXT_MOUSE_LEFT_PIN, INPUT_PULLUP);

  next_mouse_left_debouncer.mode(DELAYED, NEXT_MOUSE_DEBOUNCE_PERIOD_MS, HIGH);
  next_mouse_right_debouncer.mode(DELAYED, NEXT_MOUSE_DEBOUNCE_PERIOD_MS, HIGH);

  *next_mouse_timer = timerBegin(NEXT_MOUSE_TIMER_NR, DIVIDER_100NS, true);
  timerAttachInterrupt(*next_mouse_timer, &next_mouse_read_interrupt, false);
  timerAlarmWrite(*next_mouse_timer, NEXT_MOUSE_TIMING_100NS, true);
  timerSetAutoReload(*next_mouse_timer, true);
  timerAlarmEnable(*next_mouse_timer);
}

// https://github.com/tmk/tmk_keyboard/issues/704
// Also: ASIC pdf says the clock is 18958Hz. 1/18958 = 0.00005274818 with 2% tolerance: (~ 51.72 - 52.75 - 53.80 microseconds)
// #define NEXT_KBD_TIMING_US 52 // *microseconds
// #define NEXT_KBD_TIMING_US 53 // *microseconds
//#define NEXT_KEYBOARD_TIMING_100NS 527 // *100nanoseconds. should be 52.75 for NeXT KMS, but our KMS is 53microseconds
#define NEXT_KEYBOARD_TIMING_100NS 530 // *100nanoseconds. should be 52.75 for NeXT KMS, but our FPGA KMS is 53microseconds
#define NEXT_KEYBOARD_TIMER_NR 1

// packets from LSB(right) to MSB(left). start (low) bit, byte data, command(high)/data(low) bit, stop (high) bit
#define NEXT_KEYBOARD_QUERY_MASK 0x1FF
#define NEXT_KEYBOARD_QUERY_KEYBOARD 0b000100000
#define NEXT_KEYBOARD_QUERY_MOUSE 0b000100010
#define NEXT_KEYBOARD_RESET_MASK 0x3FFFFF
#define NEXT_KEYBOARD_RESET 0b0000000000111111011110 // byte data bit 4, when 0, means it's a write command
#define NEXT_KEYBOARD_LED_MASK 0x1FFF
#define NEXT_KEYBOARD_LED 0b0111000000000
#define NEXT_KEYBOARD_LED_LEFT_MASK 0x2000
#define NEXT_KEYBOARD_LED_RIGHT_MASK 0x4000

#define NEXT_KEYBOARD_DATA_PACKET     0b1000000000010000000000
#define NEXT_KEYBOARD_COMMAND_PACKET  0b1100000000011000000000 // FIXME: check!
#define NEXT_KEYBOARD_IDLE_PACKET     NEXT_KEYBOARD_COMMAND_PACKET
#define NEXT_KEYBOARD_BYTE_MASK       0x7F // 7bits not really a byte :)
#define NEXT_KEYBOARD_KEY_START_BIT     1
#define NEXT_KEYBOARD_KEY_PRESSED_BIT   (NEXT_KEYBOARD_KEY_START_BIT+7)
#define NEXT_KEYBOARD_MODIFIERS_START_BIT 12
#define NEXT_KEYBOARD_VALID_BIT         (NEXT_KEYBOARD_MODIFIERS_START_BIT+7)
#define NEXT_KEYBOARD_MOUSE_BUTTON1_BIT 1
#define NEXT_KEYBOARD_MOUSE_X_START_BIT (NEXT_KEYBOARD_MOUSE_BUTTON1_BIT+1)
#define NEXT_KEYBOARD_MOUSE_BUTTON2_BIT 12
#define NEXT_KEYBOARD_MOUSE_Y_START_BIT (NEXT_KEYBOARD_MOUSE_BUTTON2_BIT+1)

hw_timer_t *next_keyboard_timer;
hw_timer_t *next_mouse_timer;

enum next_keyboard_read_interrupt_state_e {
  STATE_WAITING,
  STATE_READY,
  STATE_READING,
  STATE_SHORT_READ,
  STATE_LONG_READ
};

enum next_keyboard_query_type_e {
  QUERY_NONE,
  QUERY_KEYBOARD,
  QUERY_MOUSE
};

uint32_t read_data = 0;
bool read_data_ready = false;

uint32_t keyboard_data = 0;
bool keyboard_data_ready = false;

uint32_t mouse_data = 0;
bool mouse_data_ready = false;

bool idle_data_ready = false;

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR next_keyboard_write_interrupt();

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR next_keyboard_read_interrupt() {
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
    if ((state == STATE_SHORT_READ && bits == 11) || (state == STATE_LONG_READ && bits == 23)) {  // 11 bits including start and stop bit
      state = STATE_WAITING;
      read_data = data;
      read_data_ready = true;
      timerDetachInterrupt(next_keyboard_timer);
      timerAttachInterrupt(next_keyboard_timer, &next_keyboard_write_interrupt, false);
    }
  }
}

void /*ARDUINO_ISR_ATTR*/ IRAM_ATTR next_keyboard_write_interrupt() {
  static uint8_t bits = 0;
  uint32_t data = 0;
  bool is_keyboard_query = (read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_KEYBOARD;
  bool is_mouse_query = (read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_MOUSE;

  if (is_keyboard_query && keyboard_data_ready && !idle_data_ready) {
    data = keyboard_data;
  } else if (is_mouse_query && mouse_data_ready && !idle_data_ready) {
    data = mouse_data;
  } else {
    idle_data_ready = true;
    data = NEXT_KEYBOARD_IDLE_PACKET;
  }

  digitalWrite(PIN_FROM_KBD, (bool)(data&(1<<bits)));
  bits++;
  if (bits == 22) {
    if (is_keyboard_query && keyboard_data_ready && !idle_data_ready)
      keyboard_data_ready = false;
    if (is_mouse_query && mouse_data_ready && !idle_data_ready)
      mouse_data_ready = false;
    if (idle_data_ready)
      idle_data_ready = false;
    bits = 0;
    timerDetachInterrupt(next_keyboard_timer);
    timerAttachInterrupt(next_keyboard_timer, &next_keyboard_read_interrupt, false);
  }
}

// USB Soft Host stuff
#define HID_INTERFACE_PROTO_KEYBOARD 1
#define HID_INTERFACE_PROTO_MOUSE 2
#define APP_USBD_HID_SUBCLASS_BOOT 1
void ush_handle_interface_descriptor(uint8_t ref, int cfgCount, int sIntfCount, void* Intf, size_t len)
{
  sIntfDesc *sIntf = (sIntfDesc*)Intf;
  Serial.printf("\nUSB HID %s %s: %d:%d:%d\n",
    sIntf->iSub == APP_USBD_HID_SUBCLASS_BOOT ? "(Boot)" : "",
    sIntf->iProto == HID_INTERFACE_PROTO_KEYBOARD ? "Keyboard" : (sIntf->iProto == HID_INTERFACE_PROTO_MOUSE ? "Mouse" : String(sIntf->iProto)),
    ref, cfgCount, sIntfCount);
}

#define USB_MOUSE_BUTTON_LEFT_MASK   1<<0
#define USB_MOUSE_BUTTON_RIGHT_MASK  1<<1
#define USB_MOUSE_BUTTON_MIDDLE_MASK 1<<2
#define USB_MOUSE_SCROLL_UP    1
#define USB_MOUSE_SCROLL_DOWN  -1
typedef struct {
  uint8_t byte0;  // Always 0x1
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // 0x10-0x70 == DOWN, 0xf0-0x80 UP
  uint8_t direction_up;  // 0x00 == DOWN direction? 0xff == UP direction
  int8_t scroll; // 1==up 0xff==down
} usb_mouse_data_6bytes_t; // No name "3D Optical Mouse"

typedef struct {
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // signed. >0 DOWN, <0 UP
  int8_t scroll; // 1==up 0xff==down
} usb_mouse_data_4bytes_t; // Microsoft Wheel Mouse Optical 1.1A USB and PS/2 Compatible

// NeXT keycodes:
// https://github.com/tmk/tmk_keyboard/blob/master/converter/next_usb/unimap_trans.h
// https://web.archive.org/web/20150608141822/http://www.68k.org/~degs/nextkeyboard.html

uint16_t ascii2next[256];

// Modifiers
#define KD_CNTL 0x01
#define KD_LSHIFT 0x02
#define KD_RSHIFT 0x04
#define KD_LCOMM 0x08
#define KD_RCOMM 0x10
#define KD_LALT 0x20
#define KD_RALT 0x40

// Keys
#define NEXT_KEY_NO_KEY 0
#define NEXT_KEY_BACKSPACE 0x1B
#define NEXT_KEY_HORIZONTAL_TAB 0x41
#define NEXT_KEY_ENTER 0x2A
#define NEXT_KEY_ESCAPE 0x49
#define NEXT_KEY_SPACE 0x38
#define NEXT_KEY_EXCLAMATION 0x4A|KD_LSHIFT<<8
#define NEXT_KEY_DOUBLE_QUOTES 0x2B|KD_LSHIFT<<8
#define NEXT_KEY_NUMBER_SIGN 0x4C|KD_LSHIFT<<8// hash
#define NEXT_KEY_DOLLAR 0x4D|KD_LSHIFT<<8
#define NEXT_KEY_PERCENT 0x50|KD_LSHIFT<<8
#define NEXT_KEY_AMPERSAND 0x4E|KD_LSHIFT<<8
#define NEXT_KEY_SINGLE_QUOTE 0x2B|KD_LSHIFT<<8
#define NEXT_KEY_LEFT_PARENTHESIS 0x1F|KD_LSHIFT<<8
#define NEXT_KEY_RIGHT_PARENTHESIS 0x20|KD_LSHIFT<<8
#define NEXT_KEY_ASTERISK 0x1E|KD_LSHIFT<<8
#define NEXT_KEY_PLUS 0x1C|KD_LSHIFT<<8
#define NEXT_KEY_COMMA 0x2E|KD_LSHIFT<<8
#define NEXT_KEY_MINUS 0x1D
#define NEXT_KEY_PERIOD 0x2F
#define NEXT_KEY_SLASH 0x30
#define NEXT_KEY_ZERO 0x20
#define NEXT_KEY_ONE 0x4A
#define NEXT_KEY_TWO 0x4B
#define NEXT_KEY_THREE 0x4C
#define NEXT_KEY_FOUR 0x4D
#define NEXT_KEY_FIVE 0x50
#define NEXT_KEY_SIX 0x4F
#define NEXT_KEY_SEVEN 0x4E
#define NEXT_KEY_EIGHT 0x1E
#define NEXT_KEY_NINE 0x1F
#define NEXT_KEY_COLON 0x2C|KD_LSHIFT<<8
#define NEXT_KEY_SEMICOLON 0x2C
#define NEXT_KEY_LESS_THAN 0x2E|KD_LSHIFT<<8
#define NEXT_KEY_EQUALS 0x1C
#define NEXT_KEY_GREATER_THAN 0x2F|KD_LSHIFT<<8
#define NEXT_KEY_QUESTION_MARK 0x30|KD_LSHIFT<<8
#define NEXT_KEY_AT_SIGN 0x4B|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_A 0x39|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_B 0x35|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_C 0x33|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_D 0x3B|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_E 0x44|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_F 0x3C|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_G 0x3D|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_H 0x40|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_I 0x06|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_J 0x3F|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_K 0x3E|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_L 0x2D|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_M 0x36|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_N 0x37|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_O 0x07|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_P 0x08|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Q 0x42|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_R 0x45|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_S 0x3A|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_T 0x48|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_U 0x46|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_V 0x34|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_W 0x43|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_X 0x32|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Y 0x47|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Z 0x31|KD_LSHIFT<<8
#define NEXT_KEY_OPENING_BRACKET 0x05
#define NEXT_KEY_BACKSLASH 0x28|KD_LSHIFT<<8
#define NEXT_KEY_CLOSING_BRACKET 0x04
#define NEXT_KEY_CARET_CIRCUMFLEX 0x4F|KD_LSHIFT<<8
#define NEXT_KEY_UNDERSCORE 0x1D|KD_LSHIFT<<8
#define NEXT_KEY_GRAVE_ACCENT 0x26
#define NEXT_KEY_LOWERCASE_A 0x39
#define NEXT_KEY_LOWERCASE_B 0x35
#define NEXT_KEY_LOWERCASE_C 0x33
#define NEXT_KEY_LOWERCASE_D 0x3B
#define NEXT_KEY_LOWERCASE_E 0x44
#define NEXT_KEY_LOWERCASE_F 0x3C
#define NEXT_KEY_LOWERCASE_G 0x3D
#define NEXT_KEY_LOWERCASE_H 0x40
#define NEXT_KEY_LOWERCASE_I 0x06
#define NEXT_KEY_LOWERCASE_J 0x3F
#define NEXT_KEY_LOWERCASE_K 0x3E
#define NEXT_KEY_LOWERCASE_L 0x2D
#define NEXT_KEY_LOWERCASE_M 0x36
#define NEXT_KEY_LOWERCASE_N 0x37
#define NEXT_KEY_LOWERCASE_O 0x07
#define NEXT_KEY_LOWERCASE_P 0x08
#define NEXT_KEY_LOWERCASE_Q 0x42
#define NEXT_KEY_LOWERCASE_R 0x45
#define NEXT_KEY_LOWERCASE_S 0x3A
#define NEXT_KEY_LOWERCASE_T 0x48
#define NEXT_KEY_LOWERCASE_U 0x46
#define NEXT_KEY_LOWERCASE_V 0x34
#define NEXT_KEY_LOWERCASE_W 0x43
#define NEXT_KEY_LOWERCASE_X 0x32
#define NEXT_KEY_LOWERCASE_Y 0x47
#define NEXT_KEY_LOWERCASE_Z 0x31
#define NEXT_KEY_OPENING_BRACE 0x05|KD_LSHIFT<<8
#define NEXT_KEY_VERTICAL_BAR 0x27|KD_LSHIFT<<8
#define NEXT_KEY_CLOSING_BRACE 0x04|KD_LSHIFT<<8
#define NEXT_KEY_TILDE 0x26|KD_LSHIFT<<8
#define NEXT_KEY_DELETE 0x1B|KD_LSHIFT<<8 // FIXME: Made it up

void ascii_init() {
  ascii2next[8] = NEXT_KEY_BACKSPACE;
  ascii2next[9] = NEXT_KEY_HORIZONTAL_TAB;
  ascii2next[10] = NEXT_KEY_ENTER;
  ascii2next[27] = NEXT_KEY_ESCAPE;
  ascii2next[' '] = NEXT_KEY_SPACE;
  ascii2next['!'] = NEXT_KEY_EXCLAMATION;
  ascii2next['"'] = NEXT_KEY_DOUBLE_QUOTES;
  ascii2next['#'] = NEXT_KEY_NUMBER_SIGN;
  ascii2next['$'] = NEXT_KEY_DOLLAR;
  ascii2next['%'] = NEXT_KEY_PERCENT;
  ascii2next['&'] = NEXT_KEY_AMPERSAND;
  ascii2next['\''] = NEXT_KEY_SINGLE_QUOTE;
  ascii2next['('] = NEXT_KEY_LEFT_PARENTHESIS;
  ascii2next[')'] = NEXT_KEY_RIGHT_PARENTHESIS;
  ascii2next['*'] = NEXT_KEY_ASTERISK;
  ascii2next['+'] = NEXT_KEY_PLUS;
  ascii2next[','] = NEXT_KEY_COMMA;
  ascii2next['-'] = NEXT_KEY_MINUS;
  ascii2next['.'] = NEXT_KEY_PERIOD;
  ascii2next['/'] = NEXT_KEY_SLASH;
  ascii2next['0'] = NEXT_KEY_ZERO;
  ascii2next['1'] = NEXT_KEY_ONE;
  ascii2next['2'] = NEXT_KEY_TWO;
  ascii2next['3'] = NEXT_KEY_THREE;
  ascii2next['4'] = NEXT_KEY_FOUR;
  ascii2next['5'] = NEXT_KEY_FIVE;
  ascii2next['6'] = NEXT_KEY_SIX;
  ascii2next['7'] = NEXT_KEY_SEVEN;
  ascii2next['8'] = NEXT_KEY_EIGHT;
  ascii2next['9'] = NEXT_KEY_NINE;
  ascii2next[':'] = NEXT_KEY_COLON;
  ascii2next[';'] = NEXT_KEY_SEMICOLON;
  ascii2next['<'] = NEXT_KEY_LESS_THAN;
  ascii2next['='] = NEXT_KEY_EQUALS;
  ascii2next['>'] = NEXT_KEY_GREATER_THAN;
  ascii2next['?'] = NEXT_KEY_QUESTION_MARK;
  ascii2next['@'] = NEXT_KEY_AT_SIGN;
  ascii2next['A'] = NEXT_KEY_UPPERCASE_A;
  ascii2next['B'] = NEXT_KEY_UPPERCASE_B;
  ascii2next['C'] = NEXT_KEY_UPPERCASE_C;
  ascii2next['D'] = NEXT_KEY_UPPERCASE_D;
  ascii2next['E'] = NEXT_KEY_UPPERCASE_E;
  ascii2next['F'] = NEXT_KEY_UPPERCASE_F;
  ascii2next['G'] = NEXT_KEY_UPPERCASE_G;
  ascii2next['H'] = NEXT_KEY_UPPERCASE_H;
  ascii2next['I'] = NEXT_KEY_UPPERCASE_I;
  ascii2next['J'] = NEXT_KEY_UPPERCASE_J;
  ascii2next['K'] = NEXT_KEY_UPPERCASE_K;
  ascii2next['L'] = NEXT_KEY_UPPERCASE_L;
  ascii2next['M'] = NEXT_KEY_UPPERCASE_M;
  ascii2next['N'] = NEXT_KEY_UPPERCASE_N;
  ascii2next['O'] = NEXT_KEY_UPPERCASE_O;
  ascii2next['P'] = NEXT_KEY_UPPERCASE_P;
  ascii2next['Q'] = NEXT_KEY_UPPERCASE_Q;
  ascii2next['R'] = NEXT_KEY_UPPERCASE_R;
  ascii2next['S'] = NEXT_KEY_UPPERCASE_S;
  ascii2next['T'] = NEXT_KEY_UPPERCASE_T;
  ascii2next['U'] = NEXT_KEY_UPPERCASE_U;
  ascii2next['V'] = NEXT_KEY_UPPERCASE_V;
  ascii2next['W'] = NEXT_KEY_UPPERCASE_W;
  ascii2next['X'] = NEXT_KEY_UPPERCASE_X;
  ascii2next['Y'] = NEXT_KEY_UPPERCASE_Y;
  ascii2next['Z'] = NEXT_KEY_UPPERCASE_Z;
  ascii2next['['] = NEXT_KEY_OPENING_BRACKET;
  ascii2next['\\'] = NEXT_KEY_BACKSLASH;
  ascii2next[']'] = NEXT_KEY_CLOSING_BRACKET;
  ascii2next['^'] = NEXT_KEY_CARET_CIRCUMFLEX;
  ascii2next['_'] = NEXT_KEY_UNDERSCORE;
  ascii2next['`'] = NEXT_KEY_GRAVE_ACCENT;
  ascii2next['a'] = NEXT_KEY_LOWERCASE_A;
  ascii2next['b'] = NEXT_KEY_LOWERCASE_B;
  ascii2next['c'] = NEXT_KEY_LOWERCASE_C;
  ascii2next['d'] = NEXT_KEY_LOWERCASE_D;
  ascii2next['e'] = NEXT_KEY_LOWERCASE_E;
  ascii2next['f'] = NEXT_KEY_LOWERCASE_F;
  ascii2next['g'] = NEXT_KEY_LOWERCASE_G;
  ascii2next['h'] = NEXT_KEY_LOWERCASE_H;
  ascii2next['i'] = NEXT_KEY_LOWERCASE_I;
  ascii2next['j'] = NEXT_KEY_LOWERCASE_J;
  ascii2next['k'] = NEXT_KEY_LOWERCASE_K;
  ascii2next['l'] = NEXT_KEY_LOWERCASE_L;
  ascii2next['m'] = NEXT_KEY_LOWERCASE_M;
  ascii2next['n'] = NEXT_KEY_LOWERCASE_N;
  ascii2next['o'] = NEXT_KEY_LOWERCASE_O;
  ascii2next['p'] = NEXT_KEY_LOWERCASE_P;
  ascii2next['q'] = NEXT_KEY_LOWERCASE_Q;
  ascii2next['r'] = NEXT_KEY_LOWERCASE_R;
  ascii2next['s'] = NEXT_KEY_LOWERCASE_S;
  ascii2next['t'] = NEXT_KEY_LOWERCASE_T;
  ascii2next['u'] = NEXT_KEY_LOWERCASE_U;
  ascii2next['v'] = NEXT_KEY_LOWERCASE_V;
  ascii2next['w'] = NEXT_KEY_LOWERCASE_W;
  ascii2next['x'] = NEXT_KEY_LOWERCASE_X;
  ascii2next['y'] = NEXT_KEY_LOWERCASE_Y;
  ascii2next['z'] = NEXT_KEY_LOWERCASE_Z;
  ascii2next['{'] = NEXT_KEY_OPENING_BRACE;
  ascii2next['|'] = NEXT_KEY_VERTICAL_BAR;
  ascii2next['}'] = NEXT_KEY_CLOSING_BRACE;
  ascii2next['~'] = NEXT_KEY_TILDE;
  ascii2next[0x7F] = NEXT_KEY_DELETE;
}

#define USB_MOUSE_4BYTES_NAME "USB Mouse (4 Bytes)"
static void handle_mouse_data_4bytes(uint8_t* data) {
  usb_mouse_data_4bytes_t *mouse_data = (usb_mouse_data_4bytes_t *)data;

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );

  // Serial.printf("\n%s: L=%d R=%d M=%d X=%d Y=%d S=%s\n",
  //   USB_MOUSE_4BYTES_NAME,
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK),
  //   mouse_data->x,
  //   mouse_data->y,
  //   mouse_data->scroll == USB_MOUSE_SCROLL_UP ? "UP" : (mouse_data->scroll == USB_MOUSE_SCROLL_DOWN ? "DOWN" : "_")
  // );
}

#define USB_MOUSE_6BYTES_NAME "USB Mouse (6 Bytes)"
static void handle_mouse_data_6bytes(uint8_t* data) {
  usb_mouse_data_6bytes_t *mouse_data = (usb_mouse_data_6bytes_t *)data;

  mouse_data->y /= 16; // Something wrong with the value reported by the mouse

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );

  // Serial.printf("\n%s: L=%d R=%d M=%d X=%d Y=%d S=%s\n",
  //   USB_MOUSE_6BYTES_NAME,
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK),
  //   mouse_data->x,
  //   mouse_data->y,
  //   mouse_data->scroll == USB_MOUSE_SCROLL_UP ? "UP" : (mouse_data->scroll == USB_MOUSE_SCROLL_DOWN ? "DOWN" : "_")
  // );
}

// FIXME: Instead of this hack, we should get the mouse report descriptor, and parse it to know where the data really is
static void ush_handle_data(uint8_t usbNum, uint8_t byte_depth, uint8_t* data, uint8_t data_len)
{

  // if( myListenUSBPort != usbNum ) return;
  // printf("\nDATA: usbNum=%d byte_depth=%d data_len=%d\n", usbNum, byte_depth, data_len);
  // for(int k=0;k<data_len;k++) {
  //   printf("0x%02x ", data[k] );
  // }
  // printf("\n");

  // FIXME: We could also send PGUP PGDOWN keystrokes (press and release) on scroll up and down 
  if (data_len == 4)
    handle_mouse_data_4bytes(data);
  else if (data_len == 6)
    handle_mouse_data_6bytes(data);
  else
    Serial.printf("Unknown mouse data length=%d\n", data_len);
}

usb_pins_config_t USB_Pins_Config =
{
  DP_P0, DM_P0,
  DP_P1, DM_P1,
  DP_P2, DM_P2,
  DP_P3, DM_P3
};

const char* bit_rep[16] = {
  [0] = "0000",
  [1] = "0001",
  [2] = "0010",
  [3] = "0011",
  [4] = "0100",
  [5] = "0101",
  [6] = "0110",
  [7] = "0111",
  [8] = "1000",
  [9] = "1001",
  [10] = "1010",
  [11] = "1011",
  [12] = "1100",
  [13] = "1101",
  [14] = "1110",
  [15] = "1111",
};

void print_byte(uint8_t byte) {
  Serial.printf("%s%s.", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}

#define NEXT_KEYBOARD_FROM_KBD_INITIAL_LEVEL HIGH
void next_keyboard_init(hw_timer_t **next_keyboard_timer) {
  pinMode(PIN_TO_KBD, INPUT_PULLUP);
  pinMode(PIN_FROM_KBD, OUTPUT);
  digitalWrite(PIN_FROM_KBD, NEXT_KEYBOARD_FROM_KBD_INITIAL_LEVEL);

  *next_keyboard_timer = timerBegin(NEXT_KEYBOARD_TIMER_NR, DIVIDER_100NS, true);
  timerAttachInterrupt(*next_keyboard_timer, &next_keyboard_read_interrupt, false);
  timerAlarmWrite(*next_keyboard_timer, NEXT_KEYBOARD_TIMING_100NS, true);
  timerSetAutoReload(*next_keyboard_timer, true);
  timerAlarmEnable(*next_keyboard_timer);
}

void usb_mouse_init() {
  USH.init(USB_Pins_Config, NULL, NULL);
  USH.setPrintCb(ush_handle_data);
  USH.setOnIfaceDescCb(ush_handle_interface_descriptor);
}

char c = 0;
char modifiers = 0;
bool pressed = false;

int queue_keyboard(uint8_t key, bool pressed, uint8_t modifiers) { // TODO: turn this into a real queue
  int tries = 10;
  while (tries && keyboard_data_ready) {
    printf("\nWARNING: queue_keyboard retrying...\n");
    delayMicroseconds(NEXT_KEYBOARD_TIMING_100NS/10);
    tries--;
  }

  if (tries) {
    keyboard_data = NEXT_KEYBOARD_DATA_PACKET |
      ((key&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_KEY_START_BIT) | (key && !pressed) << NEXT_KEYBOARD_KEY_PRESSED_BIT | // byte1
      ((modifiers&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MODIFIERS_START_BIT) | (((key&NEXT_KEYBOARD_BYTE_MASK) || (modifiers&NEXT_KEYBOARD_BYTE_MASK)) << NEXT_KEYBOARD_VALID_BIT); // byte2
    keyboard_data_ready = true;
    return true;
  } else {
    Serial.printf("\nERROR: queue_keyboard failed to send mouse event!\n");
    return false;
  }
}

int queue_mouse_from_interrupt(int8_t mousex, int8_t mousey, bool button1, bool button2) {
  int tries = 10;
  while (tries && mouse_data_ready) { // TODO: we really need to turn this into a real queue or update data. We can't have a while loop and a delay during an interrupt
    delayMicroseconds(NEXT_KEYBOARD_TIMING_100NS/10);
    tries--;
  }

  if (tries) {
    uint8_t dx = ((mousex&0x80)>>1) | (mousex&0x3F); // sign+6bits
    uint8_t dy = ((mousey&0x80)>>1) | (mousey&0x3F); // sign+6bits

    mouse_data = NEXT_KEYBOARD_DATA_PACKET |
      (!button1 << NEXT_KEYBOARD_MOUSE_BUTTON1_BIT) | (((~dx)&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MOUSE_X_START_BIT) | // byte1
      (!button2 << NEXT_KEYBOARD_MOUSE_BUTTON2_BIT) | (((~dy)&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MOUSE_Y_START_BIT); // byte2

    mouse_data_ready = true;
    return true;
  } else {
    return false;
  }
}

void next_keyboard_mouse_print_changes() {
  if (mouse_data_ready) {
    bool button1 = !(mouse_data&(1<<NEXT_KEYBOARD_MOUSE_BUTTON1_BIT));
    bool button2 = !(mouse_data&(1<<NEXT_KEYBOARD_MOUSE_BUTTON2_BIT));
    uint8_t tmpx = ~((mouse_data>>NEXT_KEYBOARD_MOUSE_X_START_BIT)&NEXT_KEYBOARD_BYTE_MASK);
    uint8_t tmpy = ~((mouse_data>>NEXT_KEYBOARD_MOUSE_Y_START_BIT)&NEXT_KEYBOARD_BYTE_MASK);
    int8_t dx = (tmpx&0x70)<<1|(tmpx&0x3F);
    int8_t dy = (tmpy&0x70)<<1|(tmpy&0x3F);;
    Serial.printf("\nmouse_data: L=%d R=%d dx=%d dy=%d mouse_data=0x%x \n", button1, button2, dx, dy, mouse_data);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting NeXT Keyboard\n");
  ascii_init();

  // USB Mouse. USB Soft Host needs to be initialized before next_keyboard_timer and next_mouse_timer below or USB won't work.
  usb_mouse_init();
  // This emulated NeXT Keyboard
  next_keyboard_init(&next_keyboard_timer);
  // Real NeXT Mouse
  next_mouse_init(&next_mouse_timer);
}

void loop() {
  // next_mouse_print_changes();
  next_keyboard_mouse_print_changes();

  if (c == 0 && Serial.available()) {
    char tmpc = Serial.read();
    Serial.printf("\n%c 0x%x\n", tmpc, tmpc);
    //    if (tmpc != 0x0A) {
    c = tmpc;
    pressed = true;
    //    }

    if (c == '%') {
      c = NEXT_KEY_GRAVE_ACCENT;
      modifiers = KD_RCOMM;
    } else if (c == '^') {
      c = NEXT_KEY_LOWERCASE_C;
      modifiers = KD_CNTL;
    } else if (c == '\'') {
      c = NEXT_KEY_HORIZONTAL_TAB;
    } else {
      modifiers = (ascii2next[c]&0xff00)>>8;
      c = ascii2next[c];
    }
  }

  if (c != 0) {
    if (queue_keyboard(c, pressed, modifiers)) {
      Serial.printf("\nQueued 0x%x (0x%x). pressed=%d modifiers=0x%x\n", (c&NEXT_KEYBOARD_BYTE_MASK), c, pressed, modifiers);
      if (pressed) {
        pressed = false;
      } else {
        c = 0;
        modifiers = 0;
      }
    }
  }

  if (read_data_ready) {
    // printf("0x%X\n", read_data);
    if ((read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_KEYBOARD) {
      Serial.print("K");
    } else if ((read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_MOUSE) {
      Serial.print("M");
    } else if ((read_data & NEXT_KEYBOARD_RESET_MASK) == NEXT_KEYBOARD_RESET) {
      Serial.print("_RESET_");
    } else if ((read_data & NEXT_KEYBOARD_LED_MASK) == NEXT_KEYBOARD_LED) {
      Serial.println("");

      if (read_data & NEXT_KEYBOARD_LED_LEFT_MASK)
        Serial.printf("L");
      else
        Serial.printf("l");

      if (read_data & NEXT_KEYBOARD_LED_RIGHT_MASK)
        Serial.printf("R");
      else
        Serial.printf("r");

      Serial.println("");
    } else {
      Serial.print("?");
      // print_byte((read_data >> 16) & 0xff);
      // print_byte((read_data >> 8) & 0xff);
      // print_byte(read_data & 0xff);
      // Serial.println("");
    }
    read_data_ready = false;
  }
}
