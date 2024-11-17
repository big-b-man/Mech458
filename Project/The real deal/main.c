#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"
#include "initializeMotor.h"
#include "LinkedQueue.h"

// define the global variables that can be used in every function ===========
volatile uint16_t ADC_result = 999;
volatile unsigned int ADC_result_flag = 1;
volatile unsigned char motorState = 0x02;
volatile char STATE = 0;
volatile char sorted_items[4] = {0,0,0,0};
volatile char stepNum;
volatile char sorterbin = 0;//0 = black, 1 = AL, 2 = white, 3 = FE.

const char motorSteps[] = { 0b00110000,
	0b00000110,
	0b00101000,
0b00000101 };// steps for stepper motor

int moveStepper(int dir, int moveNum);

int main() {
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRA = 0xFF;//stepper output
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
	EIMSK |= (1 << INT0) | (1 << INT1) | (1 << INT2) | (1 << INT5);                                     // enable INT0-INT2 and INT5
	EICRA |= (1 << ISC21) | (1 << ISC20) | (1 << ISC11) | (1 << ISC10) | (1 << ISC01);                  // rising edge interrupt for INT1-INT2, falling edge for INT0
	EICRB |= (1 << ISC50);					                                                            // any edge interrupt for INT5

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= (1 << ADEN);                       // enable ADC
	ADCSRA |= (1 << ADIE);                       // enable interrupt of ADC
	ADMUX  |= (1 << REFS0);						 //AVCC with external capacitor at AREF pin

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
	
	//pwm setup to 40% duty cycle
	pwm();
	pwmSet(102);
	PORTB |= motorState;

	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE:
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
		// Do whatever is necessary HERE
		dequeue(&head,&tail,&rtnLink);
		LCDClear();
		LCDWriteInt(rtnLink->e.number,1);
		switch(rtnLink->e.number){
			case(0)://Black Delrin
			{
				const uint16_t moveDir[] = {0,1,0,0};
				const uint16_t moveNum[] = {0,50,100,50};
				moveStepper(moveDir[sorterbin],moveNum[sorterbin]);
				sorterbin = 0;
				motorState = 0x02;
				PORTB |= motorState;
				break;
			}
			case(1)://White Delrin
			{
				const uint16_t moveDir[] = {0,0,0,1};
				const uint16_t moveNum[] = {100,50,0,50};
				moveStepper(moveDir[sorterbin],moveNum[sorterbin]);
				sorterbin = 2;
				motorState = 0x02;
				PORTB |= motorState;
				break;
			}
			case(2)://Steel
			{
				const uint16_t moveDir[] = {1,0,0,0};
				const uint16_t moveNum[] = {50,100,0,50};
				moveStepper(moveDir[sorterbin],moveNum[sorterbin]);
				sorterbin = 3;
				motorState = 0x02;
				PORTB |= motorState;
				break;
			}
			break;
			case(3)://Aluminum
			
			break;
			free(rtnLink);
		}
		//Reset the state variable
		STATE = 0;
		goto POLLING_STAGE;
	}
	END:
	{
		// The closing STATE ... how would you get here?
		PORTC = 0xF0;	// Indicates this state is active
		// Stop everything here...'MAKE SAFE'
		return(0);
	}
	
	ENQUEUE:
	{
		PORTL = (1 << PINL5);
		LCDClear();
		uint16_t material_types[] = {440, /*white derlin*/
			380, /*steel*/
		250 /*aluminum*/};
		int material;
		if(ADC_result > material_types[0]){
			material = 0;//black delrin
			} else if (ADC_result > material_types[1]) {
			material = 1;//white delrin
			} else if (ADC_result > material_types[2]) {
			material = 2; //Steel
			} else {
			material = 3;//aluminum
		}
		sorted_items[material]++;
		initLink(&newLink); //creates new link and stores input to linked lsit.
		newLink->e.number = material;
		enqueue(&head, &tail, &newLink);
		
		LCDGotoXY(0,0);
		LCDWriteString("BL WI FE AL");
		LCDGotoXY(12,0);
		LCDWriteInt(ADC_result,3);
		LCDGotoXY(0,1);
		LCDWriteInt(sorted_items[0],2);
		LCDGotoXY(3,1);
		LCDWriteInt(sorted_items[1],2);
		LCDGotoXY(6,1);
		LCDWriteInt(sorted_items[2],2);
		LCDGotoXY(9,1);
		LCDWriteInt(sorted_items[3],2);
		
		ADC_result = 999;//reset ADC
		STATE = 0;
		goto POLLING_STAGE;
	}
} // end main

// sensor switch: Active HIGH starts AD conversion ==========================
ISR(INT0_vect)
{
	motorState = 0x00;//stop motor
	PORTB = motorState & 0x03;
	STATE = 2;
}

ISR(INT2_vect) //Controls program pause button. Holds the program in the interupt until pause it pressed again.
{
	LCDClear();
	LCDWriteString("Program Paused");
	mTimer(20);
	motorState = 0x00;//stop motor
	PORTB = (motorState & 0x03);
	while(PIND & (1 << PIND2)){};//wait for button to be released
	mTimer(20);
	while (!(PIND & (1 << PIND2))){};//wait for button to be pressed again
	LCDClear();
	mTimer(20);
	while(PIND & (1 << PIND2)){};//wait for button to be released
	mTimer(20);
	motorState = 0x02;//start motor
	PORTB = (motorState & 0x03);
	EIFR |= (1 << INTF2);//for some reason the interrupt automatically re triggers unless I explicitly clear the flag at the end.
}

ISR(INT5_vect)// Interrupt 5, Triggered the optical sensor next to the reflectivity sensor
{
	mTimer(20);//de-bouncing
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

int moveStepper(int dir, int moveNum){
	const int forSteps[] = {0,1,2,3};
	const int backSteps[] = {2,3,0,1};
	int *ptr;
	if(dir == 0){
		ptr = forSteps;
		} else {
		ptr = backSteps;
	}
	for(int i=0; i < moveNum; i++){
		switch(stepNum){
			case(3):
			PORTA = motorSteps[*ptr];
			PORTC = 0x01;
			stepNum = *ptr;
			break;
			case(0):
			PORTA = motorSteps[*(ptr+1)];
			PORTC = 0x02;
			stepNum = *(ptr+1);
			break;
			case(1):
			PORTA = motorSteps[*(ptr+2)];
			PORTC = 0x04;
			stepNum = *(ptr+2);
			break;
			case(2):
			PORTA = motorSteps[*(ptr+3)];
			PORTC = 0x08;
			stepNum = *(ptr+3);
			break;
			default:
			break;
		}
		mTimer(20);
	}
	return(stepNum);
}