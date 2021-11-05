#include <BleKeyboard.h> // https://github.com/T-vK/ESP32-BLE-Keyboard

BleKeyboard bleKeyboard("BlueLapKey v1", "Pedro Ramalhais", 100);

#define DEBUG(X) X

//Scan Pins
int fpc2pin[] = {
  0, // 0
  35,
  15,
  34,
  0,
  36,
  2,
  13,
  0,
  12,
  4, // 10
  14,
  16,
  0,
  0,
  27,
  17,
  26,
  5,
  39,
  18, //20
  25,
  19,
  33,
  21,
  32,
  3,
  23,
  22,
  0,
  0 // 30
};

typedef enum {
  SINGLE_PIN,
  SCAN_PIN
} t_pin_type;

typedef struct {
 uint8_t    key_code;
 t_pin_type type;
 short      fpc_write;
 short      fpc_read;
} t_key;

#define KEY_POWER 0x1a  //FIXME
//#define KEY_MUTE        0x7F
//#define KEY_VOLUMEUP      0x80
//#define KEY_VOLUMEDOWN    0x81

#define KEY_1 '1'
#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_5 '5'
#define KEY_6 '6'
#define KEY_7 '7'
#define KEY_8 '8'
#define KEY_9 '9'
#define KEY_0 '0'
#define KEY_A 'a'
#define KEY_B 'b'
#define KEY_C 'c'
#define KEY_D 'd'
#define KEY_E 'e'
#define KEY_F 'f'
#define KEY_G 'g'
#define KEY_H 'h'
#define KEY_I 'i'
#define KEY_J 'j'
#define KEY_K 'k'
#define KEY_L 'l'
#define KEY_M 'm'
#define KEY_N 'n'
#define KEY_O 'o'
#define KEY_P 'p'
#define KEY_Q 'q'
#define KEY_R 'r'
#define KEY_S 's'
#define KEY_T 't'
#define KEY_U 'u'
#define KEY_V 'v'
#define KEY_W 'w'
#define KEY_X 'x'
#define KEY_Y 'y'
#define KEY_Z 'z'
#define KEY_COMMA ','
#define KEY_DOT '.'
#define KEY_SPACE ' '
#define KEY_APOSTROPHE '\''
#define KEY_GRAVE '`'
#define KEY_SECTION 245
#define KEY_RIGHTBRACE '}'
#define KEY_LEFTBRACE '{'
#define KEY_EQUAL '='
#define KEY_MINUS '-'
#define KEY_SLASH '/'
#define KEY_SEMICOLON ';'
#define KEY_BACKSLASH '\\'

#define SPECIAL_FN 255

//CONSUMER_POWER = 0x30
//HID_CONSUMER_POWER  = 0x30, // HID type OOC

