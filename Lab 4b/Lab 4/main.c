#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"

// define the global variables that can be used in every function ===========
volatile unsigned char ADC_result;
volatile unsigned int ADC_result_flag;
volatile unsigned char motorDir = 0x00;
volatile unsigned char motorState = 0x01;

int main()
{
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRB = 0x03;//sets D0 and D1 to output
	PORTL = motorState << 6;
	
	//Initialize LCD module
	InitLCD(LS_BLINK|LS_ULINE);

	//Clear the screen
	LCDClear();
	LCDWriteString("Program Setup");
	mTimer(500);
	LCDClear();
	LCDWriteString("loading");
	mTimer(2000);
	LCDClear();
	LCDWriteString("ADC Value:");
	LCDGotoXY(0,1);
	LCDWriteString("Motor dir:0");
	PORTB = motorState;
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= ( _BV(INT2));                     // enable INT2
	EICRA |= ( _BV(ISC21));                    // rising edge interrupt

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= _BV(ADEN);                       // enable ADC
	ADCSRA |= _BV(ADIE);                       // enable interrupt of ADC
	ADMUX |= _BV(ADLAR) |_BV(REFS0);           // set ADLAR Bit in ADMUX to 1, left adjust result, AVCC with external capacitor at AREF pin

	// sets the Global Enable for all interrupts ============================
	sei();
	
	pwm();

	// initialize the ADC, start one conversion at the beginning ============
	ADCSRA |= _BV(ADSC);

	
	while (1)
	{
		if (ADC_result_flag)
		{
			LCDGotoXY(10,0);
			LCDWriteInt(ADC_result,3);
			ADC_result_flag = 0x00;
			pwmSet(ADC_result);
			mTimer(20);
			ADCSRA |= _BV(ADSC);
		}
	}
} // end main

// sensor switch: Active HIGH starts AD conversion ==========================
ISR(INT2_vect)
{
	mTimer(20);
	motorDir ^= 0x01; //flips the motor bit direction
	motorState ^= 0x03; // XOR's the motor direction with 0x03 = 11 in binary (flips motor state between 0b00000010 and 0b00000001)
	PORTL = motorState << 6;
	PORTB = 0x00;
	mTimer(20);
	PORTB = motorState;
	LCDGotoXY(10,1);
	LCDWriteInt(motorDir,1);
	mTimer(20);
}

// the interrupt will be triggered if the ADC is done =======================
ISR(ADC_vect)
{
	ADC_result = ADCH;
	ADC_result_flag = 1;
}