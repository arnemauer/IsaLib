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

#include "ISA_GATEWAY_RF12MOD.h"
#include "binary.h"
#include "uart.c"
#include "log.h"
#include "log.c"
#include "RF12.cpp"

#include "GLCD_SED1531/SED1531.h"
#include "GLCD_SED1531/proportional_font.h"
#include "GLCD_SED1531/fixed_font.h"
#include <avr/pgmspace.h>

SED1531 glcd;

// You will need to initialize the radio by telling it what ID it has and what network it's on
// The NodeID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
// By default the SPI-SS line used is D10 on Atmega328. You can change it by calling .SetCS(pin) where pin can be {8,9,10}
#define NODEID        2  //network ID used for this unit
#define NETWORKID    99  //the network ID we are on
#define GATEWAYID     1  //the node ID we're sending to

#define ACK_TIME     50  // # of ms to wait for an ack
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry

char payload[] = "ARNEMAUER";
bool requestACK=true;
//SED1531 glcd;

void setup(){
//	cli(); // Turn all interrupts off!
	sei(); // Turn interrupts on.
		
	_delay_ms(4000); // Wait for startup!
	
	// SPI CS INIT - set all CS lines high!	
	DDR_RFM_CS |= (1 << BIT_RFM_CS); // Set RFM12B CS to output
	PORT_RFM_CS |= (1 << BIT_RFM_CS);  // Pull RFM12B CS high
	
	DDR_MEM_SS |= (1 << BIT_MEM_SS); // Set memory CS to output
	PORT_MEM_SS |= (1 << BIT_MEM_SS);  // Pull memory CS high		
		
	DDR_ETH_SS |= (1 << BIT_ETH_SS); // Set Ethernet CS to output
	PORT_ETH_SS |= (1 << BIT_ETH_SS);  // Pull Ethernet CS high		
	// SPI CS INIT - set all CS lines high!
	
			
	// SET ALL LED TO OUTPUT	
	LED_433RECEIVE_DDR	|= (1 << LED_433RECEIVE_BIT); // set output	
	LED_433SEND_DDR		|= (1 << LED_433SEND_BIT); // set output	
	LED_868RECEIVE_DDR	|= (1 << LED_868RECEIVE_BIT); // set output	
	LED_868SEND_DDR		|= (1 << LED_868SEND_BIT); // set output
	LED_DCF77_DDR		|= (1 << LED_DCF77_BIT); // set output	
	LED_LCD_DDR			|= (1 << LED_LCD_BIT); // set output	
	// SET ALL LED TO OUTPUT	
	
	// TEST LEDS
	blinkAllLeds(true);	_delay_ms(1000); blinkAllLeds(false);
	_delay_ms(500);	blinkAllLeds(true); _delay_ms(1000); blinkAllLeds(false);
	// TEST LEDS

	/* Initialize UART */
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)); 

	/* Initialize RFM12B*/		
	rf12_initialize(NODEID, RF12_868MHZ, NETWORKID); // see http://tools.jeelabs.org/rfm12b
	rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
	   
	   
	// Initialize library
	millis_init();

	LED_LCD_PORT			|= (1 << LED_LCD_BIT); // set output
/*
	GLCD_IO_PIN_A0_1();
	GLCD_IO_PIN_RW_1();
	GLCD_IO_PIN_E_1();
	GLCD_IO_DATA_DIR_OUTPUT();
	GLCD_IO_DATA_OUTPUT(0b11111111);
	
*/
	
	//glcd.init();
	//glcd.setFont(&proportional_font);
	//glcd.clearDisplay();
	//uint8_t ret = glcd.drawPixel(GLCD_INDICATOR_2, 48, 2);
	//uart0_putc(ret);
	//glcd.drawLine(1, 1, 100,100, GLCD_COLOR_FILLED);

	glcd.init();
	glcd.setFont(&proportional_font);
	glcd.clearDisplay();
	
   
	log_s("ISA GATEWAY \n\r");
	log_s("STARTUP OK! \n\r");		
}



