/*
 * ISA Ouderen Zender.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>

#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

int main (void)
{
	char ReceivedByte;

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */

	UBRR0H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
	UBRR0L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

	for (;;) // Loop forever
	{
		while ((UCSR0A & (1 << RXC0)) == 0) {}; // Do nothing until data have been received and is ready to be read from UDR
		ReceivedByte = UDR0; // Fetch the received byte value into the variable "ByteReceived"

		while ((UCSR0A & (1 << UDRE0)) == 0) {}; // Do nothing until UDR is ready for more data to be written to it
		UDR0 = ReceivedByte; // Echo back the received byte back to the computer
	}
}

/*

#define DEBUG_SERIAL

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
//#include "RF69_compat.h"
//#include "RF12.h"
//#include "RF69.h"
#include "millis.h"
//#include "toneAC.h"
extern "C" {
	#include "uart.h"
	#include "log.h"
};

	

	 
	
int main() {	
// disable ADC for less power 
//	ADCSRA &= ~_BV(ADEN); // ADC off 
//	sei();
	
	
	
			/* Initialize MILLIS */
	//        millis_init();
	//		_delay_ms(1000);
			/* Initialize MILLIS */

			
			
			/* Initialize UART */
			//#ifdef DEBUG_SERIAL
	//		uart_init( ((F_CPU)/((UART_BAUD_RATE)*16l)-1)) ;
		//	_delay_ms(1000);
		//	#else
		//	power_usart0_disable();
		//	#endif
			/* Initialize UART */
		
		
		//		uart0_puts("initialized!");
				
				
	/*
		DDRC |= (1 << 1);
			PORTC &= ~(1 << 1);
		
			// node id, rfband, group id
			rf12_initialize(2, RF12_868MHZ, 14);
			// see http://tools.jeelabs.org/rfm12b
		

			#ifdef DEBUG_SERIAL
			log_s("initialized!");
			_delay_ms(1000);
			#endif
			
			// initialised
			
				
				
				
	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
	    ++payload;

	  rf12_sleep(RF12_WAKEUP);
	  rf12_sendNow(3, &payload, sizeof payload);
	  rf12_sendWait(0); //idle
	  rf12_sleep(RF12_SLEEP);

			_delay_ms(5000);
			
	} // end main

	*/
	
	//}
	