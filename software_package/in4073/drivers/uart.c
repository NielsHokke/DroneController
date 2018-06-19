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
#include "drone.h"

bool txd_available = true;
QueueHandle_t ctrl_msg_queue = NULL;
QueueHandle_t para_msg_queue = NULL;

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


uint8_t  crcTable[256];

#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))

#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */

void crcInit(void){
    uint8_t  remainder;
    for (int dividend = 0; dividend < 256; ++dividend)
    {
        remainder = dividend << (WIDTH - 8);
        for (uint8_t bit = 8; bit > 0; --bit){
            if (remainder & TOPBIT){
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }else{
                remainder = (remainder << 1);
            }
        }
        crcTable[dividend] = remainder;
    }
}

uint8_t crcFast(char message[], int nBytes){
    uint8_t data;
    uint8_t remainder = 0;

    for (int byte = 0; byte < nBytes; ++byte){
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    return (remainder);
}


/*------------------------------------------------------------------
 * validate_ctrl_msg -- Validates ctrl messages by checking the CRC
 *						and adjusts the given parameters.
 * Parameters:			Void pointer to parametres (unused)
 * Author:	Niels Hokke
 * Date:	2-5-2018
 *------------------------------------------------------------------
 */
void validate_ctrl_msg(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	char ctrl_buffer[CTRL_DATA_LENGTH+2];
	taskENTER_CRITICAL();
	DEBUG_PRINT("started\n");
	taskEXIT_CRITICAL();

	for(;;){
		// Yielding till something in queue
		xQueueReceive(ctrl_msg_queue, ctrl_buffer, portMAX_DELAY);

		// TODO start critical section

		// Calculate crc
		uint8_t crc = crcFast(ctrl_buffer, CTRL_DATA_LENGTH+1);
		// Verify crc
		if(crc != ctrl_buffer[CTRL_DATA_LENGTH+1]){
			// Incorrect CRC
			DEBUG_PRINT("CTRL: incorrect CRC should be= \f");
			DEBUG_PRINTEGER(crc,3);
			DEBUG_PRINT("\n\f");
		}else{
			// Correct CRC
			
			SetPoint.yaw = ctrl_buffer[1];
			SetPoint.pitch = ctrl_buffer[2];
			SetPoint.roll = ctrl_buffer[3];
			SetPoint.lift = ctrl_buffer[4];
		}
		// TODO end critical section
	}
}


/*------------------------------------------------------------------
 * validate_para_msg -- Validates para messages by checking the CRC
 *						and adjusts the given parameters.
 * Parameters:			Void pointer to parametres (unused)
 * Author:	Niels Hokke
 * Date:	2-5-2018
 *------------------------------------------------------------------
 */
void validate_para_msg(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	char para_buffer[PARA_DATA_LENGTH+2];

	for(;;){
		// Yielding till something in queue
		xQueueReceive(para_msg_queue, para_buffer, portMAX_DELAY);

		// TODO start critical section

		// Calculate crc
		uint8_t crc = crcFast(para_buffer, PARA_DATA_LENGTH+1);

		// Verify crc
		if(crc != para_buffer[PARA_DATA_LENGTH+1]){
			// Incorrect CRC
			DEBUG_PRINT("PARAM Incorrect CRC, should be\f");
			DEBUG_PRINTEGER(crc, 3);
			DEBUG_PRINT("\n\f");
		}else{
			//dont accept parameter changes when in panic mode
			if (GLOBALSTATE == S_PANIC){
				continue;
			}

			else{
				DEBUG_PRINT("Param correct.\n\f");
				uint8_t index = (uint8_t) para_buffer[1];
				parameters[index] = para_buffer[2];
				parameters[index+1] = para_buffer[3];
				parameters[index+2] = para_buffer[4];
				parameters[index+3] = para_buffer[5];
			}
			// TODO end critical section
		}
	}
}


/*------------------------------------------------------------------
 * UartTimeoutCallback -- 	Calback function which is reset everytime
 *						  	a byte arives over uart. if for 100ms nothing
 *							is recieved, the function is executed once.
 * Parameters:			 	it's own timer handle 
 * Author:	Niels Hokke
 * Date:	16-5-2018
 *------------------------------------------------------------------
 */
void UartTimeoutCallback( TimerHandle_t xTimer ){
	//TODO dont go from save to panic
	DEBUG_PRINT("UART TIMOUT \n\f");
	GLOBALSTATE = S_PANIC;
	// DEBUG_PRINT("UART TIMOUT TRIGGERT!\n\f");
	// DEBUG_PRINT("UART TIMOUT TRIGGERT!\n\f");
	// DEBUG_PRINT("UART TIMOUT TRIGGERT!\n\f");
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
			}else if (c == 0x55){ // parameter message start byte
				byte_counter = 0; 
				para_buffer[byte_counter++] = c;
				serialstate = PARA;
			}
			//TODO reset timetout
			break;

		case CTRL:
			ctrl_buffer[byte_counter++] = c;
			if(byte_counter == CTRL_DATA_LENGTH + 2){ // data lenght + 1 CRC byte
				
				// Putting message on to para queue
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				if(xQueueOverwriteFromISR( ctrl_msg_queue, ctrl_buffer, &xHigherPriorityTaskWoken) != pdPASS){
					DEBUG_PRINT("Failed to put ctrl msg into ctrl queue\n");
				}

		        /* Writing to the queue caused a task to unblock and the unblocked task
		        has a priority higher than or equal to the priority of the currently
		        executing task (the task this interrupt interrupted).  Perform a
		        context switch so this interrupt returns directly to the unblocked
		        task. */
		        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

				serialstate = IDLE;
			}
			break;

		case PARA:
			para_buffer[byte_counter++] = c;
			if(byte_counter == PARA_DATA_LENGTH + 2){ // data lenght + 1 CRC byte

				// Putting message on to para queue
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				if(xQueueSendFromISR( para_msg_queue, para_buffer, &xHigherPriorityTaskWoken) != pdPASS){
					DEBUG_PRINT("Failed to put para msg into para queue\n");
				}

		        /* Writing to the queue caused a task to unblock and the unblocked task
		        has a priority higher than or equal to the priority of the currently
		        executing task (the task this interrupt interrupted).  Perform a
		        context switch so this interrupt returns directly to the unblocked
		        task. */
		        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

				serialstate = IDLE;
			}
			break;
		default:
			//nrf_gpio_pin_toggle(RED);
		DEBUG_PRINT("Unexpected State");
	}

	// Reset non-connectin inerupt.
	xTimerReset(UartTimeoutHandle, 0);
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
	crcInit();

	serialstate = IDLE;

	ctrl_msg_queue = xQueueCreate(1, sizeof(uint8_t)*(CTRL_DATA_LENGTH+2));
	para_msg_queue = xQueueCreate(PARA_MSG_QUEUE_SIZE, sizeof(uint8_t)*(PARA_DATA_LENGTH+2));

	UartTimeoutHandle = xTimerCreate("Uart timout checker", 1000, pdFALSE, ( void * ) 0, UartTimeoutCallback);

	nrf_gpio_cfg_output(TX_PIN_NUMBER);
	nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_NOPULL);

	NRF_UART0->PSELTXD = TX_PIN_NUMBER;
	NRF_UART0->PSELRXD = RX_PIN_NUMBER;
	NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);

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

	DEBUG_PRINT("UART intitialised\n\f");
}
