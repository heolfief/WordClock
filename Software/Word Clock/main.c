/*
*
*		OUTPUT PINS CONFIGURATION AND 8X8 MATRIX LETTERS CONFGURATION
*
*
*	PORTS	PB0		PB1		PB2		PB3		PB4		PA4		PA5		PA6
*	PORTS	___		___		___		___		___		___		___		___
*
*	PC7	|	 H		 A		 T		 W		 E		 N		 T		 Y
*
*
*	PC6	|	 F		 I		 F		 V		 T		 E		 E		 N
*
*
*	PC5	|	 L		 F		 X		 P		 A		 S		 T		 O
*
*
*	PC4	|	 N		 I		 N		 E		 I		 G		 H		 T
*
*
*	PA0	|	 O		 N		 E		 T		 H		 R		 E		 E
*
*
*	PA1	|	 T		 W		 E		 L		 E		 V		 E		 N
*
*
*	PA2	|	 F		 O		 U		 R		 F		 I		 V		 E
*
*
*	PA3	|	 S		 I		 X		 S		 E		 V		 E		 N
*
*/


#define V_LOWBAT 3.4							// Low battery voltage value
#define V_CRITBAT 3.3 							// Critical battery voltage value



#define F_CPU 1000000UL  						// MCU is running at 1MHz

// Include needed libraries
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "lib/rtcDS3231/rtcDS3231.h"			// Include RTC library

// Functions definitions
void minutes_to_matrix(uint8_t now_minutes, uint8_t *line_data);
void hours_to_matrix(uint8_t now_hours, uint8_t *line_data);
void display(uint8_t *line_data, uint16_t number_of_loops);
void sleep();
void io_setup();
void check_battery(uint8_t *lowbat, uint8_t *critbat);

// Global variables
uint8_t setmode=0;								// ...
uint8_t time_counter=0;							// ...
uint8_t bt_flag=0;								// ...To be used by the ISR

// Main program
int main()
{
	uint8_t hour, min, sec, am_pm, data[8]={0}, low_battery=0, critical_battery=0, i;	// 8 bits variables declaration

	io_setup();									// Configure the MCU's GPIO
	
	PORTD |= (1<<PD2);							// Give power to RTC
	i2c_init();									// Initialize I2C communication
	PORTD &= ~(1<<PD2);							// Cut power to RTC	
		
	while (1)
	{
		sleep();
		
		check_battery(&low_battery, &critical_battery);
		
		if(low_battery) PORTD &= ~(1<<PD4);		// if battery level is low, turn ON a LED and run the program
		
		if(critical_battery)					// if battery level is critical, blink a LED ON and OFF and don't run the program
		{
			for(i=0; i<10; i++)					// Number of LED blinks
			{
				PORTD ^= (1<<PD4);				// Invert output state of PD4 (connected to LED cathode)
				_delay_ms(250);					// Wait 250ms
			}
			PORTD |= (1<<PD4);					// Turn OFF LED (output 5v to LED cathode)
		}
		else									// If battery level is good
		{
			PORTD |= (1<<PD2);					// Give power to RTC
			i2c_init();							// Initialize I2C communication
			rtc_get_time_12h(&hour, &min, &sec, &am_pm);	// Get time
			PORTD &= ~(1<<PD2);					// Cut power to RTC
			
			minutes_to_matrix(min, data);		// Convert minutes to the data array to be displayed on the LED matrix
			if(min>35)hour++;					// If minutes are more than half an hour, then add an hour to the display
			hours_to_matrix(hour, data);		// Convert hours to the data array to be displayed on the LED matrix
			display(data, 1000);					// Display the data array for 10s (1251)
			
			if (bit_is_clear(PIND,PD3))			// If button is still pressed
			{
				MCUCR |= (1<<ISC11);			// ...
				MCUCR &= ~(1<<ISC10);			// ... Enable interrupt on falling edge of INT1
				sei();                         	// Enable interrupts
				setmode=1;						// Go to setmode
				
				for (time_counter=0;time_counter<20;time_counter++) // Blink several times
				{
					if (bt_flag)
					{
						bt_flag=0;				// Reset button flag
						min=min-min%5;			// Set min to nearest 5min step
						min+=5;					// Increase minutes by 5 on button press
						if (min>=60)min=0;		// Handle minutes overflow
						if ((min>=35) && (min<40))hour++; // Increase hour
						if (hour>=12)hour=0;	// Handle hour overflow
					}
					minutes_to_matrix(min, data);	// Convert minutes to the data array to be displayed on the LED matrix
					hours_to_matrix(hour, data);// Convert hours to the data array to be displayed on the LED matrix
					display(data, 31);			// Display the data array for 0.25s
					_delay_ms(50);				// Wait 250ms
				}
				cli();                         	// Disable interrupts
				setmode=0;						// Exit setmode
				
				PORTD |= (1<<PD2);				// Give power to RTC
				rtc_set_time_12h(hour,min,0,0);	// Set new time to RTC
				PORTD &= ~(1<<PD2);				// Cut power to RTC
			}
		}
	}
}

