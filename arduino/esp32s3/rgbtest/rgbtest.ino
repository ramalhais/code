#include <Adafruit_NeoPixel.h>

#define N_PIXELS 1
#define RGB_PIN 48
#define PIXEL_FORMAT NEO_GRB+NEO_KHZ800
Adafruit_NeoPixel pixels(N_PIXELS, RGB_PIN, PIXEL_FORMAT);
uint8_t r,g,b;
hw_timer_t *timer1;

#define TIMER1 1
void setup() {
  Serial.begin(115200);
  Serial.println("Starting sketch");
  pixels.begin();
//  timer1 = timerBegin(TIMER1, uint16_t divider, bool countUp);
}

#define DELAYVAL 500
void loop() {
  r=(r+1)%16;
  g=(g+1)%16;
  b=(b+1)%16;

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();

  delay(50);
}