int main(void)
{
	setup();
    while(1)
    {
   

   _delay_ms(1000);
   //for (byte scroll=0;scroll<4;scroll++){
	 //  glcd.setIndicator(2, GLCD_INDICATOR_2);
	  // glcd.command(GLCD_CMD_INITIAL_DISPLAY_LINE(scroll));

	glcd.setCursor(25,5*8);
	//glcd.print(F("Hello world!"));
	glcd.drawText_P(8*0,14,&fixed_font,PSTR("Arduino Pong"));
	_delay_ms(2000);
	//for (byte scroll=0;scroll<49;scroll++){
	//	glcd.command(GLCD_CMD_INITIAL_DISPLAY_LINE(scroll));
	//	_delay_ms(150);
	//}
	
	      	log_s("ALIVE! \n");
	   //_delay_ms(150);
   
   
   	   //   	log_s("ALIVE! \n");
   	    //  	_delay_ms(1000);
   
//   */
  // 	log_s("ALIVE! \n");
 //  	_delay_ms(500); // Wait for startup!
    }
}







   
   
   /*
    while (!rf12_canSend())
    rf12_recvDone();
    log_s("Versturen: [");
	log_s(payload);
	log_s("]...");
    rf12_sendStart(0, &payload, sizeof payload);
    // set the sync mode to 2 if the fuses are still the Arduino default
    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
    rf12_sendWait(0);
	
	//if(waitForAck){
//		log_s("Antwoord terug :) \n");
	//}else{
	//	log_s("GEEN! Antwoord terug :'( \n");
	//}
	
	log_s("Verstuurd! \n");
    rf12_sleep(RF12_SLEEP);
	
	log_s("RFM12B: Sleep\n");
    _delay_ms(5000); // WACHTEN!
	
	rf12_sleep(RF12_WAKEUP);
	log_s("RFM12B: Wake up\n");
   
   
   
   
   /*  RECEIIVEEE!!!!
    if (rf12_recvDone()) {
	    byte n = rf12_len;
	    if (rf12_crc == 0) {
		   log_s("OK");
		    } else {
		    log_s(" ?");
		    n = 20;
	 
	 //   if (config.group == 0) {
		//    log_s("G ");
		//   uart0_putc( rf12_grp);
	    }
	   log_s("  ");
	    uart0_putc( rf12_hdr);
	    for (byte i = 0; i < n; ++i) {
		   log_s("   ");
		    uart0_putc( rf12_data[i]);
	    }
	    log_s("\n\r");
	} REECEEIIIVVEEE */
		
		
		



// wait a few milliseconds for proper ACK, return true if received
static bool waitForAck() {
	millis_t now = millis();
	while (millis() - now <= ACK_TIME){
		 if (rf12_recvDone() && rf12_crc == 0 &&
		 // see http://talk.jeelabs.net/topic/811#post-4712
		 rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | NODEID))
		 return true; uart0_putc(rf12_hdr);
		 set_sleep_mode(SLEEP_MODE_IDLE);
		 sleep_mode();
	}
	return false;
}

// wait a few milliseconds for proper ACK, return true if received
void blinkAllLeds(bool set) {
if(set){
		LED_433RECEIVE_PORT		|= (1 << LED_433RECEIVE_BIT); // set output
		LED_433SEND_PORT		|= (1 << LED_433SEND_BIT); // set output
		LED_868RECEIVE_PORT		|= (1 << LED_868RECEIVE_BIT); // set output
		LED_868SEND_PORT		|= (1 << LED_868SEND_BIT); // set output
		LED_DCF77_PORT			|= (1 << LED_DCF77_BIT); // set output
		LED_LCD_PORT			|= (1 << LED_LCD_BIT); // set output
}else{
		LED_433RECEIVE_PORT		&= ~(1 << LED_433RECEIVE_BIT); // set output
		LED_433SEND_PORT		&= ~(1 << LED_433SEND_BIT); // set output
		LED_868RECEIVE_PORT		&= ~(1 << LED_868RECEIVE_BIT); // set output
		LED_868SEND_PORT		&= ~(1 << LED_868SEND_BIT); // set output
		LED_DCF77_PORT			&= ~(1 << LED_DCF77_BIT); // set output	
		LED_LCD_PORT			&= ~(1 << LED_LCD_BIT); // set output	
}
}
