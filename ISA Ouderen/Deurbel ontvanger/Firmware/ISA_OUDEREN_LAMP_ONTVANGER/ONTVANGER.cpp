/*
 * ISA_GATEWAY_QUADROFLY.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
 */ 

/* 
TIMER 0 - 8BIT  -  MILLIS
TIMER 1 - 16BIT -  PIEZO SOUND
TIMER 2 - 8BIT  -  LED
*/


#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/power.h>
#include <string.h>
#include <avr/pgmspace.h> // for progmem / ram declarations
#include <avr/sleep.h>


#define byte uint8_t
#include "RF12.h"
#include "delay.c"
#include "millis.h"
//#include "toneAC.h"
extern "C" {
	#include "I2C_master.h"
	#include "pca9635.h"
	#include "uart.h"
	#include "log.h"
};


int main() {

	uint8_t mychannel;
	uint8_t counter;
	static long payload;
	
	// config
	unsigned long alarm_duration = 20000; // alarm duration in ms
	
	
	//
	uint8_t active_alarm; // not used, not used, not used, not used, doorbell, phone, fire, help; 
	unsigned long active_alarm_time; // used to track end with timer;
	
	//sound
	uint8_t  sound_current_alarm; // current alarm sound 0=doorbell, 1=phone, 2= fire, 3=help
	uint8_t  sound_current_step; // current sound step from current alarm
	
	unsigned long sound_note_time; // Used to track end note with timer when playing note in the background.
	
	
	//light - flash
	uint8_t flash_current_step; // 1 - 255
	// flash_array 
	// flash_array_time
	
	/// @struct Ramp
	/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
	/// Ramps can be chained together with a non-zero ramp index in the chain field.
typedef struct {
			uint8_t led[4]; // bottom, top left, top center, top right, 0..255
			uint16_t time;   // (0 tot +65,535) time in ms before going to the next step
	} STRUCT_FLASH_PATTERN;
	
 const static STRUCT_FLASH_PATTERN flash_pattern[ ] PROGMEM = {
		
		{ 255 , 255, 255, 255, 255 }, // 0: instant off
		{ 255 , 255, 255, 255, 255 }, // 0: instant off
		{ 255 , 255, 255, 255, 255 }, // 0: instant off
		{ 255 , 255, 255, 255, 255 }, // 0: instant off
		{ 255 , 255, 255, 255, 255 }	
		};
	

	
	
	//light - icons
	uint8_t icon_current; // f
	uint8_t icon_current_step; // f
	
	
	

// disable ADC for less power 
  ADCSRA &= ~_BV(ADEN); // ADC off 


	sei();
			/* Initialize MILLIS */
	        millis_init();
			_delay_ms(3000);
			/* Initialize MILLIS */
						
						
						
						
			/* Initialize TIMER 2 */
			// Timer settings
			TCCR2A = _BV(WGM21); // prescaler 128
			TCCR2B = _BV(CS22)|_BV(CS20);
							 	
			// TIMSK2 = _BV(OCIE2A); // DO NOT ENABLE TIMER2 BY DEFAULT!
			OCR2A = ((F_CPU / 128) / 1000);
			power_timer2_disable(); // power timer2 down!
			/* Initialize TIMER 2 */			
			

			/* Initialize UART */
			uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
			uart0_puts("kak");
			_delay_ms(1000);
		
		
			I2C_init();	
			pca9635_init();
			
			log_s("PCA ok");
			_delay_ms(1000);
			
			
  // node id, rfband, group id
    rf12_initialize(2, RF12_868MHZ, 14);
    // see http://tools.jeelabs.org/rfm12b
  //  rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V

log_s("initialized!");
_delay_ms(1000);

	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
	
	//uart0_puts("WHILE");

	if (rf12_recvDone() && rf12_crc == 0) {
		// process incoming data here
				
			if (RF12_WANTS_ACK) {
				rf12_sendStart(RF12_ACK_REPLY,0,0);
				rf12_sendWait(1); // don't power down too soon
				uart0_puts("ACK-OK");
				_delay_ms(20);
			}
			
			
		uart0_puts("\n\r -DATA!- ");
		_delay_ms(20);
		        
				for (byte i = 0; i < rf12_len; ++i){
		        uart0_putc(rf12_data[i]);
				}
				
				
				////////////////		Fill alarm array		 ////////////////
				// only get the first byte
				uint8_t data = rf12_data[0]; // not used, not used, not used, start (1) or stop(0), doorbell, phone, fire, help;
				
				 if(data & 0x10){
					 // start alarm	 
					 uart0_puts("START");
					 active_alarm = active_alarm & data; // 00001111 
				 }else{
					 // stop alarm
					  uart0_puts("STOP");
					 active_alarm =  active_alarm & (~data); /* invert data, compare with active alarm array to clear the right alarm bit */		 
				 }
				 
				////////////////		Fill alarm array		 ////////////////				 
				
				
							 
				 // IS ER EEN ALARM ACTIEF in array active_alarm?
				 if(active_alarm & 0x0F){
			 
						 // Is there a active alarm thats already activated?
						 if(active_alarm_time == 0) {
							// Geen alarm actief

							 // 1. sound_current_alarm vullen met eerst alarm
							 sound_current_alarm = 4;
							 if((sound_current_alarm == 4) && (active_alarm & 0x08)) sound_current_alarm = 0;
							 if((sound_current_alarm == 4) && (active_alarm & 0x04)) sound_current_alarm = 1;
							 if((sound_current_alarm == 4) && (active_alarm & 0x02)) sound_current_alarm = 2;
							 if((sound_current_alarm == 4) && (active_alarm & 0x01)) sound_current_alarm = 3;
							 
					 					
							// 2. timer 0 - millis starten
							millis_reset();
							millis_resume();
				 
							// 3. timer 2 - alarm timer starten
							power_timer2_enable();
							TIMSK2 |= _BV(OCIE2A);
			 
						 }
				 

				 		// MELDINGSDUUR RESETTEN
				 		active_alarm_time = millis() + alarm_duration; /* 00001111 remove first 4 bits */
								  
				 
					}else{ //  if(active_alarm & 0x0F){ // IS ER EEN ALARM ACTIEF in active_alarm?
						// no active alarm in array
												
						active_alarm_time = 0; // set alarm time to zero, timers will be disabled in next timer 2 interrupt
					}
						 		
		
	} else {
    // switch into idle mode until the next interrupt - Choose our preferred sleep mode:
    set_sleep_mode(SLEEP_MODE_IDLE);
    
    // Set sleep enable (SE) bit:
    sleep_enable();
    
    // Put the device to sleep:
    sleep_cpu();
	
	// Clear sleep enable (SE) bit:
	sleep_disable();
	//_delay_ms(50);
	}

	
		} // end while(1){
		
		
	} // end main

	
	
	
	
