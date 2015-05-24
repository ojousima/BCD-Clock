/*
 * TWI.h
 *
 * Created: 30.6.2013 13:32:48
 *  Author: Otso Jousimaa
 * TODO: GPLV3 License
 */ 
 
#ifndef TWI
#define TWI
#include <avr/io.h>
#include <avr/interrupt.h> 
#define TWI_START_SENT	0x08  //Start condition sent  
#define TWI_MT_SLA_ACK	0x18  //Slave ACK has been received
#define TWI_MT_DATA_ACK	0x28  //Slave ACK has been received

//TODO IFNDEF F_CPU WARM

//void ERROR(uint8_t leds); TODO

//Call this once before using I2C.
//Disconnects pins 4 and 5 from port C
void TWIInit(void);

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
void TWIStart(void);


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
void TWIAddress(uint8_t address, uint8_t read);

void TWIWrite(uint8_t address, uint8_t *data, uint8_t length);

//Read byte with ack, this signals slave to keep on writing
uint8_t TWIReadACK(void);

//read byte with NACK, this signals to slave EOL
uint8_t TWIReadNACK(void);

//Read from ADDRESS to pointer data length bytes.
void TWIRead(uint8_t address, uint8_t *data, uint8_t length);

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
void TWIStop(void);


#endif 