/********************************************
 *
 *  Name:Zhenghao Dai
 *  Email:Zhenghad@usc.edu
 *  Section: Wednesday 3:30pm
 *  Assignment: Project
 *
 ********************************************/
//main file 
/*
	installation
	PD
		D0	RX
		D1	TX
		D2	Buffer
		D3	Echo
	PB
		D11(PB3)	acquire button
		D12(PB4) adjust button
		D13(PB5) Trig
	PC	
		A1(PC1)	Encoder
		A2		led_red
		A3		led_green
		A4		Buzzer
		A5		Encoder
*/
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "rangefinder.h"
#include "encoder.h"
#include "serial.h"

//prototype
char checkBInput(char);//for PB
void led_init(void);
void checkAcquire(void);
void print(void);
void checkAdjust(void);
void green_on(void);
void green_off(void);
void red_on(void);
void red_off(void);
char local_compare(void);
void play_note(unsigned short freq);

//timer 1 counter
volatile int counter =0;


volatile char buffer[7]="@0000$";
char buffer1[17];
char buffer2[17];

char flag =0;
volatile int distance;
volatile char mode =0;//adjust mode

//
int local, remote=0;
volatile unsigned char a, b;
unsigned char new_state, old_state, bits;
volatile unsigned char encoder_changed = 0;  // Flag for state change
volatile int encoder_count=0;//encoder counter
int local_min,remote_min;

char buzzer_flag=0;
unsigned long period;


#define FOSC 16000000           // Clock frequency
#define BAUD 9600               // Baud rate used
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0 register

//remote distance 4bytes and one '/0'
char dis_buffer[5];

int mess_counter=0;

char timeout=0;


char comp=0;
//inital
void inital(void){
	lcd_init();
	rangefinder_init();	//include acquire button
	encoder_init(new_state, old_state);
	led_init();
	init_buzzer();
	serial_init(MYUBRR);

	// init and start timer
	// we want 0.00 01s(10us) *16,000,000=160
	//so we want m = 160/1=160
	init_timer1(160);
	
	// increment every 0.00 0001s
	//0.00 0001(1us)*16,000,000=16
	init_timer0(20);

	// Enable interrupts
	UCSR0B |= (1 << RXCIE0);    // Enable receiver interrupts
	sei();

}

char buf[7];
int main(void)
{
	inital();

	// Show the splash screen
	lcd_splash();

	//clean screen
	lcd_writecommand(1);

	//check for valid EEPROM value
	encoder_count= eeprom_read_word((void*)100);
	//if is not valid
	//reset to 0
	if(encoder_count>400 || encoder_count>0){
		//reset to 0
		eeprom_update_word((void *) 100, 0);	
		encoder_count=0;
	}
	
	print();
	lcd_moveto (1,9);
	lcd_stringout("0.0");
	// Loop forever
	while (1) {
			
		checkAcquire();
		checkAdjust();
		
		
		receiver(); 
		
		
		if (encoder_changed) { // Did min change?
			encoder_changed = 0;        // Reset changed flag
			//store value to eeprom
			if(mode==0)//local mode
				eeprom_update_word((void *) 100, encoder_count); //update current encoder int
			else //remote mode
				eeprom_update_word((void *) 200, encoder_count);//update current encoder int
			print();
		}
		//print();
		
		if(flag){
			_delay_ms(20);
			//distance=counter*0.00001*340*100*10/2;
			print();
			//send distance
			sender(distance);
		}
		//determine the lcd light
		//in local mode
		if(mode ==0){
			if(local_compare()){
				green_on();
				red_off();
			}
			else 
			{
				red_on();
				green_off();
			}
		}		
		//in remote mode
		if(mode==1){
			green_off();
			red_off();

			if(remote_min> (serial_distance/10))
				if (serial_flag == 1){          // the buzzer will play a sound when the mode changed and the encoder count is larger than serial distance received
                    play_note(228);
                    serial_flag = 0;             // set the flag to 1
                }

		}

	}
	return 0;   /* never reached */

}

//interrupt for echo
ISR(PCINT2_vect){
	if(checkEcho(3)!=0)
		flag=1;//start counting
	else
	{
		flag=0;//stop counting
	}	
}


//Timer1 for pulse 
//increments every 0.00 001s
//10 us
ISR(TIMER1_COMPA_vect)
{
	//if flag for timer1 has been changed
	if(flag!=0){
		counter++;
	}
	
	//time out function
	//if measureing more than 400 cm
	//reset counter to 0
	//print too far
	if(counter >2320){
		counter =0;
		flag=0;
		lcd_moveto(0,8);
		lcd_stringout("too far");
	}
	
}


int time0_count=0;
//timer for buzzer
ISR(TIMER0_COMPA_vect){
	// increments every 0.00 0001s
	//1us
	if(buzzer_flag==1){
		time0_count++;

	

		if(time0_count>=period/2){
			time0_count=0;
			buzzer_flag=0;
		}
	}    
}



