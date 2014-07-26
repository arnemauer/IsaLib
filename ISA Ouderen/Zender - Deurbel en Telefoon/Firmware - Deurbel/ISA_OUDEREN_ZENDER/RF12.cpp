/// @file
/// RFM12B driver implementation
// 2009-02-09 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
#include "config.h"
#include "RF12.h"
#include "binary.h"
#include <string.h>
#include <avr/io.h>
#include <util/crc16.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "millis.h"
extern "C" {
#include "uart.h" 
}
// pin change interrupts are currently only supported on ATmega328's
// #define PINCHG_IRQ 1    // uncomment this to use pin-change interrupts

// maximum transmit / receive buffer: 3 header + data + 2 crc bytes
#define RF_MAX   (RF12_MAXDATA + 5)

// pins used for the RFM12B interface - yes, there *is* logic in this madness:
//
//  - leave RFM_IRQ set to the pin which corresponds with INT0, because the
//    current driver code will use attachInterrupt() to hook into that
//  - (new) you can now change RFM_IRQ, if you also enable PINCHG_IRQ - this
//    will switch to pin change interrupts instead of attach/detachInterrupt()
//  - use SS_DDR, SS_PORT, and SS_BIT to define the pin you will be using as
//    select pin for the RFM12B (you're free to set them to anything you like)
//  - please leave SPI_SS, SPI_MOSI, SPI_MISO, and SPI_SCK as is, i.e. pointing
//    to the hardware-supported SPI pins on the ATmega, *including* SPI_SS !


// RF12 command codes
#define RF_TXREG_WRITE  0xB800
#define RF_WAKEUP_TIMER 0xE000
#define RF_RECV_CONTROL 0x94A0

// RF12 status bits
#define RF_FIFO_BIT     0x8000
#define RF_POR_BIT      0x4000
#define RF_OVF_BIT      0x2000
#define RF_WDG_BIT      0x1000
#define RF_LBD_BIT      0x0400
#define RF_RSSI_BIT     0x0100

// bits in the node id configuration byte
#define NODE_BAND       0xC0        // frequency band
#define NODE_ACKANY     0x20        // ack on broadcast packets if set
#define NODE_ID         0x1F        // id of this node, as A..Z or 1..31

// transceiver states, these determine what to do with each interrupt
enum {
	TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE,
	UNINITIALIZED, POR_RECEIVED,	// indicates uninitialized RFM12b and Power-On-Reset
	TXRECV,
	TXPRE1, TXPRE2, TXPRE3, TXSYN1, TXSYN2,
};


static uint8_t nodeid;              // address of this node
static uint8_t group;               // network group
static uint8_t band;				// network band
static volatile uint8_t rxfill;     // number of data bytes in rf12_buf
static volatile int8_t rxstate;     // current transceiver state
volatile uint16_t rfmstate;         // current power management setting of the RFM12 module
volatile uint16_t state;            // last seen rfm12b state
volatile uint8_t rf12_gotwakeup;	// 1 if there was a wakeup-call from RFM12
uint8_t drssi;                      // digital rssi state (see binary search tree below and rf12_getRSSI()
uint8_t drssi_bytes_per_decision;   // number of bytes required per drssi decision

const uint8_t drssi_dec_tree[] = {
	/* state,drssi,final, returned, up,      dwn */
	/*  A,   0,    no,    0001 */  3 << 4 | 4,
	/*  *,   1,    no,     --  */  0 << 4 | 2, // starting value
	/*  B,   2,    no,    0101 */  5 << 4 | 6
	/*  C,   3,   yes,    1000 */
	/*  D,   4,   yes,    1010 */
	/*  E,   5,   yes,    1100 */
	/*  F,   6,   yes,    1110 */
};  //                    \ Bit 1 indicates final state, others the signal strength


#define RETRIES     8               // stop retrying after 8 times
#define RETRY_MS    1000            // resend packet every second until ack'ed

