/********
 * Arduino Pong
 * Original By Pete Lamonica
 * Modified for SED1531 Arduino library by Tom van Zutphen
 * 
 * A simple implementation of Pong on the Arduino using a glcd for output.
 * 
 * Creative Commons Non-Commerical/Attribution/Share-Alike license
 * creativecommons.org/licenses/by-nc-sa/2.0/
 *
 * Connected pins: one button on pin 13 (to GND)
 * 				   2 analog paddles on analog pins 0 and 1
 *
 *********/

#include "SED1531.h"
#include "proportional_font.h"
#include "fixed_font.h"
#include <avr/pgmspace.h>

#define HORZ_RES	100
#define VERT_RES	48

#define WHEEL_ONE_PIN 0 //analog pin for first paddle
#define WHEEL_TWO_PIN 1 //analog pin for second paddle
#define BUTTON_ONE_PIN 13 //digital pin for button to start game

#define PADDLE_HEIGHT 12
#define PADDLE_WIDTH 2
#define BALL_WIDTH 7

#define RIGHT_PADDLE_X (HORZ_RES-3)
#define LEFT_PADDLE_X 1

#define IN_GAME 0 //in game state
#define IN_MENU 1 //in menu state
#define GAME_OVER 2 //game over state

#define LEFT_SCORE_X (HORZ_RES/2-15)
#define RIGHT_SCORE_X (HORZ_RES/2+10)
#define SCORE_Y 0

#define MAX_Y_VELOCITY 3
#define PLAY_TO 7	//game over if one of the players reach this score

#define LEFT 0
#define RIGHT 1

SED1531 glcd;

//unsigned char x,y;
boolean buttonStatus = false;
int wheelOnePosition = 0;
int wheelTwoPosition = 0;
int rightPaddleY = 0;
int leftPaddleY = 0;
unsigned char ballX = 0;
unsigned char ballY = 0;
char ballVolX = 1;
char ballVolY = 1;

byte leftPlayerScore = 0;
byte rightPlayerScore = 0;

byte state = IN_MENU;

void processInputs() {
  wheelOnePosition = analogRead(WHEEL_ONE_PIN);
  wheelTwoPosition = analogRead(WHEEL_TWO_PIN);
  buttonStatus = digitalRead(BUTTON_ONE_PIN);
}

byte drawBall(byte ballX, byte ballY, byte color = GLCD_COLOR_INVERT) {
	byte collision;
	collision = glcd.drawBox(ballX+1, ballY, BALL_WIDTH-2, BALL_WIDTH, color);
	collision |= glcd.drawBox(ballX, ballY+1, 1, BALL_WIDTH-2, color);
	collision |= glcd.drawBox(ballX+BALL_WIDTH-1, ballY+1, 1, BALL_WIDTH-2, color);
	return collision;
}

void drawGameScreen() {
	glcd.clearDisplay();

  //draw score
  glcd.drawText(SCORE_Y,LEFT_SCORE_X,&fixed_font,'0'+leftPlayerScore);
  glcd.drawText(SCORE_Y,RIGHT_SCORE_X,&fixed_font,'0'+rightPlayerScore);

  //draw net
  for(int i=1; i<VERT_RES - 4; i+=6) {
    glcd.drawLine(HORZ_RES/2,i,HORZ_RES/2,i+3, 1);
  }

  drawBall(ballX, ballY);
}

void drawGamePaddles() {
	static byte OldrightPaddleY;
	static byte OldleftPaddleY;

  //update right paddle
  OldrightPaddleY = rightPaddleY;
  rightPaddleY = ((wheelOnePosition / 8) * (VERT_RES-PADDLE_HEIGHT))/ 128;
  glcd.drawBox(RIGHT_PADDLE_X,OldrightPaddleY,PADDLE_WIDTH,PADDLE_HEIGHT,0);
  glcd.drawBox(RIGHT_PADDLE_X,rightPaddleY,PADDLE_WIDTH,PADDLE_HEIGHT,1);

  //update paddle
  OldleftPaddleY = leftPaddleY;
  leftPaddleY = ((wheelTwoPosition / 8) * (VERT_RES-PADDLE_HEIGHT))/ 128;
  glcd.drawBox(LEFT_PADDLE_X,OldleftPaddleY,PADDLE_WIDTH,PADDLE_HEIGHT,0);
  glcd.drawBox(LEFT_PADDLE_X,leftPaddleY,PADDLE_WIDTH,PADDLE_HEIGHT,1);
}

