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
#include <util/atomic.h>


#define byte uint8_t
#include "ONTVANGER.h"
#include "RF12.h"
#include "delay.c"
#include "millis.h"
//#include "toneAC.h"
extern "C" {
	#include "uart.h"
	#include "log.h"
};



			 
	
int main() {	

DDRB |= _BV(0); // pb0 output
DDRD |= _BV(7);


PORTB |= _BV(0); // pb0 aan
PORTD |= _BV(7); // pd7 aan

_delay_ms(500);

 PORTB &= ~_BV(0); // pb0 uit
 PORTD &= ~_BV(7); // pd7 uit



// disable ADC for less power 
	ADCSRA &= ~_BV(ADEN); // ADC off 
	sei();
			/* Initialize MILLIS */
	        millis_init();
			_delay_ms(3000);
			/* Initialize MILLIS */

				

			/* Initialize UART */
			uart_init( ((F_CPU)/((UART_BAUD_RATE)*16l)-1)) ;
			//uart0_puts("kak");
			_delay_ms(1000);
			/* Initialize UART */
		
			
  // node id, rfband, group id
			rf12_initialize(2, RF12_868MHZ, 14);
    // see http://tools.jeelabs.org/rfm12b
  //  rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V


			

log_s("initialized!");
_delay_ms(1000);

	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
	
	//uart0_puts("WHILE");
//	uart0_putc(rf12_crc);
	//uart0_putc(rf12_crc >> 8);
	
	if (rf12_recvDone() && rf12_crc == 0) {
		// process incoming data here
						PORTD |= _BV(7); // pd7 aan
			if (RF12_WANTS_ACK) {
				rf12_sendStart(RF12_ACK_REPLY,0,0);
				rf12_sendWait(1); // don't power down too soon
			//	uart0_puts("ACK-OK");
				//_delay_ms(10);
			}
			
			
		uart0_puts("DATA");
		_delay_ms(10);
		        
				for (byte i = 0; i < rf12_len; ++i){
		        uart0_putc(rf12_data[i]);
				}
					_delay_ms(200);	
							 
		 PORTD &= ~_BV(7); // pd7 uit
		 			
	} else {
		
		PORTB |= _BV(0); // pb0 aan, pin 14
		
		// switch into idle mode until the next interrupt - Choose our preferred sleep mode:
		//if(deep_sleep_ok == 1){
			set_sleep_mode(SLEEP_MODE_STANDBY); // if active alarm, go in pwr save mode to keep timer 2 running
		//	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // if active alarm, go in pwr save mode to keep timer 2 running
		//}else{
			//set_sleep_mode(SLEEP_MODE_IDLE);
		//}
  //   _delay_ms(15);
	
    // Set sleep enable (SE) bit:
    sleep_enable();
    
    // Put the device to sleep:
    sleep_cpu();
	
	// Clear sleep enable (SE) bit:
	sleep_disable();
	//_delay_ms(50);
		
		PORTB &= ~_BV(0); // pb0 uit, pin 14
	}

	
		} // end while(1){
		
		
	} // end main

	
	