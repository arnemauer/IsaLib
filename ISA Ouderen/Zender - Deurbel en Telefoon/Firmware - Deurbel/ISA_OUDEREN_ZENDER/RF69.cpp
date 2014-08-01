#include <stdint.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "RF69.h"
#include "RF69_avr.h"

#define REG_FIFO            0x00
#define REG_OPMODE          0x01
#define REG_FRFMSB          0x07
#define REG_RSSIVALUE       0x24
#define REG_DIOMAPPING1     0x25
#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28
#define REG_SYNCCONFIG      0x2E
#define REG_SYNCVALUE1      0x2F
#define REG_SYNCVALUE2      0x30
#define REG_NODEADRS        0x39
#define REG_PACKETCONFIG2   0x3D
#define REG_AESKEY1         0x3E

#define MODE_SLEEP          0x00
#define MODE_STANDBY        0x04
#define MODE_RECEIVER       0x10
#define MODE_TRANSMITTER    0x0C

#define IRQ1_MODEREADY      0x80
#define IRQ1_RXREADY        0x40

#define IRQ2_FIFOFULL       0x80
#define IRQ2_FIFONOTEMPTY   0x40
#define IRQ2_FIFOOVERRUN    0x10
#define IRQ2_PACKETSENT     0x08
#define IRQ2_PAYLOADREADY   0x04

#define RF_MAX   72

// transceiver states, these determine what to do with each interrupt
enum { TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE, TXRECV };

namespace RF69 {
    uint32_t frf;
    uint8_t  group;
    uint8_t  nodeID;
    uint16_t crc;
    uint8_t  rssi;
}

static volatile uint8_t rxfill;     // number of data bytes in rf12_buf
static volatile int8_t rxstate;     // current transceiver state

static ROM_UINT8 configRegs_compat [][2] ROM_DATA = {
  /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
  /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
  /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_55555}, //default:4.8 KBPS
  /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_55555},
  /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz)
  /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000},

  /* 0x07 */ { REG_FRFMSB, (freqBand==RF69_315MHZ ? RF_FRFMSB_315 : (freqBand==RF69_433MHZ ? RF_FRFMSB_433 : (freqBand==RF69_868MHZ ? RF_FRFMSB_868 : RF_FRFMSB_915))) },
  /* 0x08 */ { REG_FRFMID, (freqBand==RF69_315MHZ ? RF_FRFMID_315 : (freqBand==RF69_433MHZ ? RF_FRFMID_433 : (freqBand==RF69_868MHZ ? RF_FRFMID_868 : RF_FRFMID_915))) },
  /* 0x09 */ { REG_FRFLSB, (freqBand==RF69_315MHZ ? RF_FRFLSB_315 : (freqBand==RF69_433MHZ ? RF_FRFLSB_433 : (freqBand==RF69_868MHZ ? RF_FRFLSB_868 : RF_FRFLSB_915))) },
  
  // looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
  // +17dBm and +20dBm are possible on RFM69HW
  // +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
  // +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
  // +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
  ///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
  ///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA)
  
  ///* 0x18*/ { REG_LNA,  RF_LNA_ZIN_200 | RF_LNA_CURRENTGAIN }, //as suggested by mav here: http://lowpowerlab.com/forum/index.php/topic,296.msg1571.html
  
  // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4khz)
  /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, //(BitRate < 2 * RxBw)
  /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
  /* 0x29 */ { REG_RSSITHRESH, 220 }, //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
  ///* 0x2d */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE } // default 3 preamble bytes 0xAAAAAA
  /* 0x2e */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
  /* 0x2f */ { REG_SYNCVALUE1, 0x2D },      //attempt to make this compatible with sync1 byte of RFM12B lib
  /* 0x30 */ { REG_SYNCVALUE2, networkID }, //NETWORK ID
  /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF },
  /* 0x38 */ { REG_PAYLOADLENGTH, 66 }, //in variable length mode: the max frame size, not used in TX
  //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering
  /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
  /* 0x3d */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
  /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0
  {255, 0}
};

uint8_t RF69::control(uint8_t cmd, uint8_t val) {
    PreventInterrupt irq0;
    return spiTransfer(cmd, val);
}

static void writeReg (uint8_t addr, uint8_t value) {
    RF69::control(addr | 0x80, value);
}

static uint8_t readReg (uint8_t addr) {
    return RF69::control(addr, 0);
}

static void flushFifo () {
    while (readReg(REG_IRQFLAGS2) & (IRQ2_FIFONOTEMPTY | IRQ2_FIFOOVERRUN))
        readReg(REG_FIFO);
}

static void setMode (uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
    // while ((readReg(REG_IRQFLAGS1) & IRQ1_MODEREADY) == 0)
    //     ;
}

static void initRadio (ROM_UINT8* init) {
   // spiInit();
    do
        writeReg(REG_SYNCVALUE1, 0xAA);
    while (readReg(REG_SYNCVALUE1) != 0xAA);
    do
        writeReg(REG_SYNCVALUE1, 0x55);
    while (readReg(REG_SYNCVALUE1) != 0x55);
    for (;;) {
        uint8_t cmd = ROM_READ_UINT8(init);
        if (cmd == 0) break;
        writeReg(cmd, ROM_READ_UINT8(init+1));
        init += 2;
    }
}

void RF69::setFrequency (uint32_t freq) {
    // Frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
    // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
    // due to this, the lower 6 bits of the calculated factor will always be 0
    // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
    // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000  
    frf = ((freq << 2) / (32000000L >> 11)) << 6;
}

