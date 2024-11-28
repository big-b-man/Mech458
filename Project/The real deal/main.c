#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"
#include "stepperMotor.h"
#include "LinkedQueue.h"

// define the global variables that can be used in every function ==========
volatile uint16_t ADC_result = 1023;
volatile uint16_t ADC_result_flag = 1;
volatile uint16_t ADCLowCount = 0;
volatile unsigned char motorState = 0x02;
volatile char STATE = 2;// for warm up
volatile char sorted_items[4] = {0,0,0,0};
volatile uint16_t sorterbin = 0;//0 = black, 1 = AL, 2 = white, 3 = FE.
/*Array telling the sorter how to move.
Rows are where we are, columns are where we are going*/
const int binMovements [4][4] =	{{0,50,100,-50},
{-50,0,50,100},
{100,-50,0,50},
{50,100,-50,0}};

enum material{
	BLACK = 0,
	AL = 1,
	WHITE = 2,
	FE = 3
};

volatile char rampDown = 0;//rampDown flag

int main() {
	int stepNum;
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRA = 0xFF;//stepper output
	DDRB = 0x03;//sets B0-B1 to output
	DDRE = 0x00;//all E pins on input
	
	//Initialize LCD module
	InitLCD(LS_BLINK|LS_ULINE);

	//Clear the screen
	LCDClear();
	LCDWriteString("Program Setup");
	mTimer(500);
	PORTB = motorState;
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= (1 << INT0) | (1 << INT1) | (1 << INT2) | (1 << INT5);                                     // enable INT0-INT2 and INT5
	EICRA |= (1 << ISC21) | (1 << ISC20) | (1 << ISC11) | (1 << ISC10) | (1 << ISC01);                  // rising edge interrupt for INT1-INT2, falling edge for INT0
	EICRB |= (1 << ISC50);			     					                                            // rising edge for INT5

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= (1 << ADEN);                       // enable ADC
	ADCSRA |= (1 << ADIE);                       // enable interrupt of ADC
	ADMUX  |= (1 << REFS0);						 //AVCC with external capacitor at AREF pin
	
	//Setup Timer 3 for belt delay
	//These two registers should already be 0 but I'm doing a sanity check
	TCCR3A = 0x00;
	TCCR3B &= ~(0xDF);
	//Prescaler to 256, page 157 for details
	TCCR3B |= (1 << CS32);
	
	//time to count to max value: 256 *(2^16-1) =16,776,960 cycles ~= 2 seconds
	//((2^16-1)-(59285))*256/(8*10^6)=0.2s, 59285 = E795
	TCNT3H = 0xE7;
	TCNT3L = 0x95;
	
	// sets the Global Enable for all interrupts ============================
	sei();
	
	//stepper initialization.
	LCDClear();
	
	stepNum = homeMotor();
	PORTA = motorSteps[stepNum];
	
	//Linked list setup
	link *head;			/* The ptr to the head of the queue */
	link *tail;			/* The ptr to the tail of the queue */
	link *newLink;		/* A ptr to a link aggregate data type (struct) */
	link *rtnLink;		/* same as the above */
	
	setup(&head,&tail);//sets up linked list

	rtnLink = NULL;
	newLink = NULL;
	
	STATE = 0;
	//pwm setup to 40% duty cycle
	pwm();
	pwmSet(127);
	motorState = 0x02;
	PORTB |= motorState;

	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE:
	if((rampDown == 1) && (tail == NULL)){
		STATE = 3;
	}
	switch(STATE){
		case (0) :
		PORTL = (1 << PINL7);   //shows what stage we are in
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
		case (4) :
		goto ENQUEUE;
		break;
		default :
		goto POLLING_STAGE;
	}//switch STATE

	REFLECTIVE_STAGE:
	{
		PORTL = (1 << PINL6);//debug light to show we're in the reflective stage
		if(ADC_result_flag == 1){
			ADC_result_flag = 0;
			ADCSRA |= (1 << ADSC);//start a new ADC convert if last one is done
		} //do nothing if an ADC convert is in progress
		goto POLLING_STAGE;
	}
	BUCKET_STAGE:
	{
		PORTL = (1 << PINL5);
		dequeue(&head,&tail,&rtnLink);
		//if item is in same bin don't move motor
		if((binMovements[sorterbin][rtnLink->e.number])){
			motorState = 0x03;//stop motor
			PORTB = (motorState & 0x03);
			while (!(TIFR3 & (1 << TOV3))){
				PORTL = (1 << PINL4) | (1 <<PINL5);
			}
			PORTL = 0x00;
			moveStepper(binMovements[sorterbin][rtnLink->e.number],&stepNum);
			sorterbin = rtnLink->e.number;
		}
		free(rtnLink);
		//Reset the state variable
		STATE = 0;
		motorState = 0x02;
		PORTB = motorState & 0x03;
		//must match value at timer setup stage
		TCNT3H = 0xE7;
		TCNT3L = 0x95;
		TIFR3 |= (1 << TOV3);
		goto POLLING_STAGE;
	}
	END:
	{
		//waits 5 seconds to make sure belt is clear
		for(int i = 0; i < 5000; i++){
			mTimer(1);
			if(!(tail == NULL)){
				STATE = 0;
				goto POLLING_STAGE;
			}
		}
		LCDGotoXY(0,0);
		LCDWriteString("BL AL WI FE");
		LCDGotoXY(0,1);
		LCDWriteInt(sorted_items[0],2);
		LCDGotoXY(3,1);
		LCDWriteInt(sorted_items[1],2);
		LCDGotoXY(6,1);
		LCDWriteInt(sorted_items[2],2);
		LCDGotoXY(9,1);
		LCDWriteInt(sorted_items[3],2);
		motorState = 0x03;
		PORTB = (motorState & 0x03);
		while(1){
			cli();
		}
	}
	
	ENQUEUE:
	{
		PORTL = (1 << PINL5);
		LCDClear();
		//Highest ADC Values for white, FE and AL
		uint16_t material_types[] = {940, /*Black derlin low limit*/
			800, //white delrin/steel boundary
		400 /*steel/aluminum boundary*/};
		LCDGotoXY(12,0);
		LCDWriteInt(ADC_result,3);
		int material;
		if(ADC_result > material_types[0]){
			material = BLACK;
			} else if (ADC_result > material_types[1]) {
			material = WHITE;
			} else if (ADC_result > material_types[2]) {
			material = FE;
			} else {
			material = AL;
		}
		sorted_items[material]++;
		initLink(&newLink); //creates new link and stores input to linked lsit.
		newLink->e.number = material;
		enqueue(&head, &tail, &newLink);
		LCDGotoXY(0,0);
		LCDWriteString("BL AL WI FE");
		LCDGotoXY(0,1);
		LCDWriteInt(sorted_items[0],2);
		LCDGotoXY(3,1);
		LCDWriteInt(sorted_items[1],2);
		LCDGotoXY(6,1);
		LCDWriteInt(sorted_items[2],2);
		LCDGotoXY(9,1);
		LCDWriteInt(sorted_items[3],2);
		ADC_result = 1023;//reset ADC
		STATE = 0;
		goto POLLING_STAGE;
	}
} // end main

