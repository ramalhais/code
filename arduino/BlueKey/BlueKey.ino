#include <BleKeyboard.h> // https://github.com/T-vK/ESP32-BLE-Keyboard

//#define DEBUG(X)
#define DEBUG(X) X

//Scan Pins
int fpc2pin[] = {
  -1, // 0
  35,
  15,
  34,
  -1,
  36,
  2,
  13,
  0,
  12,
  4, // 10
  14,
  16,
  -1,
  -1,
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
  -1,
  -1 // 30
};

typedef struct {
 uint8_t    key_code;
 short      fpc_write;
 short      fpc_read;
 short      value;
} t_key;

typedef struct {
 uint8_t    key_code;
 uint8_t    fn_key_code;
} t_fn_key;

typedef struct {
  uint8_t key_code;
  short mkr_byte;
  short mkr_bit;
} t_mediakey;

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
#define KEY_SECTION 236
#define KEY_RIGHTBRACKET ']'
#define KEY_LEFTBRACKET '['
#define KEY_EQUAL '='
#define KEY_MINUS '-'
#define KEY_SLASH '/'
#define KEY_SEMICOLON ';'
#define KEY_BACKSLASH '\\'

#define KEY_FN 255 //rbKeyMacFn 63 http://web.archive.org/web/20100501161453/http://www.classicteck.com/rbarticles/mackeyboard.php
uint8_t fn_key_keycode = KEY_FN;
short fn_key_value = HIGH;

#define KEY_BRIGHTNESS_DOWN 242 // CONSUMER_BRIGHTNESS_DOWN = 0x0070,
#define KEY_BRIGHTNESS_UP   243 // CONSUMER_BRIGHTNESS_UP = 0x006F,
#define KEY_SCALE           244
#define KEY_DASHBOARD       245
#define KEY_KBDILLUMDOWN    246
#define KEY_KBDILLUMUP      247
#define KEY_PREV_TRACK      248
#define KEY_PLAY_PAUSE      249
#define KEY_NEXT_TRACK      250
#define KEY_MUTE            251
#define KEY_VOLUME_DOWN     252
#define KEY_VOLUME_UP       253
#define KEY_POWER           254 // 0x30

t_fn_key fn_keys[] = {
  {KEY_BACKSPACE,   KEY_DELETE},
  {KEY_RETURN,      KEY_INSERT},
  {KEY_UP_ARROW,    KEY_PAGE_UP},
  {KEY_DOWN_ARROW,  KEY_PAGE_DOWN},
  {KEY_LEFT_ARROW,  KEY_HOME},
  {KEY_RIGHT_ARROW, KEY_END},
  {KEY_F1,          KEY_BRIGHTNESS_DOWN},
  {KEY_F2,          KEY_BRIGHTNESS_UP},
  {KEY_F3,          KEY_SCALE},
  {KEY_F4,          KEY_DASHBOARD},
  {KEY_F5,          KEY_KBDILLUMDOWN},
  {KEY_F6,          KEY_KBDILLUMUP},
  {KEY_F7,          KEY_PREV_TRACK},
  {KEY_F8,          KEY_PLAY_PAUSE},
  {KEY_F9,          KEY_NEXT_TRACK},
  {KEY_F10,         KEY_MUTE},
  {KEY_F11,         KEY_VOLUME_DOWN},
  {KEY_F12,         KEY_VOLUME_UP}
};

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf Page 53/61/75
// Change the report mappings in library ESP32-BLE-Keyboard/BleKeyboard.cpp _hidReportDescriptor
// https://github.com/NicoHood/HID/blob/d4938ddcff7970bc1d32a040a08afeac4915e4a9/src/HID-APIs/ConsumerAPI.h#L174
t_mediakey mediakeys[] {
  {KEY_NEXT_TRACK,      0, 0},    // USAGE (Scan Next Track)     ; bit 0: 1
  {KEY_PREV_TRACK,      0, 1},    // USAGE (Scan Previous Track) ; bit 1: 2
  {KEY_BRIGHTNESS_UP,   0, 2},    // USAGE(2), 0x6F, 0x00 // CONSUMER_BRIGHTNESS_UP
  {KEY_PLAY_PAUSE,      0, 3},    // USAGE (Play/Pause)          ; bit 3: 8
  {KEY_MUTE,            0, 4},    // USAGE (Mute)                ; bit 4: 16
  {KEY_VOLUME_UP,       0, 5},    // USAGE (Volume Increment)    ; bit 5: 32
  {KEY_VOLUME_DOWN,     0, 6},    // USAGE (Volume Decrement)    ; bit 6: 64
  {KEY_BRIGHTNESS_DOWN, 0, 7},    // USAGE(2), 0x70, 0x00 // CONSUMER_BRIGHTNESS_DOWN
  {KEY_POWER,           1, 0},    // USAGE(1), 0x30       // HID_CONSUMER_POWER
};

