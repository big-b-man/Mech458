#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "Timer.h"
#include "pwm.h"
#include "stepperMotor.h"
#include "LinkedQueue.h"

// define the global variables that can be used in every function ==========
volatile uint16_t ADC_result = 1023; //stores the result from ADC reads. Initialized to highest value
volatile uint8_t ADC_result_flag = 1; //Set when ADC conversion is finished
volatile unsigned char motorState = 0x02; //Sets the IA and IB pins on the belt motor to start and stop it
volatile uint8_t STATE = 0; //State variable for controlling stage of the program
volatile uint8_t sorted_items[4] = {0,0,0,0}; //Array for tracking sorted items
volatile uint16_t sorterbin = 0; //Bin that the sorter tray is currently stopped on, numbers allign with
								 //materialNames enum defined below

/*Array telling the sorter how to move.
Rows are where we are, columns are where we are going*/
const int8_t binMovements [4][4] =	{{0,50,100,-50},
{-50,0,50,100},
{100,-50,0,50},
{50,100,-50,0}};
volatile uint16_t unsorted_items = 0;//list of items that have yet to be sorted

//Enum so I don't have to remember what material type is associated with what value for the binMovemennts[][] and sorted_items[] arrays
enum materialNames{
	BLACK = 0,
	AL = 1,
	WHITE = 2,
	FE = 3
};

volatile char rampDown = 0;//rampDown flag

