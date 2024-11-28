#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"
#include "stepperMotor.h"
#include "lcd.h"

#define HALL_SENSOR_PIN PIND7 // Connected to D38

const char motorSteps[] = {
	0b00110101,  // Step 0
	0b00110110,  // Step 1
	0b00101110,  // Step 2
	0b00101101   // Step 3
};
int delayTable90[] = {15000, 14797.6, 14241.5, 13408.7, 12376.1, 11220.4, 10018.7, 8847.66, 7784.26, 6905.35, 6287.81, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6059.28, 6287.81, 6666.67, 7173.07, 7784.26, 8477.44, 9229.83, 10018.7, 10821.2, 11614.6, 12376.1, 13082.9, 13712.3, 14241.5, 14647.7, 14908.1, 15000};
int delayTable180[] = {15000, 14972.9, 14893.8, 14766.4, 14594.1, 14380.7, 14129.5, 13844.2, 13528.3, 13185.4, 12819, 12432.7, 12030, 11614.6, 11189.9, 10759.5, 10327, 9895.86, 9469.73, 9052.14, 8646.64, 8256.78, 7886.13, 7538.24, 7216.66, 6924.95, 6666.67, 6445.37, 6264.6, 6127.93, 6038.92, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6007.8, 6038.92, 6092.79, 6168.37, 6264.6, 6380.43, 6514.8, 6666.67, 6834.97, 7018.65, 7216.66, 7427.95, 7651.46, 7886.13, 8130.92, 8384.78, 8646.64, 8915.45, 9190.17, 9469.73, 9753.09, 10039.2, 10327, 10615.4, 10903.4, 11189.9, 11473.9, 11754.3, 12030, 12300.1, 12563.5, 12819, 13065.7, 13302.5, 13528.3, 13742.1, 13942.9, 14129.5, 14301, 14456.2, 14594.1, 14713.8, 14814, 14893.8, 14952.1, 14987.8, 15000};
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
			//mTimer(15);
			uTimer(delayTable90[i]);
			} else if (moveNum == 100){
			//mTimer(15);
			uTimer(delayTable180[i]);
			} else {
			mTimer(20);
		}
	}
	*stepNumInput = stepNum;
}