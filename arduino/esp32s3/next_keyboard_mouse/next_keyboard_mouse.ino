#include <Adafruit_NeoPixel.h>
#include "EspUsbHostKeybord.h"

static const char* LOG_TAG = "NEXT_KBD_MOUSE";

#define N_PIXELS 1
#define RGB_PIN 48
#define PIXEL_FORMAT NEO_GRB+NEO_KHZ800
Adafruit_NeoPixel pixels(N_PIXELS, RGB_PIN, PIXEL_FORMAT);
uint8_t r=0,g=0,b=16;
hw_timer_t *timer1;

class MyEspUsbHostKeybord : public EspUsbHostKeybord {
public:
  void onKey(usb_transfer_t *transfer) {
    uint8_t *const p = transfer->data_buffer;
    Serial.printf("onKey %02x %02x %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
  };
};

MyEspUsbHostKeybord usbHost;

#define TIMER1 1
void setup() {
  Serial.begin(115200);
  Serial.println("Starting sketch");

  ESP_LOGI(LOG_TAG, "INFO");
  ESP_LOGD(LOG_TAG, "DEBUG");

  pixels.begin();
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
//  timer1 = timerBegin(TIMER1, uint16_t divider, bool countUp);

  usbHost.begin();
}

void loop() {
  usbHost.task();
}