// sensor switch: Active HIGH starts AD conversion ==========================
ISR(INT0_vect)
{
	STATE = 2;
}

ISR(INT1_vect){
	mTimer(20);
	while(PIND & (1 << PIND1)){};//wait for button to be released
	mTimer(20);
	rampDown = 1;
	EIFR |= (1 << INTF1);//for some reason the interrupt automatically re triggers unless I explicitly clear the flag at the end.
}

ISR(INT2_vect) //Controls program pause button. Holds the program in the interrupt until pause it pressed again.
{
	LCDClear();
	LCDWriteString("Program Paused");
	motorState = 0x03;//stop motor
	PORTB = (motorState & 0x03);
	while(PIND & (1 << PIND2)){};//wait for button to be released
	mTimer(20);
	while (!(PIND & (1 << PIND2))){};//wait for button to be pressed again
	LCDClear();
	mTimer(20);
	while(PIND & (1 << PIND2)){};//wait for button to be released
	mTimer(20);
	if(STATE == 2) {//if in bucket stage
		//do nothing
		} else { //restart the motor otherwise
		motorState = 0x02;//start motor
		PORTB = (motorState & 0x03);
	}
	EIFR |= (1 << INTF2);//for some reason the interrupt automatically re triggers unless I explicitly clear the flag at the end.
}

ISR(INT5_vect)// Interrupt 5, Triggered the optical sensor next to the reflectivity sensor
{
	mTimer(3);//de-bouncing
	if (PINE & (1 << PINE5)) {
		//If pin is high, enter reflective stage
		STATE = 1;
		} else {
		//if pin is low, enter ENQUEUE Stage
		STATE = 4;
		// INT5 pin is low
	}
}

// the interrupt will be triggered if the ADC is done =======================
ISR(ADC_vect)
{
	uint16_t ADC_result_last = ADC_result;
	ADC_result = ADCL;
	ADC_result |= (ADCH & 0x03) << 8;
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