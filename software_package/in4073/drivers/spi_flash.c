/*------------------------------------------------------------------
 *  spi_flash.c -- configures spi flash chip
 *
 *  Sujay Narayana
 *  Embedded Software Lab
 *
 *  August 2016
 *------------------------------------------------------------------
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "in4073.h"

#define SPI_CS		17
#define SPI_MISO	18
#define SPI_WP		0
#define SPI_HOLD	13
#define SPI_SCK		11
#define SPI_MOSI	9

#define WRSR            0x01
#define BYTEWRITE       0x02
#define BYTEREAD        0x03
#define WRDI            0x04
#define RDSR            0x05
#define WREN            0x06
#define EWSR            0x50
#define CHIP_ERASE      0x60
#define AAI             0xAF 

#define SPI_FREQ_4MBPS        0x40
#define SPI_MODULE            0x01
#define SPI_BITORDER_MSB_LSB  0x00
#define SPI_MODE              0x00

typedef struct
{
    union
    {
        uint8_t SPI_cfg;            /*!< Bit mode and bit order merged, as in the SPI CONFIG register */
        struct
        {
            uint8_t bit_order : 1;  /*!< SPI master bit order */
            uint8_t mode : 2;       /*!< SPI master mode */
            uint8_t : 5;            /*!< Padding */
        }fields;
    }config;    
    uint8_t frequency;              /*!< SPI master frequency */
    uint8_t pin_SCK;                /*!< SPI master SCK pin */
    uint8_t pin_MOSI;               /*!< SPI master MOSI pin */
    uint8_t pin_MISO;               /*!< SPI master MISO pin */
    uint8_t pin_CSN;                /*!< SPI master chip select pin */
} SPI_config_t;

static SPI_config_t spi_config_table[2];
static NRF_SPI_Type *spi_base[2] = {NRF_SPI0, NRF_SPI1};
static NRF_SPI_Type *SPI;

uint32_t* spi_master_init(uint8_t spi_num, SPI_config_t *spi_config)
{
    if(spi_num > 1)
    {
        return 0;
    }
    memcpy(&spi_config_table[spi_num], spi_config, sizeof(SPI_config_t));

    /* Configure GPIO pins used for pselsck, pselmosi, pselmiso and pselss for SPI0 */
    nrf_gpio_cfg_output(spi_config->pin_SCK);
    nrf_gpio_cfg_output(spi_config->pin_MOSI);
    nrf_gpio_cfg_input(spi_config->pin_MISO, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_output(spi_config->pin_CSN);

    /* Configure pins, frequency and mode */
    spi_base[spi_num]->PSELSCK  = spi_config->pin_SCK;
    spi_base[spi_num]->PSELMOSI = spi_config->pin_MOSI;
    spi_base[spi_num]->PSELMISO = spi_config->pin_MISO;
    nrf_gpio_pin_set(spi_config->pin_CSN); /* disable Set slave select (inactive high) */

    spi_base[spi_num]->FREQUENCY = (uint32_t)spi_config->frequency << 24;

    spi_base[spi_num]->CONFIG = spi_config->config.SPI_cfg;

    spi_base[spi_num]->EVENTS_READY = 0;
    /* Enable */
    spi_base[spi_num]->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);

    return (uint32_t *)spi_base[spi_num];
}

bool spi_master_tx_rx(uint8_t spi_num, uint16_t transfer_size, const uint8_t *tx_data, uint8_t *rx_data)
{
    volatile uint32_t *SPI_DATA_READY;
    uint32_t tmp; 
    if(tx_data == 0 || rx_data == 0)
    {
        return false;
    }
    if(transfer_size < 1)
	{
		return false;
	}
    SPI = spi_base[spi_num];
    SPI_DATA_READY = &SPI->EVENTS_READY;
    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);
    
    *SPI_DATA_READY = 0; 
    
    SPI->TXD = (uint32_t)*tx_data++;
    tmp = (uint32_t)*tx_data++;
    while(--transfer_size)
    {
        SPI->TXD =  tmp;
        tmp = (uint32_t)*tx_data++;
        
        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while (*SPI_DATA_READY == 0);

        /* clear the event to be ready to receive next messages */
        *SPI_DATA_READY = 0;
        
        *rx_data++ = SPI->RXD; 
    }
      
    /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
    while (*SPI_DATA_READY == 0);

    *rx_data = SPI->RXD;

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);

    return true;
}

