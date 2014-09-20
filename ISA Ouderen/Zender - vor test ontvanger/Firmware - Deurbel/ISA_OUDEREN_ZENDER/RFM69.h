// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW, Semtech SX1231/1231H
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// 2013-06-14 (C) felix@lowpowerlab.com
// **********************************************************************************
#ifndef RFM69_h
#define RFM69_h

#include <inttypes.h>
#include "config.h"

#define MAX_DATA_LEN         61 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead)
#define RF69_IRQ_PIN          2 // INT0 on AVRs should be connected to DIO0 (ex on Atmega328 it's D2)
#define CSMA_LIMIT          -90 // upper RX signal sensitivity threshold in dBm for carrier sense access
#define RF69_MODE_SLEEP       0 // XTAL OFF
#define	RF69_MODE_STANDBY     1 // XTAL ON
#define RF69_MODE_SYNTH	      2 // PLL ON
#define RF69_MODE_RX          3 // RX MODE
#define RF69_MODE_TX		      4 // TX MODE

//available frequency bands
#define RF69_315MHZ     31  // non trivial values to avoid misconfiguration
#define RF69_433MHZ     43
#define RF69_868MHZ     86
#define RF69_915MHZ     91

#define null                  0
#define COURSE_TEMP_COEF    -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69_BROADCAST_ADDR 255
#define RF69_CSMA_LIMIT_MS 1000


class RFM69 {
  public:
    static volatile uint8_t DATA[MAX_DATA_LEN];          // recv/xmit buf, including hdr & crc bytes
    static volatile uint8_t DATALEN;
    static volatile uint8_t SENDERID;
    static volatile uint8_t TARGETID; //should match _address
    static volatile uint8_t PAYLOADLEN;
    static volatile uint8_t ACK_REQUESTED;
    static volatile uint8_t ACK_RECEIVED; /// Should be polled immediately after sending a packet with ACK request
    static volatile int RSSI; //most accurate RSSI during reception (closest to the reception)
    static volatile uint8_t _mode; //should be protected?
    
    RFM69(uint8_t slaveSelectPin=BIT_RFM_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false) {
      _slaveSelectPin = slaveSelectPin;
      _interruptPin = interruptPin;
      _mode = RF69_MODE_STANDBY;
      _promiscuousMode = false;
      _powerLevel = 31;
      _isRFM69HW = isRFM69HW;
    }

    bool initialize(uint8_t freqBand, uint8_t ID, uint8_t networkID=1);
    void setAddress(uint8_t addr);
    bool canSend();
    void send(uint8_t toAddress, const void* buffer, uint8_t bufferSize, bool requestACK=false);
    bool sendWithRetry(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries=2, uint8_t retryWaitTime=30);
    bool receiveDone();
    bool ACKReceived(uint8_t fromNodeID);
    void sendACK(const void* buffer = "", uint8_t bufferSize=0);
    void setFrequency(uint32_t FRF);
    void encrypt(const char* key);
    void setCS(uint8_t newSPISlaveSelect);
    int readRSSI(bool forceTrigger=false);
    void promiscuous(bool onOff=true);
    void setHighPower(bool onOFF=true); //have to call it after initialize for RFM69HW
    void setPowerLevel(uint8_t level); //reduce/increase transmit power level
    void sleep();
    uint8_t readTemperature(uint8_t calFactor=0); //get CMOS temperature (8bit)
    void rcCalibration(); //calibrate the internal RC oscillator for use in wide temperature variations - see datasheet section [4.3.5. RC Timer Accuracy]

    // allow hacking registers by making these public
    uint8_t readReg(uint8_t addr);
    void writeReg(uint8_t addr, uint8_t val);
    void readAllRegs();


 uint8_t spiTransferByte(uint8_t out);
 uint8_t spiTransfer(uint8_t cmd, uint8_t val);

void noInterrupts ();


void allowInterrupts ();
	 static void isr0();
	
  protected:
   
    void virtual interruptHandler();
    void sendFrame(uint8_t toAddress, const void* buffer, uint8_t size, bool requestACK=false, bool sendACK=false);

    static RFM69* selfPointer;
    uint8_t _slaveSelectPin;
    uint8_t _interruptPin;
    uint8_t _address;
    bool _promiscuousMode;
    uint8_t _powerLevel;
    bool _isRFM69HW;

	void spiInit();

    void receiveBegin();
    void setMode(uint8_t mode);
    void setHighPowerRegs(bool onOff);
    void select();
    void unselect();
};

#endif

