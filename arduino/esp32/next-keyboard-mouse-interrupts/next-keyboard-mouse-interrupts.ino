#include <ADebouncer.h>
#include "next-keycodes.h"
#include <cppQueue.h>

hw_timer_t *next_keyboard_timer;
hw_timer_t *next_mouse_timer;

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO//ARDUHAL_LOG_LEVEL_DEBUG
  #define DEBUG_ALL
#endif

#ifdef DEBUG_ALL
#define Dprintf Serial.printf
#else
#define Dprintf (void)
#endif

#if defined(ARDUINO_ESP32S3_DEV)
  // ESP32S3_DEV: Don't use: strapping pins: 0,3,45,46. PSRAM: 35,36,37. USB: 19,20. Serial UART: 43,44

  // #define RGB_ENABLED 1
  // #define USB_HOST_ENABLED 1 // Hardware USB Host
  // #define USB_SOFTHOST_ENABLED 1 // Software USB Host
  #define USB_CH9350_ENABLED 1
  #define USB_CH9350_SERIAL Serial1

  #define RGB_PIN 48

  // This NeXT Keyboard Pins
  #define PIN_TO_KBD 4    // Input to this keyboard
  #define PIN_FROM_KBD 5  // Output from this keyboard
  #define PIN_POWER_ON 6

  // USB Soft Host Pins. Must be between pins 8-31.
  #define DP_P0  11  // D+ (DM2) somehow these seem to be switched up
  #define DM_P0  12  // D- (DP1)
  #define DP_P1  -1 // 22. -1 to disable
  #define DM_P1  -1 // 23. -1 to disable
  #define DP_P2  -1 // 18. -1 to disable
  #define DM_P2  -1 // 19. -1 to disable
  #define DP_P3  -1 // 13. -1 to disable
  #define DM_P3  -1 // 15. -1 to disable

  // NeXT Mouse mini-DIN 8pin: https://deskthority.net/wiki/Bus_mouse#NeXT_.28non-ADB.29
  // 8pin mini-DIN. mouse pin1 = 5V. mouse pin8 = GND
  #define NEXT_MOUSE_XA_PIN 7 // mouse pin 2
  #define NEXT_MOUSE_XB_PIN 15 // mouse pin 3
  #define NEXT_MOUSE_YA_PIN 16 // mouse pin 4
  #define NEXT_MOUSE_YB_PIN 17 // mouse pin 5
  #define NEXT_MOUSE_RIGHT_PIN 18 // mouse pin 6
  #define NEXT_MOUSE_LEFT_PIN 8 // mouse pin 7

  #define USB_SERIAL_RX 9
  #define USB_SERIAL_TX 10

#elif defined(ARDUINO_ESP32_DEV)
  #define USB_CH9350_ENABLED 1
  #define USB_CH9350_SERIAL Serial1

  // This NeXT Keyboard Pins
  #define PIN_TO_KBD 32    // Input to this keyboard
  #define PIN_FROM_KBD 33  // Output from this keyboard
  #define PIN_POWER_ON 23

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
  #define NEXT_MOUSE_XA_PIN 18 // mouse pin 2
  #define NEXT_MOUSE_XB_PIN 19 // mouse pin 3
  #define NEXT_MOUSE_YA_PIN 21 // mouse pin 4
  #define NEXT_MOUSE_YB_PIN 25 // mouse pin 5
  #define NEXT_MOUSE_RIGHT_PIN 26 // mouse pin 6
  #define NEXT_MOUSE_LEFT_PIN 27 // mouse pin 7
#else
  #error Arduino board not supported. Please add your pin mappings
#endif

#ifdef RGB_ENABLED
#include <Adafruit_NeoPixel.h>

#define N_PIXELS 1
#define PIXEL_FORMAT NEO_GRB//+NEO_KHZ800
Adafruit_NeoPixel pixels(N_PIXELS, RGB_PIN, PIXEL_FORMAT);

void neopixel_init() {
  pixels.begin();
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(16, 0, 0)); //RED
  pixels.show();
}

void neopixel_set(uint8_t r, uint8_t g, uint8_t b) {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}
#endif

#define POWER_ON_ACTIVE LOW
#define POWER_ON_TIME_US 500 // microseconds

void poweron_init() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, !POWER_ON_ACTIVE);
}

