/* ##################################################################
# MILESTONE: 1
# PROGRAM: 1 
# PROJECT: Lab1 Demo 
# GROUP: 3?
# NAME 1: Bennett, Steers, V01025919
# NAME 2: First Cody, Short, V01025830
# DESC: This program does...cool stuff [write out a brief summary] 
# DATA
#  REVISED  ############################################################### */ 
#include <stdlib.h>//  the header of the general-purpose standard library of C programming language 
#include <avr/io.h>// the header of I/O port
#include <util/delay_basic.h>
/*  ################## MAIN ROUTINE ################## */ 

void delaynus(int n);
void delaynms(int n);

int main(int argc, char *argv[]){
DDRL = 0b11111111; //  Sets all pins on PORTL to  output
PORTL = 0b01100000; // Sets error light pins
DDRC = 0b11111111; //  Sets all pins on PORTC to  output
PORTC = 0b11000000; //	Sets 2 LEDS on the left of the red row to be high

while(1){
	//Starts moving the light string right by setting LED's high in sequence until the 4 leftmost LEDs are on
	for(int i=0;i<2;i++){
		delaynms(500);
		PORTC = (PORTC >> 1);
		PORTC |= (1 << PC7); // Set PC7 (pin 7 of PORTC) to high
	}
	
	
	//Shifts these 4 lights to the right until only 2 lights are left illuminated
	for(int i=0;i<6;i++){
		delaynms(500);
		PORTC = (PORTC >> 1);
	}
	
	
	//Turns on the 2 lights that were extinguished by the shift
	for(int i=0;i<2;i++){
		delaynms(500);
		PORTC = (PORTC << 1);
		PORTC |= (1 << PC0); // Set PC0 (pin 0 of PORTC) to high
	}
	
	//Shifts back right
	for(int i=0;i<6;i++){
		delaynms(500);
		PORTC = (PORTC << 1);
	}
	
	//Repeat infinitely
}
return (0);
// This line returns a 0 value to the calling program
//  generally means no error was returned
}

void delaynus(int n) //   delay microsecond
{
	int k;
	for(k=0;k<n;k++)
	_delay_loop_1(1);
}
void delaynms(int n) //   delay millisecond
{
	int k;
	for(k=0;k<n;k++)
	delaynus(1000);
}