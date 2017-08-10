#include <FastLED.h>

#define LED_PIN  D2 // for ESP8266

#define COLOR_ORDER RGB
#define CHIPSET     WS2811

#define BRIGHTNESS 64

// Helper functions for an two-dimensional XY matrix of pixels.
// Simple 2-D demo code is included as well.
//
//     XY(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             No error checking is performed on the ranges of x and y.
//
//     XYsafe(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             Error checking IS performed on the ranges of x and y, and an
//             index of "-1" is returned.  Special instructions below
//             explain how to use this without having to do your own error
//             checking every time you use this function.
//             This is a slightly more advanced technique, and
//             it REQUIRES SPECIAL ADDITIONAL setup, described below.

// Params for width and height
const uint8_t kMatrixWidth = 32;
const uint8_t kMatrixHeight = 9;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
// Set 'kMatrixSerpentineLayout' to false if your pixels are
// laid out all running the same way, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//     .----<----<----<----'
//     |
//     5 >  6 >  7 >  8 >  9
//                         |
//     .----<----<----<----'
//     |
//    10 > 11 > 12 > 13 > 14
//                         |
//     .----<----<----<----'
//     |
//    15 > 16 > 17 > 18 > 19
//
// Set 'kMatrixSerpentineLayout' to true if your pixels are
// laid out back-and-forth, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//                         |
//     9 <  8 <  7 <  6 <  5
//     |
//     |
//    10 > 11 > 12 > 13 > 14
//                        |
//                        |
//    19 < 18 < 17 < 16 < 15
//
// Bonus vocabulary word: anything that goes one way
// in one row, and then backwards in the next row, and so on
// is call "boustrophedon", meaning "as the ox plows."


// This function will return the right 'led index number' for
// a given set of X and Y coordinates on your matrix.
// IT DOES NOT CHECK THE COORDINATE BOUNDARIES.
// That's up to you.  Don't pass it bogus values.
//
// Use the "XY" function like this:
//
//    for( uint8_t x = 0; x < kMatrixWidth; x++) {
//      for( uint8_t y = 0; y < kMatrixHeight; y++) {
//
//        // Here's the x, y to 'led index' in action:
//        leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
//
//      }
//    }
//
//
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}


// Once you've gotten the basics working (AND NOT UNTIL THEN!)
// here's a helpful technique that can be tricky to set up, but
// then helps you avoid the needs for sprinkling array-bound-checking
// throughout your code.
//
// It requires a careful attention to get it set up correctly, but
// can potentially make your code smaller and faster.
//
// Suppose you have an 8 x 5 matrix of 40 LEDs.  Normally, you'd
// delcare your leds array like this:
//    CRGB leds[40];
// But instead of that, declare an LED buffer with one extra pixel in
// it, "leds_plus_safety_pixel".  Then declare "leds" as a pointer to
// that array, but starting with the 2nd element (id=1) of that array:
//    CRGB leds_with_safety_pixel[41];
//    CRGB* const leds( leds_plus_safety_pixel + 1);
// Then you use the "leds" array as you normally would.
// Now "leds[0..N]" are aliases for "leds_plus_safety_pixel[1..(N+1)]",
// AND leds[-1] is now a legitimate and safe alias for leds_plus_safety_pixel[0].
// leds_plus_safety_pixel[0] aka leds[-1] is now your "safety pixel".
//
// Now instead of using the XY function above, use the one below, "XYsafe".
//
// If the X and Y values are 'in bounds', this function will return an index
// into the visible led array, same as "XY" does.
// HOWEVER -- and this is the trick -- if the X or Y values
// are out of bounds, this function will return an index of -1.
// And since leds[-1] is actually just an alias for leds_plus_safety_pixel[0],
// it's a totally safe and legal place to access.  And since the 'safety pixel'
// falls 'outside' the visible part of the LED array, anything you write
// there is hidden from view automatically.
// Thus, this line of code is totally safe, regardless of the actual size of
// your matrix:
//    leds[ XYsafe( random8(), random8() ) ] = CHSV( random8(), 255, 255);
//
// The only catch here is that while this makes it safe to read from and
// write to 'any pixel', there's really only ONE 'safety pixel'.  No matter
// what out-of-bounds coordinates you write to, you'll really be writing to
// that one safety pixel.  And if you try to READ from the safety pixel,
// you'll read whatever was written there last, reglardless of what coordinates
// were supplied.

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
//CRGB nextleds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* const leds( leds_plus_safety_pixel + 1);
//CRGB* const nextleds( nextleds_plus_safety_pixel + 1);
uint8_t last_toggle[ NUM_LEDS + 1];
uint8_t* const last_on_or_off( last_toggle + 1);
uint8_t on_toggle[ NUM_LEDS + 1];
uint8_t* const on_or_off( on_toggle + 1);
uint8_t next_toggle[ NUM_LEDS + 1];
uint8_t* const next_on_or_off( next_toggle + 1);
uint16_t const loops = 2000;
uint8_t startHue8 = 5;
uint16_t XYsafe( uint8_t x, uint8_t y)
{
  if( x >= kMatrixWidth) return -1;
  if( y >= kMatrixHeight) return -1;
  return XY(x,y);
}


