#ifndef LED_BUILTIN
#define LED_BUILTIN 1
#endif

#define DEBUGLOOP(X)
//#define DEBUGLOOP(X) (X)

int idx2fpc[] = {0,0,0,7,9,11,15,17,21,23,25,1,3,19,5,27,28,0,26,24,22,20,18,16,12,10,8,6,2,0,0,0};

int all_pins[] = {11,10,9,13,12,14,27,26,25,33,32,35,34,39,36,23,22,1,3,21,19,18,5,17,16,4,0,2,15,8,7,6};
#define N_PINS (sizeof(all_pins)/sizeof(all_pins[0]))

int excluded_pins[] = {9,10,11,8,7,6,1}; //35,34,39,36,5
#define N_EXCLUDED_PINS (sizeof(excluded_pins)/sizeof(excluded_pins[0]))

// Function to set a pin as an input with a pullup so it's high unless grounded by a key press
void go_z(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}

// Function to set a pin as an output and drive it to a logic low (0 volts)
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

bool is_excluded(int pin) {
  for(int i=0; i<N_EXCLUDED_PINS; i++)
    if (pin == excluded_pins[i])
      return true;
  return false;
}

void init_pins() {
  for(int i=0; i<N_PINS; i++) {
    if (is_excluded(all_pins[i]))
      continue;
    go_z(all_pins[i]);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Txd is on pin: "+String(TX));
  init_pins();
}

int blink_count = 0; // loop counter
boolean blinky = LOW; // Blink LED state
void blink() {
  if (blink_count == 0x0a) {  
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, blinky);
    blinky = !blinky;
    blink_count = 0;
    Serial.print(".");
  }
  else {
    blink_count++;
  }
  delay(25);  // overall keyboard scan rate is about 30 milliseconds
}

void scan_up() {
  for(int i=0; i<N_PINS-2; i++) {
    DEBUGLOOP(Serial.println());
    if (is_excluded(all_pins[i])) {
      DEBUGLOOP(Serial.print("XI" + String(i) + ","));
      continue;
    }
    DEBUGLOOP(Serial.print("I" + String(i) + ","));
    go_0(all_pins[i]); // make the outer loop pin an output and send this pin low
    for(int j=i+1; j<N_PINS-1; j++) {
      if (is_excluded(all_pins[j])) {
        DEBUGLOOP(Serial.print("XJ" + String(j) + ","));
        continue;
      }
      DEBUGLOOP(Serial.print("J" + String(j) + ","));
      delayMicroseconds(50); // give time to let the signals settle out
      if (!digitalRead(all_pins[j])) {  // check for connection between inner and outer pins
        Serial.println();
        Serial.println("UP Detected I" + String(i) + ",J" + String(j) + " -> " + all_pins[i] + "," + all_pins[j] + " ->FPC " + idx2fpc[i] + "," + idx2fpc[j]);
        while(!digitalRead(all_pins[j])) {  // wait until key is released 
           ;     // if 2 pins are shorted, the code will hang here waiting for their release                                          
        }  
      }
    }
    go_z(all_pins[i]); // return the outer loop pin to float with pullup
  }
}

void scan_down() {
  for(int i=N_PINS-1; i>=1; i--) {
    DEBUGLOOP(Serial.println());
    if (is_excluded(all_pins[i])) {
      DEBUGLOOP(Serial.print("XI" + String(i) + ","));
      continue;
    }
    DEBUGLOOP(Serial.print("I" + String(i) + ","));
    go_0(all_pins[i]); // make the outer loop pin an output and send this pin low
    for(int j=i-1; j>=0; j--) {
      if (is_excluded(all_pins[j])) {
        DEBUGLOOP(Serial.print("XJ" + String(j) + ","));
        continue;
      }
      DEBUGLOOP(Serial.print("J" + String(j) + ","));
      delayMicroseconds(50); // give time to let the signals settle out
      if (!digitalRead(all_pins[j])) {  // check for connection between inner and outer pins
        Serial.println();
        Serial.println("DOWN Detected I" + String(i) + ",J" + String(j) + " -> " + all_pins[i] + "," + all_pins[j] + " ->FPC " + idx2fpc[i] + "," + idx2fpc[j]);
        while(!digitalRead(all_pins[j])) {  // wait until key is released 
           ;     // if 2 pins are shorted, the code will hang here waiting for their release                                          
        }  
      }
    }
    go_z(all_pins[i]); // return the outer loop pin to float with pullup
  }
}

void loop() {
  scan_up();
  scan_down();
  blink();
}
