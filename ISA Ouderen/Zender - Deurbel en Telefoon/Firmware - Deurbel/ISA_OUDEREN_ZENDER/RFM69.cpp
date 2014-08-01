// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW, Semtech SX1231/1231H
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// 2013-06-14 (C) felix@lowpowerlab.com
// **********************************************************************************
#include "RFM69.h"
#include "RFM69registers.h"
#include "millis.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/crc16.h>

volatile uint8_t RFM69::DATA[MAX_DATA_LEN];
volatile uint8_t RFM69::_mode;       // current transceiver state
volatile uint8_t RFM69::DATALEN;
volatile uint8_t RFM69::SENDERID;
volatile uint8_t RFM69::TARGETID; //should match _address
volatile uint8_t RFM69::PAYLOADLEN;
volatile uint8_t RFM69::ACK_REQUESTED;
volatile uint8_t RFM69::ACK_RECEIVED; /// Should be polled immediately after sending a packet with ACK request
volatile int RFM69::RSSI; //most accurate RSSI during reception (closest to the reception)
RFM69* RFM69::selfPointer;



bool RFM69::initialize(uint8_t freqBand, uint8_t nodeID, uint8_t networkID)
{
  const uint8_t CONFIG[][2] =
  {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
    /* 0x03 */ { REG_BITRATEMSB, 0x02}, //default:0x02 = 49,251 khz
    /* 0x04 */ { REG_BITRATELSB, 0x8A}, // BitRateLsb, divider = 32 MHz / 650
    /* 0x05 */ { REG_FDEVMSB, 0x05}, //default:5khz, (FDEV + BitRate/2 <= 500Khz)
    /* 0x06 */ { REG_FDEVLSB, 0xC3},

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

	spiInit();	
  
  do writeReg(REG_SYNCVALUE1, 0xAA); while (readReg(REG_SYNCVALUE1) != 0xAA);
  
	do writeReg(REG_SYNCVALUE1, 0x55); while (readReg(REG_SYNCVALUE1) != 0x55);
  
  for (byte i = 0; CONFIG[i][0] != 255; i++)
    writeReg(CONFIG[i][0], CONFIG[i][1]);

  // Encryption is persistent between resets and can trip you up during debugging.
  // Disable it during initialization so we always start from a known state.
  encrypt(0);

  setMode(RF69_MODE_STANDBY);
	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  
  
  //attachInterrupt(0, RFM69::isr0, RISING);
  			
  			 EICRA |= (1 << ISC00) | (1 << ISC01); // trigger on rising edge
			 EIMSK |= (1 << INT0); // enable int0 interrupt //bitSet(EIMSK, INT0);
			    
  selfPointer = this;
  _address = nodeID;
  return true;
}

void RFM69::spiInit(){
			DDR_SPI |= (1 << BIT_MOSI) | (1 << BIT_SCK) | (1 << BIT_RFM_CS);  // SDI, SCK,  CS output
			PORT_RFM_CS |= (1 << BIT_RFM_CS);  // Pull RFM12B CS high
			DDR_SPI &= ~(1 << BIT_MISO);  // SDO  input

			
			SPCR = _BV(SPE) | _BV(MSTR);
			#if F_CPU > 10000000
			// use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see rf12_xfer)
			SPSR |= _BV(SPI2X);
			#endif
			
			
			DDR_RFM_IRQ &= ~(1 << BIT_RFM_IRQ);  // RFM12 IRQ  input
			PORT_RFM_IRQ |= (1 << BIT_RFM_IRQ); // digitalWrite(RFM_IRQ, 1); // pull-up
}

void RFM69::setFrequency(uint32_t FRF)
{
  writeReg(REG_FRFMSB, FRF >> 16);
  writeReg(REG_FRFMID, FRF >> 8);
  writeReg(REG_FRFLSB, FRF);
}

void RFM69::setMode(byte newMode)
{
	if (newMode == _mode) return; //TODO: can remove this?

	switch (newMode) {
		case RF69_MODE_TX:
		writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
      if (_isRFM69HW) setHighPowerRegs(true);
			break;
		case RF69_MODE_RX:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
      if (_isRFM69HW) setHighPowerRegs(false);
			break;
		case RF69_MODE_SYNTH:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
			break;
		case RF69_MODE_STANDBY:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
			break;
		case RF69_MODE_SLEEP:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
			break;
		default: return;
	}

	// we are using packet mode, so this check is not really needed
  // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
	while (_mode == RF69_MODE_SLEEP && (readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady

	_mode = newMode;
}

void RFM69::sleep() {
  setMode(RF69_MODE_SLEEP);
}

void RFM69::setAddress(byte addr)
{
  _address = addr;
	writeReg(REG_NODEADRS, _address);
}

// set output power: 0=min, 31=max
// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
void RFM69::setPowerLevel(byte powerLevel)
{
  _powerLevel = powerLevel;
  writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | (_powerLevel > 31 ? 31 : _powerLevel));
}

bool RFM69::canSend()
{
  if (_mode == RF69_MODE_RX && PAYLOADLEN == 0 && readRSSI() < CSMA_LIMIT) //if signal stronger than -100dBm is detected assume channel activity
  {
    setMode(RF69_MODE_STANDBY);
    return true;
  }
  return false;
}

void RFM69::send(byte toAddress, const void* buffer, byte bufferSize, bool requestACK)
{
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  while (!canSend()) receiveDone();
  sendFrame(toAddress, buffer, bufferSize, requestACK, false);
}

// to increase the chance of getting a packet across, call this function instead of send
// and it handles all the ACK requesting/retrying for you :)
// The only twist is that you have to manually listen to ACK requests on the other side and send back the ACKs
// The reason for the semi-automaton is that the lib is ingterrupt driven and
// requires user action to read the received data and decide what to do with it
// replies usually take only 5-8ms at 50kbps@915Mhz
bool RFM69::sendWithRetry(byte toAddress, const void* buffer, byte bufferSize, byte retries, byte retryWaitTime) {
  long sentTime;
  for (byte i=0; i<=retries; i++)
  {
    send(toAddress, buffer, bufferSize, true);
    sentTime = millis();
    while (millis()-sentTime<retryWaitTime)
    {
      if (ACKReceived(toAddress))
      {
        //Serial.print(" ~ms:");Serial.print(millis()-sentTime);
        return true;
      }
    }
    //Serial.print(" RETRY#");Serial.println(i+1);
  }
  return false;
}

/// Should be polled immediately after sending a packet with ACK request
bool RFM69::ACKReceived(byte fromNodeID) {
  if (receiveDone())
    return (SENDERID == fromNodeID || fromNodeID == RF69_BROADCAST_ADDR) && ACK_RECEIVED;
  return false;
}

/// Should be called immediately after reception in case sender wants ACK
void RFM69::sendACK(const void* buffer, byte bufferSize) {
  byte sender = SENDERID;
  while (!canSend()) receiveDone();
  sendFrame(sender, buffer, bufferSize, false, true);
}

void RFM69::sendFrame(byte toAddress, const void* buffer, byte bufferSize, bool requestACK, bool sendACK)
{
  setMode(RF69_MODE_STANDBY); //turn off receiver to prevent reception while filling fifo
	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
  if (bufferSize > MAX_DATA_LEN) bufferSize = MAX_DATA_LEN;

	//write to FIFO
	select();
	spiTransferByte(REG_FIFO | 0x80);
	spiTransferByte(bufferSize + 3);
	spiTransferByte(toAddress);
    spiTransferByte(_address);
  
  
  //control byte
  if (sendACK)
    spiTransferByte(0x80);
  else if (requestACK)
    spiTransferByte(0x40);
  else spiTransferByte(0x00);
  
	for (byte i = 0; i < bufferSize; i++)
    spiTransferByte(((byte*)buffer)[i]);
	unselect();

	/* no need to wait for transmit mode to be ready since its handled by the radio */
	setMode(RF69_MODE_TX);
	while ((PIND &  (1 << 2)) == 0); //wait for DIO0 to turn HIGH signalling transmission finish
  
	//while (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT == 0x00); // Wait for ModeReady
	setMode(RF69_MODE_STANDBY);
  
}

void RFM69::interruptHandler() {
  //pinMode(4, OUTPUT);
  //digitalWrite(4, 1);
  if (_mode == RF69_MODE_RX && (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY))
  {
    setMode(RF69_MODE_STANDBY);
    select();
    spiTransferByte(REG_FIFO & 0x7f);
    PAYLOADLEN = spiTransferByte(0);
    PAYLOADLEN = PAYLOADLEN > 66 ? 66 : PAYLOADLEN; //precaution
    TARGETID = spiTransferByte(0);
    if(!(_promiscuousMode || TARGETID==_address || TARGETID==RF69_BROADCAST_ADDR)) //match this node's address, or broadcast address or anything in promiscuous mode
    {
      PAYLOADLEN = 0;
      unselect();
      //digitalWrite(4, 0);
      return;
    }
    DATALEN = PAYLOADLEN - 3;
    SENDERID = spiTransferByte(0);
    byte CTLbyte = spiTransferByte(0);
    
    ACK_RECEIVED = CTLbyte & 0x80; //extract ACK-requested flag
    ACK_REQUESTED = CTLbyte & 0x40; //extract ACK-received flag
    
    for (byte i= 0; i < DATALEN; i++)
    {
      DATA[i] = spiTransferByte(0);
    }
    unselect();
    setMode(RF69_MODE_RX);
  }
  RSSI = readRSSI();
  //digitalWrite(4, 0);
}

void RFM69::isr0() { selfPointer->interruptHandler(); }

void RFM69::receiveBegin() {
  DATALEN = 0;
  SENDERID = 0;
  TARGETID = 0;
  PAYLOADLEN = 0;
  ACK_REQUESTED = 0;
  ACK_RECEIVED = 0;
  RSSI = 0;
  if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
    writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); //set DIO0 to "PAYLOADREADY" in receive mode
  setMode(RF69_MODE_RX);
}

bool RFM69::receiveDone() {
// ATOMIC_BLOCK(ATOMIC_FORCEON)
// {
  noInterrupts(); //re-enabled in unselect() via setMode() or via receiveBegin()
  if (_mode == RF69_MODE_RX && PAYLOADLEN>0)
  {
    setMode(RF69_MODE_STANDBY); //enables interrupts
    return true;
  }
  else if (_mode == RF69_MODE_RX)  //already in RX no payload yet
  {
    allowInterrupts(); //explicitly re-enable interrupts
    return false;
  }
  receiveBegin();
  return false;
//}
}

// To enable encryption: radio.encrypt("ABCDEFGHIJKLMNOP");
// To disable encryption: radio.encrypt(null) or radio.encrypt(0)
// KEY HAS TO BE 16 bytes !!!
void RFM69::encrypt(const char* key) {
  setMode(RF69_MODE_STANDBY);
  if (key!=0)
  {
    select();
    spiTransferByte(REG_AESKEY1 | 0x80);
    for (byte i = 0; i<16; i++)
      spiTransferByte(key[i]);
    unselect();
  }
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
}

int RFM69::readRSSI(bool forceTrigger) {
  int rssi = 0;
  if (forceTrigger)
  {
    //RSSI trigger not needed if DAGC is in continuous mode
    writeReg(REG_RSSICONFIG, RF_RSSI_START);
    while ((readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); // Wait for RSSI_Ready
  }
  rssi = -readReg(REG_RSSIVALUE);
  rssi >>= 1;
  return rssi;
}

byte RFM69::readReg(byte addr)
{
	 select();
	return spiTransfer (addr, 0);
	 unselect();
}

void RFM69::writeReg(byte addr, byte value){
	 select();
	 spiTransfer (addr | 0x80, value);
	  unselect();
}

/// Select the transceiver
void RFM69::select() {
  noInterrupts();
  PORT_RFM_CS &= ~(1<<BIT_RFM_CS); //clear CS
}

/// UNselect the transceiver chip
void RFM69::unselect() {
  PORT_RFM_CS |= (1 << BIT_RFM_CS); //  bitSet(SS_PORT, cs_pin);
  allowInterrupts();
}

// ON  = disable filtering to capture all frames on network
// OFF = enable node+broadcast filtering to capture only frames sent to this/broadcast address
void RFM69::promiscuous(bool onOff) {
  _promiscuousMode=onOff;
  //writeReg(REG_PACKETCONFIG1, (readReg(REG_PACKETCONFIG1) & 0xF9) | (onOff ? RF_PACKET1_ADRSFILTERING_OFF : RF_PACKET1_ADRSFILTERING_NODEBROADCAST));
}

void RFM69::setHighPower(bool onOff) {
  _isRFM69HW = onOff;
  writeReg(REG_OCP, _isRFM69HW ? RF_OCP_OFF : RF_OCP_ON);
  if (_isRFM69HW) //turning ON
    writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); //enable P1 & P2 amplifier stages
  else
    writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | _powerLevel); //enable P0 only
}

