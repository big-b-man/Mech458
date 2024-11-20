#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "stepperMotor.h"
#include "Timer.h"

#include <avr/io.h>


int main(void)
{

	int stepNum;
	timer8MHz();//setup the chip clock to 8 MHz
	DDRA = 0xFF;//Stepper Output
	
	initializeDelayTable();
	
	
}

