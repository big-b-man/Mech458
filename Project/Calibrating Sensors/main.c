#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"

// define the global variables that can be used in every function ===========
volatile uint16_t ADC_result = 1023;
volatile unsigned int ADC_result_flag;
volatile unsigned char motorState = 0x02;// motor set forward

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
	LCDWriteString("ADC Value:");
	PORTB |= motorState;
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= ( 1 << INT2) | (1 << INT1)  | (1 << INT5);                   // enable INT 5, INT2 and INT1
	EICRA |= (1 << ISC21) | (1 << ISC20) | (1 << ISC11) | (1 << ISC10);	  // rising edge interrupt
	EICRB |= (1 << ISC51);                                                // falling edge interupt
	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= (1 << ADEN);                       // enable ADC
	ADCSRA |= (1 << ADIE);                       // enable interrupt of ADC
	ADMUX |= (1 << REFS0);           //AVCC with external capacitor at AREF pin

	// sets the Global Enable for all interrupts ============================
	sei();
	
	pwm();
	pwmSet(77);

	// initialize the ADC, start one conversion at the beginning ============
	ADCSRA |= (1<<(ADSC));
	
	while (1)
	{
		PORTL = (PINE & (1 << PINE5)) << 2;
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

ISR(INT1_vect)
{
	mTimer(20);
	motorState ^= 0x02;//stop and start motor
	PORTB = (motorState & 0x03);
	while(PIND & (1 << PIND1)){};//wait for button to be released
	mTimer(20);
	EIFR |= (1 << INTF1);//for some reason the interrupt automatically re triggers unless I explicitly clear the flag at the end.
}

ISR(INT2_vect)
{
	ADC_result = 999;
}

ISR(INT5_vect)
{
	mTimer(1000);
	ADC_result = 999;
	EIFR |= (1 << INTF5);
}

// the interrupt will be triggered if the ADC is done =======================
ISR(ADC_vect)
{
	uint16_t ADC_result_last = ADC_result;
	ADC_result = ADCL;
	ADC_result |= ADCH << 8;
	if((ADC_result < ADC_result_last)/* && (ADC_result > (ADC_result_last-20))*/){
		} else {
		ADC_result = ADC_result_last;
	}
	ADC_result_flag = 1;
}