void poweron_toggle() {
  digitalWrite(PIN_POWER_ON, POWER_ON_ACTIVE);
  delayMicroseconds(POWER_ON_TIME_US); // 400microseconds seems to be the minimum for power on "key" to be detected
  digitalWrite(PIN_POWER_ON, !POWER_ON_ACTIVE);
}

// PS2 to NeXT Mouse Arduino code: http://www.asterontech.com/Asterontech/next_ps2_mouse.html
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

bool queue_mouse_from_interrupt(int8_t mousex, int8_t mousey, bool button1, bool button2);

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

  queue_mouse_from_interrupt(dx, dy, data.left, data.right);
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
void next_mouse_init_task(void *pvParameters) {
    next_mouse_init(&next_mouse_timer);
    vTaskDelete(NULL);
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
    if (all_high_counter == 9) {  // sense continuous bits high
      all_high_counter = 0;
      state = STATE_READY;
    } else {
    }
  } else if (state == STATE_READY) {
    if (val == 0) {
      read_data_ready = false;
      data = 0;
      bits = 1;
      state = STATE_READING;
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
      read_data = data;
      read_data_ready = true;
      if (state == STATE_SHORT_READ) {
        timerDetachInterrupt(next_keyboard_timer);
        timerAttachInterrupt(next_keyboard_timer, &next_keyboard_write_interrupt, false);
      }
      state = STATE_READY;
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

void usb_update_keys(uint8_t keycode[6], uint8_t modifiers, uint8_t modifiers2) {
  static uint8_t pressed_keys[6] = {0,0,0,0,0,0};
  static uint8_t last_modifiers = 0;
  static uint8_t last_modifiers2 = 0;

  Dprintf("\nUSB: modifiers=0x%x keycode=0x%x %x %x %x %x %x modifiers2=0x%x\n", modifiers, keycode[0], keycode[1], keycode[2], keycode[3], keycode[4], keycode[5], modifiers2);
  // Ignore Key Roll Over Overflow. All bytes will be 0x01
  if (keycode[0] == 0x01) {
    Serial.printf("\nWARNING: Key Roll Over Overflow\n");
    return;
  }

  // Send key release event and remove key for keys no longer pressed
  for(int p=0; p<6; p++) {
    if (pressed_keys[p]) {
      bool found = false;

      for(int k=0; k<6 && keycode[k] != 0; k++) {
        if (pressed_keys[p] == keycode[k]) {
          found = true;
          break;
        }
      }
//#define HID_RCOMM KEYBOARD_MODIFIER_RIGHTCTRL // For using right control as right command (right control will be unusable)
#define HID_RCOMM 0 
      if (!found) {
        queue_keyboard(hid2next[pressed_keys[p]], false, hid2next_modifiers(modifiers, HID_RCOMM));
        pressed_keys[p] = 0;
      }
    }
  }

  // When no keys pressed, send modifiers if changed
  bool modifiers_changed = last_modifiers != modifiers;
  last_modifiers = modifiers;
  if (keycode[0] == 0 & modifiers_changed) {
    queue_keyboard(0, false, hid2next_modifiers(modifiers, HID_RCOMM));
    return;
  }
  // bool modifiers2_changed = last_modifiers2 != modifiers2;
  // last_modifiers2 = modifiers2;
  // if (keycode[0] == 0 & modifiers2_changed) {
  //   queue_keyboard(0, false, hid2next_modifiers(modifier2, HID_RCOMM));
  //   return;
  // }

  // Use Apple Magic Keyboard Eject key as power toggle
  #define USB_APPLE_MODIFIER2_EJECT 0x01
  #define USB_APPLE_MODIFIER2_FN    0x02
  if (modifiers2&USB_APPLE_MODIFIER2_EJECT) {
    poweron_toggle();
  }

  // Add new pressed keys and send key pressed event
  int free = -1;
  bool found = false;
  for(int k=0; k<6; k++) {
    if (keycode[k] == 0)
      break;

    if ((keycode[k] == HID_KEY_HOME) || ((hid2next[keycode[k]] == NEXT_KEY_KEYPAD_GRAVE_ACCENT) && ((hid2next_modifiers(modifiers, HID_RCOMM)&(KD_LCOMM|KD_LALT)) == (KD_LCOMM|KD_LALT)))) {
      poweron_toggle();
      continue;
    }

    for(int p=0; p<6; p++) {
      if (keycode[k] == pressed_keys[p]) {
        found = true;
        break;
      } else if (free == -1 && pressed_keys[p] == 0) {
        free = p;
      }
    }

    if (!found) {
      pressed_keys[free] = keycode[k];
      queue_keyboard(hid2next[keycode[k]], true, hid2next_modifiers(modifiers, HID_RCOMM));
    }
  }
}

// USB Host HID Keyboard stuff
#ifdef USB_HOST_ENABLED
#include <EspUsbHostKeybord.h>

class MyEspUsbHostKeybord : public EspUsbHostKeybord {
public:
  void onKey(usb_transfer_t *transfer) {

    hid_keyboard_report_t *report = (hid_keyboard_report_t *)(transfer->data_buffer);

    // Dprintf("\nonKey %02x %02x %02x %02x %02x %02x %02x %02x\n", report->modifier, report->reserved, report->keycode[0], report->keycode[1], report->keycode[2], report->keycode[3], report->keycode[4], report->keycode[5]);
    // Serial.printf("hid_modifiers=0x%x next_modifiers=0x%x\n", report->modifier, hid2next_modifiers(report->modifier, 0));
    usb_update_keys(report->keycode, report->modifier, 0);
//     for(int i=0; i<6; i++) {
//       if (report->keycode[i] && report->keycode[i] != 1) {
//         Serial.printf("p[%d]=0x%x\n", i, report->keycode[i]);
//         if ((hid2next[report->keycode[i]] == NEXT_KEY_GRAVE_ACCENT) && ((hid2next_modifiers(report->modifier,0)&(KD_LCOMM|KD_LALT)) == (KD_LCOMM|KD_LALT))) {
//           poweron_toggle();
//         } else if (queue_keyboard(hid2next[report->keycode[i]], true, hid2next_modifiers(report->modifier,0))) {
// #ifdef RGB_ENABLED
//           neopixel_set(0,16,16); // WHITE
// #endif
//           Dprintf("\nQueued 0x%x (0x%x). pressed=%d modifiers=0x%x\n", (hid2next[report->keycode[i]]&NEXT_KEYBOARD_BYTE_MASK), hid2next[report->keycode[i]], true, hid2next_modifiers(report->modifier,0));
//         }
//       }
//     }
//     // if (!(report->keycode[0] || report->keycode[1] || report->keycode[2] || report->keycode[3] || report->keycode[4] || report->keycode[5])) {
//     //   queue_keyboard(0, false, hid2next_modifiers(report->modifier,0));
//     //   Dprintf("\nQueued 0x%x (0x%x). pressed=%d modifiers=0x%x\n", 0, 0, false, hid2next_modifiers(report->modifier,0));
//     // }
  };
};

MyEspUsbHostKeybord usbHost;
#endif

#ifdef USB_SOFTHOST_ENABLED // USB Soft Host stuff
// Needs Arduino board esp32 <= 2.0.11 selected in boards manager, for ESP32-USB-Soft-Host to work
//#define FORCE_TEMPLATED_NOPS
#include <ESP32-USB-Soft-Host.h>

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
#endif

#define USB_MOUSE_BUTTON_LEFT_MASK   1<<0
#define USB_MOUSE_BUTTON_RIGHT_MASK  1<<1
#define USB_MOUSE_BUTTON_MIDDLE_MASK 1<<2
#define USB_MOUSE_SCROLL_UP    1
#define USB_MOUSE_SCROLL_DOWN  -1

typedef struct {
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // signed. >0 DOWN, <0 UP
  int8_t scroll; // 1==up 0xff==down
} usb_mouse_data_4bytes_t; // Microsoft Wheel Mouse Optical 1.1A USB and PS/2 Compatible

typedef struct {
  uint8_t byte0;  // Always 0x1
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // 0x10-0x70 == DOWN, 0xf0-0x80 UP
  uint8_t direction_up;  // 0x00 == DOWN direction? 0xff == UP direction
  int8_t scroll; // 1==up 0xff==down
} usb_mouse_data_6bytes_t; // No name "3D Optical Mouse"

typedef struct {
  uint8_t byte0;  // Always 0x2
  uint8_t button; // 1=left
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // signed. >0 DOWN, <0 UP
  uint8_t byte4; // zero
  uint8_t byte5; // zero
  uint8_t byte6; // zero
  uint8_t touch; // 1=touching
  uint8_t byte8; // zero
  uint8_t byte9; // zero
} usb_mouse_data_8bytes_t; // Apple Magic Trackpad 2

typedef struct {
  uint8_t byte0;  // Always 0x1A
  uint8_t button; // 1=left,2=right,4=middle
  int16_t x;  // signed. >0 RIGHT, <0 LEFT
  int16_t y;  // signed. >0 DOWN, <0 UP
  int16_t scroll; // 1==SCROLL_DOWN 0xffff==SCROLL_UP
  int16_t hscroll; // 1==SCROLL_LEFT 0xffff==SCROLL_RIGHT
} usb_mouse_data_10bytes_t; // Microsoft All-In-One Media Keyboard Touchpad

#define USB_MOUSE_4BYTES_NAME "USB Mouse (4 Bytes) / Microsoft Wheel Mouse Optical 1.1A"
static void handle_mouse_data_4bytes(uint8_t* data) {
  usb_mouse_data_4bytes_t *mouse_data = (usb_mouse_data_4bytes_t *)data;

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );
}

#define USB_MOUSE_6BYTES_NAME "USB Mouse (6 Bytes) / 3D Optical Mouse"
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

#define USB_MOUSE_8BYTES_NAME "USB Mouse (8 Bytes) / Apple Magic Trackpad 2"
static void handle_mouse_data_8bytes(uint8_t* data) {
  usb_mouse_data_8bytes_t *mouse_data = (usb_mouse_data_8bytes_t *)data;

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );

  Dprintf("\n%s: L=%d X=%d Y=%d TOUCH=%d\n",
    USB_MOUSE_8BYTES_NAME,
    (bool)(mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK),
    mouse_data->x,
    mouse_data->y,
    mouse_data->touch
  );
}

