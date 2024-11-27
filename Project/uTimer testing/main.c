#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "Timer.h"

int main() {
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	mTimer(500);
	sei();
	while(1){
		for(int i = 0; i < 500; i += 50){
			for(int j = 0; j < 100; j++){
				PORTL = 0xF0;
				uTimer((unsigned int)(i));
				PORTL = 0x00;
				uTimer((unsigned int)(500-i));
			}
		}
	}
}