static uint8_t ezInterval;          // number of seconds between transmits
static uint8_t ezSendBuf[RF12_MAXDATA]; // data to send
static char ezSendLen;              // number of bytes to send
static uint8_t ezPending;           // remaining number of retries
static long ezNextSend[2];          // when was last retry [0] or data [1] sent
static uint8_t fixedLength;				// length of non-standard packages
volatile uint16_t rf12_crc;         // running crc value
volatile uint8_t rf12_buf[RF_MAX];  // recv/xmit buf, including hdr & crc bytes
long rf12_seq;                      // seq number of encrypted packet (or -1)

static uint32_t seqNum;             // encrypted send sequence number
static uint32_t cryptKey[4];        // encryption key to use
void (*crypter)(uint8_t);           // does en-/decryption (null if disabled)


// interrupts need to be disabled in a few spots
// do so by disabling the source of the interrupt, not all interrupts

static void blockInterrupts () {
	EIMSK &= ~(1 << INT0); // disable pcint0 interrupt  //bitClear(EIMSK, INT0);
}

static void allowInterrupts () {
	 EIMSK |= (1 << INT0); // enable pcint1 interrupt //bitSet(EIMSK, INT0);
}


void rf12_spiInit () {

		DDR_SPI |= (1 << BIT_MOSI) | (1 << BIT_SCK) | (1 << BIT_RFM_CS);  // SDI, SCK,  CS output 
		PORT_RFM_CS |= (1 << BIT_RFM_CS);  // Pull RFM12B CS high
		DDR_SPI &= ~(1 << BIT_MISO);  // SDO  input

	#ifdef SPCR
	SPCR = _BV(SPE) | _BV(MSTR);
	#if F_CPU > 10000000
	// use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see rf12_xfer)
	SPSR |= _BV(SPI2X);
	#endif
	#else
	// ATtiny
	USICR = bit(USIWM0);
	#endif
	
	  DDR_RFM_IRQ &= ~(1 << BIT_RFM_IRQ);  // RFM12 IRQ  input
	  PORT_RFM_IRQ |= (1 << BIT_RFM_IRQ); // digitalWrite(RFM_IRQ, 1); // pull-up
}


// do a single byte SPI transfer
// only used from inside rf12_xfer and rf12_xferState which prevent race
// conditions, don't call without disabling interrupts first!
static uint8_t rf12_byte (uint8_t out) {
	#ifdef SPDR
	SPDR = out;
	// this loop spins 4 usec with a 2 MHz SPI clock
	while (!(SPSR & _BV(SPIF)))
	;
	return SPDR;
	#else
	// ATtiny
	USIDR = out;
	byte v1 = bit(USIWM0) | bit(USITC);
	byte v2 = bit(USIWM0) | bit(USITC) | bit(USICLK);
	#if F_CPU <= 5000000
	// only unroll if resulting clock stays under 2.5 MHz
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	USICR = v1; USICR = v2;
	#else
	for (uint8_t i = 0; i < 8; ++i) {
		USICR = v1;
		USICR = v2;
	}
	#endif
	return USIDR;
	#endif
}

/// @details
/// This call provides direct access to the RFM12B registers. If you're careful
/// to avoid configuring the wireless module in a way which stops the driver
/// from functioning, this can be used to adjust some settings.
/// See the RFM12B wireless module documentation.
static uint16_t rf12_xfer (uint16_t cmd) {
	blockInterrupts();

	// writing can take place at full speed, even 8 MHz works
	PORT_RFM_CS &= ~(1<<BIT_RFM_CS); //clear CS
	uint16_t res  = rf12_byte(cmd >> 8) << 8;
	res |= rf12_byte(cmd);
	PORT_RFM_CS |= (1 << BIT_RFM_CS); //  bitSet(SS_PORT, cs_pin);
	
	allowInterrupts();
	return res;
}


