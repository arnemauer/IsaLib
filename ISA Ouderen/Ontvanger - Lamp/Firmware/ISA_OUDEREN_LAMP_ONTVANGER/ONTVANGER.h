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
		  
#define sound_alarm_keys_doorbell  8
#define sound_alarm_keys_phone     96
#define sound_alarm_keys_fire	16
#define sound_alarm_keys_help  19

#define sound_alarm_volume 10

#define flash_keys  32

const uint8_t sound_alarm_keys[] = {sound_alarm_keys_doorbell, sound_alarm_keys_phone, sound_alarm_keys_fire, sound_alarm_keys_help };
const uint8_t alarm_bitmask[] = {0x08, 0x04, 0x02, 0x01};


uint8_t mychannel;
uint8_t counter;
static long payload;

// config
unsigned long alarm_duration = 20000; // alarm duration in ms


//
uint8_t deep_sleep_ok; // if true timer stopped everything so uc can go in deep sleep
uint8_t active_alarm; // not used, not used, not used, not used, doorbell, phone, fire, help;
unsigned long active_alarm_time; // used to track end with timer;

//sound
uint8_t  sound_current_alarm; // current alarm sound 0=doorbell, 1=phone, 2= fire, 3=help
uint8_t  sound_current_step; // current sound step from current alarm

unsigned long _sound_note_time; // Used to track end note with timer when playing note in the background.


//light - flash
uint8_t _flash_current_step; // 1 - 255
unsigned long _flash_time; // Used to track end flash with timer


//light - icons
uint8_t icon_current_alarm; //  current alarm sound 0=doorbell, 1=phone, 2= fire, 3=help
uint8_t icon_current_step; //   current sound step from current alarm
unsigned long _icon_time; // Used to track end flash with timer



typedef struct {
	unsigned long frequency; //
	unsigned long time;   // (0 tot +65,535) time in ms before going to the next step
} STRUCT_SOUND_PATTERN;

const static STRUCT_SOUND_PATTERN sound_pattern_doorbell[sound_alarm_keys_doorbell] PROGMEM = {  // doorbell
	//const static sound_patterns[0] PROGMEM = { // doorbell
	{ 1500	, 500 }, // 0: instant off
	{ 0		, 200 }, // 0: instant off
	{ 1200	, 500 }, // 0: instant off
	{ 0		, 1000 }, // 0: instant off
	{ 1500	, 500 }, // 0: instant off
	{ 0		, 200 }, // 0: instant off
	{ 1200	, 500 }, // 0: instant off
	{ 0		, 1500 } // 0: instant off
};



const static STRUCT_SOUND_PATTERN sound_pattern_phone[sound_alarm_keys_phone] PROGMEM = { //phone
	
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 0	    ,1000}, // 0: instant off
	
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 0	    ,1000}, // 0: instant off
	
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 1000	, 40 }, // 0: instant off
	{ 750	, 40 }, // 0: instant off
	{ 0	    ,1000} // 0: instant off
	
};



const static STRUCT_SOUND_PATTERN sound_pattern_fire[sound_alarm_keys_fire] PROGMEM = {  // doorbell
	//const static sound_patterns[0] PROGMEM = { // doorbell
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 }, // 0: instant off
	{ 2000	, 300 }, // 0: instant off
	{ 0		, 300 } // 0: instant off
};

const static STRUCT_SOUND_PATTERN sound_pattern_help[sound_alarm_keys_help] PROGMEM = {  // doorbell
	//const static sound_patterns[0] PROGMEM = { // doorbell
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 0		, 1000 },  // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 1820	, 200 }, // 0: instant off
	{ 1445	, 200 }, // 0: instant off
	{ 0		, 1000 }  // 0: instant off
};



/// @struct Ramp
/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
/// Ramps can be chained together with a non-zero ramp index in the chain field.
typedef struct {
	uint8_t led[4]; // bottom, top left, top center, top right, 0..255
	unsigned long time;   // (0 tot +65,535) time in ms before going to the next step
} STRUCT_FLASH_PATTERN;

const static STRUCT_FLASH_PATTERN flash_pattern[ ] PROGMEM = {
	// bottom, top left, top center, top right 0..255 , time in ms
	
	{ 255 , 255, 255, 255, 200 }, // flash, all on
	{	0 ,	  0,   0,   0, 200 }, //		all off
	{ 255 , 255, 255, 255, 200 }, // flash, all on
	{	0 ,	  0,   0,   0, 200 }, //		all off
	{ 255 , 255, 255, 255, 200 }, // flash, all on
	{	0 ,	  0,   0,   0, 200 }, //		all off
	{ 255 , 255, 255, 255, 200 }, // flash, all on
	{	0 ,	  0,   0,   0, 200 }, //		all off


	// FLASH TOP LEFT AND TOP RIGHT 3 times fast
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	
	// FLASH TOP CENTER AND BOTTOM 3 times fast
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	
	// FLASH TOP LEFT AND TOP RIGHT 3 times fast
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 0	  , 255,   0, 255,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	
	// FLASH TOP CENTER AND BOTTOM 3 times fast
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 }, // all off
	{ 255 ,   0, 255,   0,  50 }, //
	{	0 ,	  0,   0,   0,  50 } // all off
	

};


/// @struct Ramp
/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
/// Ramps can be chained together with a non-zero ramp index in the chain field.
typedef struct {
	uint8_t color[3]; // red, green, blue 0..255
} STRUCT_ICON_COLORS;

// RGB
const static STRUCT_ICON_COLORS icon_colors[ ] PROGMEM = {
	{ 61 , 245, 0 }, // doorbell - light green
	{	0 ,	  184,   245, }, // phone - lightblue
	{ 255 , 0, 0 }, // fire - RED
	{ 255 , 255, 0 }  // help - Yellow
};


/// @struct Ramp
/// A "Ramp" is a target RGB color and the time in seconds to reach that color.
/// Ramps can be chained together with a non-zero ramp index in the chain field.
typedef struct {
	uint8_t lednr[3]; // red, green, blue
} STRUCT_ICON_LED_NUMBERS;

// RGB
const static STRUCT_ICON_LED_NUMBERS icon_led_numbers[ ] PROGMEM = {
	{  12 , 11, 10 }, // doorbell - light green 210
	{	15, 14, 13 }, // phone - lightblue
	{ 7 , 8, 9 }, // fire - RED
	{ 2 , 1, 0 }  // help - Yellow
};



#endif /* ISA_GATEWAY_QUADROFLY_H_ */