void RFM69::setHighPowerRegs(bool onOff) {
  writeReg(REG_TESTPA1, onOff ? 0x5D : 0x55);
  writeReg(REG_TESTPA2, onOff ? 0x7C : 0x70);
}


//for debugging
void RFM69::readAllRegs()
{
  byte regVal;
	
  for (byte regAddr = 1; regAddr <= 0x4F; regAddr++)
	{
    select();
    spiTransferByte(regAddr & 0x7f);	// send address + r/w bit
    regVal = spiTransferByte(0);
    unselect();

  //  Serial.print(regAddr, HEX);
  //  Serial.print(" - ");
   // Serial.print(regVal,HEX);
  //  Serial.print(" - ");
  //  Serial.println(regVal,BIN);
	}
  unselect();
}

byte RFM69::readTemperature(byte calFactor)  //returns centigrade
{
  setMode(RF69_MODE_STANDBY);
  writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING)) ;//Serial.print('*');
  
  return ~readReg(REG_TEMP2) + COURSE_TEMP_COEF + calFactor; //'complement'corrects the slope, rising temp = rising val
}												   	  // COURSE_TEMP_COEF puts reading in the ballpark, user can add additional correction

void RFM69::rcCalibration()
{
  writeReg(REG_OSC1, RF_OSC1_RCCAL_START);
  while ((readReg(REG_OSC1) & RF_OSC1_RCCAL_DONE) == 0x00);
}



 uint8_t RFM69::spiTransferByte (uint8_t out) {
	SPDR = out;
	while (!(SPSR & _BV(SPIF)))
	;
	return SPDR;
	
}

 uint8_t RFM69::spiTransfer (uint8_t cmd, uint8_t val) {
	select();
	spiTransferByte(cmd);
	uint8_t in = spiTransferByte(val);
	unselect();
	return in;
}


 void RFM69::noInterrupts () {
	//EIMSK &= ~(1 << INT0); // disable pcint0 interrupt  //bitClear(EIMSK, INT0);
cli();
}

 void RFM69::allowInterrupts () {
	//EIMSK |= (1 << INT0); // enable pcint1 interrupt //bitSet(EIMSK, INT0);
	sei();
}