/// @details
/// Requests RFM12 state from RF module and reads back a waiting data byte if there is
/// any.
/// One of two commands accessing SPI, so interrupts disabled in here
/// @param *data Pointer to  byte where to write the received data to (if any)
static uint16_t rf12_xferState (uint8_t *data) {
	blockInterrupts();

	// writing can take place at full speed, even 8 MHz works
	//bitClear(SS_PORT, cs_pin);
	PORT_RFM_CS &= ~(1<<BIT_RFM_CS); //clear CS
	uint16_t res = rf12_byte(0x00) << 8;
	res |= rf12_byte(0x00);
	
	if (res & RF_FIFO_BIT && rxstate == TXRECV) {
		// slow down to under 2.5 MHz
		#if F_CPU > 10000000

		SPCR |= (1 << SPR0); //  bitSet(SPCR, SPR0);
		#endif
		
		*data = rf12_byte(0x00);
		
		#if F_CPU > 10000000
		SPCR &= ~(1 << SPR0); //    bitClear(SPCR, SPR0);
		#endif
	}
	
	PORT_RFM_CS |= (1 << BIT_RFM_CS); //  bitSet(SS_PORT, cs_pin);

	allowInterrupts();
	return res;
}


/// @details
/// This call provides direct access to the RFM12B registers. If you're careful
/// to avoid configuring the wireless module in a way which stops the driver
/// from functioning, this can be used to adjust frequencies, power levels,
/// RSSI threshold, etc. See the RFM12B wireless module documentation.
///
/// OBSOLETE! Use rf12_xfer instead.
///
/// This function does no longer return anything.
/// @param cmd RF12 command, topmost bits determines which register is affected.
uint16_t rf12_control(uint16_t cmd) {
	return rf12_xfer(cmd);
}


/// @details
/// Brings RFM12 in idle-mode.
static void rf12_idle() {
//	PORTB |= _BV(0); // pb0 aan

	rfmstate &= ~B11110000; // switch off synthesizer, transmitter, receiver and baseband
	rfmstate |=  B00001000; // make sure crystal is running
	rf12_xfer(rfmstate);
   //PORTB &= ~_BV(0); // pb0 uit
}


/// @details
/// Handles a RFM12 interrupt depending on rxstate and the status reported by the RF
/// module.
static void rf12_interrupt() {
	uint8_t in;
	state = rf12_xferState(&in);

	// data received or byte needed for sending
	if (state & RF_FIFO_BIT) {
		
		// RECEIVING - RECEIVING - RECEIVING!
		if (rxstate == TXRECV) {  // we are receiving

			if (rxfill == 0 && group != 0)
			rf12_buf[rxfill++] = group;

			rf12_buf[rxfill++] = in;
			rf12_crc = _crc16_update(rf12_crc, in);

		

		
			// do drssi binary-tree search
			if ( drssi < 3 && ((rxfill-2)%drssi_bytes_per_decision)==0 ) {// not yet final value
				// top nibble when going up, bottom one when going down
				//drssi = bitRead(state,8)
				drssi = ((state>>8)&0x01)
				? (drssi_dec_tree[drssi] & B1111)
				: (drssi_dec_tree[drssi] >> 4);
				if ( drssi < 3 ) {     // not yet final destination, set new threshold
					rf12_xfer(RF_RECV_CONTROL | drssi*2+1);
				}
			}

		
			// check if we got all the bytes (or maximum packet length was reached)
			if (fixedLength) {
				if (rxfill >= fixedLength || rxfill >= RF_MAX) {
					rf12_idle();
				}
			} else if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX) {
				rf12_idle();
			}
					
			// SENDING - SENDING - SENDING!
			} else {                  // we are sending
			uint8_t out;

			if (rxstate < 0) {
				uint8_t pos = 3 + rf12_len + rxstate++;
				out = rf12_buf[pos];
				rf12_crc = _crc16_update(rf12_crc, out);
			} else
			switch (rxstate++) {
				case TXSYN1: out = 0x2D; break;
				case TXSYN2: out = group; rxstate = - (2 + rf12_len); break;
				case TXCRC1: out = rf12_crc; break;
				case TXCRC2: out = rf12_crc >> 8; break;
				case TXDONE: rf12_idle(); // fall through
				default:     out = 0xAA;
			}

			rf12_xfer(RF_TXREG_WRITE | out);
		}
	}
	

	
	// power-on reset
	if (state & RF_POR_BIT) {
		rxstate = POR_RECEIVED;
	}

	// got wakeup call
	if (state & RF_WDG_BIT) {
		rf12_setWatchdog(0);
		rf12_gotwakeup = 1;
	}
	
	// fifo overflow or buffer underrun - abort reception/sending
	if (state & RF_OVF_BIT) {
		//PORTB |= _BV(0); // pb0 aan
		rf12_idle();
		//PORTB &= ~_BV(0); // pb0 uit
		rxstate = TXIDLE;
	}

}




