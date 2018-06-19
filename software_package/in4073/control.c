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
	if (SetPoint.lift > 0){
		motor[0] = ae[0];
		motor[1] = ae[1];
		motor[2] = ae[2];
		motor[3] = ae[3];
	}else{
		motor[0] = 0;
		motor[1] = 0;
		motor[2] = 0;
		motor[3] = 0;	
	}
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
 * Return:   	void, but sets global phi_offset, theta_offset, psi_offset
 * Author:    	Niels Hokke
 * Date:    	30-5-2018
 *--------------------------------------------------------------------------------------
 */
void calibrate(bool raw){
	int32_t phi_total = 0;
	int32_t theta_total = 0;
	int32_t psi_total = 0;

	phi_offset 	 	= 0;
	theta_offset	= 0;
	psi_offset 	 	= 0;


	for(uint8_t i=0; i < CALIBRATION_ROUNDS; i++){
		if(raw){

		}else{
			TickType_t xLastWakeTime = xTaskGetTickCount();
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

			vTaskDelayUntil( &xLastWakeTime, 10);
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

/*
P_yaw (uint16_t), yaw_min (uint8), yaw_max (uint8) 
P1 (uint16_t), P2 (uint16_t), angle_min (uint8), angle_max (uint8)
*/

/*--------------------------------------------------------------------------------------
 * dmp_control: Takes the values from the setpoint and sensors and implements p controler
 *				for yaw, and cascaded p controler for pitch and roll. motors are not adjusted
 				only setpoints are set, call update_motor() to effectuate the settings
 * Parameters: 	yaw_only, if true the pitch and roll control will be ignored and only the yaw control wil work
 * Return:   	void
 * Author:    	Jetse Brouwer
 * Date:    	14-5-2018
 *--------------------------------------------------------------------------------------
 */

void dmp_control(bool yaw_only){
	int32_t tempMotor[4];
	int32_t yaw_output, pitch_output, roll_output;


	// static uint8_t i = 0;

	// if (i++ > 10){
	// 	DEBUG_PRINT(".\n sr \f");
	// 	DEBUG_PRINTEGER(sr, 6);
	// 	DEBUG_PRINT(".\n\f");
	// 	i = 0;
	// }

	// YAW: P-controller for yaw
	// Step 1: make setpoint and sensor comparable by scaling them to a common unit
	// 16.4 LSB/deg/s so sr to actual angulur momentum is SR/16.4
	// SetPoint.yaw = -128 deg/s to 127 deg/s
	// We scale the setpoint by 16.4 to make them have the same size ('<< 4' = '* 16')
	yaw_output = ((- (int32_t) SetPoint.yaw << 6) - sr) * GET_PARA_16(P_P_YAW); 

	if (yaw_only) {
		pitch_output = 0;
		roll_output = 0;	
	}
	else {
		// ROLL: P controller for roll
		// Step 1: make setpoint and sensor comparable by scaling them to a common unit
		// for phi 220 maps to 1 degree. the setpoints (-128,127) map should map to (-5,5) degrees
		// (5 degrees / 128) * 220 = 8.59375 so (setpoint.roll * 8.59375) gets it in the same unit as the sensor
		// if we multiply sensor value by 2, we can multiply setpoint by 17 (with an rounding error of 1.09%)


		roll_output = 	GET_PARA_16(P_P1) * (	((int32_t) SetPoint.roll * 48) - (phi << 1)) - 	GET_PARA_16(P_P2) * sp;

		

		// PITCH: P controller for roll
		// Step 1: make setpoint and sensor comparable by scaling them to a common unit
		// for phi 140 maps to 1 degree. the setpoints (-128,127) map should map to (-5,5) degrees
		// (5 degrees / 128) * 140 = 5.46785 so (setpoint.pitch * 5.46785) gets it in the same unit as the sensor
		// if we multiply sensor value by 2, we can multiply setpoint by 11 (with an rounding error of 0.58%)
		pitch_output = 	-1 * GET_PARA_16(P_P1) * (	((int32_t) SetPoint.pitch * 48) - (theta << 1)) - GET_PARA_16(P_P2) * sq;
	}

	// The following function limits the output to the values set by the yaw min max
	// The value is scaled by a factor 4 ( << 2) so we can use a single byte to cap over the whole range (0 - 1000)	
	// if (yaw_output >= ((int32_t) GET_PARA_8(P_YAW_MAX) << 2)) {
	// 	yaw_output = ((int32_t) GET_PARA_8(P_YAW_MAX) << 2);
	// }else if (yaw_output <=  -((int32_t) GET_PARA_8(P_YAW_MIN) << 2) ){
	// 	yaw_output =  -((int32_t) GET_PARA_8(P_YAW_MIN) << 2);
	// }

	// if (roll_output >= ((int32_t) GET_PARA_8(P_ANGLE_MAX) << 2)) {
	// 	roll_output = ((int32_t) GET_PARA_8(P_ANGLE_MAX) << 2);
	// }else if (roll_output <=  -((int32_t) GET_PARA_8(P_ANGLE_MIN) << 2) ){
	// 	roll_output =  -((int32_t) GET_PARA_8(P_ANGLE_MIN) << 2);
	// }

	// if (pitch_output >= ((int32_t) GET_PARA_8(P_ANGLE_MAX) << 2)) {
	// 	pitch_output = ((int32_t) GET_PARA_8(P_ANGLE_MAX) << 2);
	// }else if (pitch_output <=  -((int32_t) GET_PARA_8(P_ANGLE_MIN) << 2) ){
	// 	pitch_output =  -((int32_t) GET_PARA_8(P_ANGLE_MIN) << 2);
	// }


	//LIFT: We use fixed point precions of 1 = 1024. We're mapping 255 (max value) to 1024000 (1000 times the 1024 fixed point precions whichs gives us 1024000/255 = 4015)
	//Pitch and roll are first scaled by 2^10 to increase percision and next divided by a fixed scaler which can be set in drone.h

	//TODO: change 1606 (which scales up to 400) back to 4015 which scales to 1000 (maybe use 4096 which can be done by '<< 12' which scales to 1020)
	tempMotor[0] =  ((int32_t) SetPoint.lift  << 13) + yaw_output - pitch_output;
	tempMotor[1] =  ((int32_t) SetPoint.lift  << 13) - yaw_output - roll_output;
	tempMotor[2] =  ((int32_t) SetPoint.lift  << 13) + yaw_output + pitch_output;
	tempMotor[3] =  ((int32_t) SetPoint.lift  << 13) - yaw_output + roll_output;


	// Minimum lift guard:
	// The first part (SetPoint.lift << 2) maps '0 - 255' to values ranging from '0 - 1024'
	// The second part (GET_PARA_16(P_MIN_LIFT) << 2) also scales from '0 - 1024'
	// we compare a shifted value (times 1024) with a none-shifted value
	// The trick here is that if the second part is 1024, it is canceled out by shifting the answer back by 10 bits (effectivly being multiplying by 1)
	// If the value is smaller we are effectivly dividing. giving us the possibility to set the minimum lift ranging from 0 - 1 times the liftsetpoint in steps of 1/256
	// min = ((SetPoint.lift << 2)  * (GET_PARA_16(P_MIN_LIFT) << 2))

	// if (tempMotor[0] < (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2))) tempMotor[0] = (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2));
	// if (tempMotor[1] < (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2))) tempMotor[1] = (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2));
	// if (tempMotor[2] < (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2))) tempMotor[2] = (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2));
	// if (tempMotor[3] < (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2))) tempMotor[3] = (((uint16_t) SetPoint.lift << 2)  * ((uint16_t) GET_PARA_8(P_MIN_LIFT) << 2));

	// Scale values back to range from 0 - 1000 again
	tempMotor[0] = tempMotor[0] >> 12;
	tempMotor[1] = tempMotor[1] >> 12;
	tempMotor[2] = tempMotor[2] >> 12;
	tempMotor[3] = tempMotor[3] >> 12;

		// Maximum RPM guard:
	// if (tempMotor[0] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[0] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	// if (tempMotor[1] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[1] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	// if (tempMotor[2] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[2] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	// if (tempMotor[3] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[3] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);

	if (tempMotor[0] < 200) tempMotor[0] = 200;
	if (tempMotor[1] < 200) tempMotor[1] = 200;
	if (tempMotor[2] < 200) tempMotor[2] = 200;
	if (tempMotor[3] < 200) tempMotor[3] = 200;

	if (tempMotor[0] > 750) tempMotor[0] = 750;
	if (tempMotor[1] > 750) tempMotor[1] = 750;
	if (tempMotor[2] > 750) tempMotor[2] = 750;
	if (tempMotor[3] > 750) tempMotor[3] = 750;


	// Set calculated values to setpoints
	ae[0] = (int16_t) tempMotor[0];
	ae[1] = (int16_t) tempMotor[1];
	ae[2] = (int16_t) tempMotor[2];
	ae[3] = (int16_t) tempMotor[3];
}


/*--------------------------------------------------------------------------------------
 * manual_control:	Takes the values from the setpoint and directly maps this to motor output
 /					DO NOT USE THIS TO FLY, YOU WILL CRASH THE DRONE. TESTING ONLY
 * Parameters: 		void
 * Return:   		void
 * Author:    		Jetse Brouwer
 * Date:    		14-5-2018
 *--------------------------------------------------------------------------------------
 */

void manual_control(void){
	int32_t tempMotor[4]; 
	//LIFT: We use fixed point precions of 1 = 1024. We're mapping 255 (max value) to 1024000 (1000 times the 1024 fixed point precions whichs gives us 1024000/255 = 4015)
	//Pitch and roll are first scaled by 2^10 to increase percision and next divided by a fixed scaler which can be set in drone.h
	//TODO: the scalars should be a on the go settable parameter.

	//TODO: chacne 1606 (which sacels up to 400) back to 4015 which scales to 1000
	tempMotor[0] = (int32_t) (SetPoint.lift << 11) 	+ (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[1] = (int32_t) (SetPoint.lift << 11) 	- (int32_t) SetPoint.roll  * 1606 / MAN_ROLL_SCALER 	+ (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[2] = (int32_t) (SetPoint.lift << 11) 	- (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[3] = (int32_t) (SetPoint.lift << 11) 	+ (int32_t)	SetPoint.roll  * 1606 / MAN_ROLL_SCALER  	+ (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;


	tempMotor[0] = tempMotor[0] >> 10;
	tempMotor[1] = tempMotor[1] >> 10;
	tempMotor[2] = tempMotor[2] >> 10;
	tempMotor[3] = tempMotor[3] >> 10;

		// Maximum RPM guard:
	if (tempMotor[0] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[0] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	if (tempMotor[1] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[1] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	if (tempMotor[2] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[2] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);
	if (tempMotor[3] > ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2)) tempMotor[3] = ((uint16_t) GET_PARA_8(P_MAX_RPM) << 2);

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
 * panic: 		This will put the drone in panic mode, in panic mode the motors slowely rev down
 *				and if they are below a ceratain threshold shut down entirely.
 				incomming messages are blocked in uart.c
 * Parameters: 	void
 * Return:   	void
 * Author:    	David Enthoven
 * Date:    	13-5-2018
 *--------------------------------------------------------------------------------------
 */
void panic(bool printing){

	static bool 	new_panic = true;
	static uint8_t 	counter = 255;
	const  uint16_t drop_motorvalue = 400;


	//first invoaction set to static dropping value for 2.5 seconds
	if (new_panic){

		// variation 1: lowest value
		// for ( uint8_t i = 0; i<4; i++) {
		// 		if (ae[i] > drop_motorvalue) ae[i] = drop_motorvalue;
		// 	}

		// variation 2 average
		uint32_t average = ae[0]+ae[1]+ae[2]+ae[3];
		average = average >> 2;

		if (average > drop_motorvalue){
			average = drop_motorvalue;
			}
		else {
			ae[0] = average;
			ae[1] = average;
			ae[2] = average;
			ae[3] = average;
		}	
		new_panic = false;
	}
	else if (counter>0){
		counter--;
	}
	else if (counter == 0){
		//rev down motors
		if ( ae[0] || ae[1] || ae[2] || ae[3]){
			for ( uint8_t i = 0; i<4; i++) {
			//underflow protection and revdown
				if (ae[i] < 0) ae[i] = 0;
				else if (ae[i] > 0) ae[i] = ae[i]-1;
			}
		}
		//if motors are off
		else {
			GLOBALSTATE = S_SAFE;
			new_panic = true;
			counter = 255;
		}
	}
}