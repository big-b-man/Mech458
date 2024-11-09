#include<avr/io.h> /* Needed for performing I/O */
#include<avr/interrupt.h> /* Needed for interrupt functionality */

void mTimer(int count); //mTimer Function Declaration

int main(int argc, char *argv[]){

    CLKPR = 0x80; /* This will be discussed later. */
    CLKPR = 0x01; /* Required to set CPU Clock to 8MHz */
    
    /* Timer instructions */
    /* Sets timer 1 to run at 1MHz, note: CPU clock is set to 8MHz.
       Disable all functions and use as pure timer */
    
    TCCR1B = _BV(CS11); /* _BV sets the bit to logic 1 */
    /* Note the register is TCCR1B1 */
    /* TCCR1 is the Timer/counter control resister 1 */
    /* B is the 'B' register and 1 is bit 1 */
    /* CS means clock select, has the pre-scaler set to 8 */

    DDRL = 0xFF; /* Sets all pins on PORTL to output */
	DDRC = 0xFF; /* Sets all pins on PORTC to output */
	PORTC = 0b11000000; //
	
    while(1){
		for(int i=0;i<6;i++){//Cycles lights right by bitshifting the register
			mTimer(1000);
			PORTC = (PORTC >> 1);
		}

		for(int i=0;i<6;i++){//Cycles lights left by bitshifting the register
			mTimer(1000);
			PORTC = (PORTC << 1);
		}
    } /* close while loop */
} /* close main */

/* This is the driver for the timer. */
void mTimer(int count){
    /* The system clock is 8 MHz. You can actually see the crystal oscillator (16MHz) which is a silver looking can on the board.
    You can use a pre-scaler on system clock to lower the speed. The Timer runs on the CPU Clock which is a function of
    the system clock. You can also use a pre-scaler on the Timer, by 1, 8, 64, 256, or 1024 to lower the speed.
    The system clock has been pre-scaled by 2. This means it's running at half speed, 8MHz. See Technical manual for
    ATmega2560 (i.e. full manual) and look up "16-bit Timer/Counter1." */

    int i; /* keeps track of loop number */

    i = 0; /* initializes loop counter */

    /* Set the Waveform Generation mode bit description to Clear Timer on Compare Match mode (CTC) only */
    TCCR1B |= _BV(WGM12); /* set WGM bits to 0100, see page 145 */
    /* Note WGM is spread over two registers. */

    OCR1A = 0x03E8; /* Set Output Compare Register for 1000 cycles = 1ms */
    TCNT1 = 0x0000; /* Sets initial value of Timer Counter to 0x0000 */
    TIMSK1 = TIMSK1 | 0b00000010; /* Enable the output compare interrupt enable */

    TIFR1 |= _BV(OCF1A); /* clear the timer interrupt flag and begin new timing */
    /* If the following statement is confusing, please ask for clarification! */

    /* Poll the timer to determine when the timer has reached 0x03E8 */
    while(i<count){
        if((TIFR1 & 0x02) == 0x02){
            TIFR1 |= _BV(OCF1A); /* clear interrupt flag by writing a ONE to the bit */
            i++; /* increment loop number */
        } /* end if */
    } /* end while */

    return;
} /* mTimer */
