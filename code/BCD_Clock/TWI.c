/*
 * TWI.c
 *
 * Created: 30.6.2013 13:32:48
 *  Author: Otso Jousimaa
 * TODO:
	-freq selection
	-function pointer for error
 */ 
#include "TWI.h"

//Disconnects pins C4 & C5 from port C!
void TWIInit(void)
{
	//F_SCL = F_CPU / (16 + 2*TWBR * prescaler)
	//Prescaler values:
	/*
	 * xxxx xx00 = 1 
	 * xxxx xx01 = 4
	 * xxxx xx10 = 16
	 * xxxx xx11 = 64
	 */
	//set SCL to 50kHz @ F_CPU of 16MHZ, TODO: Freq select
	TWSR = 0x02; //TWSR xxxx xx10, prescaler of 16
	TWBR = 0x0A; //bit rate of 0d10
	TWCR = (1<<TWEN);//enable TWI
}

/*
From atmega328 datasheet:
The first step in a TWI transmission is to transmit a START condition. This is done by
writing a specific value into TWCR, instructing the TWI hardware to transmit a START
condition. Which value to write is described later on. However, it is important that the
TWINT bit is set in the value written. Writing a one to TWINT clears the flag. The TWI will
not start any operation as long as the TWINT bit in TWCR is set. Immediately after the
application has cleared TWINT, the TWI will initiate transmission of the START condition
*/
/* 2013-06-30 ok */
void TWIStart(void)
{
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); //Clear interrupt, Write start , write enable
	/*
	When the START condition has been transmitted, the TWINT Flag in TWCR is set, and
	TWSR is updated with a status code indicating that the START condition has successfully been sent.
	*/
	while ((TWCR & (1<<TWINT)) == 0);

	/*
	The application software should now examine the value of TWSR, to make sure that the
	START condition was successfully transmitted. If TWSR indicates otherwise, the application software
	might take some special action, like calling an error routine.
	*/
	//if ((TWSR & 0xF8) != TWI_START_SENT)
	//	ERROR(LED_RED);
		
		
}

/*
Assuming that the status code is as expected, the application must load SLA+W into TWDR. Remember
that TWDR is used both for address and data. After TWDR has been loaded with the
desired SLA+W, a specific value must be written to TWCR, instructing the TWI hardware
to transmit the SLA+W present in TWDR. Which value to write is described later on.
However, it is important that the TWINT bit is set in the value written. Writing a one to
TWINT clears the flag. The TWI will not start any operation as long as the TWINT bit in
TWCR is set. Immediately after the application has cleared TWINT, the TWI will initiate
transmission of the address packet.
*/
/*2013-06-30 ok*/
void TWIAddress(uint8_t address, uint8_t read)
{
	address <<=1;
	if(read)
		++address;


	TWDR = (address);
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
	
	/*
	Check value of TWI Status
	Register. Mask prescaler bits. If
	status different from
	MT_SLA_ACK go to ERROR
	*/
	//if ((TWSR & 0xF8) != TWI_MT_SLA_ACK)
	//	ERROR(LED_RED);
	
	
}


void TWIWrite(uint8_t address, uint8_t *data, uint8_t length)
{

	TWIStart();

	TWIAddress(address, 0x00);

	//For each byte
	for(unsigned int ii = 0; ii < length; ii++)
	{

		/*
		Load DATA into TWDR Register.
		Clear TWINT bit in TWCR to
		start transmission of data
		*/
		TWDR = *data++;
		TWCR = (1<<TWINT) |	(1<<TWEN);
		
		/*
		Wait for TWINT Flag set. This
		indicates that the DATA has been
		transmitted, and ACK/NACK has
		been received.
		*/
		while (!(TWCR & (1<<TWINT)));
		
		/*
		Check value of TWI Status
		Register. Mask prescaler bits. If
		status different from
		MT_DATA_ACK go to ERROR
		*/
		//if ((TWSR & 0xF8) != TWI_MT_DATA_ACK)
		//	ERROR(LED_GREEN | LED_RED);
	}
	
	TWIStop();
	
}

uint8_t TWIReadACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}
//read byte with NACK
uint8_t TWIReadNACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

void TWIRead(uint8_t address, uint8_t *data, uint8_t length)
{
	//Start comms
	TWIStart();
	//Address slave for reading
	TWIAddress(address, 0x01);
	
	//Do not read last byte with ack
	for(unsigned int ii = 0; ii < length; ++ii)
	{
		*data++ = TWIReadACK();
	}
	//Read last bit with nack
	*data = TWIReadNACK(); 
	//Generate stop
	TWIStop();
}

/* 
Assuming that the status code is as expected, the
application must write a specific value to TWCR, instructing the TWI hardware to transmit
a STOP condition. Which value to write is described later on. However, it is important that
the TWINT bit is set in the value written. Writing a one to TWINT clears the flag. The TWI
will not start any operation as long as the TWINT bit in TWCR is set. Immediately after
the application has cleared TWINT, the TWI will initiate transmission of the STOP condition. 
Note that TWINT is NOT set after a STOP condition has been sent.
*/
/*2013-06-30 ok */
void TWIStop(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}
