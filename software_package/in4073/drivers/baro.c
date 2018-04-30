/*------------------------------------------------------------------
 *  baro.c -- Read temp, pressure and convert
 *
 *  I. Protonotarios - mods by Sujay 
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include <math.h>
#include "in4073.h"

#define CONVERT_D1_1024 0x44
#define CONVERT_D1_4096 0x48
#define CONVERT_D2_1024 0x54
#define CONVERT_D2_4096 0x58
#define MS5611_ADDR	0b01110111
#define READ		0x0
#define PROM		0xA0

static uint16_t prom[8] = {0};
static uint8_t loop_count = 0;
uint32_t D1, D2;	
static uint8_t data[3] = {0};
uint32_t initTime = 0;

void read_baro(void)
{
	switch(loop_count)
	{
		case 0:
			NRF_TWI0->ADDRESS = MS5611_ADDR;
			NRF_TWI0->SHORTS = TWI_SHORTS_BB_STOP_Msk;
			NRF_TWI0->TXD = CONVERT_D1_4096;
			NRF_TWI0->TASKS_STARTTX = 1;
	
			loop_count = 1;
			initTime = get_time_us();
			break;
    
		case 1:
			if( (get_time_us() - initTime) < 10000) break;
			i2c_read(MS5611_ADDR, READ, 3, data);
			D1 = (uint32_t) ((data[0] << 16)|(data[1] << 8)|data[2]);
	
			NRF_TWI0->ADDRESS = MS5611_ADDR;
			NRF_TWI0->SHORTS = TWI_SHORTS_BB_STOP_Msk;
			NRF_TWI0->TXD = CONVERT_D2_4096;
			NRF_TWI0->TASKS_STARTTX = 1;
	
			loop_count = 2;
			initTime = get_time_us();
			break;
	
		case 2:
			if( (get_time_us() - initTime) < 10000) break;
			i2c_read(MS5611_ADDR, READ, 3, data);
			D2 = (uint32_t) ((data[0] << 16)|(data[1] << 8)|data[2]);

			long long dT, OFFSET, SENS;
	
			dT = D2 - (prom[5]<<8);    // calculate temperature difference from reference
		
			OFFSET = ((long long)prom[2]<<16) + ((dT*prom[4])>>7);
		
  	  		SENS = ((long long)prom[1]<<15) + ((dT*prom[3])>>8);

			temperature = 2000 + ((dT*prom[6])>>23);           // First-order Temperature in degrees Centigrade
			pressure = (((D1*SENS)>>21) - OFFSET)>>15;  // Pressure in mbar or kPa
		
			loop_count = 0;
			break;
	}
	nrf_delay_us(50);
}

void baro_init(void)
{	
	static uint8_t data[2] = {0};
	
	for (uint8_t c=0;c<8;c++)
	{
		i2c_read(MS5611_ADDR, PROM+2*c, 2, data);
		prom[c] = (uint16_t)((data[0] << 8) | data[1]); 
	}

}


