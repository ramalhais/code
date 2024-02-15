// Needs Arduino board esp32 <= 2.0.11 selected in boards manager, for ESP32-USB-Soft-Host to work
//#define FORCE_TEMPLATED_NOPS
#include <ESP32-USB-Soft-Host.h>
#include <ADebouncer.h>
#include "next-keycodes.h"

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
  #define DEBUG_ALL
#endif

#ifdef DEBUG_ALL
#define Dprintf Serial.printf
#else
#define Dprintf (void)
#endif

#define PIN_TO_KBD 32    // Input to this keyboard
#define PIN_FROM_KBD 33  // Output from this keyboard
#define PIN_POWER_ON 23

void poweron_init() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, LOW);
}

void poweron_toggle() {
  digitalWrite(PIN_POWER_ON, HIGH);
  delayMicroseconds(500); // 400microseconds seems to be the minimum for power on "key" to be detected
  digitalWrite(PIN_POWER_ON, LOW);
}

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
    // Dprintf("\nGoing %s. index=%d. %d %d %d %d\n", left ? "left" : "right", index, data_old.xa, data_old.xb, data.xa, data.xb);
    Dprintf("\nNeXT Mouse: %s\n", left ? "left" : "right");
  }

  if (data.ya != data_old.ya || data.yb != data_old.yb) {
    int index = (data_old.ya<<3) | (data_old.yb<<2) | (data.ya<<1) | data.yb;
    int up = (1<<index) & QUADRATIC_DECODER_BITS;
    // Dprintf("\nGoing %s. index=%d. %d %d %d %d\n", up ? "up" : "down", index, data_old.ya, data_old.yb, data.ya, data.yb);
    Dprintf("\nNeXT Mouse: %s\n", up ? "up" : "down");
  }

  if (data.right != data_old.right) {
    Dprintf("\nNeXT Mouse: right button %s\n", data.right ? "pressed" : "released");
  }

  if (data.left != data_old.left) {
    Dprintf("\nNeXT Mouse: left button %s\n", data.left ? "pressed" : "released");
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

// packets to send. includes start (low/0) and stop (high/1) bits
#define NEXT_KEYBOARD_DATA_PACKET     0b1000000000010000000000
#define NEXT_KEYBOARD_COMMAND_PACKET  0b1000000000011000000000 // FIXME: a command packet should have last bit before stop bit as 0 to signify a data byte (2nd byte) instead of a command.
#define NEXT_KEYBOARD_IDLE_PACKET     0b1100000000011000000000
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
    if (all_high_counter == 5) {  // sense 8 continuous bits high. works with some resets. also works from 1 to 8. 5 seems to work best
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
    if (!idle_data_ready) {
      if (is_keyboard_query && keyboard_data_ready)
        keyboard_data_ready = false;
      else if (is_mouse_query && mouse_data_ready)
        mouse_data_ready = false;
    } else
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

#define USB_MOUSE_4BYTES_NAME "USB Mouse (4 Bytes)"
static void handle_mouse_data_4bytes(uint8_t* data) {
  usb_mouse_data_4bytes_t *mouse_data = (usb_mouse_data_4bytes_t *)data;

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );
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

  // Dprintf("\n%s: L=%d R=%d M=%d X=%d Y=%d S=%s\n",
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
  // Dprintf("\nDATA: usbNum=%d byte_depth=%d data_len=%d\n", usbNum, byte_depth, data_len);
  // for(int k=0;k<data_len;k++) {
  //   Dprintf("0x%02x ", data[k] );
  // }
  // Dprintf("\n");

  // FIXME: We could also send PGUP PGDOWN keystrokes (press and release) on scroll up and down 
  if (data_len == 4)
    handle_mouse_data_4bytes(data);
  else if (data_len == 6)
    handle_mouse_data_6bytes(data);
  else
    Serial.printf("WARNING: Unknown mouse data length=%d\n", data_len);
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
  Dprintf("%s%s.", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
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
    Dprintf("\nWARNING: queue_keyboard retrying...\n");
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
    Serial.printf("\nERROR: queue_keyboard failed to send event!\n");
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
    mouse_data = NEXT_KEYBOARD_DATA_PACKET |
      (((!button1)&0x01) << NEXT_KEYBOARD_MOUSE_BUTTON1_BIT) |
      (((!button2)&0x01) << NEXT_KEYBOARD_MOUSE_BUTTON2_BIT);

    if (mousex) {
      // uint8_t dx = ((mousex&0x80)>>1) | (mousex&0x3F); // sign+6bits
      // mouse_data |= ((~dx)&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MOUSE_X_START_BIT;

      // uint8_t datax = ((!(mousex<0))<<6) | ((mousex > 0 ? ~abs(mousex) : abs(mousex))&0x3F); // sign+6bits
      uint8_t datax = (((-mousex)&0x80)>>1) | (-mousex)&0x3F; // sign+6bits
      mouse_data |= datax << NEXT_KEYBOARD_MOUSE_X_START_BIT;
    }

    if (mousey) {
      // uint8_t dy = ((mousey&0x80)>>1) | (mousey&0x3F); // sign+6bits
      // mouse_data |= ((~dy)&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MOUSE_Y_START_BIT;
      uint8_t datay = (((-mousey)&0x80)>>1) | (-mousey)&0x3F; // sign+6bits
      mouse_data |= datay << NEXT_KEYBOARD_MOUSE_Y_START_BIT;
    }
// mouse_data=0b1000000001010111110110;
    mouse_data_ready = true;
    return true;
  } else {
    return false;
  }
}

void next_keyboard_mouse_print_changes() {
  if (mouse_data_ready) {
    bool button1 = mouse_data&(1<<NEXT_KEYBOARD_MOUSE_BUTTON1_BIT);
    bool button2 = mouse_data&(1<<NEXT_KEYBOARD_MOUSE_BUTTON2_BIT);
    uint8_t dx = (mouse_data>>NEXT_KEYBOARD_MOUSE_X_START_BIT)&NEXT_KEYBOARD_BYTE_MASK;
    uint8_t dy = (mouse_data>>NEXT_KEYBOARD_MOUSE_Y_START_BIT)&NEXT_KEYBOARD_BYTE_MASK;
    Dprintf("\nmouse_data on wire: L=%d R=%d dx=0x%x dy=0x%x mouse_data=0x%x \n", button1, button2, dx, dy, mouse_data);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting NeXT Keyboard\n");
  poweron_init();
  ascii_init();

  // USB Mouse. USB Soft Host needs to be initialized before next_keyboard_timer and next_mouse_timer below or USB won't work.
  usb_mouse_init();
  // This emulated NeXT Keyboard
  next_keyboard_init(&next_keyboard_timer);
  // Real NeXT Mouse
  next_mouse_init(&next_mouse_timer);
}

void loop() {
  static bool enable_key_enter = false;

  // next_mouse_print_changes();
  next_keyboard_mouse_print_changes();

  if (c == 0 && Serial.available()) {
    char tmpc = Serial.read();
    Dprintf("\n%c 0x%x\n", tmpc, tmpc);
    //    if (tmpc != 0x0A) {
    c = tmpc;
    pressed = true;
    //    }

    if (c == 0x0A) {
      // ENTER
      if (enable_key_enter) {
        c = NEXT_KEY_ENTER;
      } else {
        enable_key_enter = true;
        c = 0;
        pressed = false;
      }
    } else {
      enable_key_enter = false;
      if (c == '%') {
        // Go to PROM
        c = NEXT_KEY_GRAVE_ACCENT;
        modifiers = KD_RCOMM;
      } else if (c == '^') {
        // Ctrl+C
        c = NEXT_KEY_LOWERCASE_C;
        modifiers = KD_CNTL;
      } else if (c == '\'') {
        // Power Key
        Serial.printf("\nToggling power switch\n");
        poweron_toggle();
        c = 0;
        pressed = false;

        // c = NEXT_KEY_HORIZONTAL_TAB;
      } else {
        modifiers = (ascii2next[c]&0xff00)>>8;
        c = ascii2next[c];
      }
    }
  }

  if (c != 0) {
    if (queue_keyboard(c, pressed, modifiers)) {
      Dprintf("\nQueued 0x%x (0x%x). pressed=%d modifiers=0x%x\n", (c&NEXT_KEYBOARD_BYTE_MASK), c, pressed, modifiers);
      if (pressed) {
        pressed = false;
      } else {
        c = 0;
        modifiers = 0;
      }
    }
  }

  if (read_data_ready) {
    // Dprintf("0x%X\n", read_data);
    if ((read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_KEYBOARD) {
      Dprintf("K");
    } else if ((read_data & NEXT_KEYBOARD_QUERY_MASK) == NEXT_KEYBOARD_QUERY_MOUSE) {
      Dprintf("M");
    } else if ((read_data & NEXT_KEYBOARD_RESET_MASK) == NEXT_KEYBOARD_RESET) {
      Dprintf("_RESET_");
    } else if ((read_data & NEXT_KEYBOARD_LED_MASK) == NEXT_KEYBOARD_LED) {
      Dprintf("\n");

      if (read_data & NEXT_KEYBOARD_LED_LEFT_MASK)
        Dprintf("L");
      else
        Dprintf("l");

      if (read_data & NEXT_KEYBOARD_LED_RIGHT_MASK)
        Dprintf("R");
      else
        Dprintf("r");

      Dprintf("\n");
    } else {
      Dprintf("?");
      // print_byte((read_data >> 16) & 0xff);
      // print_byte((read_data >> 8) & 0xff);
      // print_byte(read_data & 0xff);
      // Dprintf("\n");
    }
    read_data_ready = false;
  }
  delay(10); // Somehow queue_keyboard can't be fast enough? weird
}
