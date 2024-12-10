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
int delayTable90[] = {20000, 19828, 19345, 18596, 17630, 16492, 15230, 13890, 12520, 11166, 9876, 8696, 7673, 6854, 6286, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6057, 6326, 6791, 7427, 8209, 9109, 10104, 11166, 12271, 13393, 14506, 15583, 16601, 17533, 18352, 19035, 19554, 19884, 20000};
int delayTable180[] = {20000, 19948, 19796, 19552, 19224, 18820, 18347, 17813, 17224, 16591, 15918, 15216, 14490, 13749, 13000, 12251, 11510, 10784, 10082, 9409, 8776, 8188, 7653, 7180, 6776, 6448, 6204, 6052, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 6023, 6092, 6204, 6357, 6548, 6776, 7037, 7330, 7653, 8003, 8378, 8776, 9193, 9630, 10082, 10547, 11024, 11510, 12003, 12500, 13000, 13500, 13997, 14490, 14976, 15453, 15918, 16370, 16807, 17224, 17622, 17997, 18347, 18670, 18963, 19224, 19452, 19643, 19796, 19908, 19977, 20000};
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