bool spi_master_tx(uint8_t spi_num, uint16_t transfer_size, const uint8_t *tx_data)
{
    volatile uint32_t dummyread;
    
    if(tx_data == 0)
    {
        return false;
    }
    if(transfer_size < 1)
	{
		return false;
	}
	
    SPI = spi_base[spi_num];
    
    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);
    
    SPI->EVENTS_READY = 0; 
    
    SPI->TXD = (uint32_t)*tx_data++;
    
    while(--transfer_size)
    {
        SPI->TXD =  (uint32_t)*tx_data++;

        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while (SPI->EVENTS_READY == 0);
        
        /* clear the event to be ready to receive next messages */
        SPI->EVENTS_READY = 0;
        
        dummyread = SPI->RXD;
    }
    
    /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
    while (SPI->EVENTS_READY == 0);

    dummyread = SPI->RXD;

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);
    
    dummyread++;
    return true;
}

bool spi_master_rx(uint8_t spi_num, uint16_t transfer_size, uint8_t *rx_data)
{
    if(rx_data == 0)
    {
        return false;
    }
    if(transfer_size < 1)
	{
		return false;
	}
    SPI = spi_base[spi_num];
    
    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);

    SPI->EVENTS_READY = 0; 
    
    SPI->TXD = 0;
    
    while(--transfer_size)
    {
        SPI->TXD = 0;

        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while (SPI->EVENTS_READY == 0);
        
        /* clear the event to be ready to receive next messages */
        SPI->EVENTS_READY = 0;
        
        *rx_data++ = SPI->RXD;
    }
    
    /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
    while (SPI->EVENTS_READY == 0);

    *rx_data = SPI->RXD;

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);

    return true;
}

bool spi_master_tx_rx_fast_read(uint8_t spi_num, uint16_t transfer_size, const uint8_t *tx_data, uint8_t *rx_data)
{
    volatile uint32_t dummyread;
	
    if(tx_data == 0 || rx_data == 0)
    {
        return false;
    }
    if(transfer_size < 1)
	{
		return false;
	}
    SPI = spi_base[spi_num];
    
    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);

    SPI->EVENTS_READY = 0; 

	for(int i=0; i<4; i++)
	{
		SPI->TXD = (uint32_t)*tx_data++;
		while (SPI->EVENTS_READY == 0);		
		SPI->EVENTS_READY = 0;
		dummyread = SPI->RXD;
    }

    while(transfer_size--)
    {
        SPI->TXD =  0x00;

        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while (SPI->EVENTS_READY == 0);

        /* clear the event to be ready to receive next messages */
        SPI->EVENTS_READY = 0;
        *rx_data++ = SPI->RXD; 
    }

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);
    
	dummyread++;
    return true;
}

