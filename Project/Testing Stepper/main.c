#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "stepperMotor.h"
#include "Timer.h"

#include <avr/io.h>

const char motorStepsBrake[] = {
	0b00110000,  // Step 0
	0b00000110,  // Step 1
	0b00101000,  // Step 2
	0b00000101   // Step 3
};

int main(void)
{

	int stepNum;
	timer8MHz();//setup the chip clock to 8 MHz
	DDRA = 0xFF;//Stepper Output
	DDRL = 0xFF;//debug lights
	
	precomputeDelayTables();
	
	stepNum = homeMotor();
	PORTA = motorSteps[stepNum];
	
	while(1){
		moveStepper(100,&stepNum);
		mTimer(1000);
	}
}

