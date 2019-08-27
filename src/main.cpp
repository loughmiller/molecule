#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <math.h>
#define ARM_MATH_CM4
#include <arm_math.h>
#include <algorithm>    // std::sort

#include "Segment.h"
#include "Circle.h"

#define NUM_LEDS 2550
#define DISPLAY_LED_PIN 22
#define AUDIO_INPUT_PIN A14
#define CONTROL_MODE 17
#define CONTROL_UP 16
#define CONTROL_DOWN 15

#define MODES 5
#define MODE_COUNT 0
#define MODE_CHASE 1
#define MODE_SHAPE 2
#define MODE_SPECTRUM 3
#define MODE_GRID 4

#define SATURATION 244
#define VALUE 255

#define LEVEL 0
#define NUM_CIRCLES 4
#define NUM_SEGMENTS 30

// FUNCTION DECS
void displayCount();
void displayChase();
void displayShape();
void displaySpectrum();
void displayGrid();

void displayAll(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y));
// GLOBALS
CRGB leds[NUM_LEDS];
uint_fast8_t currentMode = MODE_SHAPE;
// uint_fast8_t currentMode = MODE_CHASE;
uint8_t hueOffset = 0;
uint_fast32_t loops = 0;
uint_fast32_t setupTime = 0;
uint_fast32_t logTime = 0;
uint_fast32_t currentTime = 0;
uint_fast16_t maxX = 0;
uint_fast16_t minX = -1;