t_key row1[] = {
  {KEY_LEFT_CTRL,   0,   1},
  {KEY_LEFT_ALT,    0,   2},
  {KEY_LEFT_SHIFT,  0,   3},
  {KEY_POWER,       0,   5},
};
t_key row2[] = {
  {KEY_F1,          6,   24},
  {KEY_F2,          6,   23},
  {KEY_F3,          6,   22},
  {KEY_F4,          6,   20},
  {KEY_F5,          6,   17},
  {KEY_F6,          6,   21},
  {KEY_F7,          6,   12},
  {KEY_F8,          6,   11},
  {KEY_F9,          6,   10},
  {KEY_RIGHT_SHIFT, 6,   19},
};
t_key row3[] = {
  {KEY_1,           27,  24},
  {KEY_2,           27,  23},
  {KEY_3,           27,  22},
  {KEY_4,           27,  20},
  {KEY_5,           27,  17},
  {KEY_6,           27,  21},
  {KEY_7,           27,  12},
  {KEY_8,           27,  11},
  {KEY_9,           27,  10},
};
t_key row4[] = {
  {KEY_Q,           28,  24},
  {KEY_W,           28,  23},
  {KEY_E,           28,  22},
  {KEY_R,           28,  20},
  {KEY_T,           28,  17},
  {KEY_Y,           28,  21},
  {KEY_U,           28,  12},
  {KEY_I,           28,  11},
  {KEY_O,           28,  10},
};
t_key row5[] = {
  {KEY_A,           26,  24},
  {KEY_S,           26,  23},
  {KEY_D,           26,  22},
  {KEY_F,           26,  20},
  {KEY_G,           26,  17},
  {KEY_H,           26,  21},
  {KEY_J,           26,  12},
  {KEY_K,           26,  11},
  {KEY_L,           26,  10},
};
t_key row6[] = {
  {KEY_Z,           25,  24},
  {KEY_X,           25,  23},
  {KEY_C,           25,  22},
  {KEY_V,           25,  20},
  {KEY_B,           25,  17},
  {KEY_N,           25,  21},
  {KEY_M,           25,  12},
  {KEY_COMMA,       25,  11},
  {KEY_DOT,         25,  10},
  {KEY_LEFT_GUI,    25,  18},
};
t_key row7[] = {
  {KEY_BACKSPACE,   8,   24},
  {KEY_TAB,         8,   23},
  {KEY_FN,          8,   17},
  {KEY_SPACE,       8,   21},
  {KEY_APOSTROPHE,  8,   12},
  {KEY_LEFT_ARROW,  8,   11},
  {KEY_DOWN_ARROW,  8,   10},
  {KEY_CAPS_LOCK,   8,   15},
  {KEY_RIGHT_ALT,   8,   16},
};
t_key row8[] = {
  {KEY_F12,         7,   24},
  {KEY_GRAVE,       7,   23},
  {KEY_SECTION,     7,   22},
  {KEY_RIGHTBRACKET,7,   20},
  {KEY_LEFTBRACKET, 7,   17},
  {KEY_EQUAL,       7,   21},
  {KEY_RIGHT_ARROW, 7,   11},
  {KEY_UP_ARROW,    7,   10},
  {KEY_RIGHT_GUI,   7,   18},
};
t_key row9[] = {
  {KEY_F11,         9,   24},
  {KEY_ESC,         9,   23},
  {KEY_MINUS,       9,   17},
  {KEY_SLASH,       9,   21},
  {KEY_SEMICOLON,   9,   12},
  {KEY_BACKSLASH,   9,   10},
  {KEY_P,           9,   15},
  {KEY_F10,         9,   16},
  {KEY_0,           9,   18},
  {KEY_RETURN,      9,   19}
};

int key_rows_count[] = {
  sizeof(row1)/sizeof(t_key),
  sizeof(row2)/sizeof(t_key),
  sizeof(row3)/sizeof(t_key),
  sizeof(row4)/sizeof(t_key),
  sizeof(row5)/sizeof(t_key),
  sizeof(row6)/sizeof(t_key),
  sizeof(row7)/sizeof(t_key),
  sizeof(row8)/sizeof(t_key),
  sizeof(row9)/sizeof(t_key)  
};