ISR (TIMER2_COMPA_vect) {
	
	// check if the alarm needs to be stopped
	if(millis > active_alarm_time){
		// stop alarm
		
			// stop timer 1
			millis_pause();
			
			// stop timer 2
			TIMSK2 &= ~_BV(OCIE2A);
			power_timer2_disable();
			
			// empty alarm arrray
			active_alarm = 0x00; 
			
			// alle leds uit, pca in slaapstand
			pca9635_set_led_mode(0); // put all leds off
			pca9635_set_sleep(); // put pca9635 in sleep
						
			// automatisch slapen in loop.
			
	}else{
		// continue alarm
		isr_sound();
		isr_light_flash();
		isr_light_icon();		
	}
}


	void isr_sound(){
			
			
		}
		
	void isr_light_flash(){
			
			// haal flits arry op van current step
			for (byte j = 0; j <= 3; ++j){
				pca9635_set_led_pwm( j+3, pgm_read_byte(&(flash_pattern[flash_current_step].led[j])));
			}
			
		}
		
	void isr_light_icon(){
			
			
		}

	
	
	
	
	
	
	
	
	
	
	
	
	/*
	
					 // stop timer2
					 TIMSK2 &= ~_BV(OCIE2A);
					 power_timer2_disable()
					 
					 
					 //enable timer2
					 power_timer2_enable()
					 TIMSK2 |= _BV(OCIE2A);
					 
					 
					 
					 */
	
	
	/*
	opvragen progmem
	
			uart0_puts("XXX");
		for (byte j = 0; j < 4; ++j){
				uart0_putc(pgm_read_byte(&(flash_pattern[j].led_left)));
				uart0_putc(255);
				uart0_puts("-");
				_delay_ms(10);
			}
			uart0_puts("XXX");
			
			
			
			*/
	
	/*
	//rood
	
	pca9635_set_led_pwm(2, 255);
	_delay_ms(300);
	pca9635_set_led_pwm(7, 255);
	_delay_ms(300);	
	pca9635_set_led_pwm(12, 255);
	_delay_ms(300);
	pca9635_set_led_pwm(15, 255);
	_delay_ms(1000);
	
		//rood uit, blauw aan
	pca9635_set_led_pwm(2, 0);
	pca9635_set_led_pwm(0, 255);
	_delay_ms(300);
	pca9635_set_led_pwm(7, 0);
	pca9635_set_led_pwm(9, 255);
	_delay_ms(300);
	pca9635_set_led_pwm(12, 0);
	pca9635_set_led_pwm(10, 255);
	_delay_ms(300);
	pca9635_set_led_pwm(15, 0);
	pca9635_set_led_pwm(13, 255);
	_delay_ms(1000);
	
 // blauw uit, groen aan	
	
	pca9635_set_led_pwm(1, 255);
	pca9635_set_led_pwm(0, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(8, 255);
	pca9635_set_led_pwm(9, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(11, 255);
	pca9635_set_led_pwm(10, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(14, 255);
	pca9635_set_led_pwm(13, 0);
	_delay_ms(1000);


	pca9635_set_led_pwm(1, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(8, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(11, 0);
	_delay_ms(300);
	pca9635_set_led_pwm(14, 0);
	_delay_ms(1000);		


// knipperen

for(counter = 0; counter < 3; counter++)
{
pca9635_set_led_pwm(3, 255);
pca9635_set_led_pwm(5, 255);
_delay_ms(100);
pca9635_set_led_pwm(3, 0);
pca9635_set_led_pwm(5, 0);
	_delay_ms(100);
}

for(counter = 0; counter < 3; counter++)
{
	pca9635_set_led_pwm(4, 255);
	pca9635_set_led_pwm(6, 255);
	_delay_ms(100);
	pca9635_set_led_pwm(4, 0);
	pca9635_set_led_pwm(6, 0);
		_delay_ms(100);
}


for(counter = 0; counter < 3; counter++)
{
	pca9635_set_led_pwm(3, 255);
	pca9635_set_led_pwm(5, 255);
	_delay_ms(100);
	pca9635_set_led_pwm(3, 0);
	pca9635_set_led_pwm(5, 0);
		_delay_ms(100);
}

for(counter = 0; counter < 3; counter++)
{
	pca9635_set_led_pwm(4, 255);
	pca9635_set_led_pwm(6, 255);
	_delay_ms(100);
	pca9635_set_led_pwm(4, 0);
	pca9635_set_led_pwm(6, 0);
		_delay_ms(100);
}

_delay_ms(100);		
	
	
	/*
	    ++payload;
	    
	    while (!rf12_canSend())
	    rf12_recvDone();
	  
	    rf12_sendStart(0, &payload, sizeof payload);
	    // set the sync mode to 2 if the fuses are still the Arduino default
	    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
	    rf12_sendWait(0);
	    

	    
		//_delay_ms(3000);
		
		for(mychannel =0; mychannel <= 15; mychannel++)
		{
			for(counter = 0; counter < 255; counter++)
			{
				_delay_ms(1);
				pca9635_set_led_pwm(mychannel, counter);
				
			}
			_delay_ms(100);
			
			for(counter = 255; counter > 0; counter--)
			{
				_delay_ms(1);
				pca9635_set_led_pwm(mychannel, counter);
				
			}
		}
*/
	
	
	//}
	