Segment * segment[NUM_SEGMENTS];
Segment * virtualSegment[100];
Circle * circle[NUM_CIRCLES];

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


  // segment[0] = new Segment(19, 1688, 71);
  // segment[0]->setPosition(4, 97.5, 0 + LEVEL);  // RELATIVE TO CIRCLE 0
  // virtualSegment[0] = new Segment(1.5, 0, 0);
  // virtualSegment[0]->setRelativePosition(segment[0], 90);
  // segment[1] = new Segment(19, 1525, 71);
  // segment[1]->setRelativePosition(virtualSegment[0], 90);

  // segment[2] = new Segment(23, 1760, 85);
  // segment[2]->setRelativePosition(segment[0], 300);
  // virtualSegment[1] = new Segment(1.75, 0, 0);
  // virtualSegment[1]->setRelativePosition(segment[2], 120);
  // segment[3] = new Segment(22, 2206, 82);
  // segment[3]->setRelativePosition(virtualSegment[1], 60);

  // segment[4] = new Segment(24, 1845, 88);
  // segment[4]->setRelativePosition(segment[2], 60);
  // virtualSegment[2] = new Segment(1.75, 0, 0);
  // virtualSegment[2]->setRelativePosition(segment[4], 120);
  // segment[5] = new Segment(22, 2124, 82);
  // segment[5]->setRelativePosition(virtualSegment[2], 60);

  // segment[6] = new Segment(24, 1933, 86);
  // segment[6]->setRelativePosition(segment[4], 60);
  // virtualSegment[3] = new Segment(1.75, 0, 0);
  // virtualSegment[3]->setRelativePosition(segment[6], 120);
  // segment[7] = new Segment(22, 2042, 81);
  // segment[7]->setRelativePosition(virtualSegment[3], 60);

  // segment[8] = new Segment(18.5, 417, 67);
  // segment[8]->setRelativePosition(segment[6], 282);
  // virtualSegment[4] = new Segment(2, 0, 0);
  // virtualSegment[4]->setRelativePosition(segment[6], 0);
  // segment[9] = new Segment(17, 10, 62);
  // segment[9]->setRelativePosition(virtualSegment[4], 282);

  // segment[10] = new Segment(22.25, 2288, 82);
  // segment[10]->setRelativePosition(segment[3], 300);
  // virtualSegment[5] = new Segment(1.75, 0, 0);
  // virtualSegment[5]->setRelativePosition(segment[10], 300);
  // segment[11] = new Segment(24, 1440, 82);
  // segment[11]->setRelativePosition(virtualSegment[5], 240);

  // segment[12] = new Segment(22.25, 2370, 82);
  // segment[12]->setRelativePosition(segment[10], 300);
  // virtualSegment[6] = new Segment(1.75, 0, 0);
  // virtualSegment[6]->setRelativePosition(segment[12], 120);
  // segment[13] = new Segment(24, 1353, 86);
  // segment[13]->setRelativePosition(virtualSegment[6], 60);

  // // VIRTUAL TOUR THROUGH THE BOTTOM CIRCLE
  // virtualSegment[7] = new Segment(4.75, 0, 0);
  // virtualSegment[7]->setRelativePosition(segment[8], 0);
  // virtualSegment[8] = new Segment(4.75, 0, 0);
  // virtualSegment[8]->setRelativePosition(virtualSegment[7], 72);
  // segment[14] = new Segment(19, 544, 70);
  // segment[14]->setRelativePosition(virtualSegment[8], 0);
  // virtualSegment[9] = new Segment(1.75, 0, 0);
  // virtualSegment[9]->setRelativePosition(segment[14], 144);
  // segment[15] = new Segment(17.5, 544, 70);
  // segment[15]->setRelativePosition(virtualSegment[9], 36);
  
  // // SETUP SEGMENTS
  circle[0] = new Circle(4.5, 324, 1597, 91, 1);
  circle[0]->setPosition(10, 100);
  segment[0] = new Segment(23, 1689, 69);
  segment[0]->setPosition(14, 100, 326, false);
  segment[1] = new Segment(23, 1525, 72);
  segment[1]->setRelativePosition(segment[0], 180, false);

  // HEXAGON (INSIDE FIRST / OUTSIDE SECOND)
  segment[2] = new Segment(23, 2288, 84);
  segment[2]->setRelativePosition(segment[0], 60, false);
  segment[3] = new Segment(23, 1528 - 89, 85);
  segment[3]->setRelativePosition(segment[2], 180, false);

  segment[4] = new Segment(23, 2288 + 84, 82);
  segment[4]->setRelativePosition(segment[2], 300, false);
  segment[5] = new Segment(23, 1353, 87);
  segment[5]->setRelativePosition(segment[4], 180, false);

  // SPLIT INNER SIDE
  segment[6] = new Segment(21, 2288 + 84 + 82, 82);
  segment[6]->setRelativePosition(segment[4], 300, false);
  segment[7] = new Segment(2, 2034, 8);
  segment[7]->setRelativePosition(segment[6], 0, false);

  segment[8] = new Segment(23, 2042, 82);
  segment[8]->setRelativePosition(segment[7], 300, false);
  segment[9] = new Segment(23, 1933, 87);
  segment[9]->setRelativePosition(segment[8], 180, false);

  segment[10] = new Segment(23, 2124, 82);
  segment[10]->setRelativePosition(segment[8], 300, false);
  segment[11] = new Segment(23, 1846, 88);
  segment[11]->setRelativePosition(segment[10], 180, false);

  segment[12] = new Segment(23, 2206, 82);
  segment[12]->setRelativePosition(segment[10], 300, false);
  segment[13] = new Segment(23, 1758, 88);
  segment[13]->setRelativePosition(segment[12], 180, false);


  // PENTAGON - STARTS WITH SPLIT MIDDLE
  segment[14] = new Segment(21, 330, 82);
  segment[14]->setRelativePosition(segment[4], 300, false);
  segment[15] = new Segment(2, 0, 8);
  segment[15]->setRelativePosition(segment[14], 0, false);

  segment[16] = new Segment(17.5, 8, 66);
  segment[16]->setRelativePosition(segment[14], 72, false);
  segment[17] = new Segment(17.5, 417, 67);
  segment[17]->setRelativePosition(segment[16], 180, true);

  // VIRTUAL EXTENTION THROUGH CIRCLE
  virtualSegment[0] = new Segment(4.5, 0, 0);
  virtualSegment[0]->setRelativePosition(segment[16], 0, false);

  // CIRCLE
  circle[2] = new Circle(4.5, 162, 74, 30, 0.2);
  circle[2]->setPosition(virtualSegment[0]->getEndX(), virtualSegment[0]->getEndY());
  circle[3] = new Circle(4.5, 162, 484, 58, 0.8);
  circle[3]->setPosition(virtualSegment[0]->getEndX(), virtualSegment[0]->getEndY());

  // VIRTUAL EXTENTION THROUGH CIRCLE
  virtualSegment[1] = new Segment(4.5, 0, 0);
  virtualSegment[1]->setRelativePosition(virtualSegment[0], 72, false);

  segment[18] = new Segment(17.5, 104, 65);
  segment[18]->setRelativePosition(virtualSegment[1], 0, false);
  segment[19] = new Segment(17.5, 544, 69);
  segment[19]->setRelativePosition(segment[18], 180, true);

  segment[20] = new Segment(23, 169, 81);
  segment[20]->setRelativePosition(segment[18], 72, false);
  segment[21] = new Segment(23, 614, 83);
  segment[21]->setRelativePosition(segment[20], 180, true);

  segment[22] = new Segment(23, 250, 81);
  segment[22]->setRelativePosition(segment[20], 72, false);
  segment[23] = new Segment(23, 1268, 86);
  segment[23]->setRelativePosition(segment[22], 180, true);

  // RIGHT ARM
  segment[24] = new Segment(23, 697, 83);
  segment[24]->setRelativePosition(segment[20], 288, false);
  segment[25] = new Segment(23, 1180, 88);
  segment[25]->setRelativePosition(segment[24], 180, false);

  segment[26] = new Segment(23, 780, 85);
  segment[26]->setRelativePosition(segment[24], 288, false);
  segment[27] = new Segment(23, 1096, 87);
  segment[27]->setRelativePosition(segment[26], 180, false);

  segment[28] = new Segment(18.5, 865, 71);
  segment[28]->setRelativePosition(segment[26], 72, false);
  segment[29] = new Segment(18.5, 1028, 68);
  segment[29]->setRelativePosition(segment[28], 180, false);

  virtualSegment[2] = new Segment(4.5, 0, 0);
  virtualSegment[2]->setRelativePosition(segment[28], 0, false);

  circle[1] = new Circle(4.5, 252, 865 + 71, 91, 1);
  circle[1]->setPosition(virtualSegment[2]->getEndX(), virtualSegment[2]->getEndY());


  delay(3000);
  for (uint_fast16_t i = 0; i < NUM_SEGMENTS; i++) {
    Serial.print(i);
    Serial.print("\t");
    Serial.print(segment[i]->getStartX());
    Serial.print(", ");
    Serial.print(segment[i]->getStartY());
    Serial.print("\t");
    Serial.print(segment[i]->getEndX());
    Serial.print(", ");
    Serial.print(segment[i]->getEndY());
    Serial.println("");
  }


  // minX = min(minX, vertical1->getMinXLED());
  // minX = min(minX, circle[0]->getMinXLED());
  // maxX = max(maxX, vertical1->getMaxXLED());
  // maxX = max(maxX, circle[0]->getMaxXLED());

  // FOUR SECOND ALL-GREEN ALL-CLEAR
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(90, 255, 60);
  }
  FastLED.show();
  delay(300);

  Serial.print(minX);
  Serial.print("\t");
  Serial.println(maxX);

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
  uint_fast16_t modeRead = touchRead(CONTROL_MODE);

  if (currentTime > logTime + 10000) {
    logTime = currentTime;
    // Serial.println(modeRead);

    Serial.print("Frame Rate: ");
    Serial.println(loops / ((currentTime - setupTime) / 1000));
  }

  // uint_fast16_t modeRead = touchRead(CONTROL_MODE);
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
    case MODE_GRID: displayGrid();
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
  return CHSV(0, 0, 64);
}

