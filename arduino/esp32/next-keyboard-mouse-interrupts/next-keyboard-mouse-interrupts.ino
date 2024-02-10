// Needs Arduino board esp32 <= 2.0.11 selected in boards manager, for ESP32-USB-Soft-Host to work
//#define FORCE_TEMPLATED_NOPS
#include <ESP32-USB-Soft-Host.h>

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
// 8pin mini-DIN. pin1 = 5V. pin8 = GND
#define NX_MOUSE_XA_PIN 18 // pin 2
#define NX_MOUSE_XB_PIN 19 // pin 3
#define NX_MOUSE_YA_PIN 21 // pin 4
#define NX_MOUSE_YB_PIN 25 // pin 5
#define NX_MOUSE_RIGHT_PIN 26 // pin 6
#define NX_MOUSE_LEFT_PIN 27 // pin 7

#define NX_MOUSE_SAMPLING_MS 5 // milliseconds

// https://github.com/tmk/tmk_keyboard/issues/704
// #define NEXT_KBD_TIMING_US 52 // *microseconds
// #define NEXT_KBD_TIMING_US 53 // *microseconds
//#define NEXT_KBD_TIMING_100NS 527 // *100nanoseconds. should be 52.75 for NeXT KMS, but our KMS is 53microseconds
#define NEXT_KBD_TIMING_100NS 530 // *100nanoseconds. should be 52.75 for NeXT KMS, but our FPGA KMS is 53microseconds

#define TIMER1 1
#define DIVIDER_US 80 // 80Mhz / 80 = 1Mhz. 1/f = 1/1000000 = 1 microsecond. Timer ticks every microsecond.
#define DIVIDER_100NS 8 // 80Mhz / 8 = 10Mhz. 1/f = 1/10000000 = 100 nanosecond. Timer ticks every 100 nanoseconds.

#define KMS_QUERY_KEYBOARD 0b000100000
#define KMS_QUERY_MOUSE 0b000100010
#define KMS_RESET 0b0000000000111111011110
#define KMS_LED_PREFIX 0b0111000000000
#define KMS_LED_LEFT_MASK 0x2000
#define KMS_LED_RIGHT_MASK 0x4000

// NeXT keycodes:
// https://github.com/tmk/tmk_keyboard/blob/master/converter/next_usb/unimap_trans.h
// https://web.archive.org/web/20150608141822/http://www.68k.org/~degs/nextkeyboard.html

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

uint32_t keyboard_data = 0;
bool keyboard_data_ready = false;

uint32_t mouse_data = 0;
bool mouse_data_ready = false;

#define IDLE_DATA 0b1100000000011000000000
bool idle_data_ready = false;

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
    if ((state == STATE_SHORT_READ && bits == 11) || (state == STATE_LONG_READ && bits == 23)) {  // 11 bits including start and stop bit
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

  if (is_keyboard_query && keyboard_data_ready && !idle_data_ready) {
    data = keyboard_data;
  } else if (is_mouse_query && mouse_data_ready && !idle_data_ready) {
    data = mouse_data;
  } else {
    idle_data_ready = true;
    data = IDLE_DATA;
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
    timerDetachInterrupt(timer1);
    timerAttachInterrupt(timer1, &timer1_read_handler, false);
  }
}

// USB Soft Host stuff
#define HID_INTERFACE_PROTO_KEYBOARD 1
#define HID_INTERFACE_PROTO_MOUSE 2
#define APP_USBD_HID_SUBCLASS_BOOT 1
void ush_handle_interface_descriptor(uint8_t ref, int cfgCount, int sIntfCount, void* Intf, size_t len)
{
  sIntfDesc *sIntf = (sIntfDesc*)Intf;
  printf("USB HID %s %s: %d:%d:%d\n",
    sIntf->iSub == APP_USBD_HID_SUBCLASS_BOOT ? "(Boot)" : "",
    sIntf->iProto == HID_INTERFACE_PROTO_KEYBOARD ? "Keyboard" : (sIntf->iProto == HID_INTERFACE_PROTO_MOUSE ? "Mouse" : String(sIntf->iProto)),
    ref, cfgCount, sIntfCount);
}

#define MOUSE_BUTTON_LEFT_MASK   1<<0
#define MOUSE_BUTTON_RIGHT_MASK  1<<1
#define MOUSE_BUTTON_MIDDLE_MASK 1<<2
#define MOUSE_SCROLL_UP    0x1
#define MOUSE_SCROLL_DOWN  0xff
typedef struct {
  uint8_t byte0;  // Always 0x1
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // 0x10-0x70 == DOWN, 0xf0-0x80 UP
  uint8_t direction_up;  // 0x00 == DOWN direction? 0xff == UP direction
  int8_t scroll; // 1==up 0xff==down
} mouse_data_6bytes_t; // No name "3D Optical Mouse"

typedef struct {
  uint8_t button; // 1=left,2=right,4=middle
  int8_t x;  // signed. >0 RIGHT, <0 LEFT
  int8_t y;  // signed. >0 DOWN, <0 UP
  int8_t scroll; // 1==up 0xff==down
} mouse_data_4bytes_t; // Microsoft Wheel Mouse Optical 1.1A USB and PS/2 Compatible

uint8_t next_mouse_buttons;
int8_t next_mouse_x;
int8_t next_mouse_y;

// char ascii2next[] = {
//   [8] = NX_BACKSPACE,
//   NX_TAB,
//   NX_ENTER,
//   [27] = NX_ESCAPE,
//   [32] = NX_SPACE,
//   NX_EXCLAMATION,
//   NX_DOUBLE_QUOTES,
//   NX_NUMBER_SIGN, // hash
//   NX_DOLLAR,
//   NX_PERCENT

// };