// Demo that USES "XY" follows code below

void DrawNextFrame( uint8_t  startHue8, uint16_t &remain);

void loop()
{

    uint32_t ms = millis();

    /*
  //  int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / kMatrixWidth));
 //   int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / kMatrixHeight));
    Align( 200, 180, 180);
   //  leds[ XY(2, 2)]  = CHSV( 120, 255, 255);

    if( ms < 5000 ) {
      FastLED.setBrightness( scale8( BRIGHTNESS, (ms * 256) / 5000));
    } else {
      FastLED.setBrightness(BRIGHTNESS);
    }
    FastLED.show();
    delay(50);

for( byte y = 0; y < 5; y++) {

      Align_Next( 200, 180, 180);
    if( ms < 5000 ) {
      FastLED.setBrightness( scale8( BRIGHTNESS, (ms * 256) / 5000));
    } else {
      FastLED.setBrightness(BRIGHTNESS);
    }
    FastLED.show();
 delay (1000);
  }
    //*/

    DrawStartFrame( startHue8);

    if( ms < 5000 ) {
      FastLED.setBrightness( scale8( BRIGHTNESS, (ms * 256) / 5000));
    } else {
      FastLED.setBrightness(BRIGHTNESS);
    }
    FastLED.show();
    delay(600);

  for( uint16_t y = 0; y < loops; y++) {

    uint32_t ms = millis();
// DrawSte(  startHue8);
   DrawNextFrame( startHue8, y);
   //  leds[ XY(2, 2)]  = CHSV( 120, 255, 255);

    if( ms < 5000 ) {
      FastLED.setBrightness( scale8( BRIGHTNESS, (ms * 256) / 5000));
    } else {
      FastLED.setBrightness(BRIGHTNESS);
      if ( y < 10 ) FastLED.setBrightness(BRIGHTNESS / 2);
      if ( y < 5 ) FastLED.setBrightness(BRIGHTNESS / 4);
      }
    FastLED.show();
    delay (70);
    startHue8 += 5;
  }
}


/*
void Align( uint8_t startHue8, uint8_t yHueDelta8, uint8_t xHueDelta8)
{

  for(  byte y = 0; y < kMatrixHeight; y++) {
    for(  byte x = 0; x < kMatrixWidth; x++) {
       on_or_off[ XY(x, y)] = 0;
    }
  }
  for( byte y = 0; y < kMatrixHeight; y++) {
   on_or_off[ XY(0, y)]  = random(2);
   leds[ XY(0, y)]  = CHSV( 200, 255, 255*on_or_off[ XY(0, y)]);
  //nextleds [ XY(kMatrixWidth-1, y)]  = leds[ XY(0, y)];
   next_on_or_off[ XY(kMatrixWidth-1, y)] = on_or_off[ XY(0, y)];
   for( byte x = 5; x < kMatrixWidth; x+=5) {
     on_or_off[ XY(x, y)]  = 1;
     leds[ XY(x, y)]  = CHSV( 120, 180, 180*on_or_off[ XY(x, y)]);
     next_on_or_off[ XY(x-1, y)] = on_or_off[ XY(x, y)];
    }
  }
}

void Align_Next( uint8_t startHue8, uint8_t yHueDelta8, uint8_t xHueDelta8)
{

  for( byte y = 0; y < kMatrixHeight; y++) {
   //on_or_off[ XY(0, y)]  = random(2);
   leds[ XY(0, y)]  = CHSV( 120, 255, 255*on_or_off[ XY(0, y)]);
   next_on_or_off[ XY(kMatrixWidth-1, y)] = on_or_off[ XY(0, y)];
   for( byte x = 1; x < kMatrixWidth; x++) {
     //on_or_off[ XY(x, y)]  = 1;
     leds[ XY(x, y)]  = CHSV( 120, 180, 180*on_or_off[ XY(x, y)]);
     next_on_or_off[ XY(x-1, y)] = on_or_off[ XY(x, y)];
    }
  }

  for(  byte y = 0; y < kMatrixHeight; y++) {
    for(  byte x = 0; x < kMatrixWidth; x++) {
       on_or_off[ XY(x, y)] = next_on_or_off[ XY(x, y)];
    }
  }
}  */

