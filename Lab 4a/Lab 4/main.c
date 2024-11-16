#include <stdlib.h>
#include <avr/io.h>
#include "timer.h"
#include "pwm.h"

volatile char stepNum;

/*array containing stepper signal control sequence*/
const char motorSteps[] = { 0b00110000,
							0b00000110,
							0b00101000,
							0b00000101 };
void moveMotor(int dir, int moveNum);//0 is CW, 1 is CCW

int main(){
	timer8MHz();
	pwm();
	
	DDRA = 0xFF; //Sets PortA to output
	DDRC = 0xFF; //Sets PortC to output
	DDRL = 0xFF; //Sets PortL to output
	
	/*Stepper Setup*/
	moveMotor(0,90);
	
	/*Loop through stepper motor angles*/
	while(1){
		mTimer(1000);
		moveMotor(0,17); //30 deg cw
		mTimer(1000);
		moveMotor(0,33); //60 deg cw
		mTimer(1000);
		moveMotor(0,100); //180 deg cw
		mTimer(1000);
		moveMotor(1,17);
		mTimer(1000);
		moveMotor(1,33);
		mTimer(1000);
		moveMotor(1,100);
		mTimer(1000);
	}/*Loop through stepper motor angles*/
}

void moveMotor(int dir, int moveNum){
	//if(dir == 0){
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
	//}
}