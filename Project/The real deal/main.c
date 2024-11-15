#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"

// define the global variables that can be used in every function ===========
volatile uint16_t ADC_result = 455;
volatile unsigned int ADC_result_flag = 1;
volatile unsigned char motorState = 0x00;
volatile char STATE = 0;

int main()
{
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRB = 0x03;//sets D0 and D1 to output
	DDRE = 0x00;//all E pins on input
	PORTL = motorState << 6;
	
	//Initialize LCD module
	InitLCD(LS_BLINK|LS_ULINE);

	//Clear the screen
	LCDClear();
	LCDWriteString("Program Setup");
	mTimer(500);
	PORTB = motorState;
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= (1 << INT2) | (1 << INT5);        // enable INT2 and INT5
	EICRA |= (1 << ISC21) | (1 << ISC20);					   // rising edge interrupt for INT2
	EICRB |= (1 << ISC50);					   // any edge interrupt for INT5

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= (1 << ADEN);                       // enable ADC
	ADCSRA |= (1 << ADIE);                       // enable interrupt of ADC
	ADMUX  |= (1 << REFS0);						 //AVCC with external capacitor at AREF pin

	// sets the Global Enable for all interrupts ============================
	sei();
	
	
	//pwm setup to 40% duty cycle
	pwm();
	pwmSet(102);

	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE:
	PORTL = (PINL5 < 1);	// Indicates this state is active
	switch(STATE){
		case (0) :
		goto POLLING_STAGE;
		break;	//not needed but syntax is correct
		case (1) :
		goto REFLECTIVE_STAGE;
		break;
		case (2) :
		goto BUCKET_STAGE;
		break;
		case (3) :
		goto END;
		default :
		goto POLLING_STAGE;
	}//switch STATE

	REFLECTIVE_STAGE:
	PORTL = (PINL6 << 1);//debug light to show we're in the reflective stage
	if(ADC_result_flag == 1){
	ADCSRA |= (1 << ADSC);//start a new ADC convert if last one is done
	ADC_result_flag = 0;
	} //do nothing if an ADC convert is in progress
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
ISR(INT2_vect) //Controls program pause button. Holds the program in the interupt until pause it pressed again.
{
	LCDClear();
	LCDWriteString("Program Paused");
	mTimer(500);
	while (!(PIND & (1 << PIND2))){
		};
	LCDClear();
	mTimer(1000);
	EIFR |= (1 << INTF2);//for some reason the interrupt automatically re triggers unless I explicitly clear the flag at the end.
}

ISR(INT5_vect)// Interrupt 5, Triggered by6 the optical sensor next to the reflectivity sensor
{
    mTimer(20);//de-bouncing
	if (PINE & (1 << PINE5)) {
	    //If pin is high, enter polling state
		STATE = 1;
	    } else {
	    // INT5 pin is low
	    
		//enque code
    }
}

// the interrupt will be triggered if the ADC is done =======================
ISR(ADC_vect)
{
	uint16_t ADC_result_last = ADC_result;
	ADC_result = ADCL;
	ADC_result |= (ADCH && 0x03) << 8;
	if((ADC_result < ADC_result_last)){//gets us the lowest value read by the reflectivity sensor
		} else {
		ADC_result = ADC_result_last;
	}
	ADC_result_flag = 1;
}

ISR(BADISR_vect)
{
	PORTL = 0xF0;//light up everything to let us know it's screwed
}