void DrawSte( uint8_t startHue8)
{
leds[ XY(0, 0)]  = CHSV( startHue8, 255, 120);

}

void DrawStartFrame( uint8_t startHue8)
{

  for( byte y = 0; y < kMatrixHeight; y++) {
    on_or_off[ XY(0, y)]  = random(2);
    leds[ XY(0, y)]  = CHSV( startHue8, 255, 80*on_or_off[ XY(0, y)]);
    next_on_or_off[ XY(kMatrixWidth-1, y)] = on_or_off[ XY(0, y)];
    for( byte x = 1; x < kMatrixWidth; x++) {
      on_or_off[ XY(x, y)]  = random(2);
      leds[ XY(x, y)]  = CHSV( startHue8, 255, 80*on_or_off[ XY(x, y)]);
      next_on_or_off[ XY(x-1, y)] = on_or_off[ XY(x, y)];
    }
  }
}

void DrawNextFrame( uint8_t  startHue8, uint16_t &remain)
{
// Code to toggle the on/off nature of the LED's
  uint8_t neighbours = 0;
  uint8_t alive = 0;
  for( byte y = 0; y < kMatrixHeight; y++) {
   for( byte x = 0; x < kMatrixWidth; x++) {
     leds[ XY(x, y)]  = CHSV( startHue8, 255, 255*on_or_off[ XY(x, y)]);
     alive = on_or_off[ XY(x, y)] ;
     neighbours =   on_or_off[ XY((x-1+kMatrixWidth)%kMatrixWidth, (y+1+kMatrixHeight)%kMatrixHeight)]
                  + on_or_off[ XY( x, (y+1+kMatrixHeight)%kMatrixHeight)]
                  + on_or_off[ XY((x+1+kMatrixWidth)%kMatrixWidth, (y+1+kMatrixHeight)%kMatrixHeight)]
                  + on_or_off[ XY((x-1+kMatrixWidth)%kMatrixWidth, y)]
                  + on_or_off[ XY((x+1+kMatrixWidth)%kMatrixWidth, y)]
                  + on_or_off[ XY((x-1+kMatrixWidth)%kMatrixWidth, (y-1+kMatrixHeight)%kMatrixHeight)]
                  + on_or_off[ XY( x, (y-1+kMatrixHeight)%kMatrixHeight)]
                  + on_or_off[ XY((x+1+kMatrixWidth)%kMatrixWidth, (y-1+kMatrixHeight)%kMatrixHeight)];
     if (neighbours < 2 || neighbours > 3 ) next_on_or_off[ XY(x, y)] = 0; // Any cell with < 2 or > 3 dies
     if ( alive == 1 && neighbours == 2 ) next_on_or_off[ XY(x, y)] = 1; // A live cell with exactly 2 neighbours lives
     if ( neighbours == 3 ) next_on_or_off[ XY(x, y)] = 1; // Any cell with 3 neighbours (alive or dead) is alive next generation.
    }
  }

// Code to check something is still happening in display.
  uint8_t onon = 0; // Counter of how many lit up LED's
  uint8_t ru = 0; // Flag for adjacent generations different
  uint8_t onback = 0; // Flag for grandparent generations different
  for(  byte y = 0; y < kMatrixHeight; y++) {
    for(  byte x = 0; x < kMatrixWidth; x++) {
      if ( last_on_or_off[ XY(x, y)] != on_or_off[ XY(x, y)])      ru   = 1;
      if ( last_on_or_off[ XY(x, y)] != next_on_or_off[ XY(x, y)]) onback = 1;
      last_on_or_off[ XY(x, y)] = on_or_off[ XY(x, y)];
      on_or_off[ XY(x, y)] = next_on_or_off[ XY(x, y)];
      if ( on_or_off[ XY(x, y)] == 1 ) {
        onon++;
      }
    }
  }
// Nothing on, reset in 20 cycles
  if (onon == 0 && remain < loops - 20 ) remain = loops - 20 ;
// Nothing new, reset in 20 cycles
  if ((ru == 0 || onback == 0) && remain < loops - 20 ) remain = loops - 20 ;
// Anti-glider, probably not needed (too much corner-case)
  if (onon < 8 && remain < loops - 180 ) remain = loops - 180 ;
/*
//Insert Glider for wrap testing.
  if (onon == 0 && remain < loops - 20 ) {
    // Glider
    on_or_off[ XY(2, 0)] =  1;
    on_or_off[ XY(0, 1)] = 1;
    on_or_off[ XY(2, 1)] = 1;
    on_or_off[ XY(1, 2)] = 1;
    on_or_off[ XY(2, 2)] = 1;
  }
//*/
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
}
