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

//#define DEBUG_SERIAL

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
#include <util/atomic.h>


#define byte uint8_t
#include "ONTVANGER.h"
#include "delay.c"
#include "timer2.c"
#include "millis.h"
#include "RFM69registers.h"
#include "RFM69.h"

//#include "toneAC.h"
extern "C" {
	#include "I2C_master.h"
	#include "pca9635.h"
	#include "uart.h"
	#include "log.h"
	#include "tone.h"
};

RFM69 radio;

ISR(INT0_vect) {
	radio.InterruptHandler();
}

#define NODE_ID				3
#define NETWORKID			14  //the same on all nodes that talk to each other
#define DESTINATION_NODE_ID 2


int main() {	
// disable ADC for less power 
	ADCSRA &= ~_BV(ADEN); // ADC off 
	sei();
	

			/* Initialize MILLIS */
	        millis_init();
			_delay_ms(1000);
			/* Initialize MILLIS */

			/* Initialize TONE */						
			tone_init();	
			noTone();	
			/* Initialize TONE */						
						
						
			/* Initialize TIMER 2 */
			timer2_init();
			timer2_pause();
			/* Initialize TIMER 2 */			
			
			
			/* Initialize UART */
			#ifdef DEBUG_SERIAL
			uart_init( ((F_CPU)/((UART_BAUD_RATE)*16l)-1)) ;
			_delay_ms(1000);
			log_s("UART OK\r");
			_delay_ms(1000);
			#else
			power_usart0_disable();
			#endif
			/* Initialize UART */
		
			/* Initialize I2C */
			I2C_init();	
			/* Initialize I2C */
			
			/* Initialize PCA9634 */
			pca9635_init();
			
			// blink 3 times fast
			for (byte i = 0; i <= 2; ++i){
			pca9635_set_all_led_pwm(255);
			_delay_ms(100);
			pca9635_set_all_led_pwm(0);
			_delay_ms(100);
			}
			
			/* Initialize PCA9634 */
			
		
			// node id, rfband, group id
			
			radio.initialize(RF69_868MHZ, NODE_ID, NETWORKID); // node id, rfband, group id
			// see http://tools.jeelabs.org/rfm12b
			

	#ifdef DEBUG_SERIAL
	log_s("RF OK!");
	_delay_ms(1000);
	#endif
		
			// initialised
			
				pca9635_set_led_pwm(1, 255);
				_delay_ms(400);
				pca9635_set_led_pwm(8, 255);
				_delay_ms(400);
				pca9635_set_led_pwm(11, 255);
				_delay_ms(400);
				pca9635_set_led_pwm(14, 255);
				_delay_ms(800);
				pca9635_set_all_led_pwm(0);
				pca9635_set_sleep(1);
				
			deep_sleep_ok = 1; // put device in deep sleep after initializing
			
			#ifdef DEBUG_SERIAL
				log_s("initialized!");
				_delay_ms(1000);
			#endif
				
				
				
	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).

	if (radio.receiveDone()) { // a packet has been received
		
				#ifdef DEBUG_SERIAL
				if(radio.CRCPass()){
				log_s("CRCOK\r");
				}else{
					log_s("CRC NOOK\r");
				}
				
				//_delay_ms(5);
				#endif
			
		if(radio.CRCPass()){ //  CRC of the received packet, zero indicates correct reception.
		
			if (radio.ACKRequested()) {
				log_s(".");
				radio.SendACK();
				log_s("+");
				radio.SendWait(1); // don't power down too soon
				
			}
			
					
		// only react to packets with first byte 0x99
		if(radio.GetData()[0] == 0x99) { // 153
		// process incoming data here	
			
				////////////////		Fill alarm array		 ////////////////
				// only get the first byte
				uint8_t data = radio.GetData()[1]; // not used, not used, not used, start (1) or stop(0), doorbell, phone, fire, help;

				if(data & 0x10){
					 // start alarm	 
					 active_alarm = active_alarm | data; // 00001111 
					 
				 }else{
					 // stop alarm
					 active_alarm =  active_alarm & (~data); // invert data, compare with active alarm array to clear the right alarm bit 		 
				 }
				 
				////////////////		Fill alarm array		 ////////////////				 
				
			 // IS ER EEN ALARM ACTIEF in array active_alarm?
				 if(active_alarm & 0x0F){
				  
						 // Is there a active alarm thats already activated?
						 if(active_alarm_time == 0) {
							// Geen alarm actief
										 
							deep_sleep_ok = 0; // prevent while loop from going in deepsleep
							
							 // 1. fill icon_current_alarm and sound_current_alarm with the first alarm
							 for (byte i = 0; i <= 3; ++i){
								if(active_alarm & (alarm_bitmask[i]) ){ // check if next alarm is active
									sound_current_alarm = i;
									icon_current_alarm  = i;
									break;
								} 
							 }

							// 2. timer 0 - millis starten
							millis_reset();
							millis_resume();
							
							active_alarm_time = millis_get() + alarm_duration;
							
							// wake up pca9635!
							power_twi_enable();
							I2C_init();
							pca9635_set_sleep(0);
				 
							// 3. timer 2 - alarm timer starten
							timer2_resume();							
										 
							
								
						 }else{
							 
								if(!(data & 0x10)){	 // already a active alarm and received a packet to stop alarm?	
									// put leds off!
									for (byte i = 0; i <= 3; ++i){
										if(data & (alarm_bitmask[i]) ){ // check which alarm needs to be stopped
											for (byte j = 0; j <= 2; j++){
												pca9635_set_led_pwm( pgm_read_byte(&(icon_led_numbers[i].lednr[j])), 0); // leds off!
											}
										}
									}
								}
							
							
								// Only reset active alarm timer if there is a new alarm...
								// 0x10 = activate bit 0x0F are bits of the alarms
								if( (data & 0x10) && (data & 0x0F)){
										active_alarm_time = millis_get() + alarm_duration;
								}
														
														
														
							
						 } //  if(active_alarm_time == 0) {
				 
		 

				}else{ //  if(active_alarm & 0x0F){ // IS ER EEN ALARM ACTIEF in active_alarm?
						// no active alarm in array	
						active_alarm_time = 0; // set alarm time to zero, timers will be disabled in next timer 2 interrupt
				}
						 		


				} // if(rf12_data[0] == 0x99){
			} // if(rf12_crc == 0){ 
									
		} else {
		
			// switch into idle mode until the next interrupt - Choose our preferred sleep mode:
			if(deep_sleep_ok == 1){
								
				set_sleep_mode(SLEEP_MODE_STANDBY); // if active alarm, go in pwr save mode to keep timer 2 running
				sleep_enable();
				// turn off brown-out enable in software
				 sleep_bod_disable();
			 
				// Put the device to sleep:
				sleep_cpu();
			}else{
				// disable various adc + usart0
				set_sleep_mode(SLEEP_MODE_IDLE);
				sleep_enable();
			
				// Put the device to sleep:
				sleep_cpu();
			}
	
			// Clear sleep enable (SE) bit:
			sleep_disable();
		}


	
	} // end while(1){
		
		
	} // end main

	

	
