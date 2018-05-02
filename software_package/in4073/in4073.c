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

#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "sdk_errors.h"
#include "app_error.h"

#define CONTROL_PERIOD 10
#define SENSOR_LOOP 10

/*------------------------------------------------------------------
 * process_key -- process command keys
 *------------------------------------------------------------------
 */
/*
void process_key(uint8_t c)
{
	switch (c)
	{
		case 'q':
			ae[0] += 10;
			break;
		case 'a':
			ae[0] -= 10;
			if (ae[0] < 0) ae[0] = 0;
			break;
		case 'w':
			ae[1] += 10;
			break;
		case 's':
			ae[1] -= 10;
			if (ae[1] < 0) ae[1] = 0;
			break;
		case 'e':
			ae[2] += 10;
			break;
		case 'd':
			ae[2] -= 10;
			if (ae[2] < 0) ae[2] = 0;
			break;
		case 'r':
			ae[3] += 10;
			break;
		case 'f':
			ae[3] -= 10;
			if (ae[3] < 0) ae[3] = 0;
			break;
		case 27:
			demo_done = true;
			break;
		default:
			nrf_gpio_pin_toggle(RED);
	}
}
*/
/*------------------------------------------------------------------
 * main -- everything you need is here :)
 *------------------------------------------------------------------
 */

#define TASK_DELAY      200 

static void led_toggle_task_function (void * pvParameter)
{
    UNUSED_PARAMETER(pvParameter);
    while (true)
    {
        nrf_gpio_pin_toggle(BLUE);

        /* Delay a task for a given number of ticks */
        // printf("\n\t blink \n\n");
        vTaskDelay(TASK_DELAY);

        /* Tasks must be implemented to never return... */
    }
}

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

	uint32_t time;
	for(;;){
		time = xTaskGetTickCount();

		printf("adc %10ld \n",xTaskGetTickCount() - time);

		time = xTaskGetTickCount();
		read_baro();
		printf("baro %10ld \n",xTaskGetTickCount() - time);


		
		if (check_sensor_int_flag()) 
		{
			time = xTaskGetTickCount();
			get_dmp_data();
			run_filters_and_control();
			printf("measure shit %10ld \n",xTaskGetTickCount() - time);

			printf("%10ld | ", get_time_us());
			printf("%3d %3d %3d %3d | ",ae[0],ae[1],ae[2],ae[3]);
			printf("%6d %6d %6d | ", phi, theta, psi);
			printf("%6d %6d %6d | ", sp, sq, sr);
			printf("%4d | %4ld | %6ld \n", bat_volt, temperature, pressure);
		}
		


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
		vTaskDelay(999);

	}
}




int main(void)
{
	uart_init();
	gpio_init();
	// timers_init();
	adc_init();
	twi_init();
	imu_init(true, 100);	
	baro_init();
	spi_flash_init();
	// ble_init();

	

	/* Create task for LED0 blinking with priority set to 2 */
    // UNUSED_VARIABLE(xTaskCreate(led_toggle_task_function, "LED", 128, NULL, 2, NULL));
    //UNUSED_VARIABLE(xTaskCreate(control_loop, "control loop", 128, NULL, 5, NULL));
	UNUSED_VARIABLE(xTaskCreate(sensor_loop, "Sensor loop", 128, NULL, 4, NULL));
	UNUSED_VARIABLE(xTaskCreate(check_battery_voltage, "Battery check", 128, NULL, 1, NULL));

    /* Activate deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

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