#define USB_MOUSE_10BYTES_NAME "USB Mouse/MS All-In-One Media Keyboard Touchpad (10 Bytes)"
static void handle_mouse_data_10bytes(uint8_t* data) {
  usb_mouse_data_10bytes_t *mouse_data = (usb_mouse_data_10bytes_t *)data;

  queue_mouse_from_interrupt(
    mouse_data->x,
    mouse_data->y,
    mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK,
    mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK
  );

  // Dprintf("\n%s: L=%d R=%d M=%d X=%d Y=%d S=%s HS=%s\n",
  //   USB_MOUSE_10BYTES_NAME,
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_LEFT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_RIGHT_MASK),
  //   (bool)(mouse_data->button&USB_MOUSE_BUTTON_MIDDLE_MASK),
  //   mouse_data->x,
  //   mouse_data->y,
  //   mouse_data->scroll < 0 ? "UP" : (mouse_data->scroll > 0 ? "DOWN" : "_"),
  //   mouse_data->hscroll < 0 ? "RIGHT" : (mouse_data->hscroll > 0 ? "LEFT" : "_")
  // );
}

#ifdef USB_SOFTHOST_ENABLED
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
#endif

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
void next_keyboard_init_task(void *pvParameters) {
    next_keyboard_init(&next_keyboard_timer);
    vTaskDelete(NULL);
}