static void rf12_recvStart () {
	//LED_868RECEIVE_DDR	|= (1 << LED_868RECEIVE_BIT); // set output	
	rxfill = rf12_len = 0;
	rf12_crc = ~0;
	#if RF12_VERSION >= 2
	if (group != 0)
	rf12_crc = _crc16_update(~0, group);
	#endif
	rxstate = TXRECV;
	drssi = 1;              // set drssi to start value
	rf12_xfer(RF_RECV_CONTROL | drssi*2+1);
	rfmstate |= B11011000; // enable crystal, synthesizer, receiver and baseband
	rf12_xfer(rfmstate);
}

//#include "RF12.h"
//#include <Ports.h> // needed to avoid a linker error :(

byte rf12_recvDone();

/// @details
/// The timing of this function is relatively coarse, because SPI transfers are
/// used to enable / disable the transmitter. This will add some jitter to the
/// signal, probably in the order of 10 µsec.
///
/// If the result is true, then a packet has been received and is available for
/// processing. The following global variables will be set:
///
/// * volatile byte rf12_hdr -
///     Contains the header byte of the received packet - with flag bits and
///     node ID of either the sender or the receiver.
/// * volatile byte rf12_len -
///     The number of data bytes in the packet. A value in the range 0 .. 66.
/// * volatile byte rf12_data -
///     A pointer to the received data.
/// * volatile byte rf12_crc -
///     CRC of the received packet, zero indicates correct reception. If != 0
///     then rf12_hdr, rf12_len, and rf12_data should not be relied upon.
///
/// To send an acknowledgement, call rf12_sendStart() - but only right after
/// rf12_recvDone() returns true. This is commonly done using these macros:
///
///     if(RF12_WANTS_ACK){
///        rf12_sendStart(RF12_ACK_REPLY,0,0);
///      }
/// @see http://jeelabs.org/2010/12/11/rf12-acknowledgements/
uint8_t rf12_recvDone () {
			
		
	if (rxstate == TXRECV) {
		if (fixedLength) {
				
				if (rxfill >= fixedLength || rxfill >= RF_MAX) {
					rxstate = TXIDLE;
					rf12_crc = 1; //it is not a standard packet
				//	LED_868RECEIVE_DDR	&= ~(1 << LED_868RECEIVE_BIT); // set output	
					return 1;
				}

		} else if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX) {
		
				rxstate = TXIDLE;
			
				if (rf12_len > RF12_MAXDATA) { 
					rf12_crc = 1; // force bad crc if packet length is invalid
				}
					
				if (!(rf12_hdr & RF12_HDR_DST) || (nodeid & NODE_ID) == 31 ||
				(rf12_hdr & RF12_HDR_MASK) == (nodeid & NODE_ID)) {
				
				
					if (rf12_crc == 0 && crypter != 0){
						crypter(0);
					} else {
						rf12_seq = -1;
					}
				
					return 1; // it's a broadcast packet or it's addressed to this node
				}
			
			
		}
	}
	

		
	if (rxstate == TXIDLE){
		rf12_recvStart();
	}
	
	
	return 0;
}


// return signal strength calculated out of DRSSI bit
uint8_t rf12_getRSSI() {
	return (drssi<3 ? drssi*2+2 : 8|(drssi-3)*2);
}