bool spi_master_tx_rx_fast_write(uint8_t spi_num, uint16_t transfer_size, const uint8_t *tx_data, uint8_t *bytes)
{
	volatile uint32_t dummyread;
	uint32_t bytesWritten = 0;
	uint32_t address = tx_data[3] + (tx_data[2]<<8) + (tx_data[1]<<16);
	
	if(tx_data == 0 || bytes == 0)
		{
		return false;
	}
	if(transfer_size < 1)
	{
		return false;
	}
	SPI = spi_base[spi_num];
    
    	/* enable slave (slave select active low) */
    	nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);

    	SPI->EVENTS_READY = 0; 

	for(int i=0; i<4; i++)
	{
		SPI->TXD = (uint32_t)*tx_data++;
		while (SPI->EVENTS_READY == 0);		
		SPI->EVENTS_READY = 0;
		dummyread = SPI->RXD;
	}

	//Send first byte
	SPI->TXD = (uint32_t)*bytes++;
	while (SPI->EVENTS_READY == 0);
	SPI->EVENTS_READY = 0;
	dummyread = SPI->RXD;
	
	/* disable slave (slave select active low) */
	nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);

	while(--transfer_size)
    	{
		nrf_delay_us(15); /////////////////////////
		
		/* enable slave (slave select active low) */
        	nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);
		
		//Send AAI
		SPI->TXD = (uint32_t)AAI;

        	/* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        	while (SPI->EVENTS_READY == 0);

        	/* clear the event to be ready to receive next messages */
        	SPI->EVENTS_READY = 0;
        	dummyread = SPI->RXD; 
	
        	SPI->TXD = (uint32_t)*bytes++;

        	/* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        	while (SPI->EVENTS_READY == 0);

        	/* clear the event to be ready to receive next messages */
        	SPI->EVENTS_READY = 0;
        	dummyread = SPI->RXD; 
		
		bytesWritten++;
		/* disable slave (slave select active low) */
        	nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);
		if(address + bytesWritten >= 0x1FFFF)
		{
			if(transfer_size)
			{
				return false;
			}
		}
    	}

	nrf_delay_us(20);

	/* enable slave (slave select active low) */
	nrf_gpio_pin_clear(spi_config_table[spi_num].pin_CSN);
	
	//Send WRDI
	SPI->TXD = (uint32_t)WRDI;

    	/* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
    	while (SPI->EVENTS_READY == 0);

    	/* clear the event to be ready to receive next messages */
    	SPI->EVENTS_READY = 0;
    	dummyread = SPI->RXD; 
		
    	/* disable slave (slave select active low) */
    	nrf_gpio_pin_set(spi_config_table[spi_num].pin_CSN);
    
	dummyread++;
    	return true;
}

/**
 * Write-Enable(WREN).
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_write_enable(void)
{
	uint8_t tx_data = WREN;
	return spi_master_tx(SPI_MODULE, 1, &tx_data);
}

/**
 * Write-Disable(WRDI).
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_write_disable(void)
{
	uint8_t tx_data = WRDI;
	return spi_master_tx(SPI_MODULE, 1, &tx_data);
}

/**
 * Clears all memory locations by setting value to 0xFF.
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_chip_erase(void)
{
	uint8_t tx_data = CHIP_ERASE;
	if(!flash_write_enable())
	{
		return false;
	}
	bool result = spi_master_tx(SPI_MODULE, 1, &tx_data);
	nrf_delay_ms(100);
	return result;
}

/**
 * Read Read-Status-Register (RDSR).
 *
 * @param data pointer to a uint8_t type variable where register status is stored.
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_read_status(uint8_t *data)
{
	uint8_t tx_data[2] = {RDSR,0x00};
	uint8_t rx_data[2];
	
	bool result = spi_master_tx_rx(SPI_MODULE, 2, tx_data, rx_data);
	*data = rx_data[1];
	return result; 
}

/**
 * Enable-Write-Status-Register (EWSR). This function must be followed by flash_enable_WSR().
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_enable_WSR(void)
{
	uint8_t tx_data = EWSR;
	return spi_master_tx(SPI_MODULE, 1, &tx_data);
}

/**
 * Sets Write-Status-Register (WRSR) to 0x00 to enable memory write.
 *
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_set_WRSR(void)
{
	uint8_t tx_data[2] = {WRSR,0x00};	
	return spi_master_tx(SPI_MODULE, 2, tx_data);
}

/**
 * Writes one byte data to specified address.
 *
 * @note Make sure that the memory location is cleared before writing data. If data is already present 
 *       in the memory location (given address), new data cannot be written to that memory location unless 
 *   	 flash_chip_erase() function is called.
 *
 * @param address any address between 0x000000 to 0x01FFFF where the data should be stored.
 * @param data one byte data to be stored.
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_write_byte(uint32_t address, uint8_t data)
{
	uint8_t tx_data[5] = {BYTEWRITE,(address & 0xFFFFFF) >> 16,(address & 0xFFFF)>> 8,address & 0xFF,data};
	if(!flash_write_enable())
	{
		return false;
	}
	bool result = spi_master_tx(SPI_MODULE, 5, tx_data);
	nrf_delay_us(20);
	return result;
}

/**
 * Writes multi-byte data into memory starting from specified address. Each memory location (address) 
 * holds one byte of data.
 *
 * @note Make sure that the memory location is cleared before writing data. If data is already present 
 *       in the memory location (given address), new data cannot be written to that memory location unless 
 *   	 flash_chip_erase() function is called.
 * 
 * @param address starting address (between 0x000000 to 0x01FFFF) from which the data should be stored.
 * @param data pointer to uint8_t type array containing data.
 * @param count number of bytes to be stored.
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_write_bytes(uint32_t address, uint8_t *data, uint32_t count)
{
	if(!flash_write_enable())
	{
		return false;
	}
	uint8_t tx_data[4] = {AAI,(address & 0xFFFFFF) >> 16,(address & 0xFFFF)>> 8,address & 0xFF};
	return spi_master_tx_rx_fast_write(SPI_MODULE, count, tx_data, data);
}

/**
 * Reads one byte data from specified address.
 *
 * @param address any address between 0x000000 to 0x01FFFF from where the data should be read.
 *                The address is incremented automatically and once the data is written to last accessible 
 *                address - 0x01FFFF, the function returns immediately with failure if there is pending data to write.
 * @param buffer pointer to uint8_t type variable where the read data is saved.
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_read_byte(uint32_t address, uint8_t *buffer)
{
	uint8_t tx_data[5] = {BYTEREAD,(address & 0xFFFFFF) >> 16,(address & 0xFFFF)>> 8,address & 0xFF,0x00};
	uint8_t rx_data[5];
	bool result = spi_master_tx_rx(SPI_MODULE, 5, tx_data, rx_data);
    *buffer = rx_data[4];
	return result;
}

/**
 * Reads multi-byte data starting from specified address.
 *
 * @param address starting address (between 0x000000 to 0x01FFFF) from which the data is read.
 *                The address is incremented automatically and once the data from address 0x01FFFF 
 *                is read, the next location will be 0x000000.
 * @param buffer pointer to uint8_t type array where data is stored.
 * @param count number of bytes to be read.
 * @return
 * @retval true if operation is successful.
 * @retval false if operation is failed.
 */
