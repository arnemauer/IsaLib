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
#include "ONTVANGER.h"
#include "RF12.h"
#include "delay.c"
#include "millis.h"
//#include "toneAC.h"
extern "C" {
	#include "I2C_master.h"
	#include "pca9635.h"
	#include "uart.h"
	#include "log.h"
	#include "tone.h"
};


#define sound_alarm_keys_doorbell  8
#define sound_alarm_keys_phone     31
#define sound_alarm_keys_fire	8
#define sound_alarm_keys_help  8

#define flash_keys  32

const uint8_t sound_alarm_keys[] = {sound_alarm_keys_doorbell, sound_alarm_keys_phone, sound_alarm_keys_fire, sound_alarm_keys_help };
const uint8_t alarm_bitmask[] = {0x08, 0x04, 0x02, 0x01};


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
	
	unsigned long _sound_note_time; // Used to track end note with timer when playing note in the background.
	
	
	//light - flash
	uint8_t _flash_current_step; // 1 - 255
	unsigned long _flash_time; // Used to track end flash with timer
	

	//light - icons
	uint8_t icon_current_alarm; //  current alarm sound 0=doorbell, 1=phone, 2= fire, 3=help
	uint8_t icon_current_step; //   current sound step from current alarm
	unsigned long _icon_time; // Used to track end flash with timer
	
	

 typedef struct {
			unsigned long frequency; //
				unsigned long time;   // (0 tot +65,535) time in ms before going to the next step
} STRUCT_SOUND_PATTERN;

	const static STRUCT_SOUND_PATTERN sound_pattern_doorbell[sound_alarm_keys_doorbell] PROGMEM = {  // doorbell
//const static sound_patterns[0] PROGMEM = { // doorbell
		{ 1500	, 500 }, // 0: instant off
		{ 0		, 200 }, // 0: instant off
		{ 1200	, 500 }, // 0: instant off
		{ 0		, 1000 }, // 0: instant off
		{ 1500	, 500 }, // 0: instant off
		{ 0		, 200 }, // 0: instant off
		{ 1200	, 500 }, // 0: instant off
		{ 0		, 1500 } // 0: instant off
	};



	const static STRUCT_SOUND_PATTERN sound_pattern_phone[sound_alarm_keys_phone] PROGMEM = { //phone
		
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 }, // 0: instant off
		{ 1000	, 40 }, // 0: instant off
		{ 750	, 40 } // 0: instant off
																																													
			
		
	};
	
	
		
			const static STRUCT_SOUND_PATTERN sound_pattern_fire[sound_alarm_keys_fire] PROGMEM = {  // doorbell
				//const static sound_patterns[0] PROGMEM = { // doorbell
				{ 1500	, 500 }, // 0: instant off
				{ 0		, 500 }, // 0: instant off
				{ 1200	, 500 }, // 0: instant off
				{ 0		, 1500 }, // 0: instant off
				{ 1500	, 500 }, // 0: instant off
				{ 0		, 500 }, // 0: instant off
				{ 1200	, 500 }, // 0: instant off
				{ 0		, 1500 } // 0: instant off
			};
			
					const static STRUCT_SOUND_PATTERN sound_pattern_help[sound_alarm_keys_help] PROGMEM = {  // doorbell
						//const static sound_patterns[0] PROGMEM = { // doorbell
						{ 1500	, 500 }, // 0: instant off
						{ 0		, 500 }, // 0: instant off
						{ 1200	, 500 }, // 0: instant off
						{ 0		, 1500 }, // 0: instant off
						{ 1500	, 500 }, // 0: instant off
						{ 0		, 500 }, // 0: instant off
						{ 1200	, 500 }, // 0: instant off
						{ 0		, 1500 } // 0: instant off
					};


	
	/// @struct Ramp
	/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
	/// Ramps can be chained together with a non-zero ramp index in the chain field.