int main() {
	int stepNum;//stores the current position of the stepper motor
	timer8MHz();//setup the chip clock to 8 MHz
	DDRL = 0xFF;//sets debug lights to output
	DDRA = 0xFF;//stepper output
	DDRB = 0x03;//sets B0-B1 to output for belt control
	
	//Initialize LCD module
	InitLCD(LS_BLINK|LS_ULINE);
	LCDClear();
	LCDWriteString("Program Setup");
	mTimer(500);
	
	cli(); // disable all of the interrupt ==================================

	// config the external interrupt ========================================
	EIMSK |= (1 << INT0) | (1 << INT1) | (1 << INT2) | (1 << INT5);                                     // enable INT0-INT2 and INT5
	EICRA |= (1 << ISC21) | (1 << ISC20) | (1 << ISC11) | (1 << ISC10) | (1 << ISC01);                  // rising edge interrupt for INT1-INT2, falling edge for INT0
	EICRB |= (1 << ISC50);			     					                                            // any edge interrupt on INT5

	// config ADC ===========================================================
	// by default, the ADC input (analog input) is set to ADC0 / PORTF0
	ADCSRA |= (1 << ADEN);                       //enable ADC
	ADCSRA |= (1 << ADIE);                       //enable interrupt of ADC
	ADMUX  |= (1 << REFS0);						 //AVCC with external capacitor at AREF pin
	
	//Setup Timer 3 for belt delay
	//These two registers should already be 0 but I'm doing a sanity check
	TCCR3A = 0x00;
	TCCR3B &= ~(0xDF);//zeros everything except bit 5 which is read only.
	//Prescaler to 256, page 157 of ATMega2560 manual for details
	TCCR3B |= (1 << CS32);
	
	// sets the Global Enable for all interrupts ============================
	sei();
	
	//stepper initialization.
	stepNum = homeMotor();
	
	//Linked list setup
	link *head;			/* The ptr to the head of the queue */
	link *tail;			/* The ptr to the tail of the queue */
	link *newLink;		/* A ptr to a link aggregate data type (struct) */
	link *rtnLink;		/* same as the above */
	
	setup(&head,&tail);//sets up linked list

	rtnLink = NULL;
	newLink = NULL;
	
	//pwm setup to 50% duty cycle
	pwm();// initializes the pwm
	pwmSet(127);//sets the pwm percentage
	
	//start belt
	PORTB |= motorState;

	//enter polling loop
	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE:
	switch(STATE){
		case (0) :
		PORTL = (1 << PINL7);//debug light to show we're in polling stage
		goto POLLING_STAGE;
		break;
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
		//Checks if ADC conversion has finished
		if(ADC_result_flag == 1){
			//reset flag and start new conversion if conversion is finished
			ADC_result_flag = 0;
			ADCSRA |= (1 << ADSC);
		} //do nothing if an ADC convert is in progress
		goto POLLING_STAGE;
	}
	BUCKET_STAGE:
	{
		PORTL = (1 << PINL5);//debug light to show we're in the bucket stage
		dequeue(&head,&tail,&rtnLink);//dequeue first item from linked list
		//check if item is the same as last sorted item, skip bin movement if item is the same
		if((binMovements[sorterbin][rtnLink->e.number])){
			motorState = 0x03;//stop motor
			PORTB = (motorState & 0x03);
			while (!(TIFR3 & (1 << TOV3))){}//delay to make sure that if an item was directly in front of this item,
											//it has enough time to fall into the bin
			moveStepper(binMovements[sorterbin][rtnLink->e.number],&stepNum);//move stepper according to lookup table
			sorterbin = rtnLink->e.number;//Set current bin number for next sort
		}
		sorted_items[rtnLink->e.number]++;//increment sorted item list
		unsorted_items--;//decrement unsorted item list
		
		//Write current sorted items to display
		LCDClear();
		LCDGotoXY(0,0);
		LCDWriteString("BL AL WI FE UN");
		LCDGotoXY(0,1);
		LCDWriteInt(sorted_items[0],2);
		LCDGotoXY(3,1);
		LCDWriteInt(sorted_items[1],2);
		LCDGotoXY(6,1);
		LCDWriteInt(sorted_items[2],2);
		LCDGotoXY(9,1);
		LCDWriteInt(sorted_items[3],2);
		LCDGotoXY(12,1);
		LCDWriteInt(unsorted_items,2);
		free(rtnLink);
		//Reset the state variable
		if(rampDown == 1){
			STATE = 3;//goto end stage if ramp down button has been pressed
			} else {
			STATE = 0;//goto polling stage otherwise
		};
		motorState = 0x02;//restart belt
		PORTB = motorState & 0x03;

		//reset timer
		//time to count to max value: 256 *(2^16-1) =16,776,960 cycles ~= 2 seconds
		//((2^16-1)-(56000))*256/(8*10^6) =~ 0.23s, 56000 = D098
		TCNT3H = 0xD0;
		TCNT3L = 0x98;
		TIFR3 |= (1 << TOV3);
		goto POLLING_STAGE;
	}
	END:
	{
		//triggered when ramp down button is pressed
		PORTL = (1 << PINL7) | (1 << PINL5);//debug lights
		//delay to make sure belt is clear. for loop goes through five iterations.
		//The delay timer in each iteration counts up for roughly one second before toping out.
		//This creates a five second delay.
		for(int i = 0; i < 5; i++){
			TCNT3H = 0x80;
			TCNT3L = 0x00;
			TIFR3 |= (1 << TOV3);
			while(!(TIFR3 & (1 << TOV3))){
				if(STATE != 3){//if an interrupt switches the state while waiting, 
							   //reset timer 3 and goto polling stage
					TCNT3H = 0xE7;
					TCNT3L = 0x95;
					TIFR3 |= (1 << TOV3);
					goto POLLING_STAGE;
				}
			}
		}
		motorState = 0x03;//stop the motor
		PORTB = (motorState & 0x03);
		cli();//infinite idle loop
		while(1){	
		}
	}
	
	ENQUEUE:
	{
		//Enqueue item on falling edge of OR
		unsorted_items++;//increment unsorted item list
		PORTL = (1 << PINL5);//debug light
		LCDClear();
		
		//Following array holds Highest ADC Values for white, FE and AL for sorting
		//Values are changed during the calibration procedure
		uint16_t material_types[] = {920, /*Black derlin low limit*/
			800, //white delrin/steel boundary
		150 /*steel/aluminum boundary*/};
		LCDGotoXY(12,0);
		LCDWriteInt(ADC_result,3);//write ADC value to display for debug and calibration purposes
		uint8_t material;// variable used for storing material type according to ENUM defined at program start
		if(ADC_result > material_types[0]){
			material = BLACK;
			} else if (ADC_result > material_types[1]) {
			material = WHITE;
			} else if (ADC_result > material_types[2]) {
			material = FE;
			} else {
			material = AL;
		}
		initLink(&newLink); //creates new link and stores input to linked lsit.
		newLink->e.number = material;
		enqueue(&head, &tail, &newLink);
		LCDGotoXY(0,0);//displays current sorted items
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

// EX interrupt to enter bucket stage
ISR(INT0_vect)
{
	STATE = 2;//set state to bucket stage
}

//Interrupt for ramp down button
ISR(INT1_vect){
	mTimer(20);//button de-bouncing
	rampDown = 1;//set ramp down flag
	STATE = 3;//set STATE to END
	EIFR |= (1 << INTF1);//interrupt flag will be re triggered while waiting for the button de-bouncing.
						 //Need to clear it or else it will infinitely loop through this interrupt
}

//Controls program pause button. Holds the program in the interrupt until pause is pressed again.
ISR(INT2_vect)
{
	LCDClear();
	LCDGotoXY(0,0);
	LCDWriteString("BL AL WI FE UN");
	LCDGotoXY(0,1);
	LCDWriteInt(sorted_items[0],2);
	LCDGotoXY(3,1);
	LCDWriteInt(sorted_items[1],2);
	LCDGotoXY(6,1);
	LCDWriteInt(sorted_items[2],2);
	LCDGotoXY(9,1);
	LCDWriteInt(sorted_items[3],2);
	LCDGotoXY(12,1);
	LCDWriteInt(unsorted_items,2);
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
		motorState = 0x02;
		PORTB = (motorState & 0x03);
	}
	EIFR |= (1 << INTF2);//interrupt flag will be re triggered while waiting for the button de-bouncing.
						 //need to clear it or else it will infinitely loop through this interrupt
}

// OR Sensor Interupt
ISR(INT5_vect)
{
	mTimer(3);//de-bouncing
	if (PINE & (1 << PINE5)) {
		//If pin is high, enter reflective stage
		STATE = 1;
		} else {
		//if pin is low, enter ENQUEUE Stage
		STATE = 4;
	}
}

// The interrupt will be triggered if the ADC is done =======================
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
	PORTL = 0xF0;//light up everything to let us know something is bad
}