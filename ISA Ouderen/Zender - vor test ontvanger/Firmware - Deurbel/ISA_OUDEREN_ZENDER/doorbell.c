/*
 * doorbell.c
 *
 * Created: 7/19/2014 4:01:55 PM
 *  Author: arnefrankmauer
 */ 


#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "doorbell.h"

uint8_t triggered = 0; // if triggered is one, send package and reset this byte
uint8_t doorbell_last_state = 1; // last state 1 on startup, so avr gets into deep sleep!


/**
 * Initialize doorbell interrupt
 *
 * @param i The specified uint16_t
 */
void doorbell_init() {
	DDRD &= ~_BV(DDD3);
	EICRA &= ~(1<<ISC11) | (1<<ISC10);	// Trigger INT1 on low level
	doorbell_enable_interrupt();
}

void doorbell_disable_interrupt(){
	EIMSK &= ~(1<<INT1);					// Enable INT1
}

void doorbell_enable_interrupt(){
	EIMSK |= (1<<INT1);					// Enable INT1
}

void doorbell_interrupt(){
		//doorbell_last_state = 1;
		
		if(!(PIND & (1 << 3))){ // doorbell_last_state == 0
		doorbell_last_state = 1;
		triggered = 1;
		doorbell_disable_interrupt();
		}
		
}


ISR(INT1_vect) {
	doorbell_interrupt();
}
