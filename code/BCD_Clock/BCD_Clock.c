/*
 * BCD_Clock.c
 *
 * Created: 19.7.2013 19:38:10
 *  Author: Otso
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
#include "../../lib/TWI/TWI.h" //TODO: Packet lib to this folder.

/**
 * Leds are attahed to PC0-3 and PB0-2 
 * PB0: 0x10
 * PB1: 0x01
 * PB2: 0x20
 * PC0: 0x02
 * PC1: 0x40
 * PC2: 0x04
 * PC3: 0x08
 */

#define LED_10 0
#define LED_20 2
#define LED_40 1
#define LED_01 1
#define LED_02 0
#define LED_04 2
#define LED_08 3

#define HH 7
#define MM 6
#define SS 5

#define BUTTON_1 3
#define BUTTON_2 4
#define BUTTON_PIN PIND
#define BUTTON_PORT PORTD

volatile uint8_t tick = 0;		//Second changed
volatile uint8_t userint = 0;	//User interrupt, set time.
static uint8_t time_index = 1; //Write to SS/MM/HH

static uint8_t time[4]; //SP + SS:MM:HH in BCD

//static const int[] LEDORDER = [LED_01, LED_02, LED_04, LED_08, LED_10, LED_20, LED_40];
static const int FETORDER[] = {0, SS, MM, HH};
	
static const int BRIGHTNESS = 5; //1-500

static int loopcounter = 0;

static int mode = 0; //0-usual operation 1, 2-Set seconds 3,4-Set minutes 5,6-Set hours 7- Set brightness (not implemented)


//function declarations
void key_task(uint8_t key);
void display_time(void);
uint8_t debounce(void);

int main(void)
{
	//Take output pins low
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0x00;
	
	//Set output pins to output
	DDRB |= (1 << 0)|(1 << 1)|(1 << 2);
	DDRC |= (1 << 0)|(1 << 1)|(1 << 2)|(1 << 3);
	DDRD |= (1 << 5)|(1 << 6)|(1 << 7);
	
	//Start communicating over I2C
	TWIInit();
	
	//Register pointer, second, min, hr, date, day, month, yr, ctrl
	//uint8_t data[] = {0x00, 0x00, 0x10, 0x16, 0x00, 0x30, 0x06, 0x13, 0x10};
	
	//RP + ctrl	
	uint8_t data[] = {0x07, 0x10};	
		
	//Write to DS1307
	TWIWrite(0x68, data, 0x02);
	
	EIMSK |= 0x01;  //Enable INT0
	EICRA |= 0x03; //Trigger on high edge of INT0 - DS1307 updates on low.
	//TODO: Enable INT1
	//TODO: Trigger on LOWER EDGE of INT1
	sei(); //Allow interrupts
	
	while(1)
	{
		//Second changed, turn off drains, read time. if user is not setting time
		if(tick && (!mode))
		{
			//Clear interrupts- do not allow user to **** up data read
			cli();
			
			//take mosfets off
			PORTD &= ~( (1<<HH)|(1<<MM)|(1<<SS));
			
			//Set stack pointer of DS1307 to 0x00 (seconds)
			TWIWrite(0x68, 0x00, 0x01);
			
			//read SP + SSMMHH from DS1307
			TWIRead(0x68, time, 4);
			
			//Format time - DS1307 returns HH 0x2x as 0b 0011 xxxx. Change to 0b 0010 xxxx
			if(0x20 & time[4])
				time[4] &= ~(0x10);
				
			tick = 0;
			
			//Read done, enable interrupts
			sei();
			
		}
		
		
		
		
		display_time();
		loopcounter++;
		uint8_t key = debounce();
		
		//Handle keypress
		key_task(key);

	}
}