bool flash_read_bytes(uint32_t address, uint8_t *buffer, uint32_t count)
{
	uint8_t tx_data[4] = {BYTEREAD,(address & 0xFFFFFF) >> 16,(address & 0xFFFF)>> 8,address & 0xFF};
	return spi_master_tx_rx_fast_read(SPI_MODULE, count, tx_data, buffer);
}

/**
 * Wrapper for spi_master_init(); Use this function instead of spi_master_init();
 *
 * @return
 * @retval true if initialization is successful.
 * @retval false if initialization is failed.
 */
bool spi_flash_init(void)
{
//	uint8_t data = 0xFF;
    // Set up the SPI configuration parameters
    SPI_config_t spi_config =  {.pin_SCK                 = SPI_SCK, 
                                .pin_MOSI                = SPI_MOSI, 
                                .pin_MISO                = SPI_MISO, 
                                .pin_CSN                 = SPI_CS,  
                                .frequency               = SPI_FREQ_4MBPS, 
                                .config.fields.mode      = SPI_MODE, 
                                .config.fields.bit_order = SPI_BITORDER_MSB_LSB};
    
    // Initialize the SPI
    spi_master_init(SPI_MODULE, &spi_config);
	
	nrf_gpio_cfg_output(SPI_WP);
	nrf_gpio_cfg_output(SPI_HOLD);
	nrf_gpio_pin_set(SPI_WP);
	nrf_gpio_pin_set(SPI_HOLD);

	if(!flash_enable_WSR())
	{
		return false;
	}
	if(!flash_set_WRSR())
	{
		return false;
	}
//	if(!flash_read_status(&data) || data!=0x00)
//	{
//		return false;
//	}
	if(!flash_chip_erase())
	{
		return false;
	}
	if(!flash_write_enable())
	{
		return false;
	}
    return true;
}
