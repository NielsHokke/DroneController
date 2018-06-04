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
 * calibrate: 	Getting zero point from sensors
 * Parameters: 	bool raw indiaction if calibrating raw or dmp mode 
 * Return:   	void, but sets gloab phi_offset, theta_offset, psi_offset
 * Author:    	Niels Hokke
 * Date:    	30-5-2018
 *--------------------------------------------------------------------------------------
 */
void calibrate(bool raw){
	int32_t phi_total = 0;
	int32_t theta_total = 0;
	int32_t psi_total = 0;

	for(uint8_t i=0; i<CALIBRATION_ROUNDS; i++){
		if(raw){

		}else{
			get_dmp_data();

			phi_total += phi;
			theta_total += theta;
			psi_total += psi;

			DEBUG_PRINT("phi_total: \f");
			DEBUG_PRINTEGER(phi_total, 6);
			DEBUG_PRINT(" phi: \f");
			DEBUG_PRINTEGER(phi, 6);

			DEBUG_PRINT(" theta_total: \f");
			DEBUG_PRINTEGER(theta_total, 6);
			DEBUG_PRINT(" theta: \f");
			DEBUG_PRINTEGER(theta, 6);


			DEBUG_PRINT(" psi_total: \f");
			DEBUG_PRINTEGER(psi_total, 6);
			DEBUG_PRINT(" psi: \f");
			DEBUG_PRINTEGER(psi, 6);
			DEBUG_PRINT("\n\n\f");

			vTaskDelay(7);
		}
	}

	phi_offset 	 = phi_total/CALIBRATION_ROUNDS;
	theta_offset = theta_total/CALIBRATION_ROUNDS;
	psi_offset 	 = psi_total/CALIBRATION_ROUNDS;

	DEBUG_PRINT("phi_offset: \f");
	DEBUG_PRINTEGER(phi_offset, 6);
	DEBUG_PRINT(" theta_offset: \f");
	DEBUG_PRINTEGER(theta_offset, 6);
	DEBUG_PRINT(" psi_offset: \f");
	DEBUG_PRINTEGER(psi_offset, 6);
	DEBUG_PRINT("\n\n\f");

	GLOBALSTATE = S_SAFE;
}


/*--------------------------------------------------------------------------------------
 * yaw_control: Takes the values from the setpoint and uses sr for angulaur momentum to implement the yaw control
 * Parameters: 	pointer to function parameters
 * Return:   	void
 * Author:    	Jetse Brouwer
 * Date:    	14-5-2018
 *--------------------------------------------------------------------------------------
 */
 

void yaw_control(void){
	int32_t tempMotor[4]; 
	get_dmp_data();
	static uint8_t i = 0;

	// YAW: P-controller for yaw
	// 16.4 LSB/deg/s so sr to actual angulur momentum is SR/16.4
	// SetPoint.yaw = -128 deg/s to 127 deg/s
	// We scale the setpoint by 16.4 to make them have the same size

	if (i++ > 10){
		DEBUG_PRINT(".\n sr \f");
		DEBUG_PRINTEGER(sr, 6);
		DEBUG_PRINT(".\n\f");
		i = 0;
	}
	

	int32_t yaw_output = (SetPoint.yaw*16 - sr) * 64 *parameters[P_P_YAW]; 	
	if (yaw_output > 200){
		yaw_output = 200;
	}else if (yaw_output < -200){
		yaw_output = -200;
	}
	

	//LIFT: We use fixed point precions of 1 = 1024. We're mapping 255 (max value) to 1024000 (1000 times the 1024 fixed point precions whichs gives us 1024000/255 = 4015)
	//Pitch and roll are first scaled by 2^10 to increase percision and next divided by a fixed scaler which can be set in drone.h
	//TODO: the scalars should be a on the go settable parameter.

	// //TODO: chacne 1606 (which sacels up to 400) back to 4015 which scales to 1000
	// tempMotor[0] = ( (int32_t) SetPoint.lift * 1606) + 	 (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	+ (int32_t) yaw_output;
	// tempMotor[1] =  (int32_t) SetPoint.lift * 1606 - (int32_t) SetPoint.roll * 1606 / MAN_ROLL_SCALER - (int32_t) yaw_output;
	// tempMotor[2] =  (int32_t) (SetPoint.lift * 1606) - 	(int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	+ (int32_t) yaw_output;
	// tempMotor[3] =  (int32_t) SetPoint.lift * 1606 + (int32_t)	SetPoint.roll * 1606 / MAN_ROLL_SCALER  - (int32_t) yaw_output;


	// tempMotor[0] = ( (int32_t) SetPoint.lift * 1606) + (int32_t) yaw_output;
	// tempMotor[1] =  (int32_t) SetPoint.lift * 1606 6 / MAN_ROLL_SCALER - (int32_t) yaw_output;
	// tempMotor[2] =  (int32_t) (SetPoint.lift * 1606) - 	(int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	+ (int32_t) yaw_output;
	// tempMotor[3] =  (int32_t) SetPoint.lift * 1606 + (int32_t)	SetPoint.roll * 1606 / MAN_ROLL_SCALER  - (int32_t) yaw_output;

	tempMotor[0] =  (int32_t) (SetPoint.lift * 1606) + (int32_t) yaw_output;
	tempMotor[1] =  (int32_t) (SetPoint.lift * 1606) -  (int32_t) yaw_output;
	tempMotor[2] =  (int32_t) (SetPoint.lift * 1606) + (int32_t) yaw_output;
	tempMotor[3] =  (int32_t) (SetPoint.lift * 1606) - (int32_t) yaw_output;

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





/*--------------------------------------------------------------------------------------
 * manual_control: Takes the values from the setpoint and directly maps this to motor output
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
	tempMotor[0] = ( (int32_t) SetPoint.lift * 1606) + 	 (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[1] =  (int32_t) SetPoint.lift * 1606 - (int32_t) SetPoint.roll * 1606 / MAN_ROLL_SCALER + (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[2] =  (int32_t) (SetPoint.lift * 1606) - 	(int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[3] =  (int32_t) SetPoint.lift * 1606 + (int32_t)	SetPoint.roll * 1606 / MAN_ROLL_SCALER  + (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;


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
