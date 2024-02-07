//#define FORCE_TEMPLATED_NOPS
#include <ESP32-USB-Soft-Host.h>
//#include "usbkbd.h" // KeyboardReportParser

// Not apostrophe, it's tilde or backtick or grave
#define KEY_APOSTROPHE 0x26  // 0b00100110
#define KD_KEYMASK 0x7f
#define KD_DIRECTION 0x80

#define KD_CNTL 0x01
#define KD_LSHIFT 0x02
#define KD_RSHIFT 0x04
#define KD_LCOMM 0x08
#define KD_RCOMM 0x10
#define KD_LALT 0x20
#define KD_RALT 0x40
#define KD_VALID 0x80

#define KD_FLAGKEYS 0x7f00

char sendA[] = { 0b11100011, 0b00001000, 0b00000000, 0b10000000, 0b10111001 };
char sendCmdApos[] = { 0b11100011, 0b00001000, 0b00000000, KD_VALID | KD_RCOMM, KD_DIRECTION | KEY_APOSTROPHE };
char sendCmdAposRelease[] = { 0b11100011, 0b00001000, 0b00000000, KD_VALID, KEY_APOSTROPHE };


// https://github.com/tmk/tmk_keyboard/issues/704
#define NEXT_KBD_TIMING 52  // scope says 52.75 to 53.125
//#define NEXT_KBD_TIMING     53 // scope says close to 54us

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

// USB Soft Host stuff
static void my_USB_DetectCB( uint8_t usbNum, void * dev )
{
  sDevDesc *device = (sDevDesc*)dev;
  printf("New device detected on USB#%d\n", usbNum);
  printf("desc.bcdUSB             = 0x%04x\n", device->bcdUSB);
  printf("desc.bDeviceClass       = 0x%02x\n", device->bDeviceClass);
  printf("desc.bDeviceSubClass    = 0x%02x\n", device->bDeviceSubClass);
  printf("desc.bDeviceProtocol    = 0x%02x\n", device->bDeviceProtocol);
  printf("desc.bMaxPacketSize0    = 0x%02x\n", device->bMaxPacketSize0);
  printf("desc.idVendor           = 0x%04x\n", device->idVendor);
  printf("desc.idProduct          = 0x%04x\n", device->idProduct);
  printf("desc.bcdDevice          = 0x%04x\n", device->bcdDevice);
  printf("desc.iManufacturer      = 0x%02x\n", device->iManufacturer);
  printf("desc.iProduct           = 0x%02x\n", device->iProduct);
  printf("desc.iSerialNumber      = 0x%02x\n", device->iSerialNumber);
  printf("desc.bNumConfigurations = 0x%02x\n", device->bNumConfigurations);
  // if( device->iProduct == mySupportedIdProduct && device->iManufacturer == mySupportedManufacturer ) {
  //   myListenUSBPort = usbNum;
  // }
}

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