t_key *key_rows[] = {
  row1,
  row2,
  row3,
  row4,
  row5,
  row6,
  row7,
  row8,
  row9
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

#define LED_CAPS TX
void led_caps(bool enable) {
  DEBUG(printf("caps keys: %s\n", enable?"ON":"OFF");)
  int pin = LED_CAPS;
  
  if (enable) {
    set_pin_write(pin);
  } else {
    set_pin_read(pin);
  }
}

#define BIT_CAPS 1
class ReportCallbacks : public BLECharacteristicCallbacks {
  public:
    ReportCallbacks(void) {
    }
    void onWrite(BLECharacteristic* me) {
      uint8_t* value = (uint8_t*)(me->getValue().c_str());
      DEBUG(printf("special keys: %d\n", *value);)
      
      led_caps(*value&(1<<BIT_CAPS));
    }
};

BleKeyboard bleKeyboard("BlueKey v1", "Pedro Ramalhais", 100, new ReportCallbacks());

void setup() {
  DEBUG(Serial.begin(115200);)

  for (int kr=0; kr<sizeof(key_rows)/sizeof(key_rows[0]); kr++) {
    DEBUG(Serial.printf("Seting up key row %d\n", kr);)
    t_key *key_row = key_rows[kr];

    int write_pin = fpc2pin[key_row[0].fpc_write];
    if (write_pin != -1) {
      set_pin_read(write_pin); // Disable column
    }
    
    for (int k=0; k<key_rows_count[kr]; k++) {
      int read_pin = fpc2pin[key_row[k].fpc_read];
      set_pin_read(read_pin);
      key_row[k].value = digitalRead(read_pin);
    }
  }

  DEBUG(Serial.println("Starting BLE work!");)
  bleKeyboard.begin();
}

unsigned long last_micros = micros();
const int debouncing_micros = 20000; // 20 miliseconds

uint8_t fn_transform_keycode(uint8_t key) {
  if (fn_key_value == LOW) {
    for (int i=0; i<sizeof(fn_keys)/sizeof(t_fn_key); i++) {
      if (fn_keys[i].key_code == key)
        return fn_keys[i].fn_key_code;
    }
  }

  return key;
}

void handle_key(uint8_t key, bool pressed) {
  if (key < 239) {
    if (pressed) {
      bleKeyboard.press(key);
    } else {
      bleKeyboard.release(key);
    }
  } else {
    for (int i=0; i<sizeof(mediakeys)/sizeof(t_mediakey); i++) {
      if (mediakeys[i].key_code == key) {
        MediaKeyReport k;
        k[mediakeys[i].mkr_byte] = 1<<mediakeys[i].mkr_bit;
    
        if (pressed) {
          bleKeyboard.press(k);
        } else {
          bleKeyboard.release(k);
        }

        break;
      }
    }
  }
}

void loop() {
  if (!bleKeyboard.isConnected()) {
    Serial.print("W");
    delay(500);
    return;
  }
  unsigned long cur_micros = micros();
  if(cur_micros - last_micros > debouncing_micros) {
    last_micros = cur_micros;

    for (int kr=0; kr<sizeof(key_rows)/sizeof(key_rows[0]); kr++) {
      t_key *key_row = key_rows[kr];
  
      int write_pin = fpc2pin[key_row[0].fpc_write];
      if (write_pin != -1) {
        set_pin_write(write_pin); // Enable row
        delayMicroseconds(1);
      }
  
      for (int k=0; k<key_rows_count[kr]; k++) {
        int read_pin = fpc2pin[key_row[k].fpc_read];
        int new_pin_value = digitalRead(read_pin);

        if (key_row[k].key_code == fn_key_keycode) {
          fn_key_value = new_pin_value;
        } else {
          int transformed_keycode = fn_transform_keycode(key_row[k].key_code);
          if (new_pin_value < key_row[k].value) {
            DEBUG(Serial.printf("P0x%x\n", key_row[k].key_code);)
            handle_key(transformed_keycode,true);
          } else if (new_pin_value > key_row[k].value) {
            DEBUG(Serial.printf("R0x%x\n", key_row[k].key_code);)
            handle_key(transformed_keycode, false);
          }
        }
        key_row[k].value = new_pin_value;
      }
  
      set_pin_read(write_pin); // Disable row
      delayMicroseconds(1);
    }
  }
}
