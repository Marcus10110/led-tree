#include <Adafruit_NeoPixel.h>

#define LED_PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, LED_PIN, NEO_RGB + NEO_KHZ400);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  strip.begin();
  strip.show();
  strip.setPixelColor(0, 10, 0, 0);
  strip.show();
}

uint8_t ReadBlocking()
{
  while(!Serial.available());
  return Serial.read();
}

void loop() {
  if(ReadBlocking() != 'M') return;
  if(ReadBlocking() != 'G') return;
  uint8_t length = ReadBlocking();
  strip.clear();
  for(uint8_t i = 0; i < length; ++i)
  {
    uint8_t r, g, b;
    r = ReadBlocking();
    g = ReadBlocking();
    b = ReadBlocking();
    strip.setPixelColor(i, r, g, b);
  }
  if(ReadBlocking() != '!') return;
  strip.show();
}
