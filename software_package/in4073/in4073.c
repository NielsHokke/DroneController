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
#include "drone.h"
#include "in4073.h"


#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"

#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "sdk_errors.h"
#include "app_error.h"

#define CONTROL_PERIOD 10
#define SENSOR_LOOP 1

enum state {CALIBRATION, SAFE, PANIC, MANUAL, YAW_CONTROL, FULL_CONTROLL, RAW_MODE_1, RAW_MODE_2, RAW_MODE_3} GlobalState;






/*------------------------------------------------------------------
 * main -- everything you need is here :)
 *------------------------------------------------------------------
 */


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
	int seconds = 0;
	for(;;){
		xLastWakeTime = xTaskGetTickCount();
		if ((i++ % 100) == 0){
			seconds++;
			DEBUG_PRINT("we zitten nu op de tijd sdfjlasdfjkasldfj asdklfj lasdkfjhlkasdf%d\n", seconds);	
			
		}
		switch(GlobalState){
			case SAFE:
				motors_off();
				break;
			default:
				break;
		};
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

	const TickType_t xFrequency = SENSOR_LOOP; //period of task 

	for(;;){
		xLastWakeTime = xTaskGetTickCount();
		nrf_gpio_pin_toggle(RED);
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		
	}

}

/*--------------------------------------------------------------------------------------
 * vCheck_battery_voltage:    Checks the battery voltage and triggers panic mode if low
 * Parameters: 	pointer to function parameters
 * Return:		void
 * Author:		Jetse Brouwer
 * Date:    	2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void check_battery_voltage(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	for(;;){

		nrf_gpio_pin_toggle(BLUE);

		adc_request_sample();
		vTaskDelay(1);
		if (bat_volt > 123){
			//TODO: goto panic mode	
		}
		DEBUG_PRINT("%d, %d, %d, %d\n", parameters[0], parameters[1], parameters[2], parameters[P_MODE]);
		// DEBUG_PRINT("Battery check met extra veel tijd to improve the likelhood of collision\n");
		vTaskDelay(499);

	}
}




int main(void)
{

	GlobalState = SAFE;

	uart_init();
	gpio_init();
	timers_init();
	adc_init();
	twi_init();
	imu_init(true, 100);	
	baro_init();
	spi_flash_init();
	// ble_init();

	DEBUG_PRINT("Peripherals initialized\n");
	


    // UNUSED_VARIABLE(xTaskCreate(control_loop, "control loop", 128, NULL, 3, NULL));
	// UNUSED_VARIABLE(xTaskCreate(sensor_loop, "Sensor loop", 128, NULL, 2, NULL));
	UNUSED_VARIABLE(xTaskCreate(check_battery_voltage, "Battery check", 128, NULL, 1, NULL));
	DEBUG_PRINT("Tasks registered\n");

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
