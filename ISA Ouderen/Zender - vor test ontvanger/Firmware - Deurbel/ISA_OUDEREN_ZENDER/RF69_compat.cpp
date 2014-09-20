#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "RF12.h"
#include "RF69.h"

	
volatile uint16_t rf69_crc;
volatile uint8_t rf69_buf[72];

#define byte uint8_t
static byte nodeid; // only used in the easyPoll code

// same as in RF12
#define RETRIES     8               // stop retrying after 8 times
#define RETRY_MS    1000            // resend packet every second until ack'ed

// same as in RF12
static uint8_t ezInterval;          // number of seconds between transmits
static uint8_t ezSendBuf[RF12_MAXDATA]; // data to send
static char ezSendLen;              // number of bytes to send
static uint8_t ezPending;           // remaining number of retries
static long ezNextSend[2];          // when was last retry [0] or data [1] sent

// void rf69_set_cs (uint8_t pin) {
// }

// void rf69_spiInit () {
// }




uint8_t rf69_initialize (uint8_t id, uint8_t band, uint8_t group, uint16_t off) {
    uint8_t freq = 0;
    switch (band) {
        case RF12_433MHZ: freq = 43; break;
        case RF12_868MHZ: freq = 86; break;
        case RF12_915MHZ: freq = 90; break;
    }
    RF69::setFrequency(freq * 10000000L + band * 2500L * off);
    RF69::group = group;
    RF69::nodeID = id;
    _delay_ms(20); // needed to make RFM69 work properly on power-up
   
   
    if (RF69::nodeID != 0){
      //  attachInterrupt(0, RF69::interrupt_compat, RISING);
			 EICRA |= (1 << ISC00) | (1 << ISC01); // trigger on rising edge
			 EIMSK |= (1 << INT0); // enable int0 interrupt //bitSet(EIMSK, INT0);
			 

    }else{
        EIMSK &= ~(1 << INT0); // disable pcint0 interrupt  //bitClear(EIMSK, INT0);
	}
	


	
    RF69::configure_compat();
    return nodeid = id;
}








// void rf69_onOff (uint8_t value) {
// }



// char rf69_lowbat () {
// }

// same as in RF12
void rf69_easyInit (uint8_t secs) {
    ezInterval = secs;
}



// void rf69_encrypt (const uint8_t*) {
// }

// uint16_t rf69_control (uint16_t cmd) {
// }
