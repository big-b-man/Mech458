#include "pwm.h"
#include <avr/interrupt.h> /* Needed for interrupt functionality */

void pwm(){
	//Step 1
	// Set Timer0 to Fast PWM mode (WGM02:0 = 011)
	TCCR0A |= (1 << WGM00) | (1 << WGM01);  // Set WGM01 and WGM00
	TCCR0B &= ~(1 << WGM02);                // Clear WGM02 for Fast PWM, TOP=0xFF

	//Step 2
	// Enable the Output Compare Match A Interrupt for Timer 0
	//TIMSK0 |= (1 <<  (OCIE0A));// Enable overflow interrupt

	//step 3
	// Set Compare Match Output Mode to clear on compare match and set at TOP (non-inverting mode)
	TCCR0A |= (1 << COM0A1);  // Set COM0A1 to 1
	TCCR0A &= ~(1 << COM0A0); // Clear COM0A0 to 0
	
	//step 4
	// Set the prescaler
	TCCR0B |= (1 << CS01);// CS02:0 = 010 (clk/64 prescale)
	
	//Step 5
	//For a duty cycle of 50%, ORCA should be 127, which is half of 255 rounded to nearest integer
	OCR0A = 127;
	
	//Step 6
	DDRB |= (1 << DDB7);  // Configure PORTB7 as output (OC0A is on PB7)
}

void pwmSet(unsigned char input){//sets PWM duty cycle
	OCR0A = input;
}