t_key keys[] = {
  {KEY_LEFT_CTRL,    SINGLE_PIN, 0,   1},
  {KEY_LEFT_ALT,     SINGLE_PIN, 0,   2},
  {KEY_LEFT_SHIFT,   SINGLE_PIN, 0,   3},
  {KEY_POWER,       SINGLE_PIN, 0,   5},
  {KEY_F1,          SCAN_PIN,   6,   24},
  {KEY_F2,          SCAN_PIN,   6,   23},
  {KEY_F3,          SCAN_PIN,   6,   22},
  {KEY_F4,          SCAN_PIN,   6,   20},
  {KEY_F5,          SCAN_PIN,   6,   17},
  {KEY_F6,          SCAN_PIN,   6,   21},
  {KEY_F7,          SCAN_PIN,   6,   12},
  {KEY_F8,          SCAN_PIN,   6,   11},
  {KEY_F9,          SCAN_PIN,   6,   10},
  {KEY_1,           SCAN_PIN,   27,  24},
  {KEY_2,           SCAN_PIN,   27,  23},
  {KEY_3,           SCAN_PIN,   27,  22},
  {KEY_4,           SCAN_PIN,   27,  20},
  {KEY_5,           SCAN_PIN,   27,  17},
  {KEY_6,           SCAN_PIN,   27,  21},
  {KEY_7,           SCAN_PIN,   27,  12},
  {KEY_8,           SCAN_PIN,   27,  11},
  {KEY_9,           SCAN_PIN,   27,  10},
  {KEY_Q,           SCAN_PIN,   28,  24},
  {KEY_W,           SCAN_PIN,   28,  23},
  {KEY_E,           SCAN_PIN,   28,  22},
  {KEY_R,           SCAN_PIN,   28,  20},
  {KEY_T,           SCAN_PIN,   28,  17},
  {KEY_Y,           SCAN_PIN,   28,  21},
  {KEY_U,           SCAN_PIN,   28,  12},
  {KEY_I,           SCAN_PIN,   28,  11},
  {KEY_O,           SCAN_PIN,   28,  10},
  {KEY_A,           SCAN_PIN,   26,  24},
  {KEY_S,           SCAN_PIN,   26,  23},
  {KEY_D,           SCAN_PIN,   26,  22},
  {KEY_F,           SCAN_PIN,   26,  20},
  {KEY_G,           SCAN_PIN,   26,  17},
  {KEY_H,           SCAN_PIN,   26,  21},
  {KEY_J,           SCAN_PIN,   26,  12},
  {KEY_K,           SCAN_PIN,   26,  11},
  {KEY_L,           SCAN_PIN,   26,  10},
  {KEY_Z,           SCAN_PIN,   25,  24},
  {KEY_X,           SCAN_PIN,   25,  23},
  {KEY_C,           SCAN_PIN,   25,  22},
  {KEY_V,           SCAN_PIN,   25,  20},
  {KEY_B,           SCAN_PIN,   25,  17},
  {KEY_N,           SCAN_PIN,   25,  21},
  {KEY_M,           SCAN_PIN,   25,  12},
  {KEY_COMMA,       SCAN_PIN,   25,  11},
  {KEY_DOT,         SCAN_PIN,   25,  10},
  {KEY_LEFT_GUI,    SCAN_PIN,   25,  18},
  {KEY_BACKSPACE,   SCAN_PIN,   8,   24},
  {KEY_TAB,         SCAN_PIN,   8,   23},
  {SPECIAL_FN,      SCAN_PIN,   8,   17},
  {KEY_SPACE,       SCAN_PIN,   8,   21},
  {KEY_APOSTROPHE,  SCAN_PIN,   8,   12},
  {KEY_LEFT_ARROW,        SCAN_PIN,   8,   11},
  {KEY_DOWN_ARROW,        SCAN_PIN,   8,   10},
  {KEY_CAPS_LOCK,    SCAN_PIN,   8,   15},
  {KEY_RIGHT_ALT,    SCAN_PIN,   8,   16},
  {KEY_F12,         SCAN_PIN,   7,   24},
  {KEY_GRAVE,       SCAN_PIN,   7,   23},
  {KEY_SECTION,     SCAN_PIN,   7,   22},
  {KEY_RIGHTBRACE,  SCAN_PIN,   7,   20},
  {KEY_LEFTBRACE,   SCAN_PIN,   7,   17},
  {KEY_EQUAL,       SCAN_PIN,   7,   21},
  {KEY_RIGHT_ARROW,       SCAN_PIN,   7,   11},
  {KEY_UP_ARROW,          SCAN_PIN,   7,   10},
  {KEY_RIGHT_GUI,   SCAN_PIN,   7,   18},
  {KEY_F11,         SCAN_PIN,   9,   24},
  {KEY_ESC,         SCAN_PIN,   9,   23},
  {KEY_MINUS,       SCAN_PIN,   9,   17},
  {KEY_SLASH,       SCAN_PIN,   9,   21},
  {KEY_SEMICOLON,   SCAN_PIN,   9,   12},
  {KEY_BACKSLASH,   SCAN_PIN,   9,   10},
  {KEY_P,           SCAN_PIN,   9,   15},
  {KEY_F10,         SCAN_PIN,   9,   16},
  {KEY_0,           SCAN_PIN,   9,   18},
  {KEY_RETURN,       SCAN_PIN,   9,   19}
};

void set_pin_read(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}