typedef struct {
			uint8_t led[4]; // bottom, top left, top center, top right, 0..255
			unsigned long time;   // (0 tot +65,535) time in ms before going to the next step
	} STRUCT_FLASH_PATTERN;
	
 const static STRUCT_FLASH_PATTERN flash_pattern[ ] PROGMEM = {
// bottom, top left, top center, top right 0..255 , time in ms
		
		{ 255 , 255, 255, 255, 200 }, // flash, all on
		{	0 ,	  0,   0,   0, 200 }, //		all off
		{ 255 , 255, 255, 255, 200 }, // flash, all on
		{	0 ,	  0,   0,   0, 200 }, //		all off
		{ 255 , 255, 255, 255, 200 }, // flash, all on
		{	0 ,	  0,   0,   0, 200 }, //		all off
		{ 255 , 255, 255, 255, 200 }, // flash, all on
		{	0 ,	  0,   0,   0, 200 }, //		all off


		// FLASH TOP LEFT AND TOP RIGHT 3 times fast
		{ 0	  , 255,   0, 255,  50 }, // 
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 0	  , 255,   0, 255,  50 }, // 
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 0	  , 255,   0, 255,  50 }, // 
		{	0 ,	  0,   0,   0,  50 }, // all off
	
		// FLASH TOP CENTER AND BOTTOM 3 times fast
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
			
		// FLASH TOP LEFT AND TOP RIGHT 3 times fast
		{ 0	  , 255,   0, 255,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 0	  , 255,   0, 255,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 0	  , 255,   0, 255,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		
		// FLASH TOP CENTER AND BOTTOM 3 times fast
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 }, // all off
		{ 255 ,   0, 255,   0,  50 }, //
		{	0 ,	  0,   0,   0,  50 } // all off
			

		};
	

		/// @struct Ramp
		/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
		/// Ramps can be chained together with a non-zero ramp index in the chain field.
		typedef struct {
			uint8_t color[3]; // red, green, blue 0..255
		} STRUCT_ICON_COLORS;

	// RGB
	 const static STRUCT_ICON_COLORS icon_colors[ ] PROGMEM = {
		 { 61 , 245, 0 }, // doorbell - light green
		 {	0 ,	  184,   245, }, // phone - lightblue
		 { 255 , 0, 0 }, // fire - RED
		 { 255 , 255, 0 }  // help - Yellow
		 };
			 
			 
			 /// @struct Ramp
			 /// A "Ramp" is a target RGB color and the time in seconds to reach that color.
			 /// Ramps can be chained together with a non-zero ramp index in the chain field.
			 typedef struct {
				 uint8_t lednr[3]; // red, green, blue
			 } STRUCT_ICON_LED_NUMBERS;

			 // RGB
			 const static STRUCT_ICON_LED_NUMBERS icon_led_numbers[ ] PROGMEM = {
				 {  2 ,  1,  0 }, // doorbell - light green
				 {	7 ,	 8,  9 }, // phone - lightblue
				 { 15 , 14, 13 }, // fire - RED
				 { 12 , 11, 10 }  // help - Yellow
			 };
			 
			 
	
