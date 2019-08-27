#include <Arduino.h>
#include <FastLED.h>

#define RADIANS_PER_DEGREE 0.0174533
#define LEDS_PER_INCH 3.65
// #define PI 3.1415

class Circle {
  private :
    float x;
    float y;
    float radius;
    float startOffsetRadians;
    uint_fast16_t ledCount;
    uint_fast16_t ledOffset;
    float arcPercent;
    uint_fast16_t xPositions[200];
    uint_fast16_t yPositions[200];

  public :
    Circle(float radius,
               uint_fast16_t startDegrees,
               uint_fast16_t ledOffset,
               uint_fast16_t ledCount,
               float arcPercent);
    void setPosition(float x, float y);
    void display(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y));
    float getX();
    float getY();
    float getRadius();
    uint_fast16_t getMaxXLED();
    uint_fast16_t getMinXLED();
};
