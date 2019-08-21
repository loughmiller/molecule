#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <math.h>
#define ARM_MATH_CM4
#include <arm_math.h>
#include <algorithm>    // std::sort

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
#define MODE_SPECTRUM 3

#define SATURATION 244
#define VALUE 150


// FUNCTION DECS
void displayCount();
void displayChase();
void displayShape();
void displaySpectrum();

// GLOBALS
CRGB leds[NUM_LEDS];
uint_fast8_t currentMode = 2;
uint8_t hueOffset = 0;
uint_fast32_t loops = 0;
uint_fast32_t setupTime = 0;
uint_fast32_t logTime = 0;
uint_fast32_t currentTime = 0;
uint_fast16_t maxX = 0;
uint_fast16_t minX = -1;

Segment * vertical1;
Segment * diagonal1;
Circle * circle;

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE DETECTION CONSTANTS
const uint_fast16_t fftSize{256};               // Size of the FFT.  Realistically can only be at most 256
const uint_fast16_t fftBinSize{8};              // Hz per FFT bin  -  sample rate is fftSize * fftBinSize
const uint_fast16_t sampleCount{fftSize * 2};   // Complex FFT functions require a coefficient for the imaginary part of the
                                                // input.  This makes the sample array 2x the fftSize
const float middleA{440.0};                     // frequency of middle A.  Needed for freqeuncy to note conversion
const uint_fast16_t sampleIntervalMs{1000000 / (fftSize * fftBinSize)};  // how often to get a sample, needed for IntervalTimer

// FREQUENCY TO NOTE CONSTANTS - CALCULATE HERE: https://docs.google.com/spreadsheets/d/1CPcxGFB7Lm6xJ8CePfCF0qXQEZuhQ-nI1TC4PAiAd80/edit?usp=sharing
const uint_fast16_t noteCount{40};              // how many notes are we trying to detect
const uint_fast16_t notesBelowMiddleA{30};

// NOTE DETECTION GLOBALS
float samples[sampleCount*2];
uint_fast16_t sampleCounter = 0;
float sampleBuffer[sampleCount];
float magnitudes[fftSize];
float noteMagnatudes[noteCount];
arm_cfft_radix4_instance_f32 fft_inst;
IntervalTimer samplingTimer;

// NOTE DETECTION FUNCTIONS
void noteDetectionSetup();        // run this once during setup
void noteDetectionLoop();         // run this once per loop
void samplingCallback();

////////////////////////////////////////////////////////////////////////////////////////////////
// \ NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

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

  noteDetectionSetup();

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

  minX = max(minX, vertical1->getMinXLED());
  minX = max(minX, circle->getMinXLED());
  maxX = max(maxX, vertical1->getMaxXLED());
  maxX = max(maxX, circle->getMaxXLED());

  // FOUR SECOND ALL-GREEN ALL-CLEAR
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

  noteDetectionLoop();

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
    case MODE_SPECTRUM: displaySpectrum();
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
  if ((int_fast16_t)(x/1.9 + y/1.9) % 256 == hueOffset) {
    return CHSV(0, 0, 255);
  }

  return CHSV((hueOffset + y) % 256, SATURATION, VALUE);
}

void displayShape() {
  vertical1->display(leds, rainbow);
  // diagonal1->display(leds, rainbow);
  circle->display(leds, rainbow);

  hueOffset = (currentTime / 20) % 256;
}


float threshold = 1000;
float peak = 2000;

CHSV noteIntensity(int_fast16_t x, int_fast16_t y) {
  uint_fast16_t note = ((float)(x - minX) / (float)(maxX - minX)) * noteCount;

  if (magnitudes[note] < threshold) {
    return CHSV(0, 0, 0);
  }

  uint_fast16_t value = min(((magnitudes[note] - threshold) / (peak - threshold)) * 255, 255);
  return CHSV(((y * 3) + (currentTime / 20)) % 256, SATURATION, value);                         // 3 = more spread of color top to bottom
                                                                                                // time / 20 to slow down hue change
}

void displaySpectrum() {
  float sorted[noteCount];
  memcpy(sorted, magnitudes, sizeof(magnitudes[0]) * noteCount);
  std::sort(sorted, sorted+sizeof(sorted)/sizeof(sorted[0]));

  float cutoffMagnitude = sorted[(uint_fast16_t)(0.15 * noteCount)];  // 0.15% of LEDs should be lit on average
  float peakMagnitude = sorted[noteCount - 2];
  threshold = (threshold * (0.998)) + (cutoffMagnitude/500.0);
  peak = (peak * (0.998)) + (peakMagnitude/500.0);

  vertical1->display(leds, noteIntensity);
  circle->display(leds, noteIntensity);
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

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////

void noteDetectionSetup() {
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(10);
  analogReadAveraging(16);
  arm_cfft_radix4_init_f32(&fft_inst, fftSize, 0, 1);
  samplingTimer.begin(samplingCallback, sampleIntervalMs);
}

void noteDetectionLoop() {
  // copy the last N samples into a buffer
  memcpy(sampleBuffer, samples + (sampleCounter + 1), sizeof(float) * sampleCount);

  // FFT magic
  arm_cfft_radix4_f32(&fft_inst, sampleBuffer);
  arm_cmplx_mag_f32(sampleBuffer, magnitudes, fftSize);

  for (uint_fast16_t i=0; i<noteCount; i++) {
    noteMagnatudes[i] = 0;
  }

  for (uint_fast16_t i=1; i<fftSize/2; i++) {  // ignore top half of the FFT results
    float frequency = i * (fftBinSize);
    uint_fast16_t note = roundf(12 * (log(frequency / middleA) / log(2))) + notesBelowMiddleA;

    if (note < 0) {
      continue;
    }

    note = note % noteCount;
    noteMagnatudes[note] = max(noteMagnatudes[note], magnitudes[i]);
  }
}

void samplingCallback() {
  // Read from the ADC and store the sample data
  float sampleData = (float)analogRead(AUDIO_INPUT_PIN);

  // storing the data twice in the ring buffer array allows us to do a single memcopy
  // to get an ordered buffer of the last N samples
  uint_fast16_t sampleIndex = (sampleCounter) * 2;
  uint_fast16_t sampleIndex2 = sampleIndex + sampleCount;
  samples[sampleIndex] = sampleData;
  samples[sampleIndex2] = sampleData;

  // Complex FFT functions require a coefficient for the imaginary part of the
  // input.  Since we only have real data, set this coefficient to zero.
  samples[sampleIndex+1] = 0.0;
  samples[sampleIndex2+1] = 0.0;

  sampleCounter++;
  sampleCounter = sampleCounter % fftSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// \ NOTE DETECTION
////////////////////////////////////////////////////////////////////////////////////////////////
