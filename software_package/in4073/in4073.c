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
#define SENSOR_PERIOD 10



/*--------------------------------------------------------------------------------------
 * control_loop: task containing the control loop of the quad-copter
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:   Jetse Brouwer
 * Date:    2-5-2018
 *--------------------------------------------------------------------------------------
 */

static void control_loop(void *pvParameter){
	UNUSED_PARAMETER(pvParameter);

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = CONTROL_PERIOD; //period of task
	int i = 0;

	bool printing = false;

	gpio_init();
	timers_init();
	twi_init();
	imu_init(true, 100);	
	baro_init();
	spi_flash_init();
	// ble_init();


	for(;;){
		xLastWakeTime = xTaskGetTickCount();



		if ((i++ % 100) == 0){
			nrf_gpio_pin_toggle(GREEN);

			// DEBUG_PRINT(".TEST\n\f");

			// DEBUG_UPRINTEGER(ae[0], 3);
			// DEBUG_PRINT(", \f");
			// DEBUG_UPRINTEGER(ae[1], 3);
			// DEBUG_PRINT(", \f");
			// DEBUG_UPRINTEGER(ae[2], 3);
			// DEBUG_PRINT(", \f");
			// DEBUG_UPRINTEGER(ae[3], 3);


			// DEBUG_PRINT("\n\f");

			// DEBUG_PRINT("\n phi: \f");

			// DEBUG_PRINTEGER((phi - phi_offset) / 220, 6);
			// DEBUG_PRINT(" theta: \f");
			// DEBUG_PRINTEGER((theta - theta_offset) / 140, 6);
			// DEBUG_PRINT(" psi: \f");
			// DEBUG_PRINTEGER((psi - psi_offset) / 170, 6);

			
			// DEBUG_PRINT(".\n\f");

			// printf("%6d %6d %6d | ", sp, sq, sr); //  from the gyro

			printing = true;
		}else{
			printing = false;
		}

		get_dmp_data(); // If we don;t continously read the fifo after initializing it's throwing errors adn exceptions all over the place
		switch(GLOBALSTATE){
			case S_SAFE:
				if(printing) DEBUG_PRINT("S_SAFE\n\f");
				ae[0] = 0;
				ae[1] = 0;
				ae[2] = 0;
				ae[3] = 0;
				break;

			case S_PANIC :
				if(printing) DEBUG_PRINT("S_PANIC\n\f");
				// TODO implement mode
				ae[0] = 0;
				ae[1] = 0;
				ae[2] = 0;
				ae[3] = 0;
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
				dmp_control(false);
				if(printing) DEBUG_PRINT("S_FULL_CONTROLL\n\f");
				break;

			case S_RAW_MODE :
				if(printing) DEBUG_PRINT("S_RAW_MODE\n\f");
				// TODO implement mode
				break;

			case S_HEIGHT_CTRL :
				if(printing) DEBUG_PRINT("S_HEIGHT_CTRL\n\f");
				// TODO implement mode
				break;

			case S_WIRELESS :
				if(printing) DEBUG_PRINT("S_WIRELESS\n\f");
				// TODO implement mode
				break;

			default:
				if(printing) DEBUG_PRINT("ERROR in Default state!\n\f");
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

	for(;;){
		xLastWakeTime = xTaskGetTickCount();	

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
	int i = 0;

	for(;;){

		if ((i++ % 100) == 0){
			nrf_gpio_pin_toggle(BLUE);

			adc_request_sample();
			vTaskDelay(1);

			if (bat_volt < 1080){ // minimum = 10.8/0.007058824
				
				///DEBUG_PRINT("VOLTAGE TO LOW GOING TO PANIC MODE\n\f");
				// GLOBALSTATE = S_PANIC;
			}
		}

		downLink(GLOBALSTATE, motor[0], motor[1], motor[2], motor[3], phi, theta, psi);

		//DEBUG_PRINTEGER((int) uxTaskGetStackHighWaterMark(NULL));
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
	    printeger(xPortGetFreeHeapSize(), 4);
		print(": free heapsize \n\f");
		i=0;
	}
}


