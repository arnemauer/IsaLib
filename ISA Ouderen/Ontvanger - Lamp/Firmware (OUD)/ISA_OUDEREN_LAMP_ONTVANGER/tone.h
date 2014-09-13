/*
 * tone.h
 *
 * Created: 1/11/2014 3:36:42 PM
 *  Author: arnefrankmauer
 */ 


#ifndef TONE_H_
#define TONE_H_

#include <inttypes.h>

	void tone(unsigned long frequency, uint8_t volume);
	void noTone();
	void tone_init();



#endif /* TONE_H_ */