//player == LEFT or RIGHT
void playerScored(byte player) {
  if(player == LEFT) {
	  leftPlayerScore++;
	  glcd.drawText(SCORE_Y,LEFT_SCORE_X,&fixed_font,'0'+leftPlayerScore);  //draw score
  }
  if(player == RIGHT) {
	  rightPlayerScore++;
	  glcd.drawText(SCORE_Y,RIGHT_SCORE_X,&fixed_font,'0'+rightPlayerScore);  //draw score
  }

  //check for win
  if(leftPlayerScore == PLAY_TO || rightPlayerScore == PLAY_TO) {
    state = GAME_OVER; 
  }

  ballVolX = -ballVolX;
}

void drawMenu() {
  byte x=0;
  byte y=ballY;
  char volX = 1;
  char volY = 1;
  buttonStatus=true;
  glcd.clearDisplay();
  glcd.drawText_P(8*0,14,&fixed_font,PSTR("Arduino Pong"));
  glcd.drawText_P(8*2,24,&proportional_font,PSTR("Press Button"));
  glcd.drawText_P(8*3,33,&proportional_font,PSTR("To Start"));
  delay(1000);
  while(buttonStatus) {
    processInputs();

    if(x + volX < 1 || x + volX > HORZ_RES - BALL_WIDTH) volX = -volX;
    if(y + volY < 1 || y + volY > VERT_RES - BALL_WIDTH) volY = -volY;

    if(drawBall(x + volX, y + volY,GLCD_COLOR_COLLISION)) {
      if(drawBall(x + volX, y - volY,GLCD_COLOR_COLLISION) == 0) {
        volY = -volY;
      }
      else if(drawBall(x - volX, y + volY,GLCD_COLOR_COLLISION) == 0) {
        volX = -volX;
      }
      else {
        volX = -volX;
        volY = -volY; 
      }
    }
    drawBall(x, y, GLCD_COLOR_INVERT);
    delay(100);
    drawBall(x, y, GLCD_COLOR_INVERT); 	//undraw ball
    x += volX;
    y += volY;
  }

  drawGameScreen();
  state = IN_GAME;
}

void setup()  {
	digitalWrite(BUTTON_ONE_PIN,HIGH);	//pullup
	glcd.init();
	ballX = HORZ_RES / 2;
	ballY = VERT_RES / 2;
}

void loop() {
	processInputs();

	if(state == IN_MENU) {
		drawMenu();
	}
	if(state == IN_GAME) {
		//undraw ball
		drawBall(ballX, ballY);

		ballX += ballVolX;
		ballY += ballVolY;
		//draw ball
		drawBall(ballX, ballY);

		if(ballY < 1 || ballY >= VERT_RES-BALL_WIDTH) ballVolY = -ballVolY;
		if(ballVolX < 0 && ballX == LEFT_PADDLE_X+PADDLE_WIDTH-1 && ballY >= leftPaddleY && ballY <= leftPaddleY + PADDLE_HEIGHT) {
			ballVolX = -ballVolX;
			ballVolY += 2 * ((ballY - leftPaddleY) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);

		}
		if(ballVolX > 0 && ballX == RIGHT_PADDLE_X-BALL_WIDTH && ballY >= rightPaddleY && ballY <= rightPaddleY + PADDLE_HEIGHT) {
			ballVolX = -ballVolX;
			ballVolY += 2 * ((ballY - rightPaddleY) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
		}

		//limit vertical speed
		if(ballVolY > MAX_Y_VELOCITY) ballVolY = MAX_Y_VELOCITY;
		if(ballVolY < -MAX_Y_VELOCITY) ballVolY = -MAX_Y_VELOCITY;

		if(ballX < 1) {
			playerScored(RIGHT);
		}
		if(ballX >= HORZ_RES - BALL_WIDTH) {
			playerScored(LEFT);
		}
		drawGamePaddles();
	}
	if(state == GAME_OVER) {
		//undraw ball
		drawBall(ballX, ballY);
		glcd.drawText_P(8*3,24,&fixed_font,PSTR("GAME"));
		glcd.drawText_P(8*3,54,&fixed_font,PSTR("OVER"));
		while(buttonStatus) {
			processInputs();
			delay(50);
		}
		leftPlayerScore = 0;
		rightPlayerScore = 0;
//		ballX, ballY =0;
//		ballVolX, ballVolY =1;
		state = IN_MENU;
	}
	delay(50);
}
