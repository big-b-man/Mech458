#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"

// define the global variables that can be used in every function ===========
volatile uint16_t ADC_result = 455; //Value our reflectivity sensor displays normally
volatile unsigned int ADC_result_flag; //Sets when ADC conversion is finished
volatile unsigned char motorDir = 0x00; //Direction of Stepper Motor
volatile unsigned char motorState = 0x01; 
volatile char STATE; //Program Polling State

int main()
{
	STATE = 0;
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRB = 0x03;//sets D0 and D1 to output
	DDRE = 0b00;//all E pins on input
	PORTL = motorState << 6;
	
	//Initialize LCD module
	InitLCD(LS_BLINK|LS_ULINE);

	//Clear the screen
	LCDClear();
	LCDWriteString("Program Setup");
	mTimer(500);
	LCDClear();
	LCDWriteString("loading");
	mTimer(500);
	LCDClear();
	LCDWriteString("ADC Value:");
	LCDGotoXY(0,1);
	LCDWriteString("Optical:0");
	PORTB = motorState;
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= ( _BV(INT2));                     // enable INT2
	EICRA |= ( _BV(ISC21));                    // rising edge interrupt

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= _BV(ADEN);                       // enable ADC
	ADCSRA |= _BV(ADIE);                       // enable interrupt of ADC
	ADMUX |= _BV(REFS0);           //AVCC with external capacitor at AREF pin

	// sets the Global Enable for all interrupts ============================
	sei();
	
	pwm();
	pwmSet(102); //40% Duty cycle: 255*0.4 = 102

	// initialize the ADC, start one conversion at the beginning ============
	ADCSRA |= _BV(ADSC);
	
	goto POLLING_STAGE;
	
	POLLING_STAGE:
	PORTC |= 0xF0;	// Indicates this state is active
	switch(STATE){
		case (0) :
		goto POLLING_STAGE;
		break;	//not needed but syntax is correct
		case (1) :
		goto MAGNETIC_STAGE;
		break;
		case (2) :
		goto REFLECTIVE_STAGE;
		break;
		case (3) :
		goto BUCKET_STAGE;
		break;
		case (5) :
		goto END;
		default :
		goto POLLING_STAGE;
	}//switch STATE
	
	MAGNETIC_STAGE:
	// Do whatever is necessary HERE
	PORTC = 0x01; // Just output pretty lights know you made it here
	//Reset the state variable
	STATE = 0;
	goto POLLING_STAGE;

	REFLECTIVE_STAGE:

	PORTL = (PINE & 0b00100000) << 2;
	if (ADC_result_flag){
		LCDGotoXY(10,0);
		LCDWriteInt(ADC_result,5);
		ADC_result_flag = 0x00;
		mTimer(20);
	}
	if ()
	
	PORTC = 0x04; // Just output pretty lights know you made it here
	//Reset the state variable
	STATE = 0;
	goto POLLING_STAGE;
		
	BUCKET_STAGE:
	// Do whatever is necessary HERE
	PORTC = 0x08;
	//Reset the state variable
	STATE = 0;
	goto POLLING_STAGE;
		
	END:
	// The closing STATE ... how would you get here?
	PORTC = 0xF0;	// Indicates this state is active
	// Stop everything here...'MAKE SAFE'
	return(0);

} // end main

// sensor switch: Active HIGH starts AD conversion ==========================
ISR(INT2_vect)
{
	mTimer(20);
	motorDir ^= 0x01; //flips the motor bit direction
	motorState ^= 0x01; // XOR's the motor direction with 0x01 = 01 in binary (flips motor state between 0b00000000 and 0b00000001)
	PORTL = motorState << 6;
	PORTB = 0x00;
	mTimer(20);
	PORTB = motorState;
	mTimer(20);
}

ISR(INT4_vect)
{
	//interupt code
}

// the interrupt will be triggered if the ADC is done =======================
ISR(ADC_vect)
{
	uint16_t ADC_result_last = ADC_result;
	ADC_result = ADCL;
	ADC_result |= (ADCH && 0x03) << 8; 
	if((ADC_result < ADC_result_last)/* && (ADC_result > (ADC_result_last-20))*/){
	} else {
		ADC_result = ADC_result_last;
	}
	ADC_result_flag = 1;
}