CHSV rainbow(int_fast16_t x, int_fast16_t y) {
  // if ((int_fast16_t)(x) % 256 == hueOffset) {
  //   return CHSV(0, 0, VALUE);
  // }

  return CHSV((x + y + hueOffset) % 256 , SATURATION, VALUE);
}

CHSV grid(int_fast16_t x, int_fast16_t y) {
  if (x % 20 == 0 || y % 20 == 0) {
    return CHSV(0, 0, VALUE);
  }

  return rainbow(x, y);
}

void displayGrid() {
  displayAll(leds, grid);
  hueOffset = (currentTime / 20) % 256;
}

void displayShape() {
  displayAll(leds, rainbow);
  hueOffset = (currentTime / 20) % 256;
}


float threshold = 1000;
float peak = 2000;

CHSV noteIntensity(int_fast16_t x, int_fast16_t y) {
  uint_fast16_t note = ((float)(x - minX) / (float)(maxX - minX)) * noteCount;

  if (noteMagnatudes[note] < threshold) {
    return CHSV(0, 0, 0);
  }

  uint_fast16_t value = min(((noteMagnatudes[note] - threshold) / (peak - threshold)) * 255, 255);
  return CHSV(((x * 3) + (currentTime / 20)) % 256, SATURATION, value);                         // 3 = more spread of color top to bottom
                                                                                                // time / 20 to slow down hue change
}