//ISR for encoder
ISR(PCINT1_vect){
	
	// Read the input bits and determine A and B
	bits = PINC;
	a = (bits & (1 << PC1));//a==1 should be true or ==0x02
	b = (bits & (1 << PC5));
		
	// For each state, examine the two input bits to see if state
	// has changed, and if so set "new_state" to the new state,
	// and adjust the count value.
	if (old_state == 0) {//00
		
		// Handle A and B inputs for state 0
		if(a){//CW
		
			encoder_count++;
			new_state=1;
		}	
		else if(b){//CCW
			encoder_count--;
			new_state=2;
		}	
	}	
		
	else if (old_state == 1) {//01

		// Handle A and B inputs for state 1
		if(b){//CW
			encoder_count++;
			new_state=3;
		}
		else if(!a){//CCW
			encoder_count--;
			new_state=0;
		}
	}
	
	else if (old_state == 2) {//10
		// Handle A and B inputs for state 2
		if(!b){//CW
			encoder_count++;
			new_state=0;
		}	
		else if(a){//CCW
			encoder_count--;
			new_state=3;
		}
	}
	
	else {  //11
		// old_state = 3
		// Handle A and B inputs for state 3
		if(!a){//CW
			encoder_count++;
			new_state=2;
		}	
		else if(!b){//CCW
			encoder_count--;
			new_state=1;
		}
	}
	
	// If state changed, update the value of old_state,
	// and set a flag that the state has changed.
	if (new_state != old_state) {
	    encoder_changed = 1;
	    old_state = new_state;
	}
	
	//encoder range 1-400
	if(encoder_count<1){
		encoder_count=1;
	}
	//max value for encoder is 400
	else if (encoder_count>400)
	{
		encoder_count=400;
	}
	
}



/*
	initial LED
		Initialize appropriate DDR registers
		Initialize the LED output to 0
*/
void led_init(void){
	DDRC |= (1 << 2);
	PORTC &= ~(1<< 2);
	DDRC |= (1 << 3);
	PORTC &= ~(1<< 3);
	
}


//check port b input
//return 1 for in
char checkBInput(char bit)
{
    if ((PINB & (1 << bit)) != 0)
        return(1);
    else
        return(0);
}

//check for acquire buttion
//if the button is pressed
//then make a trig for 10 us
void checkAcquire(void){
	if(checkBInput(3)==0){
		while(checkBInput(3)==0){}
		lcd_writecommand(1);
		counter=0;
		makeTrig(1);
		_delay_us(10); 
		makeTrig(0);
		}
}

void checkAdjust(void){
	if(checkBInput(4)==0){
		while(checkBInput(4)==0){}
				
			if(mode==1){//remote
				mode=0;
				encoder_count= eeprom_read_word((void*)100);
			}
			else{//local
				mode=1;
				encoder_count= eeprom_read_word((void*)200);
			}
		print();	
			
			
	}
}

//print function that will print on lcd
//
void print(void){
	//distance has been multiplied by 10
	//no decimal/flaoating point
	//distance /10 = integer
	//distamce%10 = floating point
	distance=counter*0.00001*340*100*10/2;
	
	local_min=encoder_count;
	remote_min=encoder_count;
	
	//if local mode
	if(mode==0){
		//distance /10 = integer
		//distamce%10 = floating point
		snprintf(buffer1, 17, "Local      %3d.%d",distance/10,distance%10);
		snprintf(buffer2, 8, "Min=%3d",local_min);
	}
	else{//remote mode
		snprintf(buffer1, 17, "Remote     %3d.%d",distance/10,distance%10);
		snprintf(buffer2, 8, "Min=%3d",remote_min);
	}

	lcd_moveto(0,0);
	lcd_stringout(buffer1);
	lcd_moveto(1,0);
	lcd_stringout(buffer2);
}

void green_on(void){
	PORTC |= (1 << 3);
}
void green_off(void){
	PORTC &= ~(1<< 3);
}

void red_on(void){
	PORTC |= (1 << 2);
}

void red_off(void){
	PORTC &= ~(1<< 2);
}


//1 if local>threshold green on
//0 if local<threshold red on
char local_compare(void){
	double dis= distance/10;
	if(dis>local_min){
		return (1);
	}
	else
		return (0);
}


//Play a tone at the frequency specified for one second
void play_note(unsigned short freq)
{
    period = 1000000 / freq;      // Period of note in microseconds

    while (freq--) {
		// Make PB4 high
		PORTC |= (1 << PC4);
		//need delay
		buzzer_flag=1;
		
		if(buzzer_flag==0){
			// Make PB4 low
			PORTC &= ~(1 << PC4);
			buzzer_flag=1;
		}
		//delay for turn off
		while(buzzer_flag==0){}
    }
}      




















