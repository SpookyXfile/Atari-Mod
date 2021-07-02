#include <EEPROM.h>
#include <avr/wdt.h>

#include <MCP23009.h>


#define FIRE0   0x01 
#define FIRE1   0x02
#define RIGHT   0x04
#define DOWN    0x08
#define LEFT    0x10
#define UP      0x20

#define DATA0   0
#define DATA1   1
#define DATA2   2
#define DATA3   3
#define DATA4   4
#define DATA5   5
#define DATA6   6
#define DATA7   7

#define XA_J    8   // PB0 
#define XB_J    6   // PD6
#define YA_J    7   // PD7
#define YB_J    15  // PC1
#define RB_J    10  // PB2
#define LB_J    3   // PD3
#define TP_J    12  // PB4

#define CS      5   // PD5
#define INT     2

#define REBOOT  0   // PD0 

#define JOY_LED       13   // PB5
#define MICE_LED      14   // PC0 
#define SELECT_SRC    16   // Arduino pin A2 ( PC2 )

#define MOUSE_SELECT  1
#define JOY_SELECT    2

#define SELECT_CONTINUOUS_LED   DATA6
#define PUSH_BUTTON_LED         DATA7		
#define SELECT_ONE_CONTINUOUS   DATA1

//	Comment this to disable Serial Debug
//#define DEBUG


MCP23009 myMCP;

bool joyConPresent = false;

byte selectSource = MOUSE_SELECT;

bool selectJoyMoreClick = true;

bool autoFire = false;

volatile bool awakenByInterrupt = false;

bool buttonState;               	// the current reading from the input pin
bool buttonStateReboot; 
bool lastButtonState 	= HIGH;		// the previous reading from the input pin
bool lastButtonState_2	= HIGH;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime		= 0;  // the last time the output pin was toggled
unsigned long lastDebounceTime_2	= 0;

/*
byte getData()
{
  byte dataIO = myMCP.readGPIO();

  return  ~dataIO & 0x3F;
}
*/

void reboot()
{
  wdt_disable();
  wdt_enable( WDTO_15MS );
  
  while ( 1 ){ }
}

void setSelectSourceLED( byte source )
{
  if ( source == MOUSE_SELECT )
  {
	// Mice LED Light
	digitalWrite( JOY_LED, HIGH );
	digitalWrite( MICE_LED, LOW );
  }
  else
  {
	// Joy LED Light    
	digitalWrite( JOY_LED, LOW );
	digitalWrite( MICE_LED, HIGH );
  }
  
}

void setBlinkSourceLED( bool alternate, byte timeBlink )
{
  timeBlink = ( timeBlink < 1 ) ? 1 : timeBlink;
  
  //for ( byte i = 0; i < time = ( time < 1 ) ? 1 : time; i++ )
  for ( byte i = 0; i < timeBlink; i++ )
	{	
		if ( alternate )
		{
			digitalWrite( JOY_LED, LOW );
			digitalWrite( MICE_LED, HIGH );
			delay( 250 );
			digitalWrite( JOY_LED, HIGH );
			digitalWrite( MICE_LED, LOW );
			delay( 250 );
		}
		else
		{
			digitalWrite( JOY_LED, HIGH );
			digitalWrite( MICE_LED, HIGH );
			delay( 250 );
			digitalWrite( JOY_LED, LOW );
			digitalWrite( MICE_LED, LOW );
			delay( 250 );			
		}
	}

}	

void setSelectContinuousLED( bool state )
{
  myMCP.digitalWrite( SELECT_CONTINUOUS_LED, !state );

}

void setPushButtonLED( bool state )
{
  myMCP.digitalWrite( PUSH_BUTTON_LED, !state );

}

void setHighZPin( byte pin )
{
	pinMode( pin, INPUT );
    digitalWrite( pin, LOW );
}

void setKBPort()
{    
  myMCP.pinMode( DATA0, INPUT );
  myMCP.pinMode( DATA1, INPUT );
  myMCP.pinMode( DATA2, INPUT );
  myMCP.pinMode( DATA3, INPUT );
  myMCP.pinMode( DATA4, INPUT );
  myMCP.pinMode( DATA5, INPUT );
  myMCP.pinMode( DATA6, OUTPUT );
  myMCP.pinMode( DATA7, OUTPUT );

  myMCP.pullUp( DATA0, HIGH );
  myMCP.pullUp( DATA1, HIGH );
  myMCP.pullUp( DATA2, HIGH );
  myMCP.pullUp( DATA3, HIGH );
  myMCP.pullUp( DATA4, HIGH );
  myMCP.pullUp( DATA5, HIGH );

  myMCP.setupInterrupts( false, LOW );
  myMCP.setupInterruptPin( DATA0, FALLING );
  myMCP.setupInterruptPin( DATA1, FALLING );
  myMCP.setupInterruptPin( DATA2, FALLING );
  myMCP.setupInterruptPin( DATA3, FALLING );
  myMCP.setupInterruptPin( DATA4, FALLING );
  myMCP.setupInterruptPin( DATA5, FALLING );

  setSelectContinuousLED( selectJoyMoreClick );
  setPushButtonLED( false );
  
}

