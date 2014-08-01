#ifndef RF69_h
#define RF69_h



#define RF12_HDR_IDMASK      0x7F
#define RF12_HDR_ACKCTLMASK  0x80
#define RF12_DESTID   (rf12_hdr1 & RF12_HDR_IDMASK)
#define RF12_SOURCEID (rf12_hdr2 & RF12_HDR_IDMASK)


namespace RF69 {
    extern uint32_t frf;
    extern uint8_t  group;
    extern uint8_t  nodeID;
    extern uint8_t  rssi;

    void setFrequency (uint32_t freq);
    bool canSend ();
    bool sending ();
    void sleep (bool off);
    uint8_t control(uint8_t cmd, uint8_t val);
    
    void configure_compat ();
    uint16_t recvDone_compat (uint8_t* buf);
	uint8_t recvDone ();
	void sendWait (uint8_t mode);
	
	
    void sendStart (uint8_t toNodeID, bool requestACK, bool sendACK, const void* ptr, uint8_t len);
	void sendNow (uint8_t toNodeID, bool requestACK, bool sendACK, const void* ptr, uint8_t len);
    void interrupt_compat();
};

#endif
