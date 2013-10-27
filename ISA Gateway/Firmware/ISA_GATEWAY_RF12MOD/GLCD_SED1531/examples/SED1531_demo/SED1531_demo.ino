#include "SED1531.h"
#include "proportional_font.h"
#include "fixed_font.h"
#include <avr/pgmspace.h>

SED1531 glcd;

static void effect_delay()
{
    delay(20);
}

static void slide_delay()
{
    delay(2000);
}

static unsigned char semirandom(){
    static unsigned char number = 0;

    number <<= 1;
    if((number & 0x80) == 0)
    {
        number ^= 0x4B;
    }

    return number;
}

static void slide_transition(const __FlashStringHelper *title)
{
    unsigned char i;
    unsigned char left;
    unsigned char top;
    unsigned char width;
    unsigned char height;

    for(i = 0; i < 20; ++i)
    {
        glcd.drawBox(0, 47-i, 100, 1, 1);
        glcd.drawBox(0, 8+i, 100, 1, 1);
        effect_delay();
    }

    glcd.drawBox(0, 0, 100, 8, 0);
    glcd.setCursor(0,0);
    glcd.print(title);

    for(i = 1; i < 20; ++i)
    {
        left = 19 - i;
        top = 28 - i;
        width = i * 2 + 62;
        height = i * 2;

        glcd.drawBox(left, top, width, height, 0);
        effect_delay();
    }
}

static void slide_intro()
{
    unsigned char left = 0;
    unsigned char top = 4;
    unsigned char width = 97;
    unsigned char height = 40;

    glcd.clearDisplay();

    glcd.drawBox(left+2,top+2, width, height, 1);
    glcd.drawBox(left,top,width, height, 0);
    glcd.drawFrame(left,top,width, height, 1, 1);

    glcd.setCursor(5,1*8);
    glcd.print(F("SED1531 Lib Demo on:"));
    glcd.setCursor(5,2*8);
    glcd.print(F("- Atmel ATmega328"));
    glcd.setCursor(5,3*8);
    glcd.print(F("- 8-bits, 16 MHz"));
    glcd.setCursor(5,4*8);
    glcd.print(F("- 32KB Flash, 2KB RAM"));
    slide_delay();
    for (uint8_t i=50;i>=5;i--){
    	glcd.setCursor(i,2*8);
    	glcd.print(F("- Arduino"));
    	delay(40);
    }
}

static void slide_pixel()
{
    unsigned char x;
    unsigned char y;
    unsigned char d;

    slide_transition(F("SED1531 Lib - Pixel"));

    /* Draw background grid */
    glcd.drawFrame(0, 10, 100, 36, 1, 1);

    for(y = 16; y < 46; y += 8)
    {
        for(x = 2; x < 99; x += 2)
        {
        	glcd.drawPixel(x, y, 1);
        }
    }

    /* Draw a semi-random graph on top of the grid. */
    y = 26;

    for(x = 1; x < 99; ++x)
    {
    	glcd.drawPixel(x, y, 1);
        d = semirandom();

        if((d & 0x80) && (y < 45))
        {
           ++y;
        }
        else if(y > 12)
        {
           --y;
        }

        glcd.drawPixel(x, y, 1);
    }
}

static void slide_line()
{
    unsigned char i;

    slide_transition(F("SED1531 Lib - Line"));

    for(i = 0; i < 100; i += 4)
    {
    	glcd.drawLine(i, 10, 99-i, 45, 1);
    }
}

static void slide_circle()
{
    unsigned char i;

    slide_transition(F("SED1531 Lib - Circle"));

    for(i = 4; i < 100; i += 4)
    {
    	if (i<50) glcd.drawCircle(i-2, 46-i/2, i/4, 1);
    	else glcd.drawCircle(i-2, i/2-1, i/4, 1);
    }
    glcd.drawLine(0, 46, 99, 46, 0);

    slide_delay();

    glcd.drawCircle(80, 28, 18, 0+GLCD_COLOR_FILLED);
    glcd.setCursor(71,3*8);
    glcd.print(F("Filled"));
    glcd.drawCircle(80, 28, 16, 2+GLCD_COLOR_FILLED);
    glcd.drawCircle(10, 20, 8, 1+GLCD_COLOR_FILLED);
    glcd.drawCircle(10, 20, 4, 0);
}

static void slide_box()
{
    unsigned char x;
    unsigned char i;

    slide_transition(F("SED1531 Lib - Box"));

    x = 2;
    for(i = 2; i < 11; ++i)
    {
        unsigned char height = i * 2;
        unsigned char width = i + 3;
        glcd.drawBox(x, 45 - height, width, height, 1);
        x += 2 + width;
    }

    slide_delay();
    glcd.setCursor(13,2*8);
    glcd.print(F("Inverted box"));
    glcd.drawBox(11, 14, 60, 32, 2);
}

