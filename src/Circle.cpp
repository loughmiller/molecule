#include "Circle.h"

Circle::Circle(float radius,
               uint_fast16_t startDegrees,
               uint_fast16_t ledOffset,
               uint_fast16_t ledCount) {
  this->radius = radius;
  this->startOffsetRadians = startDegrees * RADIANS_PER_DEGREE;
  this->ledOffset = ledOffset;
  this->ledCount = ledCount;
}

void Circle::setPosition(float x, float y) {
  this->x = x;
  this->y = y;

  // precalculate x and y positions of each LED for speed!!
  float radianDelta = (2.0 * PI) / this->ledCount;
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    float radians = radianDelta * i + this->startOffsetRadians;
    int_fast16_t x = (int_fast16_t) ((this->x + (this->radius * cos(radians))) * LEDS_PER_INCH);
    int_fast16_t y = (int_fast16_t) ((this->y + (this->radius * sin(radians))) * LEDS_PER_INCH);
    this->xPositions[i] = x;
    this->yPositions[i] = y;
  }

}

void Circle::display(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y)) {
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    leds[this->ledOffset + i] = getColor(this->xPositions[i], this->yPositions[i]);
  }
}

float Circle::getX() {
  return this->x;
}

float Circle::getY() {
  return this->y;
}

float Circle::getRadius() {
  return this->radius;
}

uint_fast16_t Circle::getMaxXLED() {
  uint_fast16_t maxX = 0;
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    maxX = max(maxX, this->xPositions[i]);
  }

  return maxX;
}

uint_fast16_t Circle::getMinXLED() {
  uint_fast16_t minX = -1;
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    minX = min(minX, this->xPositions[i]);
  }

  return minX;
}