int main() {	

// disable ADC for less power 
  ADCSRA &= ~_BV(ADEN); // ADC off 


	sei();
			/* Initialize MILLIS */
	        millis_init();
			_delay_ms(3000);
			/* Initialize MILLIS */

			/* Initialize TONE */						
			tone_init();		
			/* Initialize TONE */						
						
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
			_delay_ms(1000);
			pca9635_set_led_mode(0); // put all leds off
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
				uart0_putc(data);
				_delay_ms(10);
				 if(data & 0x10){
					 // start alarm	 
					 uart0_puts("START");
					 active_alarm = active_alarm | data; // 00001111 
				 }else{
					 // stop alarm
					  uart0_puts("STOP");
					 active_alarm =  active_alarm & (~data); /* invert data, compare with active alarm array to clear the right alarm bit */		 
				 }
				 
				////////////////		Fill alarm array		 ////////////////				 
				
				
					uart0_putc(active_alarm);	 
				 // IS ER EEN ALARM ACTIEF in array active_alarm?
				 if(active_alarm & 0x0F){
			  uart0_puts("YES");
						 // Is there a active alarm thats already activated?
						 if(active_alarm_time == 0) {
							// Geen alarm actief

							 // 1. fill icon_current_alarm and sound_current_alarm with the first alarm
							 for (byte i = 0; i < 4; ++i){
								if(active_alarm & (alarm_bitmask[i]) ){ // check if next alarm is active
									sound_current_alarm = i;
									icon_current_alarm  = i;
									break;
								} 
							 }

					 					
							// 2. timer 0 - millis starten
							millis_reset();
							millis_resume();
				 
							// 3. timer 2 - alarm timer starten
							power_timer2_enable();
							TIMSK2 |= _BV(OCIE2A);
			 
						 }
				 

				 		// MELDINGSDUUR RESETTEN
				 		active_alarm_time = millis_get() + alarm_duration; 
						 uart0_putc(millis_get());
						uart0_putc(active_alarm_time);
				 
				 
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
	if(millis_get() > active_alarm_time){
		// stop alarm
		
			// stop timer 0
			millis_pause();
			
			// stop sound
			noTone();
			
			// stop timer 2
			TIMSK2 &= ~_BV(OCIE2A);
			power_timer2_disable();
			
			// empty alarm arrray
			active_alarm = 0x00; 
			
			// alle leds uit, pca in slaapstand
			pca9635_set_led_mode(0); // put all leds off
			pca9635_set_sleep(1); // put pca9635 in sleep
						
			// automatisch slapen in loop.
			
	}else{
		// continue alarm
		isr_sound();
	    isr_light_flash();
		isr_light_icon();		
	}
}


	void isr_sound(){
		//	uart0_putc(sound_current_step);
		//	uart0_putc(millis_get());
		//	uart0_putc(_sound_note_time);
			// Stop note if necessary 
			if(_sound_note_time != 0){ // sound already playing?		
				//	uart0_puts("GS");
				if(millis_get() < _sound_note_time ){ // Do we need to stop the current note?
					//uart0_puts("DS");
					return; // dont stop current note
					
				}else{
					noTone(); // stop current note
				}
	
			}
			
			// play next tone
		//	uart0_puts("PN");
		//	tone(unsigned long frequency, uint8_t volume);
		//uart0_putc(sound_current_alarm);
			if(sound_current_alarm == 0 ){
				tone(pgm_read_word(&(sound_pattern_doorbell[sound_current_step].frequency)), 10); // freq, volume
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_doorbell[sound_current_step].time)));
			}
			if(sound_current_alarm == 1 ){
				tone(pgm_read_word(&(sound_pattern_phone[sound_current_step].frequency)), 10);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_phone[sound_current_step].time)));
			}
			if(sound_current_alarm == 2 ){
				tone(pgm_read_word(&(sound_pattern_help[sound_current_step].frequency)), 10);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_help[sound_current_step].time)));
			}
			if(sound_current_alarm == 3 ){
				tone(pgm_read_word(&(sound_pattern_fire[sound_current_step].frequency)),10);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_fire[sound_current_step].time)));
			}
			
		//_sound_note_time = millis_get() + 1000;
			
			// is variable sound_current_step equal to de total steps?
			if(sound_current_step >= sound_alarm_keys[sound_current_alarm]){
				// go to next sound
				
				uint8_t done = 0;
				while(done == 0){
					sound_current_alarm++; // sound_current_alarm plus one
					if(sound_current_alarm == 4) { sound_current_alarm = 0; } // if sound_current_alarm is 4 than go back to 0
					
					if(active_alarm & (alarm_bitmask[sound_current_alarm]) ){ // check if next alarm is active
						done = 1;
					}
				
					if(!(active_alarm & 0x0F)){ // is there ANY alarm active??
						done = 1; // in case there is no active alarm anymore, just breakout while
					}

				} // while
				
				sound_current_step = 0; // reset sound_current_step to zero to start the next sound	
				
			}else{
				// set sound_current_step plus one
				sound_current_step++;
			}
	
				
		} //void isr_sound()
		
	void isr_light_flash(){
			
		// Do we need to go to the next flash?
		if(_flash_time != 0){ // is there already a flash ?
			if(millis_get() < _flash_time ){ // Do we need to stop the current note?
				return; // dont stop current flash
			}						
		}
						
			
		// get next flash and send it to the PCA9635
		for (byte j = 0; j <= 3; j++){
		pca9635_set_led_pwm( j+3, pgm_read_byte(&(flash_pattern[_flash_current_step].led[j])));
		}
		//pca9635_set_led_pwm_flash(3, pgm_read_byte(&(flash_pattern[_flash_current_step].led[0])), pgm_read_byte(&(flash_pattern[_flash_current_step].led[1])), pgm_read_byte(&(flash_pattern[_flash_current_step].led[2])), pgm_read_byte(&(flash_pattern[_flash_current_step].led[3])) );
		
		_flash_time = millis_get() + pgm_read_word(&(flash_pattern[_flash_current_step].time));
			
		// is variable _flash_current_step equal to total steps?
		if(_flash_current_step == (flash_keys -1 )) {
			//reset _flash_current_step to 0
			_flash_current_step = 0;				
		}else{
			// set _flash_current_step +1
			_flash_current_step++;
		}
		
	} // END isr_light_flash
		
		
		
		
		
	void isr_light_icon(){
		
		// Do we need to go to the next flash?
		if(_icon_time != 0){ // is there already a flash ?
			if(millis_get() < _icon_time ){ // Do we need to stop the current note?
				return; // dont stop current flash
			}
		}
		
		

	// step 0 = 255
	// step 1 = 250 - (1 x 25)  = 225
	// step 2 = 250 - (2 x 25)  = 200
	// step 11 = (if step == 11) brightness = 255
	
	// calculate intensity based on step
	uint16_t intensity;
	if(icon_current_step == 0 || icon_current_step == 11){
		intensity = 255;
		if(icon_current_step == 0) _icon_time = millis_get() + 1000;
		else if(icon_current_step == 11) _icon_time = millis_get() + 1;
	}else{
		intensity = (250 - (icon_current_step * 25));
		_icon_time = millis_get() + 50;
		}
	
	
	// get next flash and send it to the PCA9635
	for (byte j = 0; j <= 3; j++){
		uint8_t _dimmed_color = ((pgm_read_byte(&(icon_colors[icon_current_alarm].color[j])) * intensity) >> 8); // calculate dimmed color
		pca9635_set_led_pwm( pgm_read_byte(&(icon_led_numbers[icon_current_alarm].lednr[j])), _dimmed_color);
	}
	

	// is variable _flash_current_step equal to total steps?
	if(icon_current_step == 11) {
		//reset _flash_current_step to 0
		icon_current_step = 0;
		
			
			uint8_t done = 0;
			while(done == 0){
				icon_current_alarm++; // sound_current_alarm plus one
				if(icon_current_alarm == 4) { icon_current_alarm = 0; } // if sound_current_alarm is 4 than go back to 0
				
				if(active_alarm & (alarm_bitmask[icon_current_alarm]) ){ // check if next alarm is active
					done = 1;
				}
				
				if(!(active_alarm & 0x0F)){ // is there ANY alarm active??
					done = 1; // in case there is no active alarm anymore, just breakout while
				}

			} // while
			
			
		}else{
		// set _flash_current_step +1
		icon_current_step++;
	}
	
} // END isr_light_icon

	
	
	
	
	

	
	
	
	
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