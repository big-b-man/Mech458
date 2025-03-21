#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"
#include "stepperMotor.h"

#define HALL_SENSOR_PIN PIND7 // Connected to D38

const char motorSteps[] = {
	0b00110000,  // Step 0
	0b00000110,  // Step 1
	0b00101000,  // Step 2
	0b00000101   // Step 3
};

// Lookup table for 90 degree and 180 degree moves
#define MAX_STEPS_90 100
#define MAX_STEPS_180 100

int delayTable90[MAX_STEPS_90]; // Delay table for 90 degree turns
int delayTable180[MAX_STEPS_180]; // Delay table for 180 degree turns

int delayTablesInitialized = 0; // Flag to check if tables have been computed

// Function to initialize the delay table with the trapezoidal profile
void initializeDelayTable(int *delayTable, int maxSteps) {
	int accelSteps = maxSteps / 4;               // Number of steps for acceleration
	int decelSteps = maxSteps / 4;               // Number of steps for deceleration
	int constSteps = maxSteps - accelSteps - decelSteps; // Remaining steps at constant speed
	int minDelay = 5;                           // Minimum delay (top speed) in ms
	int maxDelay = 20;                          // Maximum delay (start and end) in ms

	for (int i = 0; i < maxSteps; i++) {
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
		delayTable[i] = currentDelay;
	}
}

// Function to precompute the delay tables
void precomputeDelayTables() {
	if (delayTablesInitialized) {
		return; // Skip if already initialized
	}

	// Initialize delay tables for 90 and 180 degree moves
	initializeDelayTable(delayTable90, MAX_STEPS_90);
	initializeDelayTable(delayTable180, MAX_STEPS_180);

	delayTablesInitialized = 1; // Mark as initialized
}

// Function to home the motor
int homeMotor(void) {
	int stepIdx = 0;
	while (1) {
		// Check the hall effect sensor
		if (!(PIND & (1 << HALL_SENSOR_PIN))) {
			PORTA = 0x00; // Stop the motor
			return(stepIdx);
			} else {
			PORTL = 0b11000000;
			// Move one step
			PORTA = motorSteps[stepIdx];
			stepIdx = (stepIdx + 1) % 4; // Cycle through steps
			mTimer(10); // Delay for motor movement
		}
	}
}

// Function to move the stepper motor with variable delay based on the lookup table
int moveStepper(int moveNum, int stepNum) {
	int *ptr;
	if (moveNum >= 0) {
		int forSteps[] = {0, 1, 2, 3};
		ptr = forSteps;
		} else {
		int backSteps[] = {2, 3, 0, 1};
		ptr = backSteps;
	}

	// Define the maximum steps for each move type
	int maxSteps = (moveNum == 90) ? MAX_STEPS_90 : MAX_STEPS_180;

	// Use the precomputed delay table based on the move type (90 or 180 degrees)
	int *delayTable = (moveNum == 90) ? delayTable90 : delayTable180;

	// Perform the move based on the number of steps
	for (int i = 0; i < maxSteps; i++) {
		switch (stepNum) {
			case 3:
			PORTA = motorSteps[*ptr];
			PORTC = 0x01;
			stepNum = *ptr;
			break;
			case 0:
			PORTA = motorSteps[*(ptr + 1)];
			PORTC = 0x02;
			stepNum = *(ptr + 1);
			break;
			case 1:
			PORTA = motorSteps[*(ptr + 2)];
			PORTC = 0x04;
			stepNum = *(ptr + 2);
			break;
			case 2:
			PORTA = motorSteps[*(ptr + 3)];
			PORTC = 0x08;
			stepNum = *(ptr + 3);
			break;
			default:
			break;
		}

		// Use the delay from the precomputed table
		mTimer(delayTable[i]);
	}

	return stepNum;
}