void minutes_to_matrix(uint8_t now_minutes, uint8_t *line_data)
{
	switch(now_minutes)
	{
		// Each bit correspond to the letter above him. LED matrix is active low, 0 is LED ON, 1 is LED OFF
		
		// minutes from 0 to 4 : line 0,1 and 2 OFF						   HATWENTY					  FIFVTEEN					 LFXPASTO
	case  0 : case  1 : case  2 : case  3 : case  4 : line_data[0] = 0b11111111; line_data[1] = 0b11111111; line_data[2] = 0b11111111; break;
		
		// minutes from 5 to 9 : "five" and "past" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case  5 : case  6 : case  7 : case  8 : case  9 : line_data[0] = 0b11111111; line_data[1] = 0b00101011; line_data[2] = 0b11100001; break;
		
		// minutes from 10 to 14 : "ten" and "past" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 10 : case 11 : case 12 : case 13 : case 14 : line_data[0] = 0b11010011; line_data[1] = 0b11111111; line_data[2] = 0b11100001; break;
		
		// minutes from 15 to 19 : "fifteen" and "past" words are lit	   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 15 : case 16 : case 17 : case 18 : case 19 : line_data[0] = 0b11111111; line_data[1] = 0b00010000; line_data[2] = 0b11100001; break;
		
		// minutes from 20 to 24 : "twenty" and "past" words are lit	   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 20 : case 21 : case 22 : case 23 : case 24 : line_data[0] = 0b11000000; line_data[1] = 0b11111111; line_data[2] = 0b11100001; break;
		
		// minutes from 25 to 29 : "twenty five" and "past" words are lit  HATWENTY					  FIFVTEEN					 LFXPASTO
	case 25 : case 26 : case 27 : case 28 : case 29 : line_data[0] = 0b11000000; line_data[1] = 0b00101011; line_data[2] = 0b11100001; break;
		
		// minutes from 30 to 34 : "half" and "past" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 30 : case 31 : case 32 : case 33 : case 34 : line_data[0] = 0b00111111; line_data[1] = 0b11111111; line_data[2] = 0b00100001; break;
		
		// minutes from 35 to 39 : "twenty five" and "to" words are lit	   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 35 : case 36 : case 37 : case 38 : case 39 : line_data[0] = 0b11000000; line_data[1] = 0b00101011; line_data[2] = 0b11111100; break;
		
		// minutes from 40 to 44 : "twenty" and "to" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 40 : case 41 : case 42 : case 43 : case 44 : line_data[0] = 0b11000000; line_data[1] = 0b11111111; line_data[2] = 0b11111100; break;
		
		// minutes from 45 to 49 : "fifteen" and "to" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 45 : case 46 : case 47 : case 48 : case 49 : line_data[0] = 0b11111111; line_data[1] = 0b00010000; line_data[2] = 0b11111100; break;
		
		// minutes from 50 to 54 : "ten" and "to" words are lit			   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 50 : case 51 : case 52 : case 53 : case 54 : line_data[0] = 0b11010011; line_data[1] = 0b11111111; line_data[2] = 0b11111100; break;
		
		// minutes from 55 to 59 : "five" and "to" words are lit		   HATWENTY					  FIFVTEEN					 LFXPASTO
	case 55 : case 56 : case 57 : case 58 : case 59 : line_data[0] = 0b11111111; line_data[1] = 0b00101011; line_data[2] = 0b11111100; break;
	}
}

void hours_to_matrix(uint8_t now_hours, uint8_t *line_data)
{
	switch(now_hours)
	{
		// Each bit correspond to the letter above him. LED matrix is active low, 0 is LED ON, 1 is LED OFF
		
		// 	twelve				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  0 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b00001001; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	one					   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  1 : line_data[3] = 0b11111111; line_data[4] = 0b00011111; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	two					   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  2 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b00111111; line_data[6] = 0b10111111; line_data[7] = 0b11111111; break;
		
		// 	three				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  3 : line_data[3] = 0b11111111; line_data[4] = 0b11100000; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	four				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  4 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b00001111; line_data[7] = 0b11111111; break;
		
		// 	five				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  5 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b11110000; line_data[7] = 0b11111111; break;
		
		// 	six					   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  6 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b00011111; break;
		
		// 	seven				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  7 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b11100000; break;
		
		// 	eight				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  8 : line_data[3] = 0b11100000; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	nine				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case  9 : line_data[3] = 0b00001111; line_data[4] = 0b11111111; line_data[5] = 0b11111111; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	ten					   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case 10 : line_data[3] = 0b11111110; line_data[4] = 0b11111110; line_data[5] = 0b11111110; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
		
		// 	eleven				   NINEIGHT					  ONETHREE					 TWELEVEN					FOURFIVE				   SIXSEVEN
	case 11 : line_data[3] = 0b11111111; line_data[4] = 0b11111111; line_data[5] = 0b11000000; line_data[6] = 0b11111111; line_data[7] = 0b11111111; break;
	}
}

