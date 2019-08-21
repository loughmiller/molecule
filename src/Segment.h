#include <Arduino.h>
#include <FastLED.h>

#define RADIANS_PER_DEGREE 0.0174533
#define LEDS_PER_INCH 3.65

class Segment{
  private :
    float startX;
    float startY;
    float endX;
    float endY;
    float length;
    float absoluteAngleRadians;
    uint_fast16_t ledCount;
    uint_fast16_t ledOffset;
    void validate();

  public :
    Segment(float length,
                 uint_fast16_t ledOffset,
                 uint_fast16_t ledCount);
    void setPosition(float startX, float startY, int_fast16_t absoluteAngle);
    void setRelativePosition(Segment *parent, int_fast16_t relativeAngle);
    void display(CRGB* leds, CHSV (*getColor)(int_fast16_t x, int_fast16_t y));
    float getStartX();
    float getStartY();
    float getAbsoluteAngleRadians();
    float getEndX();
    float getEndY();
    float calcEndX();
    float calcEndY();
    uint_fast16_t getMaxXLED();
    uint_fast16_t getMinXLED();
};