static void slide_text()
{

    slide_transition(F("SED1531 Lib - Text"));
    glcd.setCursor(4,2*8);
    glcd.print(F("- Proportional fonts"));
    glcd.setCursor(4,3*8);
    glcd.print(F("- Multiple fonts"));
	glcd.setFont(&fixed_font);
    glcd.setCursor(4,4*8);
    glcd.print(F("- Fixed fonts"));
    slide_delay();
    for (uint8_t i=32;i<=38;i++){
    	glcd.setCursor(4,i);
    	glcd.print(F("- Fixed fonts"));
    	delay(175);
    	if (i<38) glcd.drawBox(4,i,90,1,0);
    }
	glcd.setFont(&proportional_font);
}

static void slide_pattern()
{
	glcd.clearDisplay();
	for (byte i=2;i<80;i+=4){
		glcd.drawCircle(20, 24, i, 2+GLCD_COLOR_FILLED);
		glcd.drawCircle(80, 24, i, 2+GLCD_COLOR_FILLED);
	}
	glcd.setIndicator(GLCD_INDICATOR_3,0);	//pattern writes to line 48 which are the indicators, clear it here
}
const char PROGMEM indicators[] = {
	GLCD_INDICATOR_0,
	GLCD_INDICATOR_1,
	GLCD_INDICATOR_2,
	GLCD_INDICATOR_3,
	GLCD_INDICATOR_4,
	GLCD_INDICATOR_5
};

static void slide_indicators()
{
	for (uint8_t i=0;i<6;i++){
		glcd.setIndicator(pgm_read_byte(&indicators[i]),1);
    	delay(255);
	}
}

const char PROGMEM logo_data[] = {
0, 46, 26, 1, 28, 1, 54, 46,
54, 46,0 , 46, 1, 46, 26, 1,
28, 1, 54, 45, 54, 46, 0, 46,
1, 47, 27, 1, 27, 1, 54, 47,
54, 47, 0, 47, 28, 41, 31, 38,
28, 40, 30, 38|128,
30, 36, 3, 2, 26, 29, 6, 7,
24, 35, 2, 2, 23, 36, 1, 2,
32, 30, 2, 5, 34, 29, 2, 4,
32, 27, 3, 2, 31, 24, 2, 4,
33, 20, 1, 4, 24, 25, 5, 7,
25, 23, 4, 2, 21, 21, 8, 2,
24, 19, 4, 2, 21, 26, 3, 5,
19, 25, 3, 1, 18, 26, 1, 2,
20, 29, 2, 2, 19, 29, 1, 4,
20, 33, 1, 2|128,
27 ,0, 27 ,2, 29 ,36, 32 ,23,
32 ,20, 34 ,21, 23 ,20, 28 ,20,
28 ,20, 22 ,23, 26 ,18, 20 ,24,
21 ,35|128,
31 ,29, 33 ,30, 35 ,32, 21 ,28,
0,49, 0,61, 54,49,54,61|128
};

static void slide_logo()
{
	glcd.clearDisplay();
    uint8_t i;
    uint8_t x=22;
    uint8_t d[4];
    const PROGMEM char* p = logo_data;
    d[3]=0;
    while((d[3]&128)!=128){
    	for (i=0;i<4;i++) d[i] = pgm_read_byte(p++);
    	glcd.drawLine(d[0]+x,d[1],d[2]+x,d[3]&127,1);
    }
    d[3]=0;
    while((d[3]&128)!=128){
    	for (i=0;i<4;i++) d[i] = pgm_read_byte(p++);
    	glcd.drawBox(d[0]+x,d[1],d[2],d[3]&127,1);
    }
    while((d[1]&128)!=128){
    	for (i=0;i<2;i++) d[i] = pgm_read_byte(p++);
    	glcd.drawPixel(d[0]+x,d[1]&127,1);
    }
	glcd.drawFrame(x,49,55,13,1,2);
    d[1]=0;
    while((d[1]&128)!=128){
    	for (i=0;i<2;i++) d[i] = pgm_read_byte(p++);
    	glcd.drawPixel(d[0]+x,d[1]&127,0);
    }
	glcd.setFont(&fixed_font);
    glcd.setCursor(x+7,6*8+4);
    glcd.print(F("TkkrLab"));
	glcd.setFont(&proportional_font);
	slide_delay();
	for (uint8_t i=1;i<=16;i++) {
		glcd.command(GLCD_CMD_INITIAL_DISPLAY_LINE(i));
		delay(75);
	}
	slide_delay();
	for (char i=15;i>=0;i--) {
		glcd.command(GLCD_CMD_INITIAL_DISPLAY_LINE(i));
		delay(75);
	}
}

void setup(){
	glcd.init();
	glcd.setFont(&proportional_font);
}

void loop()
{
  	slide_delay();

  	slide_logo();
  	slide_delay();

  	slide_intro();
  	slide_delay();


	slide_delay();
	slide_pixel();

	slide_delay();
	slide_line();

	slide_delay();
	slide_circle();


	slide_delay();
	slide_box();

	slide_delay();
	slide_text();

	slide_delay();
	slide_pattern();

	slide_delay();
	slide_indicators();
	slide_delay();
}