//}


/*
PWM AANSTURING

for(mychannel =0; mychannel <= 15; mychannel++)
{
	for(counter = 0; counter < 255; counter++)
	{
		_delay_ms(1);
		pca9635_set_led_pwm(mychannel, counter);
		
	}
	_delay_ms(100);
	
	for(counter = 255; counter > 0; counter--)
	{
		_delay_ms(1);
		pca9635_set_led_pwm(mychannel, counter);
		
	}
}



*/




	/*
// WHOOP UP
	byte x;
		for(x=50;x>0;x++){
			toneAC((250*x/20) );
			_delay_ms(30); // Wait a second.
		}
		toneAC(); // Turn off toneAC, can also use noToneAC().
		_delay_ms(500);
// WHOOP UP

_delay_ms(4000);


// WHOOP DOWN
for(x=50;x>0;x--){
	toneAC((250*x/20) );
	_delay_ms(30); // Wait a second.
}
toneAC(); // Turn off toneAC, can also use noToneAC().
_delay_ms(500);
// WHOOP DOWN
		
	
_delay_ms(4000);		
		
		
// DOORBELL
for(x=0;x<2;x++){
toneAC(1500, 10, 500, true); // Play thisNote at full volume for noteDuration in the background.
_delay_ms(500);
toneAC(1200, 10, 500, true); // Play thisNote at full volume for noteDuration in the background.
_delay_ms(1500);
}
//DOORBELL		
	
	_delay_ms(4000);
	
// Phone ring
byte Duration = 1;
//byte x;
byte y;
	for(y=0;y<3;y++){
	      for(x=0;x<(15*Duration);x++)
	      {
			toneAC(1000, 10, 40); // Play thisNote at full volume for noteDuration in the background.
			toneAC(750, 10, 40); // Play thisNote at full volume for noteDuration in the background.
	      }
		  _delay_ms(1200);
	}
			  
		  
	
// Phone ring			
		
		_delay_ms(4000);

         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(200);
         toneAC(1200,10, 300);
         _delay_ms(100);
         toneAC(1200,10, 300);
         _delay_ms(100);
         toneAC(1200,10, 300);
         _delay_ms(200);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
		
		
		
		
		
		_delay_ms(10000);
	}
	
	
	
	
}


// GEBLEVEN BIJ SPI INIT, RFM INITIALISE funtie die verwijst naar SPIINIT();
// Kijken of bij elke build wel een nieuwe hex wordt gemaakt!



/*
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h> 
#include <stdio.h> 
#include <inttypes.h> 
#include <string.h>

#define byte uint8_t

#include "ONTVANGER.h"
#include "binary.h"
#include "uart.c"
#include "log.h"
#include "log.c"
#include "notes.h" //notegen.pl output

//Table consists of 4 notes (1/4 duration each)
const int Music[] = {G4, 4, A4, 4, B4, 4 G4, 4, MUSIC_END};
	
	
void setup(){
//	cli(); // Turn all interrupts off!
	sei(); // Turn interrupts on.
		
	_delay_ms(4000); // Wait for startup!
	
	
			
	
	/* Initialize UART */
	/*
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)); 
	//uart0_puts("U");
		_delay_ms(1000);
		

	log_s("ISA - lamp \n\r");

	log_s("STARTUP OK! \n\r");		
		_delay_ms(1000);
		InitMusic();
		PlayMusic();
		
		
}






int main(void)
{
	setup();
    while(1)
    {

   
   log_s(".");
   _delay_ms(500);


	   
    }
}


*/