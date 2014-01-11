/*
 * tone.h
 *
 * Created: 1/11/2014 3:36:42 PM
 *  Author: arnefrankmauer
 */ 


#ifndef TONE_H_
#define TONE_H_


	void toneAC(unsigned long frequency = 0, uint8_t volume = 10, unsigned long length = 0, uint8_t background = false);
	void Alarm(int variant,int vption);
	void noToneAC();
	void tone_init();



#endif /* TONE_H_ */