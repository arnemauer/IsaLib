// RFM12B driver definitions
// http://opensource.org/licenses/mit-license.php
// 2012-12-12 (C) felix@lowpowerlab.com
// Based on the RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)

#include "config.h"
#include "RFM12B.h"
	#include "uart.h"

uint8_t RFM12B::nodeID;                // address of this node
uint8_t RFM12B::networkID;             // network group ID
long RFM12B::rf12_seq;
uint32_t RFM12B::seqNum;
uint32_t RFM12B::cryptKey[4];
volatile uint8_t RFM12B::rxfill;       // number of data bytes in rf12_buf
volatile int8_t RFM12B::rxstate;       // current transceiver state
volatile uint16_t RFM12B::rf12_crc;    // running crc value
volatile uint8_t rf12_buf[RF_MAX];     // recv/xmit buf, including hdr & crc bytes

volatile uint16_t rfmstate;         // current power management setting of the RFM12 module


void RFM12B::SPIInit() {

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

// interrupts need to be disabled in a few spots
// do so by disabling the source of the interrupt, not all interrupts

void blockInterrupts () {
	EIMSK &= ~(1 << INT0); // disable pcint0 interrupt  //bitClear(EIMSK, INT0);
}

void allowInterrupts () {
	EIMSK |= (1 << INT0); // enable pcint1 interrupt //bitSet(EIMSK, INT0);
}


uint8_t RFM12B::Byte(uint8_t out) {
	#ifdef SPDR
	SPDR = out;
	// this loop spins 4 usec with a 2 MHz SPI clock
	while (!(SPSR & _BV(SPIF)));
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

uint16_t RFM12B::XFERSlow(uint16_t cmd) {
	// slow down to under 2.5 MHz
	#if F_CPU > 10000000
	SPCR |= (1 << SPR0);
	#endif
	PORT_RFM_CS &= ~(1<<BIT_RFM_CS);
	uint16_t reply = Byte(cmd >> 8) << 8;
	reply |= Byte(cmd);
	PORT_RFM_CS |= (1 << BIT_RFM_CS);
	#if F_CPU > 10000000
	SPCR &= ~(1 << SPR0);
	#endif
	return reply;
}

void RFM12B::XFER(uint16_t cmd) {
	#if OPTIMIZE_SPI
	// writing can take place at full speed, even 8 MHz works
	PORT_RFM_CS &= ~(1<<BIT_RFM_CS); //clear CS
	uint16_t res  = Byte(cmd >> 8) << 8;
	res |= Byte(cmd);
	PORT_RFM_CS |= (1 << BIT_RFM_CS); //  bitSet(SS_PORT, cs_pin);
	#else
	XFERSlow(cmd);
	#endif
}

// Call this once with params:
// - node ID (0-31)
// - frequency band (RF12_433MHZ, RF12_868MHZ, RF12_915MHZ)
// - networkid [optional - default = 170] (0-255 for RF12B, only 212 allowed for RF12)
// - txPower [optional - default = 0 (max)] (7 is min value)
// - AirKbps [optional - default = 38.31Kbps]
// - lowVoltageThreshold [optional - default = RF12_2v75]
void RFM12B::Initialize(uint8_t ID, uint8_t freqBand, uint8_t networkid, uint8_t txPower, uint8_t airKbps, uint8_t lowVoltageThreshold)
{

	nodeID = ID;
	networkID = networkid;
	SPIInit();

	
XFER(0x0000); // initial SPI transfer added to avoid power-up problem
XFER(RF_SLEEP_MODE); // DC (disable clk pin), enable lbd

// wait until RFM12B is out of power-up reset, this takes several *seconds*
XFER(RF_TXREG_WRITE); // in case we're still in OOK mode

while ((PIND & (1 << 2)) == 0)
XFER(0x0000);

	
	XFER(0x80C7 | (freqBand << 4)); // EL (ena TX), EF (ena RX FIFO), 12.0pF
	XFER(0xA640); // Frequency is exactly 434/868/915MHz (whatever freqBand is)
	
	XFER(0xc600 | airKbps);       // approx 49.2 Kbps, i.e. 10000/29/(1+6) Kbps
	
	XFER(0x94A2);             // VDI,FAST,134kHz,0dBm,-91dBm
	XFER(0xC2AC);             // AL,!ml,DIG,DQD4
	if (networkID != 0) {
		XFER(0xCA83);           // FIFO8,2-SYNC,!ff,DR
		XFER(0xCE00 | networkID); // SYNC=2DXX;
		} else {
		XFER(0xCA8B); // FIFO8,1-SYNC,!ff,DR
		XFER(0xCE2D); // SYNC=2D;
	}
	XFER(0xC483); // @PWR,NO RSTRIC,!st,!fi,OE,EN
	XFER(0x9850 | (txPower > 7 ? 7 : txPower)); // !mp,90kHz,MAX OUT               //last byte=power level: 0=highest, 7=lowest
	XFER(0xCC77); // OB1,OB0, LPX,!ddy,DDIT,BW0
	XFER(0xE000); // NOT USE wakeuptimer
	XFER(0xC800); // NOT USE lowduty
	XFER(0xC043); // Clock output (1.66MHz), Low Voltage threshold (2.55V)

	rxstate = TXIDLE;

	
	if (nodeID != 0){
			InitializeInterrupt();
}

	}
	
	/// @details
	/// Attach interrupts for nodeid != 0
	/// Detach interrupt for nodeid == 0
	void  RFM12B::InitializeInterrupt () {
		EICRA &= ~(1<<ISC01) | (1<<ISC00);	// Trigger INT0 on low level
		EIMSK |= (1<<INT0);					// Enable INT0
	}
	
	

// access to the RFM12B internal registers with interrupts disabled
uint16_t RFM12B::Control(uint16_t cmd) {
	
	blockInterrupts();
	
	#ifdef EIMSK
	uint16_t r = XFERSlow(cmd);
	
	#else
	
	// ATtiny
	uint16_t r = XFERSlow(cmd);
	#endif
	
	allowInterrupts();
	
	return r;
}

void RFM12B::InterruptHandler() {
	// a transfer of 2x 16 bits @ 2 MHz over SPI takes 2x 8 us inside this ISR
	// correction: now takes 2 + 8 µs, since sending can be done at 8 MHz
	XFER(0x0000);
	
	if (rxstate == TXRECV) {
		uint8_t in = XFERSlow(RF_RX_FIFO_READ);

		if (rxfill == 0 && networkID != 0)
		rf12_buf[rxfill++] = networkID;

		//Serial.print(out, HEX); Serial.print(' ');
		rf12_buf[rxfill++] = in;
		rf12_crc = _crc_xmodem_update(rf12_crc, in);

		if (rxfill >= rf12_len + 4 || rxfill >= RF_MAX)
		XFER(RF_IDLE_MODE);
		
		} else {

		uint8_t out;

		if (rxstate < 0) {
			uint8_t pos = 2 + rf12_len + rxstate++;
			out = rf12_buf[pos];
			rf12_crc = _crc_xmodem_update(rf12_crc, out);
		} else
		switch (rxstate++) {
			case TXSYN1: out = 0x2D; break;
			case TXSYN2: out = networkID; rxstate = -(1 + rf12_len); break;
			case TXCRC1: out = ~rf12_crc >> 8; break;
			case TXCRC2: out = ~rf12_crc; break;
			case TXDONE: XFER(RF_IDLE_MODE); // fall through
			default:     out = 0xAA;
		}
		
		//Serial.print(out, HEX); Serial.print(' ');
		XFER(RF_TXREG_WRITE + out);
	}
}






void RFM12B::ReceiveStart() {
	rxfill = rf12_len = 0;
	//rf12_crc = ~0;
	//if (networkID != 0)
	//rf12_crc = _crc16_update(~0, networkID);
	
	rf12_crc = 0x1d0f;
	
	rxstate = TXRECV;
	XFER(RF_RECEIVER_ON);
}

bool RFM12B::ReceiveComplete() {
	if (rxstate == TXRECV && (rxfill >= rf12_len + 4 || rxfill >= RF_MAX)) {
		rxstate = TXIDLE;
		if (rf12_len > RF12_MAXDATA)
		rf12_crc = 1; // force bad crc if packet length is invalid
		if (RF12_DESTID == 0 || RF12_DESTID == nodeID) { //if (!(rf12_hdr & RF12_HDR_DST) || (nodeID & NODE_ID) == 31 || (rf12_hdr & RF12_HDR_MASK) == (nodeID & NODE_ID)) {
			if (rf12_crc == 0 && crypter != 0)
			crypter(false);
			else
			rf12_seq = -1;
			return true; // it's a broadcast packet or it's addressed to this node
		}
	}
	if (rxstate == TXIDLE)
	ReceiveStart();
	return false;
}

bool RFM12B::CanSend() {
	// no need to test with interrupts disabled: state TXRECV is only reached
	// outside of ISR and we don't care if rxfill jumps from 0 to 1 here
	if (rxstate == TXRECV && rxfill == 0 && (Byte(0x00) & (RF_RSSI_BIT >> 8)) == 0) {
		XFER(RF_IDLE_MODE); // stop receiver
		//XXX just in case, don't know whether these RF12 reads are needed!
		// rf12_XFER(0x0000); // status register
		// rf12_XFER(RF_RX_FIFO_READ); // fifo read
		rxstate = TXIDLE;
		return true;
	}
	return false;
}

void RFM12B::SendStart(uint8_t toNodeID, bool requestACK, bool sendACK) {
//	rf12_hdr1 = toNodeID | (sendACK ? RF12_HDR_ACKCTLMASK : 0);
//	rf12_hdr2 = nodeID | (requestACK ? RF12_HDR_ACKCTLMASK : 0);
//	if (crypter != 0) crypter(true);
//	rf12_crc = ~0;
//	rf12_crc = _crc16_update(rf12_crc, networkID);

  rf12_hdr1 = toNodeID;
  rf12_hdr2 = nodeID;
  rf12_hdr3 = (sendACK ? 0x80 : 0) | (requestACK ? 0x40 : 0);
  if (crypter != 0) crypter(true);
  rf12_crc = 0x1d0f;

	rxstate = TXPRE1;
	XFER(RF_XMITTER_ON); // bytes will be fed via interrupts
}

void RFM12B::SendStart(uint8_t toNodeID, const void* sendBuf, uint8_t sendLen, bool requestACK, bool sendACK, uint8_t waitMode) {
	rf12_len = sendLen;
	 rf12_len += 3;
	memcpy((void*) rf12_data, sendBuf, sendLen);
	SendStart(toNodeID, requestACK, sendACK);
	SendWait(waitMode);
}

/// Should be called immediately after reception in case sender wants ACK
void RFM12B::SendACK(const void* sendBuf, uint8_t sendLen, uint8_t waitMode) {
	while (!CanSend()) ReceiveComplete();
	SendStart(RF12_SOURCEID, sendBuf, sendLen, false, true, waitMode);
}

void RFM12B::Send(uint8_t toNodeID, const void* sendBuf, uint8_t sendLen, bool requestACK, uint8_t waitMode)
{
	while (!CanSend()) ReceiveComplete();
	SendStart(toNodeID, sendBuf, sendLen, requestACK, false, waitMode);
}

void RFM12B::SendWait(uint8_t waitMode) {
	// wait for packet to actually finish sending
	// go into low power mode, as interrupts are going to come in very soon
	while (rxstate != TXIDLE)
	if (waitMode) {
		// power down mode is only possible if the fuses are set to start
		// up in 258 clock cycles, i.e. approx 4 us - else must use standby!
		// modes 2 and higher may lose a few clock timer ticks
		set_sleep_mode(waitMode == 3 ? SLEEP_MODE_PWR_DOWN :
		#ifdef SLEEP_MODE_STANDBY
		waitMode == 2 ? SLEEP_MODE_STANDBY :
		#endif
		SLEEP_MODE_IDLE);
		sleep_mode();
	}
}

void RFM12B::OnOff(uint8_t value) {
	XFER(value ? RF_XMITTER_ON : RF_IDLE_MODE);
}

void RFM12B::Sleep(char n) {
	if (n < 0)
	Control(RF_IDLE_MODE);
	else {
		Control(RF_WAKEUP_TIMER | 0x0500 | n);
		Control(RF_SLEEP_MODE);
		if (n > 0)
		Control(RF_WAKEUP_MODE);
	}
	rxstate = TXIDLE;
}
void RFM12B::Sleep() { Sleep(0); }
void RFM12B::Wakeup() { Sleep(-1); }

bool RFM12B::LowBattery() {
	return (Control(0x0000) & RF_LBD_BIT) != 0;
}

uint8_t RFM12B::GetSender(){
	return RF12_SOURCEID;
}

volatile uint8_t * RFM12B::GetData() { return rf12_data; }
uint8_t RFM12B::GetDataLen() { return *DataLen; }
bool RFM12B::ACKRequested() { return RF12_WANTS_ACK; }

/// Should be polled immediately after sending a packet with ACK request
bool RFM12B::ACKReceived(uint8_t fromNodeID) {
	if (ReceiveComplete())
	return CRCPass() &&
	RF12_DESTID == nodeID &&
	(RF12_SOURCEID == fromNodeID || fromNodeID == 0) &&
          (rf12_hdr3 & 0x80) &&
          !(rf12_hdr3 & 0x40);
	return false;
}


/// @details
/// Request an interrupt in the specified number of milliseconds. This Registers a wakeup-
/// interrupt in the RFM12 module. Use rf12_wakeup() to check if the wakeup-interrupt
/// fired. This allows very deep sleep states (timer off) while still being able to wakeup
/// because of the external interrupt. The RFM12b wakeup-timer only needs about 1.5µA.
/// Don't expect an accurate timing. It's about 10% off. Only one timer is supported.
/// @param m Number of milliseconds
void RFM12B::SetLowDuty(unsigned long m) {
	// calculate parameters for RFM12 module
	// T_wakeup[ms] = m * 2^r
	char r=0;
	while (m > 255) {
		r  += 1;
		m >>= 1;
	}
	
	// Disable old wakeup-timer if enabled
	//	if (bitRead(rfmstate,1)) {
	//if ((rfmstate)&0x02){
	//bitClear(rfmstate,1);
	//	rfmstate &= ~(1 << 1);
	//	rf12_xfer(rfmstate);
	//}
	
	// enable wakeup call if we have to
	//if (m>0) {
	// write time to wakeup-register
	//rf12_xfer(RF_WAKEUP_TIMER | (r<<8) | m);
	XFER(0xE1FA); //500ms
	
	XFER(0xC833); //  Low Duty-Cycle D=25 = 20% van 500ms = 102ms
	// enable wakeup
	
	//bitSet(rfmstate,1);
	//rfmstate = 0x8200;
	//rfmstate |= (1 << 1); //  In this operation mode, bit er must be cleared and bit ew must be set in the Power Management Command.
//	rfmstate &= ~(1 << 7);
//	XFER(rfmstate);
	//}
}




// XXTEA by David Wheeler, adapted from http://en.wikipedia.org/wiki/XXTEA
#define DELTA 0x9E3779B9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (cryptKey[(uint8_t)((p&3)^e)] ^ z)))
void RFM12B::CryptFunction(bool sending) {
	uint32_t y, z, sum, *v = (uint32_t*) rf12_data;
	uint8_t p, e, rounds = 6;
	
	if (sending) {
		// pad with 1..4-byte sequence number
		*(uint32_t*)(rf12_data + rf12_len) = ++seqNum;
		uint8_t pad = 3 - (rf12_len & 3);
		rf12_len += pad;
		rf12_data[rf12_len] &= 0x3F;
		rf12_data[rf12_len] |= pad << 6;
		++rf12_len;
		// actual encoding
		char n = rf12_len / 4;
		if (n > 1) {
			sum = 0;
			z = v[n-1];
			do {
				sum += DELTA;
				e = (sum >> 2) & 3;
				for (p=0; p<n-1; p++)
				y = v[p+1], z = v[p] += MX;
				y = v[0];
				z = v[n-1] += MX;
			} while (--rounds);
		}
		} else if (rf12_crc == 0) {
		// actual decoding
		char n = rf12_len / 4;
		if (n > 1) {
			sum = rounds*DELTA;
			y = v[0];
			do {
				e = (sum >> 2) & 3;
				for (p=n-1; p>0; p--)
				z = v[p-1], y = v[p] -= MX;
				z = v[n-1];
				y = v[0] -= MX;
			} while ((sum -= DELTA) != 0);
		}
		// strip sequence number from the end again
		if (n > 0) {
			uint8_t pad = rf12_data[--rf12_len] >> 6;
			rf12_seq = rf12_data[rf12_len] & 0x3F;
			while (pad-- > 0)
			rf12_seq = (rf12_seq << 8) | rf12_data[--rf12_len];
		}
	}
}

void RFM12B::Encrypt(const uint8_t* key, uint8_t keyLen) {
	// by using a pointer to CryptFunction, we only link it in when actually used
	if (key != 0) {
		for (uint8_t i = 0; i < keyLen; ++i)
		((uint8_t*) cryptKey)[i] = key[i];
		crypter = CryptFunction;
	} else crypter = 0;
}

