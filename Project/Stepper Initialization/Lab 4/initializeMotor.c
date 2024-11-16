#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"

#define HALL_SENSOR_PIN PIND7 //connected to D38

void homeMotor(void) {
	const char motorSteps[] = { 0b00110000,
		0b00000110,
		0b00101000,
	0b00000101 };

	int stepIdx = 0;
	while(1) {
		//check the hall effect signal
		if (!(PIND & (1 << HALL_SENSOR_PIN))) {
			PORTA = 0x00; // STOPS MOTOR
			PORTC = 0x00; // clear status
			} else {
			//MOVE ONE STEP
			PORTA = motorSteps[stepIdx];
			PORTC = (1 << stepIdx); //update the status
			
			stepIdx = (stepIdx + 1) % 4; //cycle through steps
			mTimer(20); //delay for motor movement
		}
	}
}