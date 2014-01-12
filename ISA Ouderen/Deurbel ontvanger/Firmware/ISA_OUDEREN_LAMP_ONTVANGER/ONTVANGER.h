/*
 * ISA_GATEWAY_QUADROFLY.h
 *
 * Created: 12-10-2013 17:10:13
 *  Author: Arne
 */ 


#ifndef ISA_GATEWAY_QUADROFLY_H_
#define ISA_GATEWAY_QUADROFLY_H_

void blinkAllLeds(bool set);
static bool waitForAck();
static void doReport();
  	void isr_sound();
  	void isr_light_flash();
  	void isr_light_icon();
		  

#endif /* ISA_GATEWAY_QUADROFLY_H_ */