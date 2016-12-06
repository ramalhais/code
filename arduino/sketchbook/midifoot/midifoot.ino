// MIDI jack pin 5 connected to Digital pin 1 (TX) through a 220 ohm resistor
// MIDI jack pin 2 connected to ground
// MIDI jack pin 4 connected to +5V through a 220 ohm resistor

#include <string.h>
#include <MIDI.h>

// Need to look into using RTP-MIDI: https://github.com/lathoub/Arduino-AppleMidi-Library
#define LED_DEBUG 1
#define PIN_LED_ONBOARD LED_BUILTIN
#define LED_BLINK_DURATION 200
#ifdef ARDUINO_ESP8266_NODEMCU
  #define LED_ON LOW
  #define LED_OFF HIGH
#else
  #define LED_ON HIGH
  #define LED_OFF LOW
#endif
//#define SERIAL_DEBUG
//#define MIDIFOOT_OTA

// Randall RM100 sends 9VAC on pins 7 and 3 (DIP Switch S2-2) and on pins 6 and 1 (DIP Switch S2-1)
#define RANDALL_MIDI_CH_RM100 16 // DIP Switches S1 4 to 1 set to OFF (mine was on CH1 with all DIP switches ON)
#define RANDALL_MIDI_CH_RM4 1
#define RANDALL_MIDI_CH_RT250 16
int midi_channel = RANDALL_MIDI_CH_RM4; // Set your channel here

#define MIDI_MODE_CC 1
#define MIDI_MODE_PC 2
int midi_mode = MIDI_MODE_CC;

#ifndef ARDUINO_ESP8266_NODEMCU
  #warning "Building for Arduino (Nano)"
int buttonPin[] = {2, 3, 4, 5};
#else // ARDUINO_ESP8266_NODEMCU
  #warning "Building for ESP8266 NodeMCU"
int buttonPin[] = {2, 3, 4, 5};
#endif // ARDUINO_ESP8266_NODEMCU
#define NBUTTONS (sizeof(buttonPin) / sizeof(buttonPin[0]))

// Randall Control Change Channel 1
#define RANDALL_CC_CH1 56
#define RANDALL_CC_CV 127
#define MIDI_CC_BUTTON1 RANDALL_CC_CH1 // Set the default CC for the first button
#define MIDI_MIN 0
#define MIDI_MAX 127

#define BUTTONMODE_CH       1
#define BUTTONMODE_UP       2
#define BUTTONMODE_DOWN     3
#define BUTTONMODE_BANKUP   4
#define BUTTONMODE_BANKDOWN 5

int buttonMode[] = {BUTTONMODE_CH, BUTTONMODE_CH, BUTTONMODE_UP, BUTTONMODE_BANKUP};
int buttonModeVal[] = {1, 2, 1, 1}; // CH1, CH2, Current PC/CC+1, Current Bank+1
int midi_cc = RANDALL_CC_CH1;
int midi_pc = 1;
int bank = 0;

int nbuttonmode_ch = 0;

#ifdef MIDIFOOT_OTA
  #warning "Enabling OTA Update"
  #define MIDIFOOT_VERSION "esp-1.0.0"
#endif // MIDIFOOT_OTA

#ifdef ARDUINO_ESP8266_NODEMCU
// https://github.com/nodemcu/nodemcu-devkit-v1.0
  #include <ESP8266WiFi.h>          // ESP8266 Core WiFi Library (you most likely already have this in your sketch)
  #include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
  #include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
  #include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

  #define WIFI_DEFAULT_SSID "midifoot"
  #define WIFI_DEFAULT_PASSWORD "itsmeleclerc"

  WiFiManager wifi_manager;
  WiFiServer wifi_server(80);

  #ifdef MIDIFOOT_OTA
    #include <ESP8266HTTPClient.h>
    #include <ESP8266httpUpdate.h>
  #endif // MIDIFOOT_OTA
#endif // ARDUINO_ESP8266_NODEMCU

#if LED_DEBUG
  int blinkStep = 0;
#endif

MIDI_CREATE_DEFAULT_INSTANCE();

