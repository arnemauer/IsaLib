/*
 * ISA_GATEWAY_QUADROFLY.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
 */ 


// GEBLEVEN BIJ SPI INIT, RFM INITIALISE funtie die verwijst naar SPIINIT();
// Kijken of bij elke build wel een nieuwe hex wordt gemaakt!
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

#include "Deurbelzender.h"
#include "binary.h"
#include "uart.c"
#include "log.h"
#include "log.c"
#include "RF12.cpp"



// You will need to initialize the radio by telling it what ID it has and what network it's on
// The NodeID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
// By default the SPI-SS line used is D10 on Atmega328. You can change it by calling .SetCS(pin) where pin can be {8,9,10}
#define NODEID        2  //network ID used for this unit
#define NETWORKID    11  //the network ID we are on
#define GATEWAYID     1  //the node ID we're sending to

#define ACK_TIME     50  // # of ms to wait for an ack
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry

char payload[] = "892511";
bool requestACK=true;
byte belAan = 1;
bool sendreport = false;
//SED1531 glcd;

void setup(){
//	cli(); // Turn all interrupts off!
	sei(); // Turn interrupts on.
		
	_delay_ms(4000); // Wait for startup!
	
			
	// SET ALL LED TO OUTPUT	
	LED_DDR	|= (1 << LED_BIT); // set output
		
	
	LED_PORT |= (1 << LED_BIT);  // led aan
	_delay_ms(1000);
	LED_PORT &= ~(1 << LED_BIT);  // led uit!
	
	/* Initialize UART */
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)); 
	//uart0_puts("U");
		_delay_ms(1000);
		
	/* Initialize RFM12B*/		
	rf12_initialize(NODEID, RF12_868MHZ, NETWORKID); // see http://tools.jeelabs.org/rfm12b
	rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
	
	log_s("ISA GATEWAY \n\r");
	
	// INT 1 - BELMELDER!   
   	BEL_MELDER_DDR |= (1<<BEL_MELDER_BIT);		// Set PD2 as input (Using for interupt INT0)
	   	BEL_MELDER_PORT |= (1<<BEL_MELDER_BIT);		// Enable PD2 pull-up resistor

	EIMSK |= (1<<INT1);					// Enable INT0
	EICRA |= (1<<ISC10);	// Trigger INT0 on rising edge
	EICRA &= ~(1<<ISC11);	// Trigger INT0 on rising edge
	
	// Initialize library
	millis_init();
   
	log_s("ISA GATEWAY \n\r");
	log_s("STARTUP OK! \n\r");		
		_delay_ms(1000);
		
}


ISR(INT1_vect) {
	//_delay_ms(200); // Software debouncing control
	log_s("I");
	if((BEL_MELDER_PIN & (1<<BEL_MELDER_BIT))==0){ // 0 = aan, 1 = uit
		//log_s("AAN ");
		
		if(belAan == 0){
			log_s("AAN1");
			belAan = 1;
			sendreport = true; // stuur melding
			// led aan!
			LED_PORT |= (1 << LED_BIT);  // led aan	
		}
	}else {
		if(belAan == 1){
		// bel variable uit!
		belAan = 0;
		// led uit!
		LED_PORT &= ~(1 << LED_BIT);  // led uit!
		
	}
	}
}



int main(void)
{
	setup();
    while(1)
    {

   
   log_s(".");
   _delay_ms(100);
if(sendreport){ doReport(); }

	   
    }
}





// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
	 	log_s("X");
 rf12_sleep(RF12_WAKEUP);
 //while (!rf12_canSend())
 //rf12_recvDone();
 
  //  log_s("Versturen: [");
   // uart0_puts_p(belAan);
   // log_s("]...");
    rf12_sendStart(0, &payload, sizeof payload);
    // set the sync mode to 2 if the fuses are still the Arduino default
    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
    rf12_sendWait(1);
    /*
    if(waitForAck){
	    log_s("Antwoord terug :) \n");
	    }else{
	    log_s("GEEN! Antwoord terug :'( \n");
    }
	 rf12_sleep(RF12_SLEEP);
	 set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	 sleep_mode();
*/
	sendreport = false;
}




// wait a few milliseconds for proper ACK, return true if received
static bool waitForAck() {
	millis_t now = millis();
	while (millis() - now <= ACK_TIME){
		 if (rf12_recvDone() && rf12_crc == 0 &&
		 // see http://talk.jeelabs.net/topic/811#post-4712
		 rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | NODEID))
		 return true; 
		 set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		 sleep_mode();
	}
	return false;
}

