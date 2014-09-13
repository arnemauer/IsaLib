/*
 * delay.c
 *
 * Created: 23-11-2013 22:07:38
 *  Author: Annie
 */ 
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#ifndef delay_c
#define delay_c


void var_delay_ms(uint16_t count) {
	while(count--) {
		_delay_ms(1);

	}
}

void var_delay_us(uint16_t count) {
	while(count--) {
		_delay_us(1);

	}
}

#endif