void rf12_setBitrate(uint8_t rate) {
	const long int decisions_per_sec = 900;
	rf12_xfer(0xc600 | rate);
	unsigned long bits_per_second = (10000000UL / 29UL / (1 + (rate & 0x7f)) / (1 + (rate >> 7) * 7));
	unsigned long bytes_per_second = bits_per_second / 8;
	drssi_bytes_per_decision = (bytes_per_second + decisions_per_sec - 1) / decisions_per_sec;
}

void rf12_setFixedLength(uint8_t packet_len) {
	fixedLength = packet_len;
	if (packet_len > 0) fixedLength++;  //add a byte for the groupid
}

/// @details
/// Call this when you have some data to send. If it returns true, then you can
/// use rf12_sendStart() to start the transmission. Else you need to wait and
/// retry this call at a later moment.
///
/// Don't call this function if you have nothing to send, because rf12_canSend()
/// will stop reception when it returns true. IOW, calling the function
/// indicates your intention to send something, and once it returns true, you
/// should follow through and call rf12_sendStart() to actually initiate a send.
/// See [this weblog post](http://jeelabs.org/2010/05/20/a-subtle-rf12-detail/).
///
/// Note that even if you only want to send out packets, you still have to call
/// rf12_recvDone() periodically, because it keeps the RFM12B logic going. If
/// you don't, rf12_canSend() will never return true.
uint8_t rf12_canSend () {
	// if (rxstate == TXRECV && rxfill == 0 && rf12_getRSSI() < 2) {
	// TODO listen-before-send disabled until we figure out how to do it right
	if (rxstate == TXRECV && rxfill == 0) {
		//PORTB |= _BV(0); // pb0 aan
		rf12_idle();
		//PORTB &= ~_BV(0); // pb0 uit
		rxstate = TXIDLE;
		return 1;
	}
	return 0;
}

void rf12_sendStart (uint8_t hdr) {
	rf12_hdr = hdr & RF12_HDR_DST ? hdr :
	(hdr & ~RF12_HDR_MASK) + (nodeid & NODE_ID);
	if (crypter != 0)
	crypter(1);
	
	rf12_crc = ~0;
	#if RF12_VERSION >= 2
	rf12_crc = _crc16_update(rf12_crc, group);
	#endif
	rxstate = TXPRE1;
	rfmstate |= B00111000; // enable crystal, synthesizer and transmitter
	rf12_xfer(rfmstate);
	// no need to feed bytes, RFM module requests data using interrupts
}

/// @details
/// Switch to transmission mode and send a packet.
/// This can be either a request or a reply.
///
/// Notes
/// -----
///
/// The rf12_sendStart() function may only be called in two specific situations:
///
/// * right after rf12_recvDone() returns true - used for sending replies /
///   acknowledgements
/// * right after rf12_canSend() returns true - used to send requests out
///
/// Because transmissions may only be started when there is no other reception
/// or transmission taking place.
///
/// The short form, i.e. "rf12_sendStart(hdr)" is for a special buffer-less
/// transmit mode, as described in this
/// [weblog post](http://jeelabs.org/2010/09/15/more-rf12-driver-notes/).
///
/// The call with 4 arguments, i.e. "rf12_sendStart(hdr, data, length, sync)" is
/// deprecated, as described in that same weblog post. The recommended idiom is
/// now to call it with 3 arguments, followed by a call to rf12_sendWait().
/// @param hdr The header contains information about the destination of the
///            packet to send, and flags such as whether this should be
///            acknowledged - or if it actually is an acknowledgement.
/// @param ptr Pointer to the data to send as packet.
/// @param len Number of data bytes to send. Must be in the range 0 .. 65.
void rf12_sendStart (uint8_t hdr, const void* ptr, uint8_t len) {
	rf12_len = len;
	memcpy((void*) rf12_data, ptr, len);
	rf12_sendStart(hdr);
}

/// @deprecated Use the 3-arg version, followed by a call to rf12_sendWait.
void rf12_sendStart (uint8_t hdr, const void* ptr, uint8_t len, uint8_t sync) {
	rf12_sendStart(hdr, ptr, len);
	rf12_sendWait(sync);
}