#ifdef USB_SOFTHOST_ENABLED
void usb_mouse_init() {
  USH.setTaskCore(1);
  USH.init(USB_Pins_Config, NULL, NULL);
  USH.setPrintCb(ush_handle_data);
  USH.setOnIfaceDescCb(ush_handle_interface_descriptor);
}
void usb_mouse_init_task(void *pvParameters) {
    usb_mouse_init();
    vTaskDelete(NULL);
}
#endif

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
    uint32_t _pressed = (key && !pressed) << NEXT_KEYBOARD_KEY_PRESSED_BIT;
    uint32_t _key = ((key&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_KEY_START_BIT);
    uint32_t _valid = (((key&NEXT_KEYBOARD_BYTE_MASK) || (modifiers&NEXT_KEYBOARD_BYTE_MASK)) << NEXT_KEYBOARD_VALID_BIT);
    uint32_t _modifiers = ((modifiers&NEXT_KEYBOARD_BYTE_MASK) << NEXT_KEYBOARD_MODIFIERS_START_BIT);
    keyboard_data = NEXT_KEYBOARD_DATA_PACKET |
      _pressed | _key | // byte1
      _valid | _modifiers; // byte2
    keyboard_data_ready = true;
    Dprintf("\npressed=0x%x, key=0x%x, valid=0x%x, modifiers=0x%x, byte1=0x%x byte2=0x%x\n",
      _pressed>>NEXT_KEYBOARD_KEY_PRESSED_BIT,
      _key>>NEXT_KEYBOARD_KEY_START_BIT,
      _valid>>NEXT_KEYBOARD_VALID_BIT,
      _modifiers>>NEXT_KEYBOARD_MODIFIERS_START_BIT,
      _pressed|_key,
      _valid|_modifiers
    );
    return true;
  } else {
    Serial.printf("\nERROR: queue_keyboard failed to send event!\n");
    return false;
  }
}

