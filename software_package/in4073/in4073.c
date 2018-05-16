/*------------------------------------------------------------------
 *  in4073.c -- test QR engines and sensors
 *
 *  reads ae[0-3] uart rx queue
 *  (q,w,e,r increment, a,s,d,f decrement)
 *
 *  prints timestamp, ae[0-3], sensors to uart tx queue
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  June 2016
 *------------------------------------------------------------------
 */


#include "in4073.h"
#include "drone.h"

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"

#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "sdk_errors.h"
#include "app_error.h"



#define CONTROL_PERIOD 10
#define SENSOR_PERIOD 1



/*--------------------------------------------------------------------------------------
 * control_loop: task containing the control loop of the quad-copter
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void control_loop(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = CONTROL_PERIOD; //period of task
	int i = 0;

	for(;;){
		xLastWakeTime = xTaskGetTickCount();

		if ((i++ % 100) == 0){
			nrf_gpio_pin_toggle(GREEN);
		}
		switch(GlobalState){
			case SAFE:
				ae[0] = 0;
				ae[1] = 0;
				ae[2] = 0;
				ae[3] = 0;
				break;
			case MANUAL:
				manual_control();
				break;
			default:
				break;
		};
		update_motors();
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}	
}

/*--------------------------------------------------------------------------------------
 * sensor_loop: task to read out and perform filtering on the acllerco +gyro data
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void sensor_loop(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	TickType_t xLastWakeTime;

	const TickType_t xFrequency = SENSOR_PERIOD; //period of task 
	int i = 0;

	for(;;){
		xLastWakeTime = xTaskGetTickCount();	
		if ((i++ % 5000) == 0){
			nrf_gpio_pin_toggle(BLUE);
		}
		vTaskDelayUntil( &xLastWakeTime, xFrequency );	
	}
}



/*--------------------------------------------------------------------------------------
 * vCheck_battery_voltage:    Checks the battery voltage and triggers panic mode if low
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void check_battery_voltage(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	for(;;){

		nrf_gpio_pin_toggle(YELLOW);

		adc_request_sample();
		vTaskDelay(1);
		if (bat_volt > 123){
			//TODO: goto panic mode	
		}
		DEBUG_PRINTEGER((uint) uxTaskGetStackHighWaterMark(NULL));
		DEBUG_PRINT(": battcheck \n\f");
		vTaskDelay(1203);

	}
}

void print_heil_hendrik(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	//char henk[] = 
	for(;;){
		DEBUG_PRINTEGER((uint) uxTaskGetStackHighWaterMark(NULL));
		DEBUG_PRINT(": hendrik\n\f");
		vTaskDelay(900);
	}
}


/*------------------------------------------------------------------
 * main -- everything you need is here :)
 *------------------------------------------------------------------
 */

int main(void)
{

	GlobalState = SAFE;

	SetPoint.pitch = 0;
	SetPoint.yaw = 0;
	SetPoint.roll = 0;
	SetPoint.lift = 0;

	uart_init();
	gpio_init();
	timers_init();
	adc_init();
	twi_init();
	imu_init(true, 100);	
	baro_init();
	spi_flash_init();
	// ble_init();

	print("Peripherals initialized\n\f");
	
	UNUSED_VARIABLE(xTaskCreate(print_heil_hendrik, "Validate and execute ctrl message", configMINIMAL_STACK_SIZE, NULL, 3, NULL));
	UNUSED_VARIABLE(xTaskCreate(validate_para_msg, "Validate and execute para message", configMINIMAL_STACK_SIZE  + 10, NULL, 2, NULL));

    UNUSED_VARIABLE(xTaskCreate(control_loop, "control loop", configMINIMAL_STACK_SIZE, NULL, 1, NULL));
	UNUSED_VARIABLE(xTaskCreate(sensor_loop, "Sensor loop", configMINIMAL_STACK_SIZE, NULL, 1, NULL));
	UNUSED_VARIABLE(xTaskCreate(check_battery_voltage, "Battery check", configMINIMAL_STACK_SIZE, NULL, 1, NULL));
	print("Tasks registered\n\f");

    /* Activate deep sleep mode */
    // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* Start FreeRTOS scheduler. */
    //UNUSED_VARIABLE(xTimerCreateTimerTask);
    vTaskStartScheduler();

    while (true)
    {
        /* FreeRTOS should not be here... FreeRTOS goes back to the start of stack
         * in vTaskStartScheduler function. */
    }
	NVIC_SystemReset();
}

/* Functions that will be used by freertos if eneabled in FreeRTOSCONFIG.h   */	

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) xTask;
    int i = 0;
    print("someone caused a stack overflow\f\n");
    for( ;; ){
	    while(i++ < 2000000){};
	    i = 0;
	    nrf_gpio_pin_toggle(RED);
    }
}

void vApplicationIdleHook( void )
{
	uint16_t i = 0;
    for(;;){
	    while(i++ < 65535){};
	    printeger(xPortGetFreeHeapSize());
		print(": free heapsize \n\f");
		i=0;
	}
}