void muteJoy()
{
  digitalWrite( XA_J, HIGH );
  digitalWrite( XB_J, HIGH );
  digitalWrite( YA_J, HIGH );
  digitalWrite( YB_J, HIGH );
  digitalWrite( LB_J, HIGH );
  digitalWrite( RB_J, HIGH );
  digitalWrite( TP_J, HIGH );
  
  setPushButtonLED( false );
  
}

void setHighZJoy()
{
	setHighZPin( XA_J );
	setHighZPin( XB_J );
	setHighZPin( YA_J );
	setHighZPin( YB_J );
	setHighZPin( LB_J );
	setHighZPin( RB_J );
	setHighZPin( TP_J );
	
}	

void setSource( byte Source )
{
	setHighZJoy();
	delay( 100 );
	
	pinMode( XA_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( XB_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( YA_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( YB_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( LB_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( RB_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );
	pinMode( TP_J, ( Source == MOUSE_SELECT ) ? INPUT_PULLUP : OUTPUT );

#ifdef DEBUG
  Serial.println();
  Serial.print( "GET SOURCE CHANGED : " );
  Serial.print( selectSource, DEC );
#endif

  if ( Source == JOY_SELECT )
  {
    digitalWrite( CS, HIGH );
	
	setKBPort();
	  
    muteJoy();

    attachInterrupt( digitalPinToInterrupt( INT ), intCallBack, FALLING );
  }
  else
  {
	//setHighZJoy();
	
	digitalWrite( CS, LOW );

    setSelectContinuousLED( false );
	
    detachInterrupt( digitalPinToInterrupt( INT ) );

    finishInterrupt();

    cleanInterrupts();
  }
  
  saveSelectSource( Source );
  
  setSelectSourceLED( Source );
  delay( 20 );
  
}

void saveSelectSource( byte source )
{
  
  EEPROM.write( 0xAB, source );
  
}

byte readSelectSource()
{
  /*
  byte Buf = EEPROM.read( 0xAB );
  
  if ( Buf != JOY_SELECT )
    Buf = MOUSE_SELECT;
  
  return Buf;
  */
  
  return ( EEPROM.read( 0xAB ) >= JOY_SELECT ) ? JOY_SELECT : MOUSE_SELECT;
}

/*		TODO
byte extractSelectSource( byte value )
{
  byte Buf = readSelectSource() ^ 0x01;

  return Buf;
}
*/

void saveSelectContinuousClick( bool state )
{  
  EEPROM.write( 0xAC, (byte)state );
  
}

bool readSelectContinuousClick()
{

  return ( EEPROM.read( 0xAC ) >= 1 ) ? true : false;
}

void setup()
{
#ifdef DEBUG
  Serial.begin( 9600 );

  while( !Serial )
  {
    ;
  }
  
  Serial.println();
  Serial.print( "Start Atari Mouse/Joy" );
#endif

	setHighZJoy();
	delay( 100 );
	
  pinMode( CS, OUTPUT );
  digitalWrite( CS, HIGH );	
    
  //pinMode( INT, INPUT );
  pinMode( INT, INPUT_PULLUP );
  //digitalWrite( INT, HIGH );

  pinMode( SELECT_SRC, INPUT_PULLUP );
  //digitalWrite( SELECT_SRC, HIGH );

  pinMode( REBOOT, INPUT_PULLUP );
  //digitalWrite( SELECT_SRC, HIGH );

  pinMode( MICE_LED, OUTPUT );
  pinMode( JOY_LED, OUTPUT );
  
  joyConPresent = myMCP.begin( 0x00 );
    
  selectSource = readSelectSource();
  
  selectJoyMoreClick = readSelectContinuousClick();
  
  setBlinkSourceLED( false, 2 );
  
  if ( joyConPresent )
  {
#ifdef DEBUG
    Serial.println();
    Serial.print( "Detecting Joy DONE" );
	Serial.println();
    Serial.print( "selectJoyMoreClick : " );
	Serial.print( selectJoyMoreClick, DEC );
#endif

    setKBPort();
  }
  else
  {
#ifdef DEBUG
    Serial.println();
    Serial.print( "Joy Not Present or CON Problem" );
#endif

    setBlinkSourceLED( true, 2 );
	
	selectSource = MOUSE_SELECT;
  }

  ////attachInterrupt( digitalPinToInterrupt( INT ), intCallBack, FALLING );

  setSource( selectSource );

}

void intCallBack()
{
  awakenByInterrupt = true;
  
}

byte handleInterrupt()
{  
  byte pin   = myMCP.getLastInterruptPin();
  byte value = myMCP.getLastInterruptPinValue();

#ifdef DEBUG
  Serial.println();
  Serial.print( "HandleInterrupt - PIN : " );
  Serial.print( pin, DEC  );
  Serial.println();
  Serial.print( "HandleInterrupt - VALUE : " );
  Serial.print( value, DEC  );
  Serial.println();
#endif  
  
  byte atariJoy = 0;

#ifdef DEBUG  
  Serial.println();
#endif

  switch( pin )
  {
    //  Left Button ( Fire 0 )
    case DATA0 :
      atariJoy = LB_J;
      #ifdef DEBUG
      Serial.print( "Left Button - Fire 0" );
      #endif
      break;

    // Select Source
    case SELECT_ONE_CONTINUOUS :
      atariJoy = 0xAC;
      
	  selectJoyMoreClick = !selectJoyMoreClick;
	  saveSelectContinuousClick( selectJoyMoreClick );
	  setSelectContinuousLED( selectJoyMoreClick );
	  
      #ifdef DEBUG
      Serial.print( "Select Push Button Method : " );
      Serial.print( selectJoyMoreClick, DEC );
      #endif
	  
      break;

    //  Right - YB_J
    case DATA2 :
      #ifdef DEBUG
      Serial.print( "Right" );
      #endif
      atariJoy = XA_J;
      break;

    //  Down - XA_J
    case DATA3 :
      #ifdef DEBUG
      Serial.print( "Down" );
      #endif
      atariJoy = YB_J;
      break;

    //  Left - YA_J
    case DATA4 :
      #ifdef DEBUG
      Serial.print( "Left" );
      #endif
      atariJoy = XB_J;
      break;

    //  UP - XB_J
    case DATA5 :
      #ifdef DEBUG
      Serial.print( "UP" );
      #endif
      atariJoy = YA_J;
      break;

    
	
    //  UP - XB_J
    case 0x1E :
      #ifdef DEBUG
      Serial.print( "FIRE0 + UP" );
      #endif
      //atariJoy = ;
      break;    
        
    default :
      atariJoy = 0;
      break;
   }

   if ( ( atariJoy != 0 || atariJoy != 0xAC ) && ( selectJoyMoreClick == false ) )
   {
     if ( atariJoy != LB_J )
	 {	 
		 setPushButtonLED( true );
		 digitalWrite( atariJoy, LOW ); 
		 delay( 12 );
		 setPushButtonLED( false );
		 digitalWrite( atariJoy, HIGH );
		 delay( 12 );
	 }
	 else
	 {
	  //setPushButtonLED( true );
	  //digitalWrite( LB_J, LOW ); 
     }
	 
   }
   else if ( atariJoy == 0xAC ) 
   {   
     //selectJoyOneClick = !selectJoyOneClick;
	 
   }
   else
   {
	  muteJoy();
   }

  finishInterrupt();

  cleanInterrupts();

  return atariJoy;
  
}

void finishInterrupt()
{
  //	????? Maybe clean " myMCP.digitalRead( DATA1 ) "
  while ( !( myMCP.digitalRead( DATA0 ) && myMCP.digitalRead( DATA1 ) && myMCP.digitalRead( DATA2 ) &&
             myMCP.digitalRead( DATA3 ) && myMCP.digitalRead( DATA4 ) && myMCP.digitalRead( DATA5 ) ) )
  {
    ;    
  }
  
}

// handy for interrupts triggered by buttons
// normally signal a few due to bouncing issues
void cleanInterrupts()
{
#if defined( __AVR_ATmega8__ )
  GIFR |= 1 << INTF0;
#else
  EIFR = 0x01;
#endif
  
  awakenByInterrupt = false;
  
}

byte getJoyData()
{
  byte atariJoy = 0xFF;

  if ( myMCP.digitalRead( DATA0 ) == LOW )      atariJoy = LB_J;
  ////else if ( myMCP.digitalRead( DATA1 ) == LOW ) atariJoy =;
  else if ( myMCP.digitalRead( DATA2 ) == LOW ) atariJoy = XA_J;
  else if ( myMCP.digitalRead( DATA3 ) == LOW ) atariJoy = YB_J;
  else if ( myMCP.digitalRead( DATA4 ) == LOW ) atariJoy = XB_J;
  else if ( myMCP.digitalRead( DATA5 ) == LOW ) atariJoy = YA_J;
  else atariJoy = 0xFF;

  return atariJoy;
}  

void loop()
{
  while ( !awakenByInterrupt )
  {
    bool reading_SourceSelect = digitalRead( SELECT_SRC );
    bool reading_Reboot       = digitalRead( REBOOT );
	
	
	// If the switch changed, due to noise or pressing:
	if ( reading_SourceSelect != lastButtonState )
	{
		delay( 10 );
		reading_SourceSelect = digitalRead( SELECT_SRC );
		
		if ( reading_SourceSelect == LOW )
		{
			// reset the debouncing timer
			lastDebounceTime = millis();
		}
			
		if ( reading_SourceSelect == HIGH )
		{
			// but no longer pressed, how long was it down?
			unsigned long currentMillis = millis();
				  
			if ( ( currentMillis - lastDebounceTime >= 100 ) && !( currentMillis - lastDebounceTime >= 500 ) )
			{
				setPushButtonLED( true );
				delay( 250 );
				setPushButtonLED( false );
				delay( 250 );
				setPushButtonLED( true );
				delay( 250 );
				setPushButtonLED( false );
				
				autoFire = !autoFire;
				
			#ifdef DEBUG
				Serial.println();
				Serial.print( "Select AutoFire : " );
				Serial.print( autoFire, DEC );
			#endif
				
				digitalWrite( LB_J, ( autoFire ) ? LOW : HIGH );
									
			}
			  
			if ( ( currentMillis - lastDebounceTime >= 500 ) )
			{
				selectSource = ( selectSource == MOUSE_SELECT ) ? JOY_SELECT : MOUSE_SELECT;

			#ifdef DEBUG
				Serial.println();
				Serial.print( "GET SOURCE CHANGED : " );
				Serial.print( selectSource, DEC );
			#endif
				  
				setSource( selectSource );				
			
			}
		  
		}
			
		// used to detect when state changes
		lastButtonState	= reading_SourceSelect;
	}
  
	if ( reading_Reboot != lastButtonState_2 )
	{
		delay( 10 );
		reading_Reboot = digitalRead( REBOOT );
		
		if ( reading_Reboot == LOW )
		{
		  // reset the debouncing timer
		  lastDebounceTime_2 = millis();
		}
		
		if ( reading_Reboot == HIGH )
		{
			// but no longer pressed, how long was it down?
			unsigned long currentMillis_2 = millis();
			  
			if ( ( currentMillis_2 - lastDebounceTime_2 >= 100 ) && !( currentMillis_2 - lastDebounceTime_2 >= 500 ) )
			{
				// short press detected.
			#ifdef DEBUG
				Serial.println();
				Serial.print( "GET REBOOT SHORT CLICK: " );
			#endif
			
				// Add Short Click Event Function 
			
			}
		  
			if ( ( currentMillis_2 - lastDebounceTime_2 >= 500 ) )
			{
				// the long press was detected
			#ifdef DEBUG	
				Serial.println();
				Serial.print( "LONG PRESS REBOOT" );
			#endif	
			
				reboot();
			}
		  
		}
	
		lastButtonState_2 = reading_Reboot;
	}

	if ( !autoFire )
	{	
		digitalWrite( LB_J, HIGH );
	
	}
    
  }
  
  while( awakenByInterrupt && ( selectJoyMoreClick == true ) && ( getJoyData() != 0xFF ) && ( selectSource == JOY_SELECT ) )
  {     
	byte Buf = getJoyData();
	 
	#ifdef DEBUG
        Serial.println();
        Serial.print( "AtariJot Value : " );
        Serial.print( Buf, DEC );
        Serial.println();
        Serial.print( "PUSH, PUSH, PUSH - atariJoy Value : HIGH" );
        Serial.println();
        Serial.print( "PUSH, PUSH, PUSH - atariJoy Value : LOW" );
    #endif

    if ( Buf != 0xFF )
    {	 
		if ( Buf != LB_J )
	    {	
			setPushButtonLED( true );
		    digitalWrite( Buf, LOW ); 
		    delay( 12 );
		    setPushButtonLED( false );
		    digitalWrite( Buf, HIGH );
		    delay( 12 );
		}
		else if ( !autoFire )
		{
			setPushButtonLED( true );
			digitalWrite( Buf, LOW );
		}
		
    }
    else if ( !autoFire )
    {
		setPushButtonLED( false );
		digitalWrite( Buf, HIGH ); 
    }

  }
  
  if ( awakenByInterrupt && ( selectSource == JOY_SELECT ) )
  {    
		handleInterrupt();

		cleanInterrupts();
	
  }

}  