bool queue_mouse_from_interrupt(int8_t mousex, int8_t mousey, bool button1, bool button2) {
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
    // FIXME: it's always printing after first click on button2, even when no change. why?
    Dprintf("\nmouse_data on wire: L=%d R=%d dx=0x%x dy=0x%x mouse_data=0x%x \n", button1, button2, dx, dy, mouse_data);
  }
}

#ifdef USB_CH9350_ENABLED
typedef struct {
  uint8_t type;
  uint8_t len;
  uint8_t labeling;
  uint8_t data[72];
  uint8_t serial;
  uint8_t checksum;
} USBKMSerialEvent;

cppQueue USBKMSerialQueue(sizeof(USBKMSerialEvent), 20, FIFO, false);

enum {
  USB_IDLE,
  USB_INIT1,
  USB_INIT2,
  USB_STATUS_CHANGE_COMMAND,
  USB_DEVICE_CONNECTION_FRAME,
  USB_STATUS_REQ_FRAME,
  USB_VALID_FRAME,
  USB_DEVICE_DISCONNECT_COMMAND,
  USB_VALID_FRAME_88,
  USB_FRAME_LENGTH,
  USB_FRAME_DATA,
  USB_FRAME_DATA_DONE,
  USB_FRAME_SERIAL,
  USB_FINISH
};

void USBKMSerialOnReceiveCb() {
  static bool locked = false;
  static int state = USB_IDLE;
  static uint8_t len_left = 0;
  USBKMSerialEvent ev;

  if (locked) {
    Dprintf("\nWARNING: USBKMSerialOnReceiveCb already running\n");
    return;
  }

  locked = true;
  while(USB_CH9350_SERIAL.available() && state != USB_FINISH) {
    char c = USB_CH9350_SERIAL.read();
    switch (state) {
      case USB_IDLE:
        if (c == 0x57)
          state = USB_INIT1;
        break;
      case USB_INIT1:
        if (c == 0xAB) {
          state = USB_INIT2;
        } else {
          state = USB_FINISH;
          Dprintf("\nUSB_INIT1: Ignoring c=0x%x\n", c);
        }
        break;
      case USB_INIT2:
        switch(c) {
          case 0x80:
            ev.type = USB_STATUS_CHANGE_COMMAND;
            state = USB_STATUS_CHANGE_COMMAND;
            break;
          case 0x81:
            ev.type = USB_DEVICE_CONNECTION_FRAME;
            state = USB_DEVICE_CONNECTION_FRAME;
            break;
          case 0x82:
            ev.type = USB_STATUS_REQ_FRAME;
            state = USB_STATUS_REQ_FRAME;
            break;
          case 0x83:
            Serial.printf("\nWARNING: Received USB_VALID_FRAME\n");
            ev.type = USB_VALID_FRAME;
            state = USB_VALID_FRAME;
            break;
          // case 0x86:
          //   ev.type = USB_FINISH;
          //   state = USB_FINISH;
          //   break;
          case 0x88:
            ev.type = USB_VALID_FRAME_88;
            state = USB_VALID_FRAME_88;
            break;
          default:
            state = USB_FINISH;
            Serial.printf("\nWARNING: USB_INIT2 Received 0x%x\n", c);
            break;
        }
        break;
      case USB_STATUS_CHANGE_COMMAND:
        Serial.printf("\nWARNING: Received USB_STATUS_CHANGE_COMMAND 0x%x. Probable disconnect.\n", c);
        state = USB_FINISH;
        break;
      case USB_DEVICE_CONNECTION_FRAME:
        // 1-byte ID, 2-byte Payload length, Payload, 2-byte ID, 1-byte parity check
        // TODO: read 5 bytes and save device IDs for each of the 2 ports. May need to handle disconnect aswell.
        Serial.printf("\nWARNING: Received USB_DEVICE_CONNECTION_FRAME 0x%x\n", c);
        state = USB_FINISH;
        break;
      case USB_STATUS_REQ_FRAME:
        // Serial.printf("\nUSB_STATUS_REQ_FRAME value=0x%x io_status=0x%x\n", c, c&0x0F);
        USB_CH9350_SERIAL.write("\x57\xAB\x12\x00\x00\x00\x00\xFF\x80\x00\x20", 11); // Specific Data Frame. Sending this disables STATUS_REQ_FRAME being sent by CH9350.
        state = USB_FINISH;
        break;
      case USB_VALID_FRAME:
      case USB_VALID_FRAME_88:
        bzero(&ev, sizeof(ev));
        ev.len = c;
        len_left = ev.len-3;
        state = USB_FRAME_LENGTH;
        break;
      case USB_FRAME_LENGTH:
        ev.labeling = c;
        state = USB_FRAME_DATA;
        break;
      case USB_FRAME_DATA:
        ev.data[ev.len-3-len_left] = c;
        len_left--;
        if (!len_left)
          state = USB_FRAME_DATA_DONE;
        break;
      case USB_FRAME_DATA_DONE:
        ev.serial = c;
        state = USB_FRAME_SERIAL;
        break;
      case USB_FRAME_SERIAL:
        ev.checksum = c;
        if (!USBKMSerialQueue.push(&ev)) {
          Serial.printf("\nWARNING: failed to push KM data to queue\n");
        }
        state = USB_FINISH;
        break;
    }
  }

  if (state == USB_FINISH) {
    state = USB_IDLE;
  }

  locked = false;
}