/// @details
/// Wait until transmission is possible, then start it as soon as possible.
/// @note This uses a (brief) busy loop and will discard any incoming packets.
/// @param hdr The header contains information about the destination of the
///            packet to send, and flags such as whether this should be
///            acknowledged - or if it actually is an acknowledgement.
/// @param ptr Pointer to the data to send as packet.
/// @param len Number of data bytes to send. Must be in the range 0 .. 65.
void rf12_sendNow (uint8_t hdr, const void* ptr, uint8_t len) {
	while (!rf12_canSend())
	rf12_recvDone(); // keep the driver state machine going, ignore incoming
	rf12_sendStart(hdr, ptr, len);
}

/// @details
/// Wait for completion of the preceding rf12_sendStart() call, using the
/// specified low-power mode.
/// @note rf12_sendWait() should only be called right after rf12_sendStart().
/// @param mode Power-down mode during wait: 0 = NORMAL, 1 = IDLE, 2 = STANDBY,
///             3 = PWR_DOWN. Values 2 and 3 can cause the millisecond time to
///             lose a few interrupts. Value 3 can only be used if the ATmega
///             fuses have been set for fast startup, i.e. 258 CK - the default
///             Arduino fuse settings are not suitable for full power down.
void rf12_sendWait (uint8_t mode) {
	// wait for packet to actually finish sending
	// go into low power mode, as interrupts are going to come in very soon
	while (rxstate != TXIDLE)
	if (mode) {
		// power down mode is only possible if the fuses are set to start
		// up in 258 clock cycles, i.e. approx 4 us - else must use standby!
		// modes 2 and higher may lose a few clock timer ticks
		set_sleep_mode(mode == 3 ? SLEEP_MODE_PWR_DOWN :
		#ifdef SLEEP_MODE_STANDBY
		mode == 2 ? SLEEP_MODE_STANDBY :
		#endif
		SLEEP_MODE_IDLE);
		sleep_mode();
	}
		//LED_868SEND_PORT &= ~(1 << LED_868SEND_BIT); // set led out
}

/// @details
/// Attach interrupts for nodeid != 0
/// Detach interrupt for nodeid == 0
void rf12_interruptcontrol () {

	EIMSK |= (1<<INT0);					// Enable INT0
	
	//EICRA &= ~(1<<ISC01) | (1<<ISC00);	// Trigger INT0 on low level
	//EICRA |= (1<<ISC01);	// Trigger INT0 on every change
}

/// @details
/// Call this once with the node ID (0-31), frequency band (0-3), and
/// optional group (0-255 for RFM12B, only 212 allowed for RFM12).
/// @param id The ID of this wireless node. ID's should be unique within the
///           netGroup in which this node is operating. The ID range is 0 to 31,
///           but only 1..30 are available for normal use. You can pass a single
///           capital letter as node ID, with 'A' .. 'Z' corresponding to the
///           node ID's 1..26, but this convention is now discouraged. ID 0 is
///           reserved for OOK use, node ID 31 is special because it will pick
///           up packets for any node (in the same netGroup).
/// @param band This determines in which frequency range the wireless module
///             will operate. The following pre-defined constants are available:
///             RF12_433MHZ, RF12_868MHZ, RF12_915MHZ. You should use the one
///             matching the module you have.
/// @param g Net groups are used to separate nodes: only nodes in the same net
///          group can communicate with each other. Valid values are 1 to 212.
///          This parameter is optional, it defaults to 212 (0xD4) when omitted.
///          This is the only allowed value for RFM12 modules, only RFM12B
///          modules support other group values.
/// @returns the nodeId, to be compatible with rf12_config().
///
/// Programming Tips
/// ----------------
/// Note that rf12_initialize() does not use the EEprom netId and netGroup
/// settings, nor does it change the EEPROM settings. To use the netId and
/// netGroup settings saved in EEPROM use rf12_config() instead of
/// rf12_initialize. The choice whether to use rf12_initialize() or
/// rf12_config() at the top of every sketch is one of personal preference.
/// To set EEPROM settings for use with rf12_config() use the RF12demo sketch.
uint8_t rf12_initialize (uint8_t id, uint8_t b, uint8_t g, uint16_t frequency) {
	nodeid = id;
	group = g;
	band = b;
		
	rf12_spiInit();
	
	rf12_interruptcontrol();
	// reset RFM12b module
	rf12_xfer(0xCA82); // enable software reset
	rf12_xfer(0xFE00); // do software reset
	rxstate = UNINITIALIZED;

	// wait until RFM12B is out of power-up reset, this could takes several *seconds*
	// normally about 50ms
	set_sleep_mode(SLEEP_MODE_IDLE);
	 
	while (rxstate==UNINITIALIZED) {
		
	#if PINCHG_IRQ
		while (digitalRead(RFM_IRQ)==LOW)
		rf12_interrupt();
	#else
		sleep_mode();
	#endif
	}
	
	
	rf12_restore(id, b, g, frequency);
	return nodeid;
}

