#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
  #error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

#define BT_DISCOVER_TIME 10000

BTAddress kb_address;
bool kb_found = false;

// https://www.ampedrftech.com/datasheets/cod_definition.pdf
#define KEYBOARD_COD 0x002540

void btAdvertisedDeviceFound(BTAdvertisedDevice* pDevice) {
  if (pDevice->getCOD() == KEYBOARD_COD) {
    kb_address = pDevice->getAddress();
    kb_found = true;
    Serial.printf("Found keyboard: %s\n", pDevice->toString().c_str());
  }

}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("NeXT Keyboard Emulator"); //Bluetooth device name

  if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
    while (!kb_found) {
      Serial.println("Searching for keyboard in pairing mode...");
      delay(500);
    }
    SerialBT.discoverAsyncStop();
  } else {
    Serial.println("Error: discoverAsync failed");
  }

  SerialBT.disableSSP();
  bool connected = SerialBT.connect(kb_address);
  Serial.printf("connected=%d", connected);
  while(!SerialBT.connected(10000)) {
    Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app.");
    delay(500);
  }
  SerialBT.requestRemoteName((uint8_t*)kb_address.getNative());
  char rmt_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
  SerialBT.readRemoteName((char *)&rmt_name);
  Serial.printf("Name: %s\n", &rmt_name);
}

void loop() {
  delay(100);
}