static void handle_mouse_data_4bytes(uint8_t* data) {
  mouse_data_4bytes_t *mouse_data = (mouse_data_4bytes_t *)data;

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

// NexT keyboard protocol stuff
static inline void out_lo(void) {
  digitalWrite(PIN_FROM_KBD, LOW);
}

static inline void out_hi(void) {
  digitalWrite(PIN_FROM_KBD, HIGH);
}

#define out_delay(is_high) \
  do { \
    if (is_high) out_hi(); \
    else out_lo(); \
    delayMicroseconds(NEXT_KBD_TIMING); \
  } while (0);
#define out_hi_delay(intervals) \
  do { \
    out_hi(); \
    delayMicroseconds((NEXT_KBD_TIMING)*intervals); \
  } while (0);
#define out_lo_delay(intervals) \
  do { \
    out_lo(); \
    delayMicroseconds((NEXT_KBD_TIMING)*intervals); \
  } while (0);
#define query_delay(intervals) \
  do { \
    query(); \
    delayMicroseconds((NEXT_KBD_TIMING)*intervals); \
  } while (0);
#define reset_delay(intervals) \
  do { \
    reset(); \
    delayMicroseconds((NEXT_KBD_TIMING)*intervals); \
  } while (0);

#define PACKET_SIZE 10
#define QUERY_SIZE 22

bool kms_is_ready() {
  int tries = 41 * 2;
  int count = 0;

  cli();
  while (digitalRead(PIN_TO_KBD) && tries) {
    asm("");
    delayMicroseconds(NEXT_KBD_TIMING / 2);
    tries--;
    count++;
  }
  sei();

  if (!tries || count < 7 * 2) {
    return false;
  }

  return true;
}

uint32_t kms_read() {
  uint32_t result = 0;

  cli();
  int bit = 0;
  for (; bit < PACKET_SIZE; bit++) {
    result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
    delayMicroseconds(NEXT_KBD_TIMING);
  }
  // Stop bit
  result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
  delayMicroseconds(NEXT_KBD_TIMING);
  bit++;

#define BIT_RW 5
  if (result & (1 << BIT_RW)) {
    //    delayMicroseconds(NEXT_KBD_TIMING);
    return result;
  }

  // Stop bit 2
  result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
  delayMicroseconds(NEXT_KBD_TIMING);
  bit++;

  for (; bit < QUERY_SIZE; bit++) {
    result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
    delayMicroseconds(NEXT_KBD_TIMING);
  }
  sei();
  Serial.println("");
  return result;
}

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
  Serial.println("Starting NeXT Keyboard Mouse");

  // USB Soft Host
//  USH.init( USB_Pins_Config, my_USB_DetectCB, my_USB_PrintCB );
  USH.init(USB_Pins_Config, NULL, NULL);
  USH.setPrintCb(ush_handle_data);
  USH.setOnIfaceDescCb(ush_handle_interface_descriptor);

  pinMode(PIN_TO_KBD, INPUT_PULLUP);
  pinMode(PIN_FROM_KBD, OUTPUT);
  out_hi();
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
    } 
    else if (c == 0x0A) {
    //   c = 0x2A;  // ENTER/RETURN
    }
     else {
#define NEXT_KEY_A 0x39
      c = c - ('a' - NEXT_KEY_A);
    }
  }

  if (kms_is_ready()) {
    uint32_t result = kms_read();

    if ((result & 0x1FF) == 0b000100000) {
      Serial.printf("K");
      if (c) {
        sendKey(c, pressed, modifiers);
        Serial.printf("\nSent %c (0x%x). pressed=%d modifiers=0x%x\n", c, c, pressed, modifiers);
        if (pressed) {
          pressed = false;
        } else {
          c = 0;
          modifiers = 0;
        }
      } else {
        sendKey(0, false, 0);
      }
    } else if ((result & 0x1FF) == 0b000100010) {
      Serial.printf("M");
      char mousex = 1;
      char mousey = 1;
      bool button1 = false;
      bool button2 = false;
      // Only if mouse moved or button pressed
      //sendMouse(mousex, mousey, button1, button2);
      sendKey(0, false, 0);
    } else if ((result & 0x3FFFFF) == 0b0000000000111111011110) {
      Serial.println("\nRESET");
    } else if ((result & 0x1FFF) == 0b0111000000000) {
      Serial.println("");
      if (result & 0x2000)
        Serial.printf("L");
      else
        Serial.printf("l");

      if (result & 0x4000)
        Serial.printf("R");
      else
        Serial.printf("r");

      Serial.println("");
    } else {
      //    Serial.printf("kms_read=0x%x\n", result);
      Serial.println("");
      print_byte((result >> 16) & 0xff);
      print_byte((result >> 8) & 0xff);
      print_byte(result & 0xff);
      Serial.println("");
      //      Serial.printf("?");
      // 00000000.00000110.01100010.
      // 00000000.00000110.01100000.
      // 00000000.00000110.01100110.
      // 00000000.00000110.01100100.
      // 00111111.11111110.00000000.
      //
    }
  } else {
    //Serial.print("X");
  }
}

static inline void sendKey(char key, bool pressed, char modifiers) {
  cli();

  // Start bit
  out_lo_delay(1);

  // 8 bit data
  for (int bit = 0; bit <= 6; bit++)
    out_delay(key & (1 << bit));
  out_delay(key && !pressed);

  // C/D (Command/Data) = 0 (Data). If no data, send 1 (Command) and data all zeroes
  out_delay(!(key || modifiers));

  // Stop bit
  out_hi_delay(1);

  // Start bit
  out_lo_delay(1);

  // 8 bit data
  for (int bit = 0; bit <= 6; bit++)
    out_delay(modifiers & (1 << bit));
  out_delay((key || modifiers));

  // C/D (Command/Data) = 0 (Data). If not data, send 1 (Command) and data all zeroes
  out_delay(!(key || modifiers));

  // Stop bit
  out_hi_delay(1);

  sei();
}

static inline void sendMouse(char mousex, char mousey, bool button1, bool button2) {
  cli();

  out_lo_delay(1);  // fixed 0

  out_delay(!button1);
  for (int bit = 0; bit <= 6; bit++)
    out_delay((~mousex) & (1 << bit));

  out_lo_delay(1);  // fixed 0
  out_hi_delay(1);  // fixed 1
  out_hi_delay(1);  // fixed 1 ?? are you sure drakware?
  out_lo_delay(1);  // fixed 0

  out_delay(!button2);
  for (int bit = 0; bit <= 6; bit++)
    out_delay((~mousey) & (1 << bit));

  out_lo_delay(1);  // fixed 0

  out_hi_delay(12);  // OFF high
  sei();
}
