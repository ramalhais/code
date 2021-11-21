// BikeMon for ESP32

#include "BluetoothSerial.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

#define LED_ON LOW
#define LED_OFF HIGH

// wideband APSX D1
// 5V use voltage divider 1k/2k or 2.2k/3.3k
#define PIN_ANALOG_AFR      23
//
#define PIN_NARROWBAND_AFR  4
#define PIN_RX_DIGITAL_AFR  16
#define PIN_TX_SACRIFICE    17

BluetoothSerial SerialBT;
typedef struct {
  int cycle_ms;
  int last_cycle_ms;
  int type;
  char *name;
  bool enabled;
} observations;

void setup() {
  setDefaults();
  //Serial.begin(115200);
  Serial.begin(9600);
//  Serial.println("Serial Txd is on pin: "+String(TX));
//  Serial.println("Serial Rxd is on pin: "+String(RX));
  Serial2.begin(9600, SERIAL_8N1, PIN_RX_DIGITAL_AFR, PIN_TX_SACRIFICE, true);
  Serial2.setRxBufferSize(1);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  while(!SerialBT.begin("BikeMon", false)) {
    delay(500);
  }

  // Wait for Bluetooth client
  while(!SerialBT.hasClient()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
  }
  digitalWrite(LED_BUILTIN, LED_OFF);
}

int rpm = 1300;
byte afr = 190;
String lineEnd = "\r";
String version = "ELM327 v1.5";
String ECU_ADDR_E = "7E0";  // Engine control module ECU address
String ECU_R_ADDR_E = "7E8"; // Responses sent by Engine ECU - ECM (engine control module) 7E0/7E8
String ECU_HONDA = "48 6B 10";
bool headers;
bool spaces;
bool echo;
String protocol;

void setDefaults() {
  headers = false;
  spaces = true;
  echo = true;
  protocol = "AUTO, ISO 9141-2";
}

void btElmSend(String str, String origCmd="", String strDataBytes="", bool prompt=true) {
  String sendStr = "";

  if (echo && origCmd != "") {
    sendStr += origCmd + lineEnd;
    SerialBT.print(sendStr);
    Serial.println(sendStr);
  }

  sendStr = "";
  if (headers && strDataBytes != "") {
    //sendStr = ECU_R_ADDR_E + " " + strDataBytes + " ";
    sendStr = ECU_HONDA + " ";
    if (!spaces) {
      sendStr.replace(" ","");
    }
  }
  sendStr += str;
  if (strDataBytes != "" && !spaces) {
    sendStr.replace(" ","");
  }
  sendStr += lineEnd;
  SerialBT.print(sendStr);
  Serial.println(sendStr);

  if (prompt) {
    sendStr = ">";
    SerialBT.print(sendStr);
    Serial.println(sendStr);
  }
}

