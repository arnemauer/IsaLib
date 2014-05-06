/*
 * timer2.c
 *
 * Created: 5/6/2014 8:25:38 PM
 *  Author: arnefrankmauer
 */ 

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>

// Initialise library
void timer2_init()
{
	// Timer settings
	TCCR2A = _BV(WGM21); // prescaler 128
	TCCR2B = _BV(CS22)|_BV(CS20);
	
	OCR2A = ((F_CPU / 128) / 1000);
		
	TIMSK2 &= ~_BV(OCIE2A);
	power_timer2_disable(); // powerdown timer2!
	
}


// Turn on timer and resume interrupts
void timer2_resume()
{
	power_timer2_enable();
	TIMSK2 |= _BV(OCIE2A);
}

// Pause interrupts and turn off timer to save power
void timer2_pause()
{
	TIMSK2 &= ~_BV(OCIE2A);
	power_timer2_disable();
}


			