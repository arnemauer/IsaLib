/*
 * ISA Ouderen Zender.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
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
#include "ISA_OUDEREN_ZENDER.h"
#include "RF69_compat.h"
#include "RF12.h"
#include "RF69.h"
#include "millis.h"
//#include "toneAC.h"
extern "C" {
	#include "uart.h"
	#include "log.h"
};

		 
	
int main() {	
// disable ADC for less power 
	ADCSRA &= ~_BV(ADEN); // ADC off 
	sei();
	
			/* Initialize MILLIS */
	        millis_init();
			_delay_ms(1000);
			/* Initialize MILLIS */

			
			
			/* Initialize UART */
			#ifdef DEBUG_SERIAL
			uart_init( ((F_CPU)/((UART_BAUD_RATE)*16l)-1)) ;
			_delay_ms(1000);
			#else
			power_usart0_disable();
			#endif
			/* Initialize UART */
		
	

			
		
			// node id, rfband, group id
			rf12_initialize(2, RF12_868MHZ, 14);
			// see http://tools.jeelabs.org/rfm12b
		

			#ifdef DEBUG_SERIAL
			log_s("initialized!");
			_delay_ms(1000);
			#endif
			
			// initialised
			
				
				
				
	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
	

	} // end main

	
	
	}
	