void display(uint8_t *line_data, uint16_t number_of_loops)
{
	uint8_t i, temp, temprev[8];				// 8 bits variables declaration
	uint16_t y;									// 16 bits variables declaration
	
	for(i=0; i<8; i++)							// For hardware pin mapping reason, reverse each byte of data (8 bytes, 1 byte per line and 1 bit per column )
	{	
		temprev[i] = (line_data[i] * 0x0202020202ULL & 0x010884422010ULL) % 1023;   // Reverse binary (0bABCDEFGH --> 0bHGFEDCBA)
	}
	
	for(y=0; y<number_of_loops; y++)			// Loop multiplexing
	{
		for(i=0; i<8; i++)						// For each line
		{
			temp = temprev[i] & 0b00011111;		// Only the 5 LSB are needed
			PORTB &= 0b11100000;				// Clear the 5 LSB of PORTB
			PORTB |= temp;						// Set the 5 LSB of PORTB to line_data according to schematics
			
			temp = temprev[i]>>5;				// Shift 5 times to the right, to keep only the 3 MSB
			temp = temp<<4;						// Shift 4 times to the left
			PORTA &= 0b10001111;				// Clear the needed bits of PORTA
			PORTA |= temp;						// Set the needed bits of PORTA to line_data according to schematics
			
			switch(i)							// Give power to one line at each time (5v to line number i LED anodes)
			{
			case 0 : PORTC |=  (1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 1 : PORTC &= ~(1<<7); PORTC |=  (1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 2 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC |=  (1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 3 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC |=  (1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 4 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA |=  (1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 5 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA |=  (1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); break;
			case 6 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA |=  (1<<2); PORTA &= ~(1<<3); break;
			case 7 : PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA |=  (1<<3); break;
			}
			_delay_ms(1);						// 1ms delay between each line = multiplexing at 1/(8*0.001s) = 125Hz
		}
		// 8ms passed, do it again 1250 times to display for 10s
	}
	PORTC &= ~(1<<7); PORTC &= ~(1<<6); PORTC &= ~(1<<5); PORTC &= ~(1<<4); PORTA &= ~(1<<0); PORTA &= ~(1<<1); PORTA &= ~(1<<2); PORTA &= ~(1<<3); // Display is OFF
	PORTB &= 0b11100000;						// Display is OFF
	PORTA &= 0b10001111;						// Display is OFF
}

void sleep()
{
	PORTD |= (1<<PD4);							// Turn OFF LED on PD4 (output 5v to LED cathode)
	
	ACSR |= (1<<ACD);          					// Disable Analog comparator	
	
	GICR |= (1<<INT1);							// Enable interrupt	INT1
	MCUCR &= ~((1<<ISC11) | (1<<ISC10));		// Enable interrupt on low level of INT1 (only compatible mode with sleep mode wake up on INT1)
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);   	 	// Set sleep mode to "power down mode"
	sleep_enable();                        		// Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
	sei();                                 		// Enable interrupts
	sleep_cpu();                           		// sleep

	//...Wait for interrupt to wake-up MCU...
	
	// Interruption occurred : ISR(INT1_vect) is called
	
	cli();                                  	// Disable interrupts
	sleep_disable();                        	// Clear Sleep Enable bit (SE bit)
}

ISR(INT1_vect)									// Interrupt service routine
{
	if (setmode)								// If user is changing the time
	{
		time_counter=0;							// Reset time counter, allowing 5 more seconds to change the time before returning to sleep mode
		bt_flag = 1;							// Set button flag to 1, to change time
	} 
	else										// If the interruption occurred during sleep mode
	{
		sleep_disable();						// When interruption occurred, disable sleep mode (and so wake MCU up)
	}
}

void io_setup()
{
	DDRA = 0x7F;								// PORTA as output except PA7 as input
	DDRB = 0xFF;								// PORTB as output
	DDRC = 0xFC;								// PORTC as output
	DDRD = 0x14;								// PD3 as input, PD4 and PD2 as outputs
	
	PORTA = 0x00;								// All outputs low
	PORTB = 0x00;								// All outputs low
	PORTC = 0x00;								// All outputs low
	PORTD = 0x10;								// All outputs low except PD4 (led OFF)
	PORTD |= (1<<PD3);							// Enable pullup on PD3
}

void check_battery(uint8_t *lowbat, uint8_t *critbat)
{
	float adcvalue, voltage;					// Variables declaration
	
	ADMUX = 0x47;								// Select ADC7 and reference voltage on AVCC (with capacitor to GND at AREF pin)
	ADCSRA |= (1<<ADPS1) | (1<<ADPS0);			// ADC clock prescaler /8
	ADCSRA |= (1<<ADEN);						// Enable ADC
	ADCSRA |= (1<<ADSC);						// Start ADC conversion
	loop_until_bit_is_clear(ADCSRA, ADSC); 		// Wait until done
	adcvalue = ADC;								// Read ADC
	ADCSRA &= ~(1<<ADEN);      					// Disable ADC
	
	voltage = adcvalue*3.3*43/1023/33;			// Actual battery voltage
	
	if(voltage < V_LOWBAT) *lowbat = 1;			// If battery level is low, rise a corresponding flag
	else *lowbat = 0;
	
	if(voltage < V_CRITBAT) *critbat = 1;		// If battery level is critical, rise a corresponding flag
	else *critbat = 0;
}

