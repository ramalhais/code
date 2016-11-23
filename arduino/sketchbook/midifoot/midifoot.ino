// 

// MIDI jack pin 5 connected to Digital pin 1 (TX) through a 220 ohm resistor
// MIDI jack pin 2 connected to ground
// MIDI jack pin 4 connected to +5V through a 220 ohm resistor

#include <MIDI.h>

#define LED_DEBUG 1

#define PIN_LED_ONBOARD 13
int buttonPin[] = {2, 3, 4, 5};
#define NBUTTONS (sizeof(buttonPin) / sizeof(buttonPin[0]))
#define LED_BLINK_DURATION 200

// Randall RM100 sends 9VAC on pins 7 and 3 (DIP Switch S2-2) and on pins 6 and 1 (DIP Switch S2-1)

// Randall Control Change Channel 1
#define RANDALL_CC_CH1 56
#define RANDALL_CC_CV 127
#define RANDALL_MIDI_CH_RM100 16 // DIP Switches S1 4 to 1 set to OFF (mine was on CH1 with all DIP switches ON)
#define RANDALL_MIDI_CH_RM4 1
#define RANDALL_MIDI_CH_RT250 16
#define RANDALL_MIDI_CH_DEFAULT RANDALL_MIDI_CH_RM4 // Set your channel here

#define MIDI_MODE_CC 1
#define MIDI_MODE_PC 2
int midi_mode = MIDI_MODE_CC;

#if LED_DEBUG
  int blinkStep = 0;
#endif

MIDI_CREATE_DEFAULT_INSTANCE();

void blink(unsigned int times, unsigned int duration) {
  while (times--) {
    digitalWrite(PIN_LED_ONBOARD, HIGH);
    delay(duration);
    digitalWrite(PIN_LED_ONBOARD, LOW);
    delay(duration);
  }
}

void buttonToMIDI(int button) {
  if (midi_mode == MIDI_MODE_CC) {
    MIDI.sendControlChange(RANDALL_CC_CH1+button-1, RANDALL_CC_CV, RANDALL_MIDI_CH_DEFAULT);
  } else if (midi_mode == MIDI_MODE_PC) {
    MIDI.sendProgramChange(button, RANDALL_MIDI_CH_DEFAULT);
  }
  blink(button, LED_BLINK_DURATION);
}

void setup() {
#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION*4);
#endif

  pinMode(PIN_LED_ONBOARD, OUTPUT);
  for (int i; i< NBUTTONS; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  // Launch MIDI and listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION*4);
#endif
}

void loop() {
  static int pinVal[] = {HIGH,HIGH,HIGH,HIGH};

  for (int i=0; i<NBUTTONS; i++) {
    int pinValNew = digitalRead(buttonPin[i]);
    if (pinVal[i] != pinValNew) {
      pinVal[i] = pinValNew;
      buttonToMIDI(i+1);
      delay(500);
      break;
    }
  }

  //MIDI.read();
}