void log(String message) {
#ifdef SERIAL_DEBUG
  Serial.println(message);
#endif  
}

void blink(unsigned int times, unsigned int duration) {
  log("Blink " + String(times) + " times, " + String(duration) + "ms duration");
  while (times--) {
    digitalWrite(PIN_LED_ONBOARD, LED_ON);
    delay(duration);
    digitalWrite(PIN_LED_ONBOARD, LED_OFF);
    delay(duration);
  }
  delay(duration*2);
}

#ifdef MIDIFOOT_OTA
void MidifootOTA() {
  HTTPClient http;
  String payload = MIDIFOOT_VERSION;

  http.begin("https://raw.githubusercontent.com/ramalhais/midifoot/esp/midifoot-latest.txt", "21 99 13 84 63 72 17 13 B9 ED 0E 8F 00 A5 9B 73 0D D0 56 58");
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    http.end();      
  } else {
    // USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  //  ESPhttpUpdate.update("github.com", 80, "/ramalhais/bla/arduino.bin");
  String midifootVersion = payload;
  String firmwareUrl = "https://raw.githubusercontent.com/ramalhais/midifoot/esp/midifoot-" + midifootVersion + ".bin";
  String fingerprint = "21 99 13 84 63 72 17 13 B9 ED 0E 8F 00 A5 9B 73 0D D0 56 58";
  if (midifootVersion != MIDIFOOT_VERSION) {
    t_httpUpdate_return ret = ESPhttpUpdate.update(firemwareUrl, "", fingerprint);
  }
}
#endif // MIDIFOOT_OTA

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(38400);
  delay(1000);
#endif

#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION);
#endif

  for (int b=0; b<NBUTTONS; b++) {
    if (buttonMode[b] == BUTTONMODE_CH)
      nbuttonmode_ch++;
  }

  pinMode(PIN_LED_ONBOARD, OUTPUT);
  for (int i; i< NBUTTONS; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }
#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION);
#endif

  // Launch MIDI and listen to all channels
//  MIDI.begin(MIDI_CHANNEL_OMNI);
#if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION);
#endif

#ifdef ARDUINO_ESP8266_NODEMCU
  wifi_manager.setConfigPortalTimeout(180);
  #ifdef SERIAL_DEBUG
  wifi_manager.setDebugOutput(true);
  #else
  wifi_manager.setDebugOutput(false);
  #endif // SERIAL_DEBUG
  wifi_manager.autoConnect(WIFI_DEFAULT_SSID, WIFI_DEFAULT_PASSWORD);
  #if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION);
  #endif

  #ifdef MIDIFOOT_OTA
  MidifootOTA();
  #endif // MIDIFOOT_OTA

  wifi_server.begin();
  #if LED_DEBUG
  blink(++blinkStep, LED_BLINK_DURATION);
  #endif
#endif
}

