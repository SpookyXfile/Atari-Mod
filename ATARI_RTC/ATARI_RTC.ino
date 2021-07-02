#include <Wire.h>
#include <Rtc_Pcf8563.h>

//init the real time clock
Rtc_Pcf8563 rtc;

uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

void setup()
{
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
