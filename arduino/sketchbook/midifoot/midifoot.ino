// MIDI jack pin 5 connected to Digital pin 1 (TX) through a 220 ohm resistor
// MIDI jack pin 2 connected to ground
// MIDI jack pin 4 connected to +5V through a 220 ohm resistor

#include <MIDI.h>

#define LED_DEBUG 1

#define PIN_LED_ONBOARD 13
#define PIN_BUTTON_1 2
#define PIN_BUTTON_2 3
#define PIN_BUTTON_3 4
#define PIN_BUTTON_4 5
#define LED_BLINK_DURATION 200

// Randall RM100 sends 9VAC on pins 7 and 3 (DIP Switch S2-2) and on pins 6 and 1 (DIP Switch S2-1)

// Randall Control Change Channels
#define RANDALL_CC_CH1 56
#define RANDALL_CC_CV 127
#define RANDALL_MIDI_CH_RM100 16 // DIP Switches S1 4 to 1 set to OFF
#define RANDALL_MIDI_CH_RM4 1
#define RANDALL_MIDI_CH_RT250 16
#define RANDALL_MIDI_CH_DEFAULT RANDALL_MIDI_CH_RM4

#define MIDI_MODE_CC 1
#define MIDI_MODE_PC 2
int midi_mode = MIDI_MODE_PC;

#if LED_DEBUG
  int blinkStep = 0;
#endif

MIDI_CREATE_DEFAULT_INSTANCE();


void setup() {
#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION*4);
#endif

  pinMode(PIN_LED_ONBOARD, OUTPUT);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  pinMode(PIN_BUTTON_4, INPUT_PULLUP);

  // Launch MIDI and listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION*4);
#endif
}

void loop() {
  int pin1 = digitalRead(PIN_BUTTON_1);
  int pin2 = digitalRead(PIN_BUTTON_2);
  int pin3 = digitalRead(PIN_BUTTON_3);
  int pin4 = digitalRead(PIN_BUTTON_4);

  if (pin1 == LOW) {
    buttonToMIDI(1);
  } else if (pin2 == LOW) {
    buttonToMIDI(2);
  } else if (pin3 == LOW) {
    buttonToMIDI(3);
  } else if (pin4 == LOW) {
    buttonToMIDI(4);
  }

  //MIDI.read();
  //sendProgramChange(DataByte inProgramNumber, Channel inChannel);
}

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

