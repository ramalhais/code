#include <BleKeyboard.h> // https://github.com/T-vK/ESP32-BLE-Keyboard

BleKeyboard bleKeyboard("BlueKeyTest MacSection", "Pedro Ramalhais", 100);

void setup() {
  bleKeyboard.begin();
}

void loop() {
  delay(10000);
  for (int i=210; i<256; i++) {
    char data[256];
    sprintf ( (char*)data,   "code=%d\n",   i);
    bleKeyboard.print(data);
    bleKeyboard.press(i);
    bleKeyboard.release(i);
    bleKeyboard.press(i);
    bleKeyboard.release(i);
    bleKeyboard.print("code end\n");
    delay(2000);
  }
}
