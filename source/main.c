/*	Author: Yong Hwei Toon
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #14  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *	Pong
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *			
 *			DEMO LINK: https://youtu.be/c67mg_xQXr0
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include <stdlib.h>
#include <scheduler.h>
#include <timer.h>
#endif

unsigned char p1_pat = 0x01;
unsigned char p1_row = 0xF1;
unsigned short p1_spd = 0;
unsigned char r1 = 0;
unsigned char r2 = 0;
unsigned char bot_pat = 0x80;
unsigned char bot_row = 0xF1;
unsigned char ball_pat = 0x08;
unsigned char ball_row = 0xFB;
unsigned short ball_spd = 250;
unsigned char lr = 1;
unsigned char ud = 1;
unsigned char turnflag = 1;
unsigned char GAME = 0;
unsigned char MODE = 0;
unsigned char mode_val = 0;
unsigned char p1_score = 0;
unsigned char bot_score = 0;
unsigned char WINNER_WINNER_CHICKEN_DINNER = 0;

void transmit_data(unsigned char data1, unsigned char data2) {
    int i;
    for (i = 0; i < 8 ; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        PORTC = 0x08;
        // set SER = next bit of data to be sent.
        PORTC |= ((data1 >> i) & 0x01);
        // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
        PORTC |= 0x02;  
    }
    int j;
    for (j = 0; j < 5 ; ++j) {
        PORTD = 0x08;
        PORTD |= ((data2 >> j) & 0x01);
        PORTD |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    PORTC |= 0x04;
    PORTD |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
    PORTD = 0x00;
}

void A2D_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}

void Set_A2D_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<15; i++ ) { asm("nop"); } 
}

//Basic Requirements

enum p1Speed_States{p1Speed_wait};

int p1SpeedSMTick(int state){
	unsigned short in = ADC;
	switch(state){
		case p1Speed_wait:
			state = p1Speed_wait;
            if(in < 168 || in > 890)
                p1_spd = 50;
            else if(in < 297 || in > 780)
                p1_spd = 100;
            else if(in < 426 || in > 670)
                p1_spd = 150;
            else if(in < 555 || in > 560)
                p1_spd = 200;
			break;
		default:
			state = p1Speed_wait;
			break;
	}
	switch(state){
		case p1Speed_wait:
			break;
	}
	return state;
}

enum p1_States{p1_wait};

int p1SMTick(int state){
	unsigned short in = ADC;
    static unsigned short p1_t = 0;
	switch(state){
		case p1_wait:
			state = p1_wait;
            if(p1_t < p1_spd)
				++p1_t;
			else{
                if(in < 347){
                    if(p1_row != 0xE3){
                        p1_row = (p1_row << 1) | 0x01;

                    }
                }
                else if(in > 750){
                    if(p1_row != 0xF8)
                        p1_row = (p1_row >> 1) | 0x80;
                }
                p1_t = 0;
            }
			break;
		default:
			state = p1_wait;
			break;
	}
	switch(state){
		case p1_wait:
			break;
	}
	return state;
}

enum rng_States{rng_wait};

int rngSMTick(int state){
	switch(state){
		case rng_wait:
			state = rng_wait;
			r1 = rand() % 6;
			r2 = rand() % 3;
			break;
		default:
			state = rng_wait;
			break;
	}
	switch(state){
		case rng_wait:
			break;
	}
	return state;
}

unsigned char power(unsigned char x, unsigned char e){
	if(!e)
		return 0;
	unsigned char y = x;
	if(e > 1){
		for(unsigned char i = 0; i < e - 1; ++i)
			y *= x;
	}
	return y;
}

unsigned char follow(){
	unsigned char bot = ~bot_row;
	unsigned char ball = ~ball_row;
	unsigned char c = 0;
	while(!(bot & 1)){
		bot >>= 1;
		++c;
	}
	unsigned char x = power(2, c + 1);
	if(ball > x)
		return 2;
	else if(ball < x)
		return 0;
	else
		return 1;
}

enum bot_States{bot_wait, bot_pressed};

int botSMTick(int state){
	unsigned char bot_in = ~PINA;
	unsigned char f = follow();
	static unsigned short bot_t = 0;
	switch(state){
		case bot_wait:
			if(MODE){

//Advancement 2: Multiplayer mode split

				if(bot_in & 8 || bot_in & 16){
					state = bot_pressed;
					if(bot_in & 8){
						if(bot_row != 0xE3)
							bot_row = (bot_row << 1) | 0x01;
					}
					else{
						if(bot_row != 0xF8)
							bot_row = (bot_row >> 1) | 0x80;
					}
				}
				else
					state = bot_wait;
			}
			else{
				state = bot_wait;
				if(bot_t < 500)
					++bot_t;
				else{
					if(r1 < 3){
						if(f != 1){
							if(f){
								if(bot_row != 0xE3)
									bot_row = (bot_row << 1) | 0x01;
							}
							else{
								if(bot_row != 0xF8)
									bot_row = (bot_row >> 1) | 0x80;
							}
						}
					}
					else{

//Advancement 3: Bot will move by itself when not following the ball (more organic movements)

						if(lr){
							if(r2 == 2){
								if(bot_row != 0xE3)
										bot_row = (bot_row << 1) | 0x01;
							}
							else if(r2){
								if(bot_row != 0xF8)
										bot_row = (bot_row >> 1) | 0x80;
							}
						}
					}
					bot_t = 0;
				}
			}
			break;
		case bot_pressed:
			if(bot_in & 8 || bot_in & 16)
				state = bot_pressed;
			else
				state = bot_wait;
			break;
		default:
			state = bot_wait;
			break;
	}
	switch(state){
		case bot_wait:
			break;
		case bot_pressed:
			break;
	}
	return state;
}
//in < 347 || in > 750
//ud = {{0, left}, {1, /0}, {2, right}}

//Advancement 1: The function below calculates spnin and variable speed.

void direction(unsigned char ball, unsigned char x){
	unsigned short in = ADC;
	unsigned char Ball = ball;
	unsigned char X = x;
	while(!(X & 1)){
		X >>= 1;
		Ball >>= 1;
	}
	if(in >= 347 && in <= 750){
		if(Ball == 4){
			ud = 0;
			ball_spd = 210;
		}
		else if(Ball == 2){
			ud = 1;
			ball_spd = 250;
		}
		else{
			ud = 2;
			ball_spd = 210;
		}
	}
	else if(in < 347){
		if(ud == 1){
			if(Ball == 4){
				ud = 1;
				ball_spd = 210;
			}
			else if(Ball == 2){
				ud = 2;
				ball_spd = 210;
			}
			else{
				ud = 2;
				ball_spd = 170;
			}
		}
		else if(ud == 2){
			if(Ball == 4){
				ud = 0;
				ball_spd = 210;
			}
			else if(Ball == 2){
				ud = 2;
				ball_spd = 250;
			}
			else{
				ud = 2;
				ball_spd = 170;
			}
		}
		else{
			if(Ball == 4){
				ud = 1;
				ball_spd = 210;
			}
			else if(Ball == 2){
				ud = 1;
				ball_spd = 250;
			}
			else{
				ud = 1;
				ball_spd = 210;
			}
		}
	}
	else{
		if(ud == 1){
			if(Ball == 4){
				ud = 1;
				ball_spd = 210;
			}
			else if(Ball == 2){
				ud = 1;
				ball_spd = 250;
			}
			else{
				ud = 1;
				ball_spd = 210;
			}
		}
		else if(ud == 2){
			if(Ball == 4){
				ud = 0;
				ball_spd = 170;
			}
			else if(Ball == 2){
				ud = 0;
				ball_spd = 250;
			}
			else{
				ud = 2;
				ball_spd = 210;
			}
		}
		else{
			if(Ball == 4){
				ud = 0;
				ball_spd = 170;
			}
			else if(Ball == 2){
				ud = 0;
				ball_spd = 210;
			}
			else{
				ud = 1;
				ball_spd = 210;
			}
		}
	}
}

//ud = {{0, left}, {1, /0}, {2, right}}
enum ball_States{ball_wait};

int ballSMTick(int state){
	unsigned char p1 = ~p1_row;
	unsigned char bot = ~bot_row;
	unsigned char ball = ~ball_row;
	static unsigned short ball_t = 0;
	switch(state){
		case ball_wait:
			state = ball_wait;
			if(ball_t < ball_spd)
				++ball_t;
			else{
				if(ball_pat == 0x01 || ball_pat == 0x80){ //did someone just scored lmao
					ball_row = 0xFB;
					ud = 1;
					if(ball_pat == 0x01){
						ball_pat = 0x08;
						lr = 0;
						if(GAME){
							if(!bot_score)
								++bot_score;
							else{
								bot_score = 3;
								WINNER_WINNER_CHICKEN_DINNER = 8;
								GAME = 0;
							}
						}
					}
					else{
						ball_pat = 0x10;
						lr = 1;
						if(GAME){
							if(!p1_score)
								++p1_score;
							else{
								p1_score = 3;
								WINNER_WINNER_CHICKEN_DINNER = 2;
								GAME = 0;
							}
						}
					}
					break;
				}
				if(lr && ball_pat == 0x02 && ball & p1){ //check if it is reflected by p1
					lr = 0;
					direction(ball, p1); // where the ball should go
					if(ball_row == 0xFE)
						turnflag = 2;
					else if(ball_row == 0xEF)
						turnflag = 0;
				}
				else if(!lr && ball_pat == 0x40 && ball & bot){ //same thing but with p2/bot
					lr = 1;
					direction(ball, bot);
					if(ball_row == 0xFE)
						turnflag = 2;
					else if(ball_row == 0xEF)
						turnflag = 0;
				}
				if(lr)
					ball_pat >>= 1;
				else
					ball_pat <<= 1;
				if(ud == 0){
					if(ball_row != 0xEF)
						ball_row = (ball_row << 1) | 0x01;
					if(ball_row == 0xEF)
						ud = 2;
				}
				else if(ud == 2){
					if(ball_row != 0xFE)
						ball_row = (ball_row >> 1) | 0x80;
					if(ball_row == 0xFE)
						ud = 0;
				}
				if(turnflag != 1){
					if(turnflag)
						ball_row = (ball_row << 1) | 0x01;
					else
						ball_row = (ball_row >> 1) | 0x80;
					turnflag = 1;
				}
				ball_t = 0;
			}
			break;
		default:
			state = ball_wait;
			break;
	}
	switch(state){
		case ball_wait:
			break;
	}
	return state;
}

enum game_States{game_wait, game_pressed};

int gameSMTick(int state){
	unsigned char game_in = ~PINA;
	static unsigned char x = 0;
	static unsigned char y = 0;
	switch(state){
		case game_wait:
			if(game_in & 4){
				state = game_pressed;
				if(GAME)
					GAME = 0;
				else
					x = 1;
			}
			else
				state = game_wait;
			break;
		case game_pressed:
			if(game_in & 4)
				state = game_pressed;
			else{
				state = game_wait;
				if(x){
					x = 0;
					y = 1;
				}
			}
			break;
		default:
			state = game_wait;
			break;
	}
	switch(state){
		case game_wait:
			break;
		case game_pressed:
			break;
	}
	if(y){
		GAME = 1;
		p1_pat = 0x01;
		p1_row = 0xF1;
		bot_pat = 0x80;
		bot_row = 0xF1;
		ball_pat = 0x08;
		ball_row = 0xFB;
		ball_spd = 250;
		lr = 1;
		ud = 1;
		turnflag = 1;
		p1_score = 0;
		bot_score = 0;
		y = 0;
	}
	return state;
}

enum modeSelect_States{modeSelect_wait, modeSelect_pressed};

int modeSelectSMTick(int state){
	unsigned char modeSelect_in = ~PINA;
	unsigned short a = ADC;
	if(!GAME){
		if(a < 347)
			MODE = 0;
		else if(a > 751)
			MODE = 1;
	}
	switch(state){
		case modeSelect_wait:
			if(modeSelect_in & 8){
				state = modeSelect_pressed;
				if(!GAME){
					if(MODE)
						MODE = 0;
					else
						MODE = 1;
				}
			}
			else
				state = modeSelect_wait;
			break;
		case modeSelect_pressed:
			if(modeSelect_in & 8)
				state = modeSelect_pressed;
			else
				state = modeSelect_wait;
			break;
		default:
			state = modeSelect_wait;
			break;
	}
	switch(state){
		case modeSelect_wait:
			break;
		case modeSelect_pressed:
			break;
	}
	return state;
}

//part of Advancement 4: When game is not ongoing, one of two LEDs are set to blink to show which game mode is selected.

enum modeBlink_States{modeBlink_wait};

int modeBlinkSMTick(int state){
	static unsigned short modeBlink_t = 0;
	switch(state){
		case modeBlink_wait:
			if(modeBlink_t < 500)
				++modeBlink_t;
			else{
				if(mode_val == 0){
					if(MODE)
						mode_val = 1;
					else
						mode_val = 4;
				}
				else
					mode_val = 0;
				modeBlink_t = 0;
			}
			break;
		default:
			state = modeBlink_wait;
			break;
	}
	switch(state){
		case modeBlink_wait:
			break;
	}
	return state;
}

enum display_States{display_wait};

int displaySMTick(int state){
	unsigned char out_pat[3] = {p1_pat, bot_pat, ball_pat};
	unsigned char out_row[3] = {p1_row, bot_row, ball_row};
	
//part of Advancement 4: When game is not ongoing, display numbers 1 and 2 on the LED matrix.
	
	unsigned char default_pat[7] = {0x46, 0xC1, 0x42, 0x44, 0xE7, 0xE7, 0xE7};
	unsigned char default_row[7] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xEF};
	static unsigned char i = 0;
	unsigned char bot_shifted = bot_score << 2;
	switch(state){
		case display_wait:
			state = display_wait;
			if(GAME){
				if(i >= 3)
					i = 0;
			}
			else{
				if(i >= 7)
					i = 0;
			}
			if(GAME)
				transmit_data(out_pat[i], out_row[i]);
			else
				transmit_data(default_pat[i], default_row[i]);
			++i;
			break;
		default:
			state = display_wait;
			break;
	}
	switch(state){
		case display_wait:
			break;
	}
	if(GAME)
		PORTB = bot_shifted | p1_score;
	else
		PORTB = mode_val | WINNER_WINNER_CHICKEN_DINNER;
	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    
    A2D_init();
    Set_A2D_Pin(1);
    
    static task task1, task2, task3, task4, task5, task6, task7, task8, task9;
    task *tasks[] = {&task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8, &task9};
    const unsigned short numTasks = sizeof(tasks) / sizeof(task*);
    
    const char start = -1;
    
    task1.state = start;
    task1.period = 1;
    task1.elapsedTime = task1.period;
    task1.TickFct = &p1SpeedSMTick;

    task2.state = start;
    task2.period = 1;
    task2.elapsedTime = task2.period;
    task2.TickFct = &p1SMTick;

    task3.state = start;
    task3.period = 500;
    task3.elapsedTime = task3.period;
    task3.TickFct = &rngSMTick;
    
    task4.state = start;
    task4.period = 1;
    task4.elapsedTime = task4.period;
    task4.TickFct = &botSMTick;
    
    task5.state = start;
    task5.period = 1;
    task5.elapsedTime = task5.period;
    task5.TickFct = &ballSMTick;
    
	task6.state = start;
    task6.period = 1;
    task6.elapsedTime = task6.period;
    task6.TickFct = &gameSMTick;
    
	task7.state = start;
    task7.period = 1;
    task7.elapsedTime = task7.period;
    task7.TickFct = &modeSelectSMTick;
	
	task8.state = start;
    task8.period = 1;
    task8.elapsedTime = task8.period;
    task8.TickFct = &modeBlinkSMTick;
    
	task9.state = start;
    task9.period = 1;
    task9.elapsedTime = task9.period;
    task9.TickFct = &displaySMTick;
	
    unsigned short i;
    
    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; ++i)
    	GCD = findGCD(GCD,tasks[i]->period);
    
    TimerSet(GCD);
    TimerOn();
    
    while (1) {
		for(i = 0; i < numTasks; ++i){
			if(tasks[i]->elapsedTime == tasks[i]->period){
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += GCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
    return 0;
}