void key_task (uint8_t key)
{
	//index to write time in
	uint8_t index = ((mode-1) >> 1) + 1;
		
	//Store last value of time index to allow blinking field to be set.
	static uint8_t stored_value = 1;
	
	//Set bits not to be changed high
	uint8_t keep_mask = 0x0F;			
	if(mode%2)
		keep_mask = 0xF0;
	
	//Blink area to be modified - TODO: separate from key task
	if(mode)
	{
		int counter = loopcounter & 0xFF; // approx 1.5 Hz
		if(counter < 0x8F)
		{
			
			time[index] &= keep_mask;
			
		}
		else
		{
			time[index] |= stored_value;
		}
	}
		
		
	//Return if key was not pressed
	if(!key)
		return;
		
	if(key == (1 << BUTTON_1))
	{		
		
		if(mode)
		{
			//Write stored value to time before going to next mode
			time[index] &= keep_mask;
			time[index] |= stored_value;
		}
		
		//Increment mode if key 1 was pressed
		mode++;

		
		if(mode%2)
			stored_value = 0x01; //set 1 at default show new value
		else 
			stored_value = 0x10;
		
		//back to usual operation if all options are set
		if(mode > 6)
		{
		mode = 0;
		//Write data to DS1307
		time[0] = 0x00;
		TWIWrite(0x68, time, 0x04);
		}
	}
	
	//Increment time if key 2 was pressed
	if(key == (1 << BUTTON_2))
	{
		uint8_t increment = 0x10;
		
		if(mode%2)
			increment = 0x01;
			
		//Highest value field can get
		uint8_t max_value = 9; // 9 by default
		
		//5 at tens of seconds and minutes
		if(mode == 2 || mode == 4)
			max_value = 0x50;
			
		//2 at tens of hours
		if(mode == 6)
			max_value = 0x20;
			
		// increment
		stored_value += increment;
		
		//roll to 0 if max was exceeded.
		if(stored_value > max_value)
			stored_value = 0;
		
	}
}



uint8_t debounce (void)
{
	//read buttons 
	static int pressed_counter = 0;
	uint8_t key = BUTTON_PIN & (1<<BUTTON_1 | 1<<BUTTON_2);
	
	//was button 1 pulled down?
	if(!(key & (1<<BUTTON_1)))
		key = 1 << BUTTON_1;
	//was key 2 pulled down?
	else if(!(key & (1 << BUTTON_2)))
		key = 1 << BUTTON_2;
	else
		key = 0;
	
	//If no key is pressed, reset counters and return 0
	if(!key)
	{
	pressed_counter = 0;
	return 0;
	}
	
	//If key was pressed
	else 
	{
		//Store counter value on first round
		if(!pressed_counter)
			pressed_counter = loopcounter;
			
		//Trigger key press on 5th round (approx 15 ms debounce)
		if(loopcounter == (pressed_counter + 5))
		{
			return key; //Retun keypress
		}
	}
	
	return 0;
}

void display_time(void)
{
	
	//Show time
	
	//Turn off mosfets and leds
	PORTB &= ~((1 << 0)|(1 << 1)|(1 << 2));
	PORTC &= ~((1 << 0)|(1 << 1)|(1 << 2)|(1 << 3));
	PORTD &= ~((1 << 5)|(1 << 6)|(1 << 7));
	
	//Calculate port positions

	if(time[time_index] & 0x01)
	PORTB |= (1 << LED_01);
	
	if(time[time_index] & 0x02)
	PORTC |= (1 << LED_02);
	
	if(time[time_index] & 0x04)
	PORTC |= (1 << LED_04);
	
	if(time[time_index] & 0x08)
	PORTC |= (1 << LED_08);
	
	if(time[time_index] & 0x10)
	PORTB |= (1 << LED_10);
	
	if(time[time_index] & 0x20)
	PORTB |= (1 << LED_20);
	
	if(time[time_index] & 0x40)
	PORTC |= (1 << LED_40);
	
	//Turn appropriate mosfet on
	//target 100Hz - delay 3ms
	
	for(int ii = 0; ii < 6; ii++)
	{
		PORTD |= (1 << FETORDER[time_index]);
		_delay_us(BRIGHTNESS);

		PORTD &= ~(1 << FETORDER[time_index]);
		_delay_us(500-BRIGHTNESS);
	}

	
	//scale index between 1-3
	time_index++;
	time_index %= 4;
	if(!time_index)
	time_index++;
}

//Interrupt handler
ISR(INT0_vect)
{
	tick = 1;//Set flag to read I2C
}