/// @details
/// Call this when the settings of the RFM12B have been overwritten by the
/// application with rf12_control(), to restore the original settings. Call
/// this with the node ID (0-31), frequency band (0-3), and
/// optional group (0-255 for RFM12B, only 212 allowed for RFM12).
/// @param id The ID of this wireless node. ID's should be unique within the
///           netGroup in which this node is operating. The ID range is 0 to 31,
///           but only 1..30 are available for normal use. You can pass a single
///           capital letter as node ID, with 'A' .. 'Z' corresponding to the
///           node ID's 1..26, but this convention is now discouraged. ID 0 is
///           reserved for OOK use, node ID 31 is special because it will pick
///           up packets for any node (in the same netGroup).
/// @param band This determines in which frequency range the wireless module
///             will operate. The following pre-defined constants are available:
///             RF12_433MHZ, RF12_868MHZ, RF12_915MHZ. You should use the one
///             matching the module you have.
/// @param g Net groups are used to separate nodes: only nodes in the same net
///          group can communicate with each other. Valid values are 1 to 212.
///          This parameter is optional, it defaults to 212 (0xD4) when omitted.
///          This is the only allowed value for RFM12 modules, only RFM12B
///          modules support other group values.
void rf12_restore (uint8_t id, uint8_t b, uint8_t g, uint16_t frequency) {
	nodeid = id;
	group = g;
	band = b;

	//interrupts may be attached or detached for OOK
	rf12_interruptcontrol();
	//undo settings for foreign-FSK use
	rf12_setFixedLength(0);
	blockInterrupts();
	rfmstate = 0x8205;              // RF_SLEEP_MODE
	rf12_xfer(rfmstate);            // DC (disable clk pin), enable lbd
	
	rf12_xfer(0x80C7 | (band << 4));// EL (ena TX), EF (ena RX FIFO), 12.0pF
    rf12_xfer(0xA000 + frequency); // 96-3960 freq range of values within band
	rf12_setBitrate(0x06);          // approx 49.2 Kbps, i.e. 10000/29/(1+6) Kbps
	rf12_xfer(0x94A2);              // VDI,FAST,134kHz,0dBm,-91dBm
	rf12_xfer(0xC2AC);              // AL,!ml,DIG,DQD4
	if (group != 0) {
		rf12_xfer(0xCA83);          // FIFO8,2-SYNC,!ff,DR
		rf12_xfer(0xCE00 | group);  // SYNC=2DXX;
		} else {
		rf12_xfer(0xCA8B);          // FIFO8,1-SYNC,!ff,DR
		rf12_xfer(0xCE2D);          // SYNC=2D;
	}
	rf12_xfer(0xC483);              // AFC@VDI,NO RSTRIC,!st,!fi,OE,EN
	rf12_xfer(0x9850);              // !mp,90kHz,MAX OUT
	rf12_xfer(0xCC77);              // OB1,OB0, LPX,!ddy,DDIT,BW0
	rf12_xfer(0xE000);              // NOT USE
	rf12_xfer(0xC800);              // NOT USE
	rf12_xfer(0xC049);              // 1.66MHz,3.1V

	rxstate = TXIDLE;
	
	allowInterrupts();
}

