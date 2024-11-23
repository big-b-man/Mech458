#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"
#include "stepperMotor.h"

#define HALL_SENSOR_PIN PIND7 // Connected to D38

const char motorSteps[] = {
	0b00110101,  // Step 0
	0b00110110,  // Step 1
	0b00101110,  // Step 2
	0b00101101   // Step 3
};

// Lookup table for 90 degree and 180 degree moves

int delayTable90[] = {46,19,15,12,11,10,9,9,8,8,7,7,7,7,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,7,7,7,7,8,8,9,9,10,11,13,15,19,46}; // Delay table for 90 degree turns
int delayTable180[] = {46,19,15,12,11,10,9,9,8,8,7,7,7,7,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,7,7,7,7,8,8,9,9,10,11,13,15,19,46};

// Function to home the motor
int homeMotor(void) {
	int stepIdx = 0;
	while (1) {
		// Check the hall effect sensor
		if (!(PIND & (1 << HALL_SENSOR_PIN))) {
			PORTA = 0x00; // Stop the motor
			return(stepIdx);
			} else {
			// Move one step
			stepIdx = (stepIdx + 1) % 4; // Cycle through steps
			PORTA = motorSteps[stepIdx];
			mTimer(20); // Delay for motor movement
		}
	}
}

// Function to move the stepper motor with variable delay based on the lookup table
void moveStepper(int moveNum, int* stepNumInput){
	int stepNum = *stepNumInput;
	int *ptr;
	if(moveNum >= 0){
		static int forSteps[] = {0,1,2,3};
		ptr = forSteps;
		} else {
		static int backSteps[] = {2,3,0,1};
		ptr = backSteps;
		moveNum = -moveNum;
	}
	for(int i=0; i < moveNum; i++){
		switch(stepNum){
			case(3):
			PORTA = motorSteps[*ptr];
			stepNum = *ptr;
			break;
			case(0):
			PORTA = motorSteps[*(ptr+1)];
			stepNum = *(ptr+1);
			break;
			case(1):
			PORTA = motorSteps[*(ptr+2)];
			stepNum = *(ptr+2);
			break;
			case(2):
			PORTA = motorSteps[*(ptr+3)];
			stepNum = *(ptr+3);
			break;
			default:
			break;
		}
		if (moveNum == 50){
			mTimer(delayTable90[i]);
			} else if (moveNum == 100){
			mTimer(delayTable180[i]);
			} else {
			mTimer(20);
		}
	}
	*stepNumInput = stepNum;
}