#ifdef ARDUINO_ESP8266_NODEMCU
void HandleHttpRequest() {
  WiFiClient wclient = wifi_server.available();
  if (wclient) {
    while(!wclient.available()) {
      delay(1);
    }

    String request = wclient.readStringUntil('\r');
    wclient.flush();

    String reply = "HTTP/1.1 200 OK\n"
      "Content-Type: text/html\n"
      "Connection: close\n"
      "\n"
      "<!DOCTYPE HTML>\n"
      "<html>\n"
      "Request was:<br />\n"
      "--------<br />\n"
      + request +
      "<br />\n"
      "--------<br />\n"
      "<a href=\"/push?button=1\"><button>Push button 1</button></a><br />\n"
      "<a href=\"/push?button=2\"><button>Push button 2</button></a><br />\n"
      "<a href=\"/push?button=3\"><button>Push button 3</button></a><br />\n"
      "<a href=\"/push?button=4\"><button>Push button 4</button></a><br />\n"
      "<a href=\"/midi?setchannel=1\"><button>Set channel 1</button></a><br />\n"
      "<a href=\"/midi?setmode=CC\"><button>Set mode to Control Change</button></a><br />\n"
      "<a href=\"/midi?setmode=PC\"><button>Set mode to Program Change</button></a><br />\n"
      "<a href=\"/buttonsetmode?button=1&mode=CH&modeval=1\"><button>Set button 1 to mode CH with value 1</button></a><br />\n"
      "<a href=\"/buttonsetmode?button=2&mode=CH&modeval=2\"><button>Set button 2 to mode CH with value 2</button></a><br />\n"
      "<a href=\"/buttonsetmode?button=3&mode=UP&modeval=1\"><button>Set button 3 to mode UP with value 1</button></a><br />\n"
      "<a href=\"/buttonsetmode?button=4&mode=BANKUP&modeval=1\"><button>Set button 4 to mode BANKUP with value 1</button></a><br />\n"
      "<a href=\"/resetwifi\"><button>Reset WiFi Configuration</button></a><br />\n"
      "<br />\n";

//    String uriPath = request.substring(request.indexOf('/')+1, request.indexOf('?'));

    if (request.indexOf("GET / ") != -1)  {
      log("Called index");
    } else if (request.indexOf("GET /push?button=") != -1)  {
      String b = request.substring(request.indexOf('=')+1, request.indexOf(' ', request.indexOf(' ')+1));
      int button = b.toInt();

      String message = "Button pushed from HTTP: " + String(button);
      log(message);
      reply += message + "<br />\n";;

      ButtonPress(button);
    } else if (request.indexOf("GET /midi?setchannel=") != -1)  {
      String c = request.substring(request.indexOf('=')+1, request.indexOf(' ', request.indexOf(' ')+1));
      int channel = c.toInt();
      midi_channel = channel;
      
      String message = "Channel set to: " + String(channel);
      log(message);
      reply += message + "<br />\n";
    } else if (request.indexOf("GET /midi?setmode=") != -1)  {
      String m = request.substring(request.indexOf('=')+1, request.indexOf(' ', request.indexOf(' ')+1));
      if (m == "CC")
        midi_mode = MIDI_MODE_CC;
      else if (m == "PC")
        midi_mode = MIDI_MODE_PC;

      String message = "MIDI mode set to: " + m;
      log(message);
      reply += message + "<br />\n";
    } else if (request.indexOf("GET /buttonsetmode?button=") != -1)  { // &mode=CH|UP|DOWN|BANKUP|BANKDOWN&modeval=x
      String b = request.substring(request.indexOf('=')+1, request.indexOf('&'));
      String m = request.substring(request.indexOf('=', request.indexOf('&'))+1, request.indexOf('&', request.indexOf('&')+1));
      String mv = request.substring(request.indexOf('=', request.indexOf('&', request.indexOf('&')+1)+1), request.indexOf(' ', request.indexOf(' ')+1) );

      if (m == "CH")
        buttonMode[b.toInt()-1] = BUTTONMODE_CH;
      else if (m == "UP")
        buttonMode[b.toInt()-1] = BUTTONMODE_UP;
      else if (m == "DOWN")
        buttonMode[b.toInt()-1] = BUTTONMODE_DOWN;
      else if (m == "BANKUP")
        buttonMode[b.toInt()-1] = BUTTONMODE_BANKUP;
      else if (m == "BANKDOWN")
        buttonMode[b.toInt()-1] = BUTTONMODE_BANKDOWN;

      buttonModeVal[b.toInt()-1] = mv.toInt();

      String message = "Button " + b + " set to mode " + m + " with value " + mv;
      log(message);
      reply += message + "<br />\n";
    } else if (request.indexOf("GET /resetwifi") != -1)  {
      reply = "HTTP/1.1 200 OK\n"
        "Content-Type: text/html\n"
        "Connection: close\n"
        "\n"
        "<!DOCTYPE HTML>\n"
        "<html><head><META HTTP-EQUIV=REFRESH CONTENT=\"60; URL=http://midifoot/\" /></head><body>"
        "Redirecting to <a href=\"http://midifoot/\">WifiManager</a> in 60 seconds.<br />\n"
        "Please connect to wireless:<br />\n"
        "SSID: " + String(WIFI_DEFAULT_SSID) + "<br />\n"
        "Password: " + String(WIFI_DEFAULT_PASSWORD) + "<br /><br />\n"
        "If the redirect does not work, <a href=\"http://midifoot/\">click here</a><br />\n"
        "</body></html>";
      log("Redirecting to WifiManager");
      wclient.println(reply);

      delay(1000);
      wclient.stop();
      delay(1000);
      wifi_server.stop();
      delay(1000);

      WiFi.disconnect();
      delay(1000);
      ESP.reset();
      return;
    } else {
      reply = "HTTP/1.1 404 Not Found\n"
        "Content-Type: text/html\n"
        "Connection: close\n"
        "\n"
        "<!DOCTYPE HTML>\n"
        "<html>\n"
        "Could not fulfill your request:<br />\n"
        + request +
        "<br />\n";
      log("4-oh-4");
    }

    reply += "</html>\n";
   
    wclient.println(reply);
    wclient.stop();
    delay(1);
   
  }
}
#endif // ARDUINO_ESP8266_NODEMCU

