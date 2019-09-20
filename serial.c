#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "serial.h"

#define FOSC 16000000           // Clock frequency
#define BAUD 9600               // Baud rate used
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0 register

unsigned int received_count = 0;
char receive_distance[5];    // accurate data
char raw_distance[7];  // raw distance with@ and $
char null_char = '\0';
char start_flag = 0;
char valid_flag = 0;
//declare for for loop iniital
int i;
int serial_distance = 0;

void init_timer0(unsigned short m)
{
	// Set to CTC mode
	TCCR0B |= (1 << WGM01);

	// Enable Timer Interrupt
	TIMSK0 |= (1 << OCIE0A);

	// Load the MAX count
	// Assuming prescalar=256
	// counting to 15625 =
	// 0.25s w/ 16 MHz clock
	OCR0A = m;
	
	// Set prescalar = 8
	// and start counter
    
    TCCR0B |= (1 << CS01 );
}

void init_buzzer(void){
	DDRC |= (1 << 4);
	PORTC &= ~(1<< 4);
}

void serial_init(unsigned short ubrr_value)
{

    // Set up USART0 registers
    // Enable tri-state
	UBRR0 = MYUBRR;             // Set baud rate
	UCSR0C = (3 << UCSZ00);               // Async., no parity,
                                          // 1 stop bit, 8 data bits
    UCSR0B |= (1 << TXEN0 | 1 << RXEN0);  // Enable RX and TX

    //enabel buffer
	DDRD |= (1 << PD2);
	PORTD &= ~(1 << PD2);
	
}

void serial_txchar(char ch)
{
	// Wait for transmitter data register empty 
    while ((UCSR0A & (1<<UDRE0)) == 0);
    UDR0 = ch;
}

void serial_stringout (char *s)
{
    // Call serial_txchar in loop to send a string
    for (i = 0; i < 7; i++)
    {
        serial_txchar (s[i]);      // send each char inside the string with for loop
    }
}


void clear_distance(){
    //clear acc dis
    receive_distance[0] = '\0';
    
}

void clear_raw(){
    //clear raw dis
    raw_distance[0]='\0';
   
}

void display (char distance[]){
    int num;
    char buffer[7];
    // convert to int
    sscanf(distance, "%d", &num);
    serial_distance = num;

    //if range is more than 400 cm
    if (num > 4000){
        //if it is too far away
        lcd_moveto (1,9);
        lcd_stringout("too far");
        receive_distance[0] = '\0';   
    }
    //if data valid
    else{
        lcd_moveto (1,9);
        snprintf(buffer, 7, "%d.%d", serial_distance/10, serial_distance%10);   // print out the distance
        lcd_stringout(buffer);
        clear_distance();
    }
    //reset
    valid_flag = 0;
}

void sender(int distances){
    clear_raw();      // firstly initialize this string
    snprintf(raw_distance,7,"@%d$", distances);   // send data according to requirement
    serial_stringout(raw_distance);
}

void receiver(){
   if(valid_flag == 1){   // data valid
       display(receive_distance);
   }
}

// Interrupts

ISR (USART_RX_vect){
    char ch;
    ch = UDR0;  // Get the received charater
    
    //not start yet
    if (start_flag == 0){ 
        if (ch == '@'){
            //reset;
            clear_distance();
            received_count = 0;  
            start_flag = 1;
        }
    }
    //if start 
    else{ 
        //if end
        if (ch == '$'){    
            if (received_count > 0){
                // is valid
                valid_flag = 1;           
            }
            //no range data received 
            //reset to 0
            else{   
                clear_distance();
                valid_flag = 0;          
            }
            received_count = 0;
            start_flag = 0;
        }
        //while receving
        //receivde @
        //processed to receive again

        else if (ch == '@'){ 
            // reset
            clear_distance();
            received_count = 0;    
            valid_flag = 0; 
        }
        
        //receive a valid data
        else if ((ch >= '0') && (ch <= '9' )) { 
            //get data to ch
            receive_distance[received_count] = ch;
            received_count ++;
            //too many digits
            if (received_count > 4){  
                //reset 
                clear_distance(); 
                received_count = 0;
                valid_flag = 0;          
                start_flag = 0;
            }
        }
        //not valid
        //no $ or @ recived
        else{  
            clear_distance(); 
            start_flag = 0;    
            received_count = 0;
            valid_flag = 0;
            
            
        }
    }
    serial_flag=1;
}