bool RF69::canSend () {
    if (rxstate == TXRECV && rxfill == 0) {
        rxstate = TXIDLE;
        setMode(MODE_STANDBY);
        return true;
    }
    return false;
}

bool RF69::sending () {
    return rxstate < TXIDLE;
}

void RF69::sleep (bool off) {
    setMode(off ? MODE_SLEEP : MODE_STANDBY);
    rxstate = TXIDLE;
}

// References to the RF12 driver above this line will generate compiler errors!
#include "RF69_compat.h"
#include "RF12.h"

void RF69::configure_compat () {
    initRadio(configRegs_compat);    
    // FIXME doesn't seem to work, nothing comes in but noise for group 0
    // writeReg(REG_SYNCCONFIG, group ? 0x88 : 0x80);
    writeReg(REG_SYNCVALUE2, group);

    writeReg(REG_FRFMSB, frf >> 16);
    writeReg(REG_FRFMSB+1, frf >> 8);
    writeReg(REG_FRFMSB+2, frf);

    rxstate = TXIDLE;
}




uint8_t* recvBuf;

uint16_t RF69::recvDone_compat (uint8_t* buf) {
    switch (rxstate) {
    case TXIDLE:
        rxfill = rf12_len = 0;
        crc = _crc16_update(~0, group);
        recvBuf = buf;
        rxstate = TXRECV;
        flushFifo();
        setMode(MODE_RECEIVER);
        break;
    case TXRECV:
        if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX) {
            rxstate = TXIDLE;
            setMode(MODE_STANDBY);
            if (rf12_len > RF12_MAXDATA)
                crc = 1; // force bad crc for invalid packet
            if (!(RF12_DESTID) || nodeID == 31 || (RF12_SOURCEID) == nodeID)
                return crc;
        }
    }
    return ~0;
}

uint8_t RF69::recvDone () {
	rf69_crc = RF69::recvDone_compat((uint8_t*) rf69_buf);
	return rf69_crc != ~0;
}

void RF69::sendWait (uint8_t mode) {
	while (sending())
	if (mode) {
		set_sleep_mode(mode == 3 ? SLEEP_MODE_PWR_DOWN :
		#ifdef SLEEP_MODE_STANDBY
		mode == 2 ? SLEEP_MODE_STANDBY :
		#endif
		SLEEP_MODE_IDLE);
		sleep_mode();
	}
}



void RF69::sendStart(uint8_t toNodeID, bool requestACK, bool sendACK, const void* ptr, uint8_t len) {
    rf12_len = len;
	rf12_grp = group;
    for (int i = 0; i < len; ++i)
        rf12_data[i] = ((const uint8_t*) ptr)[i];
		
   // rf12_hdr = hdr & RF12_HDR_DST ? hdr : (hdr & ~RF12_HDR_MASK) + node;
	rf12_hdr1 = toNodeID | (sendACK ? RF12_HDR_ACKCTLMASK : 0);
	rf12_hdr2 = nodeID | (requestACK ? RF12_HDR_ACKCTLMASK : 0);
	   
    rf12_crc = _crc16_update(~0, group);
	
    rxstate = - (3 + rf12_len); // preamble and SYN1/SYN2 are sent by hardware
    flushFifo();
    setMode(MODE_TRANSMITTER);
    writeReg(REG_DIOMAPPING1, 0x00); // PacketSent
    
    // use busy polling until the last byte fits into the buffer
    // this makes sure it all happens on time, and that sendWait can sleep
    while (rxstate < TXDONE)
        if ((readReg(REG_IRQFLAGS2) & IRQ2_FIFOFULL) == 0) {
            uint8_t out = 0xAA;
            if (rxstate < 0) {
                out = recvBuf[4 + rf12_len + rxstate];
                crc = _crc16_update(crc, out);
            } else {
                switch (rxstate) {
                    case TXCRC1: out = crc; break;
                    case TXCRC2: out = crc >> 8; break;
                }
            }
            writeReg(REG_FIFO, out);
            ++rxstate;
        }
}

void RF69::sendNow (uint8_t toNodeID, bool requestACK, bool sendACK, const void* ptr, uint8_t len) {
	while (!canSend())
	recvDone();
	sendStart(toNodeID, requestACK, sendACK, ptr, len);
}


void RF69::interrupt_compat () {
    if (rxstate == TXRECV) {
        rssi = readReg(REG_RSSIVALUE);
        IRQ_ENABLE; // allow nested interrupts from here on
        for (;;) { // busy loop, to get each data byte as soon as it comes in
            if (readReg(REG_IRQFLAGS2) & (IRQ2_FIFONOTEMPTY|IRQ2_FIFOOVERRUN)) {
                if (rxfill == 0)
                    recvBuf[rxfill++] = group;
					
                uint8_t in = readReg(REG_FIFO);
                recvBuf[rxfill++] = in;
                crc = _crc16_update(crc, in);   
				           
                if (rxfill >= rf12_len + 6 || rxfill >= RF_MAX)
                    break;
            }
        }
    } else if (readReg(REG_IRQFLAGS2) & IRQ2_PACKETSENT) {
        // rxstate will be TXDONE at this point
        rxstate = TXIDLE;
        setMode(MODE_STANDBY);
        writeReg(REG_DIOMAPPING1, 0x80); // SyncAddress
    }
}

ISR(INT0_vect) {
	//PORTB |= _BV(0); // pb0 aan

	RF69::interrupt_compat();
	//PORTB &= ~_BV(0); // pb0 uit
}
