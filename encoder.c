#include <avr/io.h>


void encoder_init(unsigned char new_state, unsigned char old_state ){
    //enable the PC1 and PC5 pull-ups
    PORTC |= ((1<< PC1)|(1<< PC5));
    //enable interrupts
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1<< PCINT13)|(1<<PCINT9);


    unsigned char a, b;
    unsigned char bits; 
    // Read the A and B inputs to determine the intial state
    // Warning: Do NOT read A and B separately.  You should read BOTH inputs
    // at the same time, then determine the A and B values from that value.    
	bits = PINC;//snap the pinc
	a = (bits & (1 << PC1));//set a equal to pc1
	b = (bits & (1 << PC5));//set b equal to pc5

	//determine the intial state
    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;
}

