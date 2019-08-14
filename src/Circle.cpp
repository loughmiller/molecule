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
}

void Circle::display(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y)) {
  float radianDelta = (2.0 * PI) / ledCount;
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    float radians = radianDelta * i + this->startOffsetRadians;
    int_fast16_t x = (int_fast16_t) ((this->x + (this->radius * cos(radians))) * LEDS_PER_INCH);
    int_fast16_t y = (int_fast16_t) ((this->y + (this->radius * sin(radians))) * LEDS_PER_INCH);
    
    leds[this->ledOffset + i] = getColor(x, y);
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