/// @details
/// This can be used to send out slow bit-by-bit On Off Keying signals to other
/// devices such as remotely controlled power switches operating in the 433,
/// 868, or 915 MHz bands.
///
/// To use this, you need to first call rf12initialize() with a zero node ID
/// and the proper frequency band. Then call rf12onOff() in the exact timing
/// you need for sending out the signal. Once done, either call rf12onOff(0) to
/// turn the transmitter off, or reinitialize the wireless module completely
/// with a call to rf12initialize().
/// @param value Turn the transmitter on (if true) or off (if false).
/// @note The timing of this function is relatively coarse, because SPI
/// transfers are used to enable / disable the transmitter. This will add some
/// jitter to the signal, probably in the order of 10 µsec.
void rf12_onOff (uint8_t value) {
	if (value) {
		rfmstate |= B01111000; // switch on transmitter and all needed components
		} else {
		rfmstate &= ~B00100000; // switch off transmitter
	}
	rf12_xfer(rfmstate);
}


/// @details
/// This function can put the radio module to sleep and wake it up again.
/// In sleep mode, the radio will draw only one or two microamps of current.
///
/// @param n If RF12SLEEP (0), put the radio to sleep
///          If RF12WAKEUP (-1), wake the radio up so that the next call to
///          rf12_recvDone() can restore normal reception.
void rf12_sleep (char n) {
	if (n < 0){
			//PORTB |= _BV(0); // pb0 aan
			rf12_idle();
			//PORTB &= ~_BV(0); // pb0 uit
	} else {
		rfmstate &= ~B11111000; // make sure everything is switched off (except bod, wkup, clk)
		rf12_xfer(rfmstate);
	}
	rxstate = TXIDLE;
}


/// @details
/// Request an interrupt in the specified number of milliseconds. This Registers a wakeup-
/// interrupt in the RFM12 module. Use rf12_wakeup() to check if the wakeup-interrupt
/// fired. This allows very deep sleep states (timer off) while still being able to wakeup
/// because of the external interrupt. The RFM12b wakeup-timer only needs about 1.5µA.
/// Don't expect an accurate timing. It's about 10% off. Only one timer is supported.
/// @param m Number of milliseconds
void rf12_setWatchdog (unsigned long m) {
	// calculate parameters for RFM12 module
	// T_wakeup[ms] = m * 2^r
	char r=0;
	while (m > 255) {
		r  += 1;
		m >>= 1;
	}
	
	// Disable old wakeup-timer if enabled
//	if (bitRead(rfmstate,1)) {
	if ((rfmstate>>1)&0x01){
		//bitClear(rfmstate,1);
		rfmstate &= ~(1 << 1);
		rf12_xfer(rfmstate);
	}
	
	// enable wakeup call if we have to
	if (m>0) {
		// write time to wakeup-register
		rf12_xfer(RF_WAKEUP_TIMER | (r<<8) | m);
		// enable wakeup
		//bitSet(rfmstate,1);
		rfmstate |= (1 << 1);
		rf12_xfer(rfmstate);
	}
}


/// @details
/// This checks the status of the RF12 low-battery detector. It wil be 1 when
/// the supply voltage drops below 3.1V, and 0 otherwise. This can be used to
/// detect an impending power failure, but there are no guarantees that the
/// power still remaining will be sufficient to send or receive further packets.
char rf12_lowbat () {
	return (state & RF_LBD_BIT) != 0;
}

/// @details
/// This function returns 1 if there was a wakeup-interrupt from the RFM12 module. Use it
/// together with rf12_setWatchdog() for ultra low-power watchdog.
char rf12_watchdogFired() {
	uint8_t res = rf12_gotwakeup;
	rf12_gotwakeup = 0;
	return res;
}