void loop() {
  if(Serial2.available() > 0) {
      afr = Serial2.read();
//      Serial.println("APSX DEC AFR: " + String(b,DEC));
//      Serial.println("APSX HEX AFR: " + String(b,HEX));
//      Serial.println("APSX BIN AFR: " + String(b,BIN));
//      Serial.println("INV: " + String(b^0xff,BIN));
//      Serial.print(char(b));
      //Serial.write(b);
//      Serial.flush();
//      SerialBT.println("APSX DEC AFR: " + String(b,DEC));
//      SerialBT.println("APSX HEX AFR: " + String(b,HEX));
  }
  if(SerialBT.available() > 0) {
    digitalWrite(LED_BUILTIN, LED_ON);
    String line = SerialBT.readStringUntil('\r');
    if (line == "") {
      String line = SerialBT.readStringUntil('\n');
    }
    if (line == "") {
      return;
    }
    line.trim();
    Serial.println("Received: '" + line + "' Length: " + line.length() + " Last Char: " + String(line[line.length()-1], HEX));
    if(line == "CMD=RESET") {
      SerialBT.println("RESET=1;");
      Serial.println("RESET=1;");
      ESP.restart();
    } else if(line == "CMD=RPM") {
      SerialBT.printf("RPM=%d;\n",rpm);
      Serial.printf("RPM=%d;\n", rpm);
    } else if(line == "CMD=AFR") {
      SerialBT.printf("AFR=%d;\n",afr);
      Serial.printf("AFR=%d;\n", afr);
    } else if(line == "ATZ") { // AT RESET: Reset and sleep 0.5 seconds
      btElmSend("\r\r" + version, line);
    } else if(line == "ATE0") { // AT ECHO: set ECHO ON/OFF
      btElmSend("OK", line);
      echo = false;
    } else if(line == "ATE1") { // AT ECHO: set ECHO ON/OFF
      btElmSend("OK", line);
      echo = true;
    } else if(line == "ATH0") { // AT HEADERS: set HEADERS ON/OFF
      headers = false;
      btElmSend("OK", line);
    } else if(line == "ATH1") { // AT HEADERS: set HEADERS ON/OFF
      headers = true;
      btElmSend("OK", line);
    } else if(line.startsWith("ATAT")) { // AT Set adaptive timing mode: Set adaptive timing
      btElmSend("OK", line);
    } else if(line == "ATPC") { // AT PROTOCOL CLOSE
      btElmSend("OK", line);
    } else if(line == "ATM0") { // AT Memory off or on: set MEMORY ON/OFF
      btElmSend("OK", line);
    } else if(line == "ATL0") { // AT LINEFEEDS: set LINEFEEDS ON/OFF
      lineEnd = "\r";
      btElmSend("OK", line);
    } else if(line == "ATL1") { // AT LINEFEEDS: set LINEFEEDS ON/OFF
      lineEnd = "\r\n";
      btElmSend("OK", line);
    } else if(line == "ATS0") { // AT Spaces off or on: set SPACES
      spaces = false;
      btElmSend("OK", line);
    } else if(line == "ATS1") { // AT Spaces off or on: set SPACES
      spaces = true;
      btElmSend("OK", line);
    } else if(line == "AT@1") { // AT Device description
      btElmSend("OBDII to RS232 Interpreter", line);
    } else if(line == "ATI") { // AT ELM327 version string
      btElmSend(version, line);
    } else if(line == "ATDP") { // AT_DESCRIBE_PROTO: AT set DESCRIBE PROTO
//      btElmSend("AUTO, ISO 15765-4 (CAN 11/500)");
//      btElmSend("ISO 9141-2", line);
      btElmSend(protocol, line);
    } else if(line == "ATDPN") { // AT_DESCRIBE_PROTO_N: AT Display Protocol Number
      btElmSend("A3", line);
    } else if(line == "ATD") { // AT DEFAULT: Set all configuration to defaults
      btElmSend("OK", line);
      setDefaults();
    } else if(line.startsWith("ATD")) { // AT_DLC: AT Display of the DLC on/off
      btElmSend("OK", line);
    } else if(line.startsWith("ATSP")) { // AT PROTO: set PROTO
      if (line[4] == '0') {
        protocol = "AUTO, ISO 9141-2";
      } else if (line[4] == '1') {
        protocol = "AUTO"; // SAE J1850 PWM
      } else if (line[4] == '2') {
        protocol = "AUTO"; // SAE J1850 VPW
      } else if (line[4] == '3') {
        protocol = "ISO 9141-2"; // ISO 9141-2
      } else if (line[4] == '4') {
        protocol = "AUTO"; // ISO 14230-4 (KWP 5BAUD)
      } else if (line[4] == '5') {
        protocol = "AUTO"; // ISO 14230-4 (KWP FAST)
      } else if (line[4] == '6') {
        protocol = "AUTO"; // ISO 15765-4 (CAN 11/500)
      } else if (line[4] == '7') {
        protocol = "AUTO"; // ISO 15765-4 (CAN 29/500)
      } else if (line[4] == '8') {
        protocol = "AUTO"; // ISO 15765-4 (CAN 11/250)
      } else if (line[4] == '9') {
        protocol = "AUTO"; // ISO 15765-4 (CAN 29/250)
      }
      
      btElmSend("OK", line);
    } else if(line.startsWith("STI")) { // ST_ID: ST Print firmware ID string
      btElmSend(version, line);
      // ELM: ?
    } else if(line == "ATAL") { // AT_LONG_MSG: AT Allow long messages
      btElmSend("OK", line);
    } else if(line.startsWith("ATST")) { // AT SET TIMEOUT
      btElmSend("OK", line);
      // ELM says OK
    } else if(line.startsWith("ATFCSH")) { // AT_FCSH: AT FLOW CONTROL SET HEADER
      btElmSend("OK", line);
    } else if(line.startsWith("ATFCSD")) { // AT_FCSD: AT FLOW CONTROL SET DATA
      btElmSend("OK", line);
    } else if(line.startsWith("ATFCSM")) { // AT_FCSM: AT Fow Control Set Mode
      btElmSend("OK", line);
    } else if(line.startsWith("0100")) { // PIDS_A: Supported PIDs [01-20]
//      btElmSend("41 01 00 07 A1 00", line, "06");
      btElmSend("41 00 BE 3E B8 11 C9", line, "06"); // Jazz
      // ELM: 486B104100BE3EB811C9
    } else if(line.startsWith("0120")) { // PIDS_B: Supported PIDs [21-40]
//      btElmSend("41 20 90 15 B0 15", line, "06");
      btElmSend("41 20 80 01 A0 01", line, "06");
    } else if(line.startsWith("010D")) { // SPEED
//      btElmSend("41 0D 0A", line, "03");
      btElmSend("41 0D "+String(afr, HEX), line, "03");
    } else if(line.startsWith("0113")) { // O2_SENSORS: O2 Sensors Present
      btElmSend("41 13 03", line, "03");
    } else if(line.startsWith("0114")) { // O2 S1 Voltage???
      btElmSend("41 14 5A 80", line, "04");
    } else if(line.startsWith("0124")) { // O2_S1_WR_VOLTAGE: 02 Sensor 1 WR Lambda Voltage
//      btElmSend("41 24 7B A8 65 D9", line, "06");
//      btElmSend("41 24 3F 80 00 00", line, "06"); //1V
      btElmSend("41 24 40 A0 00 00", line, "06"); //5V
    } else if(line.startsWith("0133")) { // BAROMETRIC_PRESSURE: Barometric Pressure
      btElmSend("41 33 61", line, "03");
    } else if(line.startsWith("010B")) { // INTAKE_PRESSURE: Intake Manifold Pressure
      btElmSend("41 0B 26", line, "03");
    } else if(line.startsWith("010C")) { // RPM: Engine RPM
      btElmSend("41 0C 14 5F", line, "04");
    } else if(line.startsWith("0902")) { // VIN: Vehicle Identification Number (18 bytes)
      String vin = "HONDAXR4001997";
      String vinHex = "";
      int lineNr = 1;
      int nBytes = 0;
      for (int i=0; i<20-vin.length(); i++) {
        vinHex += " 30";
        nBytes++;
        if (nBytes%4 == 0) {
          btElmSend("49 02 0" + String(lineNr) + vinHex, lineNr == 1 ? line : "", "", false);
          lineNr++;
          vinHex = "";
        }
      }
      for (int i=0; i<vin.length(); i++) {
        vinHex += " " + String(vin[i], HEX);
        nBytes++;
        if (nBytes%4 == 0) {
          btElmSend("49 02 0" + String(lineNr) + vinHex, "", "", lineNr == 5 ? true : false);
          lineNr++;
          vinHex = "";
        }
      }
      //30 30 30 30 30 30 48 4f 4e 44 41 58 52 34 30 30 31 39 39 37
    } else {
      btElmSend("?", line);
    }
  } else {
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  

}
