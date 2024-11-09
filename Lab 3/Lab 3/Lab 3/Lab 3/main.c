/* 	
	Course		: UVic Mechatronics 458
	Milestone	: 3
	Title		: Data structures for MCUs and the Linked Queue Library

	Name 1:Bennett Steers					Student ID: V01025919
	Name 2:Cody Short    					Student ID: V01025830

*/
#include <stdlib.h>
#include <avr/io.h>
#include "LinkedQueue.h"
#include "Timer.h"

int debug(char input); //drives the debug LED's on PORTC

int main(int argc, char *argv[]){
	char readInput;
	
	timer8MHz();//sets the clock speed to 8MHz
	
	DDRA = 0x00; //Sets Port A register pins to input for button states
	DDRC = 0xFF; //Sets Port C register pins to output so we can control LEDs
	DDRL = 0xFF; //Sets Port L register pins to output so we can control LEDs

	link *head;			/* The ptr to the head of the queue */
	link *tail;			/* The ptr to the tail of the queue */
	link *newLink;		/* A ptr to a link aggregate data type (struct) */
	link *rtnLink;		/* same as the above */

	rtnLink = NULL;
	newLink = NULL;

	//main program loop
	while(1){
		setup(&head,&tail);//sets up linked list
		while(size(&head,&tail) < 0x04){ //executes while loop if there are less than 4 items in the linked list, otherwise skips to element dequeing.
			if((PINA & 0x04) == 0x00){//checks if push button is being pushed
				PORTL = 0x00;
				readInput = PINA & 0x03; //reads input. Since we know PA2 is low we can mask to only read the least 2 bits
				debug(readInput); //debug pin set. Refer to debug subroutine for codes
				initLink(&newLink); //creates new link and stores input to linked lsit.
				newLink->e.number = readInput;
				enqueue(&head, &tail, &newLink);
				mTimer(30);//waits 30ms for contact bounce
				while ((PINA & 0x04) == 0x00){//waits for button to be released
				}
				mTimer(30);//waits 30 ms for contact bounce
			} else {//
				PORTL = 0x80; //debug pin set. Refer to debug subroutine for codes
			}
		}
		PORTL = 0b00010000;//debug code. 0b00010000 displays last yellow LED, which is always iluminated when displaying data to red LEDs
		dequeue(&head,&rtnLink);//dequeues first item since it isn't used
		dequeue(&head,&rtnLink);//dequeues second item
		PORTC = rtnLink->e.number;//sets lowest 2 bits of PORTC to value of dequed item
		free(rtnLink);
		mTimer(1000);
		dequeue(&head,&rtnLink);//deque next item
		PORTC = PORTC + (rtnLink->e.number<<2);//display new item to LEDS
		mTimer(1000);
		dequeue(&head,&rtnLink);//deque next item
		PORTC = PORTC + (rtnLink->e.number<<4);//display new item to LEDS
		clearQueue(&head,&tail);//destroys queue since it's reinitialized at the start of the program loop
		
		while((PINA & 0x04) != 0x00){//checks for push button input to restart program
		}
		mTimer(30);
		
		PORTC = 0x00;//resets debug code
		while((PINA & 0x04) == 0x00){
		}
		mTimer(30);
	}
	return 0;
}

//reads input and sets Debug LEDS. IDK how Atmel compiles function calls so I made this inline so that it's inserted into the code for more speed
inline int debug(char input){
	switch (input){
		case (0x00):
			PORTL = 0b00000000;//data 01 writen, display 00 on green LEDS
			break;
		case (0x01):
			PORTL = 0b00100000;//data 01 writen, display 01 on green LEDS
			break;
		case (0x02):
			PORTL = 0b01000000;//data 10 writen, display 10 on green LEDS
			break;
		case (0x03):
			PORTL = 0b01100000;//data 11 writen, display 11 on green LEDS
			break;
		default:
			PORTL = 0b00000000;//default case
			break;
	}/*switch*/
	return(input);
}/*debug*/