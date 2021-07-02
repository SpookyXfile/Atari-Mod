/******************************************************************
 Created with PROGRAMINO IDE for Arduino - 06.08.2018 15:23:03
 Project     : Atari ST IKBD clock injector with DS3231 RTC
 Libraries   : SoftwareSerial, Wire
 Author      : TzOk
 Description : ARD_RX0 ( ARD Pin 0 - PD0 ) from KB_5 ( KB CON : Pin 5 - SD to CPU ), 
			   ARD_TX1 ( ARD_D11 - PB3 ) to ST_5, 
			   ARD_RX1 ( ARD_D10 - PB2 ) from/to KB/ST_6 ( KB CON : Pin 6 - SD from CPU )
******************************************************************/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Rtc_Pcf8563.h>


#define LED			13
#define CLOCK_IN	15

//#define DEBUG

SoftwareSerial Ctrl( 10, 11 );		// RX1, TX1
//SoftwareSerial RX( 8, 9 );
//SoftwareSerial RX( 3, 4 );
//SoftwareSerial RX( 2, 3 );		// RX1 ( MON ), TX1


// init the real time clock
Rtc_Pcf8563 rtc;

byte cmd;

byte inj = 255;

byte rtcDate[ 7 ];

//					  ss,   mm,   hh,   DD,   MM,   YY
byte stDate[ 7 ] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC };


uint8_t bcd2bin( uint8_t val ){ return val - 6 * ( val >> 4 ); }
uint8_t bin2bcd( uint8_t val ){ return val + 6 * ( val / 10 ); }

void setup()
{
	rtc.initClock();
	rtc.setSquareWave( SQW_1HZ );
	  
	Serial.begin( 7812 );
	  
#ifdef DEBUG	
	Serial.begin( 9600 );

	while( !Serial )
	{
		;
	}
	  
	Serial.println();
	Serial.print( "Start Atari RTC" );
#endif
	  
	Ctrl.begin( 7812 );
	Ctrl.listen();
	  
	Wire.begin();

	pinMode( CLOCK_IN, INPUT );
	
	pinMode( LED, OUTPUT );
	digitalWrite( LED, LOW );
	delay( 250 );
	digitalWrite( LED, HIGH );
	delay( 250 );
	digitalWrite( LED, LOW );
	delay( 250 );
	digitalWrite( LED, HIGH );

}

void loop()
{
	if ( Ctrl.available() )
	{
		cmd = Ctrl.read();
    
		if ( cmd == 0x1C )
		{
			// Read PCF8563 RTC
			rtcDate[ 0 ] = bin2bcd( rtc.getYear() );		// YY
			rtcDate[ 1 ] = bin2bcd( rtc.getMonth() );		// MM
			rtcDate[ 2 ] = bin2bcd( rtc.getDay() );			// dd
			rtcDate[ 3 ] = ( rtcDate[ 1 ] & 0x80 ) ? 0 : 1;	// Centuty => 1: < 2000 0: >2000 
			rtcDate[ 4 ] = bin2bcd( rtc.getHour() );		// hh
			rtcDate[ 5 ] = bin2bcd( rtc.getMinute() );		// mm
			rtcDate[ 6 ] = bin2bcd( rtc.getSecond() );		// ss
			
					
			stDate[ 5 ] = ( rtcDate[ 3 ] == 0 ) ? ( rtcDate[ 0 ] + 0xA0 ) : rtcDate[ 0 ]; // YY : if CENTURY bit is SET => stDate = rtcDate + 100 ( or 160 -> A0 )
			stDate[ 4 ] = rtcDate[ 1 ] & 0x1F;		// MM
			stDate[ 3 ] = rtcDate[ 2 ];				// DD
			stDate[ 2 ] = rtcDate[ 4 ] & 0x3F;		// hh
			stDate[ 1 ] = rtcDate[ 5 ];				// mm
			stDate[ 0 ] = rtcDate[ 6 ];				// ss
		  
			inj = 6;

			digitalWrite( LED, LOW ); 
			  
			for ( byte i = 6; i < 255; i-- )
			{
				while( !Ctrl.available() )
				{
					
				};
				
				Ctrl.write( stDate[ i ] );
				
			}
			  
			digitalWrite( LED, HIGH );
			
		}
		else if ( cmd == 0x1B )
		{
			// Write to PCF8563 RTC
			digitalWrite( LED, LOW ); 
		  
			for ( byte i = 5; i < 255; i-- )
			{
				while( !Ctrl.available() )
				{
					
				};
				
				stDate[ i ] = Ctrl.read();
				
			}

			digitalWrite( LED, HIGH ); 

			// 0x1B,0xB8,0x04,0x06,0x19,0x00,0x45
			rtcDate[ 0 ] = ( stDate[ 5 ] < 0xA0 ) ? stDate[ 5 ] : ( stDate[ 5 ] - 0xA0 );		// YY : if stDate > 99 => rtcDate = stDate - 100
			//rtcDate[1] = (stDate[5] < 0xA0) ? stDate[4] : stDate[4] + 0x80;					// MM : if stDate > 99 => set CENTURY bit
			rtcDate[ 1 ] = stDate[ 4 ];        													// MM
			rtcDate[ 2 ] = stDate[ 3 ];															// DD
			rtcDate[ 3 ] = ( stDate[ 5 ] < 0xA0 ) ? 1 : 0;                           			// Centuty => 1: < 2000 0: >2000 
			rtcDate[ 4 ] = stDate[ 2 ];        													// hh
			rtcDate[ 5 ] = stDate[ 1 ];															// mm
			rtcDate[ 6 ] = stDate[ 0 ];															// ss

			// CAUTION : year val is 00 to 99, xx with the highest bit of month = century 0=20xx, 1=19xx
			//byte century = ( stDate[5] >= 79 ) || ( stDate[5] <= 99 ) ? 1 : 0;
			byte century = ( stDate[ 5 ] < 0xA0 ) ? 1 : 0;
        
			rtc.setDate( bcd2bin( rtcDate[ 2 ] ), bcd2bin( rtcDate[ 3 ] ), bcd2bin( rtcDate[ 1 ] ), century, bcd2bin( rtcDate[ 0 ] ) );		// day, weekday, month, century, year 
			rtc.setTime( bcd2bin( rtcDate[ 4 ] ), bcd2bin( rtcDate[ 5 ] ), bcd2bin( rtcDate[ 6 ] ) );										// hh, mm, ss
		  
		}
	}
	
}

void serialEvent()
{
	if ( inj == 255 )
		Serial.write( Serial.read() );
	else
	{
		Serial.read();
		Serial.write( stDate[ inj-- ] );
	}
  
}
