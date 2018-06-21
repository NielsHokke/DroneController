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

#define CONTROL_PERIOD 9
#define SENSOR_PERIOD 1


/*--------------------------------------------------------------------------------------
 * control_loop: task containing the control loop of the quad-copter
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:   Niels Hokke
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void control_loop(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = CONTROL_PERIOD; //period of task
	int i = 0;

	#ifdef TIMETRACE
		uint32_t start = 0;
	#endif

	bool printing = false;

	gpio_init();
	timers_init();
	twi_init();


	imu_init(true, 100);	
	//imu_init(false, 1000);

	baro_init();
	spi_flash_init();
	// ble_init();

	for(;;){
		xLastWakeTime = xTaskGetTickCount();

		#ifdef TIMETRACE	
			start = get_time_us();
		#endif

		if ((i++ % 100) == 0){
			printing = true;
		}else{
			printing = false;
		}

		get_dmp_data(); // If we don;t continously read the fifo after initializing it's throwing errors adn exceptions all over the place
		
		nrf_gpio_pin_set(RED);
		nrf_gpio_pin_set(GREEN);
		switch(GLOBALSTATE){
			case S_SAFE:
				nrf_gpio_pin_clear(GREEN);
				if(printing) DEBUG_PRINT("S_SAFE\n\f");
				ae[0] = 0;
				ae[1] = 0;
				ae[2] = 0;
				ae[3] = 0;
				break;

			case S_PANIC :
				nrf_gpio_pin_clear(RED);
				if(printing) DEBUG_PRINT("S_PANIC\n\f");
				panic();
				break;

			case S_MANUAL:
				manual_control();
				if(printing) DEBUG_PRINT("S_MANUAL\n\f");
				break;

			case S_CALIBRATION :
				if(printing) DEBUG_PRINT("S_CALIBRATION\n\f");
				calibrate(false);
				break;

			case S_YAW_CONTROL :
				dmp_control(true); // dmp_control with yaw mod only set to true
				if(printing) DEBUG_PRINT("S_YAW_CONTROL\n\f");
				break;

			case S_FULL_CONTROLL :
				dmp_control(false); //dmp_control with yaw only set to false
				if(printing) DEBUG_PRINT("S_FULL_CONTROLL\n\f");
				break;

			case S_RAW_MODE :
				if(printing) DEBUG_PRINT("S_RAW_MODE\n\f");
				dmp_control(false); //dmp_control with yaw only set to false
				break;

			case S_HEIGHT_CTRL :
				if(printing) DEBUG_PRINT("S_HEIGHT_CTRL\n\f");
				// TODO implement mode
				break;

			case S_WIRELESS :
				if(printing) DEBUG_PRINT("S_WIRELESS\n\f");
				// TODO implement mode

				break;
			case S_SYSTEM_RESET :
				if(printing) DEBUG_PRINT("IN SYSTEM RESET MODE\n\f");
				NVIC_SystemReset();
				break;

			default:
				GLOBALSTATE = S_PANIC;
				break;
		};
		update_motors();

		#ifdef TIMETRACE
			if(printing) DEBUG_PRINT("\n\f");
			if(printing) DEBUG_UPRINTEGER(get_time_us()- start, 6);
			if(printing) DEBUG_PRINT(" us.\n\f");
		#endif
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}	
}


/*--------------------------------------------------------------------------------------
 * sensor_loop: task to read out and perform filtering on raw data
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

void sensor_loop(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);
	TickType_t xLastWakeTime;

	const TickType_t xFrequency = SENSOR_PERIOD; //period of task 

	for(;;){
		xLastWakeTime = xTaskGetTickCount();	
		get_raw_sensor_data();
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
	int16_t i = 0;

	for(;;){

		if ((i % 10) == 0){
			nrf_gpio_pin_toggle(BLUE);

			adc_request_sample();
			vTaskDelay(1);
		}

		if (bat_volt < 1080){ // minimum = 10.8/0.007058824				
			/* ifdef to disable bat. check for working with board only */
			#ifdef BATTERY_CHECK_ACTIVE 
			DEBUG_PRINT("VOLTAGE TO LOW GOING TO PANIC MODE\n\f");
			GLOBALSTATE = S_PANIC;
			#endif 
		}

		downLink(GLOBALSTATE, motor[0], motor[1], motor[2], motor[3], theta, phi, psi, bat_volt, get_time_us());

		i++;
		vTaskDelay(99);
	}
}


/*------------------------------------------------------------------
 * main -- everything you need is here :)
 *------------------------------------------------------------------
 */

int main(void)
{

	GLOBALSTATE = S_SAFE; //manual mode 0 = safe

	SetPoint.pitch = 0;
	SetPoint.yaw = 0;
	SetPoint.roll = 0;
	SetPoint.lift = 0;

	parameters[P_MIN_LIFT] = 100;
	parameters[P_MAX_RPM] = 175; // Value gets scaled by 4, 137 = 548


	uart_init();
	adc_init();



	print("Peripherals initialized\n\f");
	
	UNUSED_VARIABLE(xTaskCreate(validate_ctrl_msg, "Validate and execute ctrl message", configMINIMAL_STACK_SIZE, NULL, 2, NULL));
	UNUSED_VARIABLE(xTaskCreate(validate_para_msg, "Validate and execute para message", configMINIMAL_STACK_SIZE, NULL, 2, NULL));

    UNUSED_VARIABLE(xTaskCreate(control_loop, "control loop", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL));
	//UNUSED_VARIABLE(xTaskCreate(sensor_loop, "Sensor loop", configMINIMAL_STACK_SIZE, NULL, 1, NULL));
	UNUSED_VARIABLE(xTaskCreate(check_battery_voltage, "Battery check", configMINIMAL_STACK_SIZE, NULL, 1, NULL));
	print("Tasks registered\n\f");

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    while (true)
    {
        /* FreeRTOS should not be here... FreeRTOS goes back to the start of stack
         * in vTaskStartScheduler function. */
    }
	NVIC_SystemReset();
}


/*--------------------------------------------------------------------------------------
 * vApplicationStackOverflowHook:    Callback function when task caused stack overflow
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */


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

/*--------------------------------------------------------------------------------------
 * vApplicationIdleHook:   callback function for idle task (when enabled in freeRTOSconfig.h)
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

void vApplicationIdleHook( void )
{
	uint16_t i = 0;
    for(;;){
	    while(i++ < 65535){};
	    printeger(xPortGetFreeHeapSize(), 4);
		print(": free heapsize \n\f");
		i=0;
	}
}


