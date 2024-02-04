// Not apostrophe, it's tilde or backtick or grave
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


// https://github.com/tmk/tmk_keyboard/issues/704
#define NEXT_KBD_TIMING     52 // scope says 52.75 to 53.125
//#define NEXT_KBD_TIMING     53 // scope says close to 54us

#define PIN_TO_KBD    32  // Input to this keyboard
#define PIN_FROM_KBD  33  // Output from this keyboard

static inline void out_lo(void) {
  digitalWrite(PIN_FROM_KBD, LOW);
}

static inline void out_hi(void) {
  digitalWrite(PIN_FROM_KBD, HIGH);
}

#define out_delay(is_high)       do { if (is_high) out_hi(); else out_lo(); delayMicroseconds(NEXT_KBD_TIMING); } while (0);
#define out_hi_delay(intervals)  do { out_hi(); delayMicroseconds((NEXT_KBD_TIMING) * intervals); } while (0);
#define out_lo_delay(intervals)  do { out_lo(); delayMicroseconds((NEXT_KBD_TIMING) * intervals); } while (0);
#define query_delay(intervals)   do { query();  delayMicroseconds((NEXT_KBD_TIMING) * intervals); } while (0);
#define reset_delay(intervals)   do { reset();  delayMicroseconds((NEXT_KBD_TIMING) * intervals); } while (0);

#define PACKET_SIZE 10
#define STOP_SIZE 2
#define QUERY_SIZE 22

bool kms_is_ready() {
    int tries = 50000;
    int count = 0;

    cli();
    while (digitalRead(PIN_TO_KBD) && tries)  {
        asm(""); delayMicroseconds(NEXT_KBD_TIMING/2); tries--; count++;
    }
    sei();

    if (!tries || count < (QUERY_SIZE/2)*2) {
        return false;
    }

    return true;
}

uint32_t kms_read() {
  uint32_t result = 0;

  cli();
  int bit=0;
  for (; bit<PACKET_SIZE; bit++) {
    result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
    delayMicroseconds(NEXT_KBD_TIMING);
  }
  // Stop bit
  result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
  delayMicroseconds(NEXT_KBD_TIMING);
  bit++;

  #define BIT_RW 5
  if (result&(1<<BIT_RW)) {
//    delayMicroseconds(NEXT_KBD_TIMING);
    return result;
  }

  // Stop bit 2
  result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
  delayMicroseconds(NEXT_KBD_TIMING);
  bit++;

  for (; bit<QUERY_SIZE; bit++) {
    result |= (digitalRead(PIN_TO_KBD) == HIGH) << bit;
    delayMicroseconds(NEXT_KBD_TIMING);
  }
  sei();
  Serial.println("");
  return result;
}

const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(uint8_t byte) {
    Serial.printf("%s%s.", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Serial");

  pinMode(PIN_TO_KBD, INPUT_PULLDOWN);
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
    } else if (c == 0x0A) {
      c = 0x2A; // ENTER/RETURN
    } else {
      #define NEXT_KEY_A 0x39
      c = c-('a'-NEXT_KEY_A);
    }
  }

  if (kms_is_ready()) {
    uint32_t result = kms_read();

    if ((result&0x1FF) == 0b000100000) {
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
    } else if ((result&0x1FF) == 0b000100010) {
      Serial.printf("M");
      char mousex = 1;
      char mousey = 1;
      bool button1 = false;
      bool button2 = false;
      sendMouse(mousex, mousey, button1, button2);
    } else if ((result&0x3FFFFF) == 0b0000000000111111011110) {
      Serial.println("\nRESET");
    } else if ((result&0x1FFF) == 0b0111000000000) {
      Serial.println("");
      if (result&0x2000)
        Serial.printf("L");
      else
        Serial.printf("l");

      if (result&0x4000)
        Serial.printf("R");
      else
        Serial.printf("r");
      
      Serial.println("");
    } else {
  //    Serial.printf("kms_read=0x%x\n", result);
      Serial.println("");
      print_byte((result >> 16)&0xff);
      print_byte((result >>  8)&0xff);
      print_byte( result       &0xff);
      Serial.println("");
//      Serial.printf("?");
    }
  }

}

static inline void sendKey(char key, bool pressed, char modifiers) {
  cli();

  // Start bit
  out_lo_delay(1);

  // 8 bit data
  for (int bit=0; bit<=6; bit++)
    out_delay(key&(1<<bit));
  out_delay(key&&!pressed);
  
  // C/D (Command/Data) = 0 (Data). If no data, send 1 (Command) and data all zeroes
  out_delay(!(key||modifiers));

  // Stop bit
  out_hi_delay(1);

  // Start bit
  out_lo_delay(1);

  // 8 bit data
  for (int bit=0; bit<=6; bit++)
    out_delay(modifiers&(1<<bit));
  out_delay((key||modifiers));

  // C/D (Command/Data) = 0 (Data). If not data, send 1 (Command) and data all zeroes
  out_delay(!(key||modifiers));

  // Stop bit
  out_hi_delay(1);

  sei();
}

static inline void sendMouse(char mousex, char mousey, bool button1, bool button2) {
  cli();

  out_lo_delay(1); // fixed 0

  out_delay(!button1);
  for (int bit=0; bit<=6; bit++)
    out_delay((~mousex)&(1<<bit));

  out_lo_delay(1); // fixed 0
  out_hi_delay(1); // fixed 1
  out_hi_delay(1); // fixed 1 ?? are you sure drakware?
  out_lo_delay(1); // fixed 0

  out_delay(!button2);
  for (int bit=0; bit<=6; bit++)
    out_delay((~mousey)&(1<<bit));

  out_lo_delay(1); // fixed 0

  out_hi_delay(12); // OFF high
  sei();
}
