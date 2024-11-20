#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "stepperMotor.h"
#include "timer.h"

volatile int stepNum;//This stores which step the motor is currently stopped on

int main(){
	timer8MHz();//setup the chip clock to 8 MHz
	DDRA = 0xFF;//Sets all of PORTA to output as this controls the stepper
	DDRL = 0xF0;//sets bits 4-7 in the PORTL Register to output. These 4 pins control LED's on the board that can be used for debugging
	sei();//enables all interupts for the timer

	while(1){
		stepNum = homeMotor();
		
		stepNum = moveStepper(50,stepNum);//move motor 90 degrees forward.
		stepNum = moveStepper(100,stepNum);//move motor 180 degrees forward.
		stepNum = moveStepper(-50,stepNum);//move motor 90 degrees backwards.
		stepNum = moveStepper(-100,stepNum);//move motor 180 degrees backwards.
	}
	return(0);
}