#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"
#include "stepperMotor.h"

#define HALL_SENSOR_PIN PIND7 //connected to D38

const char motorSteps[] = { 0b00110000,
	0b00000110,
	0b00101000,
0b00000101 };// steps for stepper motor

int homeMotor(void) {//This is the function you made in the lab Cody. The only difference is that I made it return the current step of the motor once the motor has hit it's home position.
	int stepIdx = 0;
	while(1) {
		//check the hall effect signal
		if (!(PIND & (1 << HALL_SENSOR_PIN))) {
			PORTA = 0x00; // STOPS MOTOR
			return(stepIdx);
			} else {
			PORTL = 0b11000000;
			//MOVE ONE STEP
			PORTA = motorSteps[stepIdx];
			stepIdx = (stepIdx + 1) % 4; //cycle through steps
			mTimer(10); //delay for motor movement
		}
	}
}

int moveStepper(int moveNum, int stepNum){//You will write a subroutine for this function to enable variable time delay between the motor steps.
	int *ptr;
	if(moveNum >= 0){
		int forSteps[] = {0,1,2,3};
		ptr = forSteps;
		} else {
		int backSteps[] = {2,3,0,1};
		ptr = backSteps;
	}
	
	
	// Define trapezoidal profile parameters
	int accelSteps = moveNum / 4;              // Number of steps for acceleration
	int decelSteps = moveNum / 4;              // Number of steps for deceleration
	int constSteps = moveNum - accelSteps - decelSteps; // Remaining steps at constant speed
	int minDelay = 5;                          // Minimum delay (top speed) in ms
	int maxDelay = 20;                         // Maximum delay (start and end) in ms

	for (int i = 0; i < moveNum; i++) {
		
		// Calculate the current delay based on position in the profile
		int currentDelay;
		if (i < accelSteps) {
			// Acceleration phase: delay decreases
			currentDelay = maxDelay - (i * (maxDelay - minDelay) / accelSteps);
			} else if (i < accelSteps + constSteps) {
			// Constant speed phase: delay is constant
			currentDelay = minDelay;
			} else {
			// Deceleration phase: delay increases
			currentDelay = minDelay + ((i - accelSteps - constSteps) * (maxDelay - minDelay) / decelSteps);
		}
		
		switch(stepNum){
			case(3):
			PORTA = motorSteps[*ptr];
			PORTC = 0x01;
			stepNum = *ptr;
			break;
			case(0):
			PORTA = motorSteps[*(ptr+1)];
			PORTC = 0x02;
			stepNum = *(ptr+1);
			break;
			case(1):
			PORTA = motorSteps[*(ptr+2)];
			PORTC = 0x04;
			stepNum = *(ptr+2);
			break;
			case(2):
			PORTA = motorSteps[*(ptr+3)];
			PORTC = 0x08;
			stepNum = *(ptr+3);
			break;
			default:
			break;
		}
		mTimer(currentDelay); //turn this into a function that changes the delay with each step.
	}
	return(stepNum);
}


