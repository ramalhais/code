#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
BLEMIDI_CREATE_INSTANCE("AmpMIDI", MIDI)

#ifndef LED_BUILTIN
#define LED_BUILTIN 22 // ESP32 WeMos LOLIN32 Lite (w/ battery connector)
//#define LED_BUILTIN 32
#endif
#define LED_ON LOW
#define LED_OFF HIGH

#define PIN_ENABLE LOW
#define PIN_DISABLE HIGH

#define BUT_SEL_PIN 12 // ESP32 WeMos LOLIN32 Lite (w/ battery connector)
//#define BUT_SEL_PIN 0 // ESP32 DevKitC V4
#define BUT_SEL_OFF HIGH
#define BUT_SEL_ON LOW
#define BUT_SEL_MIN_PRESS_MILLIS 100
#define BUT_SEL_LONG_PRESS_MILLIS 2000
#define BUT_SEL_VERY_LONG_PRESS_MILLIS 4000

bool but_sel_state = BUT_SEL_OFF;
unsigned long but_sel_pressed_millis = millis();

int channel_pins[] = {32, 33, 25}; // ESP32 WeMos LOLIN32 Lite (w/ battery connector)
//int channel_pins[] = {25, 26, 27}; // ESP32 DevKitC V4
#define CHANNEL_COUNT (sizeof(channel_pins)/sizeof(int))
int channel_current = -1;

typedef struct {
  unsigned short midiPC;
  int channel;
} t_midi_pc_config;
t_midi_pc_config *midi_pc_configs;
int midi_pc_configs_count = 0;

unsigned short midi_pc_current = 0;

#define DEBUG(X) X

void mute() {
  for (int i=0; i<CHANNEL_COUNT; i++) {
    pinMode(channel_pins[i], OUTPUT);
    digitalWrite(channel_pins[i], PIN_DISABLE);
  }
  channel_current = -1;
}

int channel_select(int channel_new) {
  if (channel_current >= 0) {
    digitalWrite(channel_pins[channel_current], PIN_DISABLE);
  }
  if (channel_new >= 0) {
    digitalWrite(channel_pins[channel_new], PIN_ENABLE);
  }
  channel_current = channel_new;

  DEBUG(Serial.printf("CH=%d;\n", channel_current));

  return channel_current;
}

int channel_down() {
  DEBUG(Serial.printf("CMD=CHDOWN;\n"));

  int channel_new = channel_current - 1;
  if (channel_new < -1) {
    channel_new = -1;
  }

  return channel_select(channel_new);
}

int channel_up() {
  DEBUG(Serial.printf("CMD=CHUP;\n"));

  int channel_new = (channel_current+1) % CHANNEL_COUNT;
  return channel_select(channel_new);
}

void channel_save() {
  DEBUG(Serial.printf("CMD=SAVE;\n"));

  // TODO: Save midi command assignment to settings (channel)
  midi_pc_configs = (t_midi_pc_config*) calloc(midi_pc_configs_count+1, sizeof(t_midi_pc_config));
  int s = sizeof(midi_pc_configs);
  int n = sizeof(midi_pc_configs)/sizeof(t_midi_pc_config);
  DEBUG(Serial.printf("MIDIPCSIZE=s=%d,n=%d;\n", s, n));
}

void OnConnected() {
  digitalWrite(LED_BUILTIN, LED_OFF);
}

void OnDisconnected() {
  digitalWrite(LED_BUILTIN, LED_ON);
}

/*
RM100 MIDI notes: https://www.randallamplifiers.com/wp-content/uploads/sites/9/2020/03/RM100-MIDI-guide.pdf

Program Changes 1 to 128 select EMB presets 1 to 128, which are saved in EEPROM
memory.

 Control Change #56 (w/Control Value 127) allows for “Instant Access” selection of Channel 1.
 Control Change #57 (w/Control Value 127) allows for “Instant Access” selection of Channel 2.
 Control Change #58 (w/Control Value 127) allows for “Instant Access” selection of Channel 3.
 Control Change #59 (w/Control Value 127) allows for “Instant Access” selection of Channel 4.
*/
void handleControlChange(byte midiChannel, byte cc, byte value) {
  DEBUG(Serial.printf("MIDI=CH%d,CC=%d,V=%d;\n", midiChannel, cc, value));

  if (cc >= 55 && value == 127) {
    channel_select(cc-56);
  }
  if (cc == 52 && value == 127) {
    channel_save();
  }
  if (cc == 53 && value == 127) {
    channel_down();
  }
  if (cc == 54 && value == 127) {
    channel_up();
  }
}

void handleProgramChange(byte midiChannel, byte pc) {
  DEBUG(Serial.printf("MIDI=CH%d,PC=%d;\n", midiChannel, pc));
  midi_pc_current = pc;
}

void setup() {
  mute();
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BUT_SEL_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  BLEMIDI.setHandleConnected(OnConnected);
  BLEMIDI.setHandleDisconnected(OnDisconnected);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.println("Setup complete");
}

void loop() {
  MIDI.read();

  if (Serial.available()) {
    String line = Serial.readStringUntil(';');
    Serial.read(); // Consume ';'

    DEBUG(Serial.printf("RECV=%s;\n", line.c_str()));

    if (line == "CMD=R") {
      ESP.restart();
    } else if(line == "CMD=CH") {
      DEBUG(Serial.printf("CH=%d;\n", channel_current));
    } else if(line == "CMD=CH1") {
      channel_select(0);
    } else if(line == "CMD=CH2") {
      channel_select(1);
    } else if(line == "CMD=CH3") {
      channel_select(2);
    } else if(line == "CMD=CHDOWN") {
      channel_down();
    } else if(line == "CMD=CHUP") {
      channel_up();
    } else if(line == "CMD=MUTE") {
      channel_select(-1);
    } else if(line == "CMD=FORCEMUTE") {
      mute();
    } else if(line == "CMD=SAVE") {
      channel_save();
    }
  }

  int pressed_duration_millis = millis() - but_sel_pressed_millis;
  int but_sel_state_new = digitalRead(BUT_SEL_PIN);
  if (pressed_duration_millis >= BUT_SEL_MIN_PRESS_MILLIS && but_sel_state != but_sel_state_new) {
    but_sel_pressed_millis = millis();
    but_sel_state = but_sel_state_new;

    if (but_sel_state_new == BUT_SEL_ON) {
      DEBUG(Serial.printf("BUTSEL=pressed;\n"));
    } else {
      DEBUG(Serial.printf("BUTSEL=millis=%d;\n", pressed_duration_millis));

      if (pressed_duration_millis >= BUT_SEL_VERY_LONG_PRESS_MILLIS) {
        channel_save();
      } else if (pressed_duration_millis >= BUT_SEL_LONG_PRESS_MILLIS) {
        channel_down();
      } else {
        channel_up();
      }
    }
  }
}
