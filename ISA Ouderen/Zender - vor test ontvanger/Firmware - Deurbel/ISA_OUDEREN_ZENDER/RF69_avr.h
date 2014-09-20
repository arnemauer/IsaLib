#include <avr/interrupt.h>
#include <util/crc16.h>
#include "config.h"

// prog_uint8_t appears to be deprecated in avr libc, this resolves it for now
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>

#define ROM_UINT8       const prog_uint8_t
#define ROM_READ_UINT8  pgm_read_byte
#define ROM_DATA        PROGMEM

#define IRQ_ENABLE      sei()


static void spiConfigPins () {
	
		DDR_SPI |= (1 << BIT_MOSI) | (1 << BIT_SCK) | (1 << BIT_RFM_CS);  // SDI, SCK,  CS output
		PORT_RFM_CS |= (1 << BIT_RFM_CS);  // Pull RFM12B CS high
		DDR_SPI &= ~(1 << BIT_MISO);  // SDO  input
	
}


#ifndef EIMSK
#define EIMSK GIMSK // ATtiny
#endif

struct PreventInterrupt {
    PreventInterrupt () { EIMSK &= ~ _BV(INT0); }
    ~PreventInterrupt () { EIMSK |= _BV(INT0); }
};

static void spiInit (void) {
    spiConfigPins();
    
#ifdef SPCR    
    SPCR = _BV(SPE) | _BV(MSTR);
	#if F_CPU > 10000000
	// use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see rf12_xfer)
	SPSR |= _BV(SPI2X);
	#endif

#else
    USICR = _BV(USIWM0); // ATtiny
#endif    
    
    // pinMode(RFM_IRQ, INPUT);
    // digitalWrite(RFM_IRQ, 1); // pull-up
			DDR_RFM_IRQ &= ~(1 << BIT_RFM_IRQ);  // RFM12 IRQ  input
			PORT_RFM_IRQ |= (1 << BIT_RFM_IRQ); // digitalWrite(RFM_IRQ, 1); // pull-up
			
}

static uint8_t spiTransferByte (uint8_t out) {
#ifdef SPDR
    SPDR = out;
    while (!(SPSR & _BV(SPIF)))
        ;
    return SPDR;
#else
    USIDR = out; // ATtiny
    uint8_t v1 = _BV(USIWM0) | _BV(USITC);
    uint8_t v2 = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
    for (uint8_t i = 0; i < 8; ++i) {
        USICR = v1;
        USICR = v2;
    }
    return USIDR;
#endif
}

static uint8_t spiTransfer (uint8_t cmd, uint8_t val) {
   PORT_RFM_CS &= ~(1<<BIT_RFM_CS); //clear CS
    spiTransferByte(cmd);
    uint8_t in = spiTransferByte(val);
    PORT_RFM_CS |= (1 << BIT_RFM_CS); //  bitSet(SS_PORT, cs_pin);
    return in;
}
