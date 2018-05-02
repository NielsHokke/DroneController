/*------------------------------------------------------------------
 *  uart.c -- configures uart
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "in4073.h"

bool txd_available = true;

void uart_put(uint8_t byte)
{
	NVIC_DisableIRQ(UART0_IRQn);

	if (txd_available) {txd_available = false; NRF_UART0->TXD = byte;}
	else enqueue(&tx_queue, byte);

	NVIC_EnableIRQ(UART0_IRQn);
}

// Reroute printf
int _write(int file, const char * p_char, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		uart_put(*p_char++);
	}
	return len;
}

/*------------------------------------------------------------------
 * handle_serial_rx -- 	Handles incoming Serial bytes via a state
 *						Machine. scheduls 'CTRL Validation' and 
 *						'PARA Validation' tasks
 * Parameters:			char c newest recieved input char
 * Author:	Niels Hokke
 * Date:	2-5-2018
 *------------------------------------------------------------------
 */
void handle_serial_rx(char c){

	static char ctrl_buffer[CTRL_DATA_LENGTH+2];
	static char para_buffer[PARA_DATA_LENGTH+2];
	static uint8_t byte_counter;
		
	switch(serialstate){
		case IDLE:
			if(c == 0xAA){ // control message start byte
				byte_counter = 0;
				ctrl_buffer[byte_counter++] = c;
				serialstate = CTRL;
			}else if (c == 0x55){// parameter message start byte
				byte_counter = 0; 
				para_buffer[byte_counter++] = c;
				serialstate = PARA;
			}

			DEBUG_PRINT("recieved: dec %d\n", c);
			//TODO reset timetout
			break;

		case CTRL:
			ctrl_buffer[byte_counter++] = c;
			if(byte_counter == CTRL_DATA_LENGTH + 2){ // data lenght + 1 CRC byte
				//TODO schedule CTRL Validation task
				
				serialstate = IDLE;
				DEBUG_PRINT("CTRL message recieved:\n");
				DEBUG_PRINT("strt_byte: %d\n", ctrl_buffer[0]);
				DEBUG_PRINT("yaw: %d\n", ctrl_buffer[1]);
				DEBUG_PRINT("pitch:%d\n", ctrl_buffer[2]);
				DEBUG_PRINT("roll: %d\n", ctrl_buffer[3]);
				DEBUG_PRINT("lift: %d\n", ctrl_buffer[4]);
				DEBUG_PRINT("CRC: %d\n", ctrl_buffer[5]);
			}
			break;

		case PARA:
			para_buffer[byte_counter++] = c;
			if(byte_counter == PARA_DATA_LENGTH + 2){ // data lenght + 1 CRC byte
				//TODO schedule PARA Validation task

				serialstate = IDLE;
				DEBUG_PRINT("PARA message recieved\n");
				DEBUG_PRINT("strt_byte: %d\n", para_buffer[0]);
				DEBUG_PRINT("Reg.addres: %d\n", para_buffer[1]);
				DEBUG_PRINT("data%d\n", para_buffer[2]);
				DEBUG_PRINT("data: %d\n", para_buffer[3]);
				DEBUG_PRINT("data: %d\n", para_buffer[4]);
				DEBUG_PRINT("data: %d\n", para_buffer[5]);
				DEBUG_PRINT("CRC: %d\n", para_buffer[6]);
			}
			break;
		default:
			nrf_gpio_pin_toggle(RED);
	}
}


void UART0_IRQHandler(void)
{
	if (NRF_UART0->EVENTS_RXDRDY != 0)
	{
		NRF_UART0->EVENTS_RXDRDY  = 0;

		handle_serial_rx(NRF_UART0->RXD);

	}

	if (NRF_UART0->EVENTS_TXDRDY != 0)
	{
		NRF_UART0->EVENTS_TXDRDY = 0;
		if (tx_queue.count) NRF_UART0->TXD = dequeue(&tx_queue);
		else txd_available = true;
	}

	if (NRF_UART0->EVENTS_ERROR != 0)
	{
		NRF_UART0->EVENTS_ERROR = 0;
		printf("uart error: %lu\n", NRF_UART0->ERRORSRC);
	}
}

void uart_init(void)
{
	init_queue(&rx_queue); // Initialize receive queue
	init_queue(&tx_queue); // Initialize transmit queue

	serialstate = IDLE;

	nrf_gpio_cfg_output(TX_PIN_NUMBER);
	nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_NOPULL); 
	NRF_UART0->PSELTXD = TX_PIN_NUMBER;
	NRF_UART0->PSELRXD = RX_PIN_NUMBER;
	NRF_UART0->BAUDRATE        = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);

	NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->EVENTS_RXDRDY    = 0;
	NRF_UART0->EVENTS_TXDRDY    = 0;
	NRF_UART0->TASKS_STARTTX    = 1;
	NRF_UART0->TASKS_STARTRX    = 1;

	NRF_UART0->INTENCLR = 0xffffffffUL;
	NRF_UART0->INTENSET = 	(UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos) |
                          	(UART_INTENSET_TXDRDY_Set << UART_INTENSET_TXDRDY_Pos) |
                          	(UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

	NVIC_ClearPendingIRQ(UART0_IRQn);
	NVIC_SetPriority(UART0_IRQn, 3); // either 1 or 3, 3 being low. (sd present)
	NVIC_EnableIRQ(UART0_IRQn);
}
