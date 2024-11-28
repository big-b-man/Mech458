#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "Timer.h"

int main() {
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRC = 0xFF;
	
	cli(); // disable all of the interrupt ==================================
	
	//Setup Timer 3 for belt delay
	//Prescaler to 256, page 157 for details
	TCCR3A = 0x00;
	TCCR3B = 0x00;
	TCCR3B |= (1 << CS32);
	//time to count to max value: 256 *(2^16-1) =16,776,960 cycles ~= 2 seconds
	
	//set registers on timer to 0 (Set to value if you want delay less than 2 seconds
	TCNT3H = 0x00;
	TCNT3L = 0x00;
	
	
	// sets the Global Enable for all interrupts ============================
	sei();
	while(1){
		while (!(TIFR3 & (1 << TOV3))){
			PORTC = !(TIFR3 & (1 << TOV3));
			mTimer(1);
		}
		PORTC = !(TIFR3 & (1 << TOV3));
		mTimer(500);
		TCNT3H = 0x00;
		TCNT3L = 0x00;
		TIFR3 |= (1 << TOV3);
	}
}