#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>

// FAST LED
#define NUM_LEDS 2880
#define DISPLAY_LED_PIN 22

#define AUDIO_INPUT_PIN A14

CRGB leds[NUM_LEDS];

void setup() {
  delay(1000);

  Serial.begin(9600);
  Serial.println("setup started");

  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, 0);
  }

  FastLED.show();

  FastLED.addLeds<NEOPIXEL, DISPLAY_LED_PIN>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );;

  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, 55);
  }

  FastLED.show();
  delay(1000);

  Serial.println("setup complete");
}

void loop() {

  // alternating colors every 10 LEDs
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    uint_fast8_t hue = floor(i / 10);
    hue = (hue % 2) * 64;
    leds[i] = CHSV(hue, 255, 100);

    if (i % 100 == 0) {
      leds[i] = CHSV(128, 255, 100);
    }

    // ANDERS: copy pasting these lines will allow you to set any pixel to designated hue

    hue = 192;
    leds[55] = CHSV(hue, 255, 100);  // THIS SETS PIXEL 55 TO hue 192
    leds[1000] = CHSV(222, 255, 100);  // THIS SETS PIXEL 1000 TO hue 222, OVERRIDING THE 100th PIXEL COLOR
    leds[2000] = CHSV(92, 255, 100);  // THIS SETS PIXEL 2000 TO hue 92, OVERRIDING THE 100th PIXEL COLOR
  }

  FastLED.show();
}
