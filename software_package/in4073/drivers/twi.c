/*------------------------------------------------------------------
 *  twic.c-- i2c driver. It had to be blocking so that the invensense
 *			sdk could be used. ~650us to read the fifo
 *			-=sucks=- how much faster can you make it??
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "in4073.h"

static volatile bool sent = false;
static volatile bool read = false;

bool i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t data_length, uint8_t *data)
{
	if (!data_length) return -1;

	sent = false;
	read = false;
	NRF_TWI0->ADDRESS = slave_addr;
	NRF_TWI0->TXD = reg_addr;
	NRF_TWI0->SHORTS = 0;
	NRF_TWI0->TASKS_STARTTX = 1;

	while (!sent);
	sent = false;
	
	if (data_length-1 == 0) NRF_TWI0->SHORTS = TWI_SHORTS_BB_STOP_Msk;
	else NRF_TWI0->SHORTS = TWI_SHORTS_BB_SUSPEND_Msk;

	NRF_TWI0->TASKS_STARTRX = 1;

	while (true)
	{
		while (!read);
		read = false;
	
		*data++ = NRF_TWI0->RXD;
		if (--data_length == 1) NRF_TWI0->SHORTS = TWI_SHORTS_BB_STOP_Msk;
		NRF_TWI0->TASKS_RESUME = 1;
		if (data_length == 0) break;	
	}
	return 0;			
}

bool i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t data_length, uint8_t const *data)
{
	if (!data_length) return -1;

	sent = false;
	NRF_TWI0->ADDRESS = slave_addr;
	NRF_TWI0->SHORTS = 0;
	NRF_TWI0->TXD = reg_addr;
	NRF_TWI0->TASKS_STARTTX = 1;

	while(!sent);
	sent = false;  // address sent

	while (data_length-- != 0)
	{
		NRF_TWI0->TXD = *data++;

		while(!sent);
		sent = false;  
	}
	
	NRF_TWI0->TASKS_STOP = 1;

	return 0;
}
	
void SPI0_TWI0_IRQHandler(void) 
{
	if(NRF_TWI0->EVENTS_RXDREADY != 0)
	{
		NRF_TWI0->EVENTS_RXDREADY = 0;
//		printf("\revent rxready\n");
		read = true;
	}

        if(NRF_TWI0->EVENTS_TXDSENT != 0)
	{
		NRF_TWI0->EVENTS_TXDSENT = 0;
//		printf("\revent txdsent\n");
		sent = true;
  	}
        
	if(NRF_TWI0->EVENTS_ERROR != 0)
    	{
		printf("\rTWI error, code: %lx | at %lu usecs, from device: %ld\n", NRF_TWI0->ERRORSRC, get_time_us(), NRF_TWI0->ADDRESS);
		NRF_TWI0->ERRORSRC = 3;
        	NRF_TWI0->EVENTS_ERROR = 0;
	}

/*	if(NRF_TWI0->EVENTS_STOPPED != 0)
    	{
//		printf("\revent stopped\n");
        	NRF_TWI0->EVENTS_STOPPED = 0;
    	}        

	if(NRF_TWI0->EVENTS_SUSPENDED != 0)
    	{
//		printf("\revent suspended\n");
        	NRF_TWI0->EVENTS_SUSPENDED = 0;
    	}        

	if(NRF_TWI0->EVENTS_BB != 0)
    	{
//		printf("\revent bb\n");
        	NRF_TWI0->EVENTS_BB = 0;
    	}	
*/
}


void twi_init(void)
{
	nrf_gpio_cfg(TWI_SCL, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE); 
	nrf_gpio_cfg(TWI_SDA, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE); 

  	NRF_TWI0->PSELSCL	  = TWI_SCL;
	NRF_TWI0->PSELSDA 	  = TWI_SDA;
 	NRF_TWI0->EVENTS_RXDREADY = 0;
	NRF_TWI0->EVENTS_TXDSENT  = 0;
    	NRF_TWI0->FREQUENCY       = TWI_FREQUENCY_FREQUENCY_K400;
	NRF_TWI0->INTENSET	  = TWI_INTENSET_TXDSENT_Msk | TWI_INTENSET_RXDREADY_Msk | TWI_INTENSET_ERROR_Msk;// | TWI_INTENSET_SUSPENDED_Msk | TWI_INTENSET_BB_Msk | TWI_INTENSET_STOPPED_Msk;

	NRF_TWI0->SHORTS	  = 0;
	NRF_TWI0->ENABLE          = TWI_ENABLE_ENABLE_Enabled;

	NVIC_ClearPendingIRQ(SPI0_TWI0_IRQn);
	NVIC_SetPriority(SPI0_TWI0_IRQn, 3);
	NVIC_EnableIRQ(SPI0_TWI0_IRQn);
}
