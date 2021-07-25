#ifndef PONG_H
#define PONG_H
#include <avr/interrupt.h>

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

#endif //PONG_H