void set_pin_write(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

volatile unsigned long last_interrupt_micros = micros();
const int debouncing_micros = 20000; // 20 miliseconds
void IRAM_ATTR SinglePinHandler(void *arg) {
  t_key *key_ptr = (t_key *)arg;
  noInterrupts();
  unsigned long interrupt_micros = micros();
  if (interrupt_micros - last_interrupt_micros > debouncing_micros) {
    last_interrupt_micros = interrupt_micros;
    DEBUG(Serial.println("SinglePinHandler()");)
    if (digitalRead(fpc2pin[key_ptr->fpc_read]) == LOW) {
      bleKeyboard.press(key_ptr->key_code);
    } else {
      bleKeyboard.release(key_ptr->key_code);
    }
  }
  interrupts();
}

void IRAM_ATTR ScanPinHandler() {
  noInterrupts();
  unsigned long interrupt_micros = micros();
  if (interrupt_micros - last_interrupt_micros > debouncing_micros) {
    last_interrupt_micros = interrupt_micros;
    DEBUG(Serial.println("ScanPinHandler()");)
  }
  interrupts();
}

int n_single_pins_read = 0;
int *single_pins_read = NULL;

int n_scan_pins_read = 0;
int *scan_pins_read = NULL;
int n_scan_pins_write = 0;
int *scan_pins_write = NULL;

void setup() {
  Serial.begin(115200);

  for (int k=0; k<sizeof(keys)/sizeof(keys[0]); k++) {
    DEBUG(Serial.printf("Reading key %d\n", k);)

    if (keys[k].type == SINGLE_PIN) {
      bool found = false;
      for (int p=0; p<n_single_pins_read; p++) {
        if (single_pins_read[p] == keys[k].fpc_read) {
          found = true;
          break;
        }
      }
      if (!found) {
        n_single_pins_read++;
        single_pins_read = (int*)realloc(single_pins_read, n_single_pins_read*sizeof(int));
        single_pins_read[n_single_pins_read-1] = keys[k].fpc_read;

        int read_pin = fpc2pin[keys[k].fpc_read];
        set_pin_read(read_pin);
        attachInterruptArg(digitalPinToInterrupt(read_pin), SinglePinHandler, &keys[k], CHANGE);
      }
    } else if(keys[k].type == SCAN_PIN) {
      bool found = false;
      for (int p=0; p<n_scan_pins_read; p++) {
        if (scan_pins_read[p] == keys[k].fpc_read) {
          found = true;
          break;
        }
      }
      if (!found) {
        n_scan_pins_read++;
        scan_pins_read = (int*)realloc(scan_pins_read, n_scan_pins_read*sizeof(int));
        scan_pins_read[n_scan_pins_read-1] = keys[k].fpc_read;
        
        int read_pin = fpc2pin[keys[k].fpc_read];
        set_pin_read(read_pin);
        attachInterrupt(digitalPinToInterrupt(read_pin), ScanPinHandler, CHANGE);
      }

      found = false;
      for (int p=0; p<n_scan_pins_write; p++) {
        if (scan_pins_write[p] == keys[k].fpc_write) {
          found = true;
          break;
        }
      }
      if (!found) {
        n_scan_pins_write++;
        scan_pins_write = (int*)realloc(scan_pins_write, n_scan_pins_write*sizeof(int));
        scan_pins_write[n_scan_pins_write-1] = keys[k].fpc_write;
      }
    
    }

  }

  DEBUG(Serial.printf("Single pins: %d\n", n_single_pins_read);)
  DEBUG(Serial.printf("Scan read pins: %d\n", n_scan_pins_read);)
  DEBUG(Serial.printf("Scan write pins: %d\n", n_scan_pins_write);)
  
  DEBUG(Serial.println("Starting BLE work!");)
  bleKeyboard.begin();
}

void loop() {
  if(bleKeyboard.isConnected()) {
//    bleKeyboard.press(KEY_VOLUMEDOWN);
//    bleKeyboard.press(KEY_VOLUMEUP);
  }
  Serial.print(".");
  for (int p=0; p<n_scan_pins_write; p++) {
    int write_pin = fpc2pin[scan_pins_write[p]];
    noInterrupts();
    set_pin_write(write_pin);
    interrupts();
    delay(2);
    noInterrupts();
    set_pin_read(write_pin);
    interrupts();
  }
}