void displaySpectrum() {
  noteDetectionLoop();
  float sorted[noteCount];
  memcpy(sorted, noteMagnatudes, sizeof(noteMagnatudes[0]) * noteCount);
  std::sort(sorted, sorted+sizeof(sorted)/sizeof(sorted[0]));

  float cutoffMagnitude = sorted[(uint_fast16_t)(0.85 * noteCount)];  // 0.15% of LEDs should be lit on average
  float peakMagnitude = sorted[noteCount - 2];
  threshold = (threshold * (0.998)) + (cutoffMagnitude/500.0);
  peak = (peak * (0.998)) + (peakMagnitude/500.0);

  displayAll(leds, noteIntensity);
}

void displayChase() {
  uint8_t hue = hueOffset;

  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    uint_fast8_t r = random(256);
    if (r == 0) {
      leds[i] = CHSV(0, 0, 255);
    } else if (r == 1) {
      leds[i] = CHSV(0, 0, 0);
    } else {
      hue = (hue + 1) % 256;
      leds[i] = CHSV(hue, SATURATION, VALUE);
    }
  }
  hueOffset = (hueOffset + 7) % 256;
}

void displayCount() {
  // alternating colors every 10 LEDs
  for (uint_fast16_t i = 0; i < NUM_LEDS; i++) {
    uint_fast8_t hue = floor(i / 10);
    hue = (hue % 2) * 64;
    leds[i] = CHSV(hue, SATURATION, VALUE);

    // if (i % 100 == 0) {
    //   leds[i] = CHSV(128, SATURATION, VALUE);
    // }

    // ANDERS: copy pasting these lines will allow you to set any pixel to designated hue

    // leds[1596] = CHSV(0, 0, 255);  // THIS SETS PIXEL 2000 TO hue 92, OVERRIDING THE 100th PIXEL COLOR
    // left circle[0] start
    leds[1597] = CHSV(0, 0, 255);  // THIS SETS PIXEL 2000 TO hue 92, OVERRIDING THE 100th PIXEL COLOR
    // left circle[0] end
    leds[1687] = CHSV(0, 0, 255);  // THIS SETS PIXEL 2000 TO hue 92, OVERRIDING THE 100th PIXEL COLOR
  }
}


void displayAll(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y)) {
  for (uint_fast16_t i = 0; i < NUM_CIRCLES; i++) {
    circle[i]->display(leds, getColor);
  }

  for (uint_fast16_t i = 0; i < NUM_SEGMENTS; i++) {
    segment[i]->display(leds, getColor);
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