void ButtonUpdateState(int button) {
  if (button < 1 || button > NBUTTONS) {
    log("Button " + String(button) + " is not valid. NBUTTONS=" + NBUTTONS + ".");
    return;
  }
  log("Button " + String(button) + " pushed.");
  if (buttonMode[button-1] == BUTTONMODE_CH) {
    midi_cc = MIDI_CC_BUTTON1 + buttonModeVal[button-1] - 1 + (bank*nbuttonmode_ch);
    midi_pc = button + (bank*nbuttonmode_ch);
  } else if (buttonMode[button-1] == BUTTONMODE_UP) {
    midi_cc += 1;
    midi_pc += 1;
  } else if (buttonMode[button-1] == BUTTONMODE_DOWN) {
    midi_cc -= 1;
    midi_pc -= 1;
  } else if (buttonMode[button-1] == BUTTONMODE_BANKUP) {
      bank += 1;
  } else if (buttonMode[button-1] == BUTTONMODE_BANKDOWN) {
      bank -= 1;
  }
  log("Updated button state.");
}

void SanitizeState() {
  log("Sanitizing button state.");
  if (nbuttonmode_ch < 1)
    nbuttonmode_ch = 1;
  if (midi_cc < MIDI_MIN)
    midi_cc = MIDI_MAX;
  else if (midi_cc > MIDI_MAX)
    midi_cc = MIDI_MIN;

  if (midi_pc < MIDI_MIN)
    midi_pc = MIDI_MAX;
  else if (midi_pc > MIDI_MAX)
    midi_pc = MIDI_MIN;

  if (bank > (MIDI_MAX-MIDI_MIN+1)/nbuttonmode_ch)
    bank = 0;
  if (bank < 0)
    bank = (MIDI_MAX-MIDI_MIN+1)/nbuttonmode_ch;
  log("Sanitized button state.");
}

void MIDISendState() {
  if (midi_mode == MIDI_MODE_CC) {
    log("Sending MIDI: CC:" + String(midi_cc) + " CV:" + String(RANDALL_CC_CV) + " CH:" + String(midi_channel) + " .");
//    MIDI.sendControlChange(midi_cc, RANDALL_CC_CV, midi_channel);
  } else if (midi_mode == MIDI_MODE_PC) {
    log("Sending MIDI: PC:" + String(midi_pc) + " CH:" + String(midi_channel) + " .");
//    MIDI.sendProgramChange(midi_pc, midi_channel);
  }
}

void ButtonPress(int button) {
  ButtonUpdateState(button);
  SanitizeState();
  MIDISendState();
}

void HandleButtons() {
  static int pinVal[] = {HIGH,HIGH,HIGH,HIGH};

  for (int i=0; i<NBUTTONS; i++) {
    int pinValNew = digitalRead(buttonPin[i]);
    if (pinVal[i] != pinValNew) {
      pinVal[i] = pinValNew;
      ButtonPress(i+1);
      blink(i+1, LED_BLINK_DURATION);
//      delay(500);
      break;
    }
  }
}

void loop() {
  HandleButtons();
#ifdef ARDUINO_ESP8266_NODEMCU
  HandleHttpRequest();
#endif // ARDUINO_ESP8266_NODEMCU

  //MIDI.read();
}

