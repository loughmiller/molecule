#include "Segment.h"

Segment::Segment(float length, 
                 uint_fast16_t ledOffset, 
                 uint_fast16_t ledCount) {
  this->length = length;
  this->ledOffset = ledOffset;
  this->ledCount = ledCount;
}

void Segment::setPosition(float startX, float startY, int_fast16_t absoluteAngle) {
  this->startX = startX;
  this->startY = startY;
  this->absoluteAngleRadians = absoluteAngle * RADIANS_PER_DEGREE;
  this->endX = calcEndX();
  this->endY = calcEndY();
}

void Segment::setRelativePosition(Segment *parent, int_fast16_t relativeAngle) {
  this->startX = parent->getEndX();
  this->startY = parent->getEndY();
  this->absoluteAngleRadians = parent->getAbsoluteAngleRadians() + (relativeAngle * RADIANS_PER_DEGREE);
  this->endX = calcEndX();
  this->endY = calcEndY();
}

void Segment::display(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y)) {
  float xDelta = (this->endX - this->startX) / this->ledCount;
  float yDelta = (this->endY - this->startY) / this->ledCount;
  float x = this->startX + (xDelta / 2);
  float y = this->startY + (yDelta / 2);
  
  for (uint_fast16_t i = 0; i < this->ledCount; i++) {
    int_fast16_t xL = (int_fast16_t) (LEDS_PER_INCH * x);
    int_fast16_t yL = (int_fast16_t) (LEDS_PER_INCH * y);

    leds[this->ledOffset + i] = getColor(xL, yL);
    x += xDelta;
    y += yDelta;
  }
}

float Segment::getStartX() {
  return this->startX;
}

float Segment::getStartY() {
  return this->startY;
}

float Segment::getAbsoluteAngleRadians() {
  return this->absoluteAngleRadians;
}

float Segment::calcEndX() {
  return this->startX + (cos(this->absoluteAngleRadians) * this->length);
}

float Segment::calcEndY() {
  return this->startY + (sin(this->absoluteAngleRadians) * this->length);
}

float Segment::getEndX() {
  return this->endX;
}

float Segment::getEndY() {
  return this->endY;
}
