/*
	main file see project.c
*/

#include <avr/io.h>
#include <avr/interrupt.h>

void init_timer1(unsigned short m)
{
	// Set to CTC mode
	TCCR1B |= (1 << WGM12);

	// Enable Timer Interrupt
	TIMSK1 |= (1 << OCIE1A);

	// Load the MAX count
	// Assuming prescalar=256
	// counting to 15625 =
	// 0.25s w/ 16 MHz clock
	OCR1A = m;
	
	// Set prescalar = 1
	// and start counter
	TCCR1B |= (1 << CS10);
	
	
}

void rangefinder_init(void){
	//inital for trig
	DDRB |= (1 << 5);
	PORTB &= ~(1<< 5);

	//inital for echo
	PORTD |= (1<<3);

	//inital for acquire button
	PORTB |= (1<< 3);

	//inital for adjust button
	PORTB |= (1<< 4);

	//ISR for echo
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1<< PCINT19);
}

//1 will make trig pulse
//0 will turn off the trig
void makeTrig(char value){
	if(value == 0){
		PORTB &= ~(1<< 5);
	}
	else {
		PORTB |= (1 << 5);
	}
}

/*
  checkInput(bit) - Checks the state of the input bit specified by the
  "bit" argument (0-7), and returns either 0 or 1 depending on its state.
*/
char checkEcho(char bit){
    if ((PIND & (1 << bit)) != 0)
        return(1);
    else
        return(0);
}