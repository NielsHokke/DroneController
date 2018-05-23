/*------------------------------------------------------------------
 *  control.c -- here you can implement your control algorithm
 *		 and any motor clipping or whatever else
 *		 remember! motor input =  0-1000 : 125-250 us (OneShot125)
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "in4073.h"
#include "drone.h"

void update_motors(void)
{					
	//TODO check for max
	motor[0] = ae[0];
	motor[1] = ae[1];
	motor[2] = ae[2];
	motor[3] = ae[3];
}

void motors_off(void)
{					
	motor[0] = 0;
	motor[1] = 0;
	motor[2] = 0;
	motor[3] = 0;
}

void run_filters_and_control()
{
	// fancy stuff here
	// control loops and/or filters

	// ae[0] = xxx, ae[1] = yyy etc etc
	update_motors();
}



/*--------------------------------------------------------------------------------------
 * sensor_loop: Takes the values from the setpoint and directly maps this to motor output
 /				DO NOT USE THIS TO FLY, YOU WILL CRASH THE DRONE. TESTING ONLY
 * Parameters: pointer to function parameters
 * Return:   void
 * Author:    Jetse Brouwer
 * Date:    14-5-2018
 *--------------------------------------------------------------------------------------
 */

void manual_control(void){
	int32_t tempMotor[4]; 
	//LIFT: We use fixed point precions of 1 = 1024. We're mapping 255 (max value) to 1024000 (1000 times the 1024 fixed point precions whichs gives us 1024000/255 = 4015)
	//Pitch and roll are first scaled by 2^10 to increase percision and next divided by a fixed scaler which can be set in drone.h
	//TODO: the scalars should be a on the go settable parameter.

	//TODO: chacne 1606 (which sacels up to 400) back to 4015 which scales to 1000
	tempMotor[0] = ( (int32_t) SetPoint.lift * 1606) + 	 (int32_t) SetPoint.pitch * 1024 / MAN_PITCH_SCALER 	+ (int32_t) SetPoint.yaw * 1024 / MAN_YAW_SCALER;
	tempMotor[1] =  (int32_t) SetPoint.lift * 1606 + (int32_t) SetPoint.roll * 1024 / MAN_ROLL_SCALER - (int32_t) SetPoint.yaw * 1024 / MAN_YAW_SCALER;
	tempMotor[2] =  (int32_t) (SetPoint.lift * 1606) - 	(int32_t) SetPoint.pitch * 1024 / MAN_PITCH_SCALER 	+ (int32_t) SetPoint.yaw * 1024 / MAN_YAW_SCALER;
	tempMotor[3] =  (int32_t) SetPoint.lift * 1606 - (int32_t)	SetPoint.roll * 1024 / MAN_ROLL_SCALER  - (int32_t) SetPoint.yaw * 1024 / MAN_YAW_SCALER;


	tempMotor[0] = tempMotor[0] /1024;
	tempMotor[1] = tempMotor[1] /1024;
	tempMotor[2] = tempMotor[2] /1024;
	tempMotor[3] = tempMotor[3] /1024;

	if (tempMotor[0] > 400) tempMotor[0] = 400;
	if (tempMotor[1] > 400) tempMotor[1] = 400;
	if (tempMotor[2] > 400) tempMotor[2] = 400;
	if (tempMotor[3] > 400) tempMotor[3] = 400;

	if (tempMotor[0] < 0) tempMotor[0] = 0;
	if (tempMotor[1] < 0) tempMotor[1] = 0;
	if (tempMotor[2] < 0) tempMotor[2] = 0;
	if (tempMotor[3] < 0) tempMotor[3] = 0;

	ae[0] = (int16_t) tempMotor[0];
	ae[1] = (int16_t) tempMotor[1];
	ae[2] = (int16_t) tempMotor[2];
	ae[3] = (int16_t) tempMotor[3];
}
