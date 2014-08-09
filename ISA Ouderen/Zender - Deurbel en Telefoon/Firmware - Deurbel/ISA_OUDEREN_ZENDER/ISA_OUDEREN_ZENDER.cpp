/*
 * ISA Ouderen Zender.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
 */ 


#define DEBUG_SERIAL

#define byte uint8_t
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
//#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/power.h>
#include <string.h>
#include <avr/pgmspace.h> // for progmem / ram declarations
#include <avr/sleep.h>
#include <util/atomic.h>

#include "ISA_OUDEREN_ZENDER.h"
#include "millis.h"
#include "doorbell.h"
#include "RFM69registers.h"
#include "RFM69.h"
//#include "RF69_avr.h"
//#include "RF12.h"
//#include "RF69.h"

//#include "toneAC.h"
extern "C" {
	#include "uart.h"
	#include "log.h"
};

#define NODE_ID				3
#define NETWORKID			14  //the same on all nodes that talk to each other
#define DESTINATION_NODE_ID 2 

#define RETRY_LIMIT     20   // maximum number of times to retry
#define ACK_TIME        50  // number of milliseconds to wait for an ack

//struct {
//	uint8_t house_code: 0x99;     // housecode: 0x99 =153
//	uint8_t alarm : 0x18;  // alarm array: 00011000 // not used, not used, not used, start (1) or stop(0), doorbell, phone, fire, help;
//} payload;

//uint8_t payload[2] = {0x99, 0x18}; 
uint8_t pre_payload[50] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
uint8_t payload[2] = {0x99, 0x18}; // 10 DUMMY BYTES

RFM69 radio;
	

	ISR(INT0_vect) {
		RFM69::isr0();
	}
	
	
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
				uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)) ;
				log_s("UART OK\r");
				_delay_ms(1000);
			#else
				power_usart0_disable();
			#endif
				

				
			/* Initialize RFM12B */
			#ifdef DEBUG_SERIAL
				log_s("RF12 INIT..."); _delay_ms(1000);
			#endif
			
		
		radio.initialize(RF69_868MHZ, NODE_ID, NETWORKID); // node id, rfband, group id

			#ifdef DEBUG_SERIAL
				log_s(" OK\r"); _delay_ms(1000);
			#endif

		#ifdef DEBUG_SERIAL
		log_s("DOORBELL INIT..."); _delay_ms(1000);
		#endif
		
			doorbell_init();
		#ifdef DEBUG_SERIAL
			log_s(" OK \r"); _delay_ms(1000);
		#endif

				
	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).



	if(triggered){
		millis_resume();
			#ifdef DEBUG_SERIAL
			log_s("Send Package...");
			_delay_ms(30);
			#endif
			
		// send message
		//sendpackage();
		
		uint8_t response = radio.sendWithRetry(DESTINATION_NODE_ID, payload, sizeof(payload), RETRY_LIMIT, ACK_TIME);
		
		if(response){
			log_s("ACK OK");
			_delay_ms(30);
		}else{
			log_s("NO ACK");
			_delay_ms(30);
		}
		
		//radio.send(DESTINATION_NODE_ID, payload, sizeof(payload), 1);
		
		triggered = 0; // reset trigger
		
	}
	
	if((PIND & (1 << 3)) && doorbell_last_state == 1){ // if pin is high again
		
					#ifdef DEBUG_SERIAL
					log_s("\rPWR_DOWN\r");
					 _delay_ms(30);
					#endif
		cli();			
		millis_pause();
		doorbell_last_state = 0;
		doorbell_enable_interrupt();
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
	     sei();
	    sleep_cpu();
	     sleep_disable();

		   
	}else{
			#ifdef DEBUG_SERIAL
		//	log_s("Wait for doorbell to go high again... Sleep... (PWR_SAVE) \r"); _delay_ms(100);
			#endif
		set_sleep_mode(SLEEP_MODE_PWR_SAVE); // keep millis timer enabled, so avr wakes every ms
		sleep_mode();
				
		
	}
	
	
	}



	 // doorbell_enable_interrupt
	  
			_delay_ms(5000);			
	} // end main

	
	

	
void sendpackage(){ 
	/*
	
	    for (byte i = 0; i < RETRY_LIMIT; ++i) {
			
		   		//    radio.sleep(RF12_WAKEUP);

			//rf69_sendNow( RF12_HDR_DST | DESTINATION_NODE_ID, &pre_payload, sizeof(pre_payload));
			//rf69_sendWait(1); //idle
			// _delay_ms(2);
			
			//DESTINATION_NODE_ID
		  //  radio.sendNow(6, 1, 0, &payload, sizeof(payload));
		 //  radio.sendWait(1); //idle
			
			//byte acked = radio.sendWithRetry(DESTINATION_NODE_ID, &payload, sizeof(payload), 3, 25);
			
			radio.send(DESTINATION_NODE_ID, &payload, sizeof(payload), 1);
			
		  //  byte acked = waitForAck();
		//     RF69::sleep(1);
			 
			 
		    if (acked) {
			  #ifdef DEBUG_SERIAL
			   log_s("ACK OK\r");
			  _delay_ms(20);
			   #endif
				
			    // reset scheduling to start a fresh measurement cycle
			  //  scheduler.timer(MEASURE, MEASURE_PERIOD);
			    return;
		    }
			
			 #ifdef DEBUG_SERIAL
			// log_s("NO ACK... RETRY!\r");
		//	_delay_ms(20);
			 #endif
		    
		    _delay_ms(RETRY_PERIOD);
	    }
		
		 #ifdef DEBUG_SERIAL
		 log_s("NO ACK... Failed sending package! \r");
		 _delay_ms(20);
		 #endif
		 
		 */
	
	
	}
	
	
		
	
	// wait a few milliseconds for proper ACK to me, return true if indeed received
	static byte waitForAck() {
		unsigned long long ACK_WAIT_TILL = millis_get() + ACK_TIME; 
		while (millis_get() <= ACK_WAIT_TILL) {
			if (radio.receiveDone()) { // a packet has been received
				
				log_s("\rPAKKET ONTVANGEN - CRC=");
				//uart0_putc(rf12_crc);
				log_s("; RF12HDR=");

				_delay_ms(400);
				 	//if (rf12_crc == 0 && rf12_hdr == ( RF12_HDR_CTL | DESTINATION_NODE_ID)){
						return 1;
					//}
					
			set_sleep_mode(SLEEP_MODE_IDLE);
			sleep_mode();
			}
		}
		return 0;
	}
	
	