void USBSerialLED(bool scroll_lock, bool caps_lock, bool num_lock) {
  char buf[] = {0x57,0xAB,0x12,0x00,0x00,0x00,0x00,0x00,0x31,0x0F,0x20};
#define USBSERIAL_NUM_LOCK_BIT    0
#define USBSERIAL_CAPS_LOCK_BIT   1
#define USBSERIAL_SCROLL_LOCK_BIT 2
  buf[7] = scroll_lock<<USBSERIAL_SCROLL_LOCK_BIT | caps_lock<<USBSERIAL_CAPS_LOCK_BIT | num_lock<<USBSERIAL_NUM_LOCK_BIT;
  USB_CH9350_SERIAL.write(buf, 11);
}
#endif // USB_CH9350_ENABLED

void setup() {
#ifdef RGB_ENABLED
  neopixel_init();
#endif

  Serial.begin(115200);
  Serial.printf("Starting NeXT Keyboard\n");

#ifdef USB_CH9350_ENABLED
  // Serial for USB chip WCH CH9350
  USB_CH9350_SERIAL.begin(115200, SERIAL_8N1, USB_SERIAL_RX, USB_SERIAL_TX);
  USB_CH9350_SERIAL.onReceive(USBKMSerialOnReceiveCb);
  // USBSerialLED(true, true, true);
  USBSerialLED(true, true, false);
#endif // USB_CH9350_ENABLED

  // Setup POWER_ON pin
  poweron_init();
  // Setup ASCII to NeXT and HID to NeXT keycode tables
  next_keycodes_init();

#ifdef USB_HOST_ENABLED
  // Hardware USB Host
  usbHost.begin();
#endif

#ifdef USB_SOFTHOST_ENABLED
  // USB Mouse. USB Soft Host needs to be initialized before next_keyboard_timer and next_mouse_timer below or USB won't work.
  usb_mouse_init();
  // xTaskCreatePinnedToCore(usb_mouse_init_task, "usb_mouse_init_task", 2000, NULL, 6, NULL, 1); // Gives guru meditation error :(
#endif

  // This emulated NeXT Keyboard
  // next_keyboard_init(&next_keyboard_timer);
  xTaskCreatePinnedToCore(next_keyboard_init_task, "next_keyboard_init_task", 2000, NULL, 6, NULL, 0);
  // Real NeXT Mouse
  // next_mouse_init(&next_mouse_timer);
  xTaskCreatePinnedToCore(next_mouse_init_task, "next_mouse_init_task", 2000, NULL, 6, NULL, 1);

#ifdef RGB_ENABLED
  neopixel_set(0, 16, 0); // GREEN
#endif
}

