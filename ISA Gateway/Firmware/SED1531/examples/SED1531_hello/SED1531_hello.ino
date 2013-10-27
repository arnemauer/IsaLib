#include "SED1531.h"
#include "proportional_font.h"

SED1531 glcd;

void setup(){
	glcd.init();
	glcd.setFont(&proportional_font);
	glcd.clearDisplay();
}

void loop()
{
	glcd.setCursor(25,5*8);
	glcd.print(F("Hello world!"));
	delay(2000);
	for (byte scroll=0;scroll<49;scroll++){
		glcd.command(GLCD_CMD_INITIAL_DISPLAY_LINE(scroll));
		delay(150);
	}
}
