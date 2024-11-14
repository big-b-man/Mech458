#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"

// define the global variables that can be used in every function ===========
volatile uint16_t ADC_result = 455;
volatile unsigned int ADC_result_flag;
volatile unsigned char motorState = 0x00;

int main()
{
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
	mTimer(2000);
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
	pwmSet(102);

	// initialize the ADC, start one conversion at the beginning ============
	ADCSRA |= _BV(ADSC);
	
	while (1)
	{
		PORTL = (PINE & 0b00100000) << 2;
		if (ADC_result_flag)
		{
			LCDGotoXY(10,0);
			LCDWriteInt(ADC_result,5);
			ADC_result_flag = 0x00;
			mTimer(20);
			ADCSRA |= _BV(ADSC);
		}
	}
} // end main

// sensor switch: Active HIGH starts AD conversion ==========================
ISR(INT2_vect)
{
	ADC_result = 999;
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