void loop() {
#ifdef USB_HOST_ENABLED
  // Hardware USB Host update
  usbHost.task();
#endif

#ifdef USB_CH9350_ENABLED

  if (USB_CH9350_SERIAL.available())
    USBKMSerialOnReceiveCb();

  while (!USBKMSerialQueue.isEmpty()) {
    USBKMSerialEvent ev;
    USBKMSerialQueue.pop(&ev);
    if ((ev.labeling&0x30) == 0x10) {
      if ((ev.len-3) == 8) {
        usb_update_keys(&ev.data[2], ev.data[0], 0);
        Dprintf("\nDEBUG: keyboard data length=%d. data=0x%x %x %x %x %x %x %x %x %x %x\n", ev.len-3, ev.data[0], ev.data[1], ev.data[2], ev.data[3], ev.data[4], ev.data[5], ev.data[6], ev.data[7], ev.data[8], ev.data[9]);
      } else if ((ev.len-3) == 10) {
        // Apple Magic Keyboard 2: ev.data[0] is always 0x1. ev.data[9]: bit1=Eject bit2=Fn
        usb_update_keys(&ev.data[3], ev.data[1], ev.data[9]);
      } else {
        Serial.printf("\nWARNING: Unknown keyboard data length=%d. data=0x%x %x %x %x %x %x %x %x %x %x\n", ev.len-3, ev.data[0], ev.data[1], ev.data[2], ev.data[3], ev.data[4], ev.data[5], ev.data[6], ev.data[7], ev.data[8], ev.data[9]);
      }
    } else if ((ev.labeling&0x30) == 0x20) {
      if ((ev.len-3) == 4) {
        handle_mouse_data_4bytes(&ev.data[0]);
      } else if ((ev.len-3) == 6) {
        handle_mouse_data_6bytes(&ev.data[0]);
      } else if ((ev.len-3) == 8) {
        handle_mouse_data_8bytes(&ev.data[0]);
      } else if ((ev.len-3) == 10) {
        handle_mouse_data_10bytes(&ev.data[0]);
      } else {
        Serial.printf("WARNING: Unknown mouse data length=%d. data=0x%x %x %x %x %x %x %x %x %x %x\n", ev.len-3, ev.data[0], ev.data[1], ev.data[2], ev.data[3], ev.data[4], ev.data[5], ev.data[6], ev.data[7], ev.data[8], ev.data[9]);
      }
    } else if ((ev.labeling&0x30) == 0x30) {
      Serial.printf("\nWARNING: multimedia keyboard\n");
    } else if ((ev.labeling&0x30) == 0x00) {
      Serial.printf("\nWARNING: other USB device\n");
    } else {
      Serial.printf("\nWARNING: unexpected USB device\n");
    }
    // Serial.printf("\nlen=%d data_len=%d labeling=0x%x data=0x%x %x %x %x %x %x %x %x serial=%d checksum=0x%x\n", ev.len, ev.len-3, ev.labeling&0x30, ev.data[0], ev.data[1], ev.data[2], ev.data[3], ev.data[4], ev.data[5], ev.data[6], ev.data[7], ev.serial, ev.checksum);
  }
#endif // USB_CH9350_ENABLED

  static bool enable_key_enter = false;

  // next_mouse_print_changes();
  next_keyboard_mouse_print_changes();

  if (c == 0 && Serial.available()) {
#ifdef RGB_ENABLED
  neopixel_set(0, 0, 16); // BLUE
#endif

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
        c = NEXT_KEY_KEYPAD_GRAVE_ACCENT;
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
#ifdef RGB_ENABLED
      neopixel_set(16,16,16); // WHITE
#endif
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
      Dprintf("?\n");
      print_byte((read_data >> 16) & 0xff);
      print_byte((read_data >> 8) & 0xff);
      print_byte(read_data & 0xff);
      Dprintf("\n");
    }
    read_data_ready = false;
  }
  delay(10/*100*/); // Don't hog the cpu
#ifdef RGB_ENABLED
  neopixel_set(0, 16, 0); // GREEN
#endif
}