static void handle_mouse_data_4bytes(uint8_t* data) {
  mouse_data_4bytes_t *mouse_data = (mouse_data_4bytes_t *)data;

  queue_mouse(mouse_data->x, mouse_data->y, mouse_data->button&MOUSE_BUTTON_LEFT_MASK || mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK, mouse_data->button&MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK);

  if (mouse_data->button&MOUSE_BUTTON_LEFT_MASK)
    printf("BUTTON LEFT\n");
  if (mouse_data->button&MOUSE_BUTTON_RIGHT_MASK)
    printf("BUTTON RIGHT\n");
  if (mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK)
    printf("BUTTON MIDDLE\n");

  if (mouse_data->x > 0)
    printf("RIGHT %d\n", mouse_data->x);
  if (mouse_data->x < 0)
    printf("LEFT %d\n", mouse_data->x);

  if (mouse_data->y > 0)
    printf("DOWN %d\n", mouse_data->y);
  if (mouse_data->y < 0)
    printf("UP %d\n", mouse_data->y);

  if (mouse_data->scroll > 0)
    printf("SCROLL_UP\n");
  if (mouse_data->scroll < 0)
    printf("SCROLL_DOWN\n");
}

static void handle_mouse_data_6bytes(uint8_t* data) {
  mouse_data_6bytes_t *mouse_data = (mouse_data_6bytes_t *)data;

  queue_mouse(mouse_data->x, mouse_data->y, mouse_data->button&MOUSE_BUTTON_LEFT_MASK || mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK, mouse_data->button&MOUSE_BUTTON_RIGHT_MASK || mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK);

  if (mouse_data->button&MOUSE_BUTTON_LEFT_MASK)
    printf("BUTTON LEFT\n");
  if (mouse_data->button&MOUSE_BUTTON_RIGHT_MASK)
    printf("BUTTON RIGHT\n");
  if (mouse_data->button&MOUSE_BUTTON_MIDDLE_MASK)
    printf("BUTTON MIDDLE\n");

  if (mouse_data->x > 0)
    printf("RIGHT %d\n", mouse_data->x);
  if (mouse_data->x < 0)
    printf("LEFT %d\n", mouse_data->x);

  // FIXME: up and down data is a bit screwed up. Maybe crazy mouse.
  if (mouse_data->y > 0)
    printf("DOWN %d\n", mouse_data->y);
  if (mouse_data->y < 0)
    printf("UP %d\n", mouse_data->y);

  if (mouse_data->scroll > 0)
    printf("SCROLL_UP\n");
  if (mouse_data->scroll < 0)
    printf("SCROLL_DOWN\n");
}

// FIXME: Instead of this hack, we should get the mouse report description, and parse it to know where the data really is
static void ush_handle_data(uint8_t usbNum, uint8_t byte_depth, uint8_t* data, uint8_t data_len)
{

  // if( myListenUSBPort != usbNum ) return;
  // printf("\nDATA: usbNum=%d byte_depth=%d data_len=%d\n", usbNum, byte_depth, data_len);
  // for(int k=0;k<data_len;k++) {
  //   printf("0x%02x ", data[k] );
  // }
  // printf("\n");

  if (data_len == 4)
    handle_mouse_data_4bytes(data);
  else if (data_len == 6)
    handle_mouse_data_6bytes(data);
  else
    printf("Unknown mouse data length=%d\n", data_len);
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

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting sketch\n");
  pinMode(PIN_TO_KBD, INPUT_PULLUP);
  pinMode(PIN_FROM_KBD, OUTPUT);
  digitalWrite(PIN_FROM_KBD, HIGH);

  // USB Soft Host. Needs to be initialized before timer1 below or it won't work.
  USH.init(USB_Pins_Config, NULL, NULL);
  USH.setPrintCb(ush_handle_data);
  USH.setOnIfaceDescCb(ush_handle_interface_descriptor);

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

int queue_keyboard(char key, bool pressed, char modifiers) { // TODO: turn this into a real queue
  if (!keyboard_data_ready) {
    keyboard_data = 0b1000000000010000000000 | ((key&0x7F) << 1) | (key && !pressed) << 8 | ((modifiers&0x7F) << 12) | ((key&0x7F) || (modifiers&0x7F)) << 19;
    keyboard_data_ready = true;
    return true;
  } else {
    return false;
  }
}

int queue_mouse(char mousex, char mousey, bool button1, bool button2) {
  if (!mouse_data_ready) {
    mouse_data = 0b1000000000010000000000 | (!button1 << 1) | (((!mousex)&0x7F) << 2) | (!button2 << 12) | (((!mousey)&0x7F) << 13);
    mouse_data_ready = true;
    return true;
  } else {
    return false;
  }
}

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

  if (c != 0 && queue_keyboard(c, pressed, modifiers)) {
    Serial.printf("\nQueued 0x%x (0x%x). pressed=%d modifiers=0x%x\n", (c&0x7F), c, pressed, modifiers);
    if (pressed) {
      pressed = false;
    } else {
      c = 0;
      modifiers = 0;
    }
  }

  if (read_data_ready) {
    // printf("0x%X\n", read_data);
    if ((read_data & 0x1FF) == KMS_QUERY_KEYBOARD) {
      Serial.print("K");
    } else if ((read_data & 0x1FF) == KMS_QUERY_MOUSE) {
      Serial.print("M");
    } else if ((read_data & 0x3FFFFF) == KMS_RESET) {
      Serial.print("_RESET_");
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
      Serial.print("?");
      // print_byte((read_data >> 16) & 0xff);
      // print_byte((read_data >> 8) & 0xff);
      // print_byte(read_data & 0xff);
      // Serial.println("");
    }
    read_data_ready = false;
  }
}