ISR (TIMER2_COMPA_vect) {

	// check if the alarm needs to be stopped
	if(millis_get() >= active_alarm_time || active_alarm_time == 0){
		// stop alarm
			
			// reset steps of sound, flash and icon
			sound_current_step		= 0;
			_sound_note_time		= 0;
										
			_flash_current_step		= 0;
			_flash_time				= 0;
										
			icon_current_step		= 0;
			_icon_time				= 0;
										
			// empty alarm arrray
			active_alarm = 0x00; 
			active_alarm_time = 0;
					
			// alle leds uit, pca in slaapstand
			pca9635_set_all_led_pwm(0); // dimm all leds to zero
			pca9635_set_sleep(1); // put pca9635 in sleep
			power_twi_disable();
			
			// automatisch slapen in loop.

			//uart0_puts("STOPINT");
			// stop timer 0
			millis_pause();
			
			// stop sound
			noTone();
			
			// stop timer 2
			timer2_pause();
			
			deep_sleep_ok = 1;
					
	}else{
		// continue alarm
		isr_sound();
	    isr_light_flash();
		isr_light_icon();		
	}
						
}


	void isr_sound(){
		
			// Stop note if necessary 
			if(_sound_note_time != 0){ // sound already playing?		
				if(millis_get() < _sound_note_time ){ // Do we need to stop the current note?
					return; // dont stop current note
					
				}else{
					noTone(); // stop current note
				}
	
			}
			
			// play next tone
			if(sound_current_alarm == 0 ){
				tone(pgm_read_word(&(sound_pattern_doorbell[sound_current_step].frequency)), sound_alarm_volume); // freq, volume
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_doorbell[sound_current_step].time)));
			}
			else if(sound_current_alarm == 1 ){
				tone(pgm_read_word(&(sound_pattern_phone[sound_current_step].frequency)), sound_alarm_volume);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_phone[sound_current_step].time)));
			}
			else if(sound_current_alarm == 2 ){
				tone(pgm_read_word(&(sound_pattern_fire[sound_current_step].frequency)), sound_alarm_volume);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_fire[sound_current_step].time)));
			}
			else if(sound_current_alarm == 3 ){
				tone(pgm_read_word(&(sound_pattern_help[sound_current_step].frequency)),sound_alarm_volume);
				_sound_note_time = (millis_get() + pgm_read_word(&(sound_pattern_help[sound_current_step].time)));
			}
			
		//_sound_note_time = millis_get() + 1000;
			
			// is variable sound_current_step equal to de total steps?
			if(sound_current_step >= (sound_alarm_keys[sound_current_alarm] -1)){
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
	if(icon_current_step == 0 || icon_current_step == 11){ // if current step is 0 or 11 then intensity is 255
		intensity = 255;
		
		if(icon_current_step == 0){
			_icon_time = millis_get() + 650;
		}else if(icon_current_step == 11){ // last step - set led full on, and go to next icon
			_icon_time = millis_get() + 1;
		}
			
	}else{
		intensity = (250 - (icon_current_step * 25)); // step 1 to 10, 
		_icon_time = millis_get() + 30;
		}
	
	
	// get next flash and send it to the PCA9635
	for (byte j = 0; j <= 2; j++){
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