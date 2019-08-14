#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <math.h>
#include "Segment.h"
#include "Circle.h"

#define NUM_LEDS 288
#define DISPLAY_LED_PIN 22
#define AUDIO_INPUT_PIN A14
#define CONTROL_MODE 17
#define CONTROL_UP 16
#define CONTROL_DOWN 15

#define MODES 3
#define MODE_COUNT 0
#define MODE_CHASE 1
#define MODE_SHAPE 2

#define SATURATION 244
#define VALUE 100


// FUNCTION DECS
void displayCount();
void displayChase();
void displayShape();

// GLOBALS
CRGB leds[NUM_LEDS];
uint_fast8_t currentMode = 2;
uint8_t hueOffset = 0;
uint_fast32_t loops = 0;
uint_fast32_t setupTime = 0;
uint_fast32_t logTime = 0;
uint_fast32_t currentTime = 0;

Segment * vertical1;
Segment * diagonal1;
Circle * circle;

///////////////////////////////////////////////////////////////////////////////
// SETUP
///////////////////////////////////////////////////////////////////////////////
void setup() {
  delay(1000);

  Serial.begin(9600);
  Serial.println("setup started");

  pinMode(CONTROL_UP, INPUT);
  pinMode(CONTROL_DOWN, INPUT);
  pinMode(CONTROL_MODE, INPUT);

  // FastLED SETUP
  FastLED.addLeds<NEOPIXEL, DISPLAY_LED_PIN>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );;

  // TURN ALL OFF
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, 0);
  }
  FastLED.show();

  // SETUP SEGMENTS
  vertical1 = new Segment(39.4, 0, 144);
  vertical1->setPosition(40, 0, 90);

  // diagonal1 = new Segment(39.4, 144, 144);
  // diagonal1->setRelativePosition(vertical1, 135);

  circle = new Circle(6.27, 90, 144, 144);
  circle->setPosition(40, 39.4 - 6.27);

  // ONE SECOND ALL-GREEN ALL-CLEAR  
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(90, 255, 60);
  }
  FastLED.show();
  delay(4000);

  // TURN ALL OFF
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, 0);
  }
  FastLED.show();

  Serial.println("setup complete");

  // Serial.print(diagonal1->getStartX());
  // Serial.print(", ");
  // Serial.print(diagonal1->getStartY());
  // Serial.print(" - ");
  // Serial.print(diagonal1->getEndX());
  // Serial.print(", ");
  // Serial.println(diagonal1->getEndY());

  setupTime = millis();
}

void loop() {
  loops++;
  currentTime = millis();
  if (currentTime > logTime + 10000) {
    logTime = currentTime;

    Serial.print("Frame Rate: ");
    Serial.println(loops / ((currentTime - setupTime) / 1000));
  }


  uint_fast16_t modeRead = touchRead(CONTROL_MODE);
  if (modeRead > 1000) {
    currentMode = (currentMode + 1) % MODES;
  }

  switch(currentMode) {
    case MODE_COUNT: displayCount();
      break;
    case MODE_CHASE: displayChase();
      break;
    case MODE_SHAPE: displayShape();
      break;
  }

  FastLED.show();

  if (modeRead > 1000) {
    delay(1000);
  }

  // Serial.print(touchRead(15));
  // Serial.print("\t");
  // Serial.print(touchRead(16));
  // Serial.print("\t");
  // Serial.print(touchRead(17));
  // Serial.println("");
  // delay(500);
}

CHSV white(int_fast16_t x, int_fast16_t y) {
  return CHSV(0, 0, 100);
}

CHSV rainbow(int_fast16_t x, int_fast16_t y) {
  return CHSV((((hueOffset + y) / 10) * 60) % 256, SATURATION, VALUE);
}

void displayShape() {
  vertical1->display(leds, rainbow);
  // diagonal1->display(leds, rainbow);
  circle->display(leds, rainbow);

  hueOffset = (currentTime / 100) % 256;
}

void displayChase() {
  uint8_t hue = hueOffset;
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, SATURATION, VALUE);
    hue = (hue + 1) % 256;
  }
  hueOffset = (hueOffset + 1) % 256;
}

void displayCount() {
  // alternating colors every 10 LEDs
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    uint_fast8_t hue = floor(i / 10);
    hue = (hue % 2) * 64;
    leds[i] = CHSV(hue, SATURATION, VALUE);

    if (i % 100 == 0) {
      leds[i] = CHSV(128, SATURATION, VALUE);
    }

    // ANDERS: copy pasting these lines will allow you to set any pixel to designated hue

    hue = 192;
    leds[55] = CHSV(hue, 255, 100);  // THIS SETS PIXEL 55 TO hue 192
    leds[1000] = CHSV(222, 255, 100);  // THIS SETS PIXEL 1000 TO hue 222, OVERRIDING THE 100th PIXEL COLOR
    leds[2000] = CHSV(92, 255, 100);  // THIS SETS PIXEL 2000 TO hue 92, OVERRIDING THE 100th PIXEL COLOR
  }
}
