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

// placeholder defines
#define P_P1 0
#define P_P2 0
#define P_YAW_MAX 0 
#define P_YAW_MIN 0
#define P_ANGLE_MIN 0 
#define P_ANGLE_MAX 0
#define P_MIN_LIFT 0
#define P_MAX_RPM 0

#define BUTTERWORTH_GAIN  1.482463775e+01
//#define KALMAN_C1 100
//#define KALMAN_C2 1000000
#define P2PHI 0.0081 //0.023

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

	for(uint8_t i=0; i < CALIBRATION_ROUNDS; i++){
		if(raw) {

		} 
		else {
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


/*--------------------------------------------------------------------------------------
 * scale_thrust: scale the thrust after limiting max yaw pitch and roll values
 * Parameters: 	 yaw_only, if true the pitch and roll control will be ignored and only the yaw control wil work
 * Return:   	 void
 * Author:    	 Jetse Brouwer
 * Date:    	 14-5-2018
 *--------------------------------------------------------------------------------------
 */
void scale_thrust(int16_t yaw_output, int16_t pitch_output, int16_t roll_output)
{
	int32_t tempMotor[4];
	// The following function limits the output to the values set by the yaw min max
	// The value is scaled by a factor 4 ( << 2) so we can use a single byte to cap over the whole range (0 - 1000)	
	if (yaw_output >= ((int32_t) parameters[P_YAW_MAX] << 2)) {
		yaw_output = ((int32_t) parameters[P_YAW_MAX] << 2);
	} 
	else if (yaw_output <=  -((int32_t) parameters[P_YAW_MIN] << 2)) {
		yaw_output =  -((int32_t) parameters[P_YAW_MIN] << 2);
	}

	if (roll_output >= ((int32_t) parameters[P_ANGLE_MAX] << 2)) {
		roll_output = ((int32_t) parameters[P_ANGLE_MAX] << 2);
	} 
	else if (roll_output <=  -((int32_t) parameters[P_ANGLE_MIN] << 2)) {
		roll_output =  -((int32_t) parameters[P_ANGLE_MIN] << 2);
	}

	if (pitch_output >= ((int32_t) parameters[P_ANGLE_MAX] << 2)) {
		pitch_output = ((int32_t) parameters[P_ANGLE_MAX] << 2);
	}
	else if (pitch_output <=  -((int32_t) parameters[P_ANGLE_MIN] << 2)) {
		pitch_output =  -((int32_t) parameters[P_ANGLE_MIN] << 2);
	}


	//LIFT: We use fixed point precions of 1 = 1024. We're mapping 255 (max value) to 1024000 (1000 times the 1024 fixed point precions whichs gives us 1024000/255 = 4015)
	//Pitch and roll are first scaled by 2^10 to increase percision and next divided by a fixed scaler which can be set in drone.h

	//TODO: change 1606 (which scales up to 400) back to 4015 which scales to 1000 (maybe use 4096 which can be done by '<< 12' which scales to 1020)
	tempMotor[0] =  (int32_t) SetPoint.lift * 1606 + yaw_output + pitch_output;
	tempMotor[1] =  (int32_t) SetPoint.lift * 1606 - yaw_output - roll_output;
	tempMotor[2] =  (int32_t) SetPoint.lift * 1606 + yaw_output + roll_output;
	tempMotor[3] =  (int32_t) SetPoint.lift * 1606 - yaw_output - pitch_output;


	// Minimum lift guard:
	// The first part (SetPoint.lift << 2) maps '0 - 255' to values ranging from '0 - 1024'
	// The second part (parameters[P_MIN_LIFT] << 2) also scales from '0 - 1024'
	// we compare a shifted value (times 1024) with a none-shifted value
	// The trick here is that if the second part is 1024, it is canceled out by shifting the answer back by 10 bits (effectivly being multiplying by 1)
	// If the value is smaller we are effectivly dividing. giving us the possibility to set the minimum lift ranging from 0 - 1 times the liftsetpoint in steps of 1/256
	// min = ((SetPoint.lift << 2)  * (parameters[P_MIN_LIFT] << 2))

	if (tempMotor[0] < ((SetPoint.lift << 2) * (parameters[P_MIN_LIFT] << 2))) tempMotor[0] = ((SetPoint.lift << 2)  * (parameters[P_MIN_LIFT] << 2));
	if (tempMotor[1] < ((SetPoint.lift << 2) * (parameters[P_MIN_LIFT] << 2))) tempMotor[1] = ((SetPoint.lift << 2)  * (parameters[P_MIN_LIFT] << 2));
	if (tempMotor[2] < ((SetPoint.lift << 2) * (parameters[P_MIN_LIFT] << 2))) tempMotor[2] = ((SetPoint.lift << 2)  * (parameters[P_MIN_LIFT] << 2));
	if (tempMotor[3] < ((SetPoint.lift << 2) * (parameters[P_MIN_LIFT] << 2))) tempMotor[3] = ((SetPoint.lift << 2)  * (parameters[P_MIN_LIFT] << 2));

	// Scale values back to range from 0 - 1000 again
	tempMotor[0] = tempMotor[0] >> 10;
	tempMotor[1] = tempMotor[1] >> 10;
	tempMotor[2] = tempMotor[2] >> 10;
	tempMotor[3] = tempMotor[3] >> 10;

	// Maximum RPM guard:
	if (tempMotor[0] > (parameters[P_MAX_RPM] << 2)) tempMotor[0] = (parameters[P_MAX_RPM] << 2);
	if (tempMotor[1] > (parameters[P_MAX_RPM] << 2)) tempMotor[1] = (parameters[P_MAX_RPM] << 2);
	if (tempMotor[2] > (parameters[P_MAX_RPM] << 2)) tempMotor[2] = (parameters[P_MAX_RPM] << 2);
	if (tempMotor[3] > (parameters[P_MAX_RPM] << 2)) tempMotor[3] = (parameters[P_MAX_RPM] << 2);

	// Set calculated values to setpoints
	ae[0] = (int16_t) tempMotor[0];
	ae[1] = (int16_t) tempMotor[1];
	ae[2] = (int16_t) tempMotor[2];
	ae[3] = (int16_t) tempMotor[3];
}

/*
P_yaw (uint16), yaw_min (uint8), yaw_max (uint8) 
P1 (uint16), P2 (uint16), angle_min (uint8), angle_max (uint8)
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
void dmp_control(bool yaw_only) {
	
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
	yaw_output = (((int32_t) SetPoint.yaw << 4) - sr) * parameters[P_P_YAW]; 

	if (yaw_only) {
		pitch_output = 0;
		roll_output  = 0;	
	}
	else {
		// ROLL: P controller for roll
		// Step 1: make setpoint and sensor comparable by scaling them to a common unit
		// for phi 220 maps to 1 degree. the setpoints (-128,127) map should map to (-5,5) degrees
		// (5 degrees / 128) * 220 = 8.59375 so (setpoint.roll * 8.59375) gets it in the same unit as the sensor
		// if we multiply sensor value by 2, we can multiply setpoint by 17 (with an rounding error of 1.09%)
		roll_output = parameters[P_P1] * (((int32_t) SetPoint.roll * 17) - (phi << 1)) - parameters[P_P2] * sp;
		

		// PITCH: P controller for roll
		// Step 1: make setpoint and sensor comparable by scaling them to a common unit
		// for phi 140 maps to 1 degree. the setpoints (-128,127) map should map to (-5,5) degrees
		// (5 degrees / 128) * 140 = 5.46785 so (setpoint.pitch * 5.46785) gets it in the same unit as the sensor
		// if we multiply sensor value by 2, we can multiply setpoint by 11 (with an rounding error of 0.58%)
		pitch_output = parameters[P_P1] * (((int32_t) SetPoint.pitch * 11) - (theta << 1)) - parameters[P_P2] * sq;
	}

	scale_thrust(yaw_output, pitch_output, roll_output);
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
	tempMotor[0] = (int32_t) (SetPoint.lift * 1606) + (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[1] = (int32_t) (SetPoint.lift * 1606) - (int32_t) SetPoint.roll  * 1606 / MAN_ROLL_SCALER 	+ (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[2] = (int32_t) (SetPoint.lift * 1606) - (int32_t) SetPoint.pitch * 1606 / MAN_PITCH_SCALER 	- (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;
	tempMotor[3] = (int32_t) (SetPoint.lift * 1606) + (int32_t)	SetPoint.roll  * 1606 / MAN_ROLL_SCALER  	+ (int32_t) SetPoint.yaw * 1606 / MAN_YAW_SCALER;

	tempMotor[0] = tempMotor[0] >> 10;
	tempMotor[1] = tempMotor[1] >> 10;
	tempMotor[2] = tempMotor[2] >> 10;
	tempMotor[3] = tempMotor[3] >> 10;

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
 * run_filter: 	run selected filter to give smooth values like DMP
 * Parameters: 	select kalman (pitch and roll) or butterworth filter (yaw)
 * Return:   	populated struct of filtered phi theta psi, sp sq sr
 * Author:    	Nilay
 * Date:    	5-6-2018
 *--------------------------------------------------------------------------------------
 */
void run_filter(filter_select_t filter)
{	
	/*
	 * keep the initializations to static, since the filters use the last saved values
	 */
	static int16_t p_bias, q_bias;
	static int16_t p, q;

	/*--------------------------------------------------------------------------------------
	 * filter roll and pitch using a filter resembling kalman filter
	 * reference: Arjan J.C. van Gemund (IN4073: QR controller theory)
	 * todo: check if the signs of gyro and accelerometer are changing together
	 *--------------------------------------------------------------------------------------
	 */
	if (filter == ROLL) {
		p      = sp     - p_bias;							      // remove the bias
		phi_f  = phi_f  + p * P2PHI;						      // weight of increments
		phi_f  = phi_f  - (phi_f - phi) / parameters[KALMAN_C1];  // sensor fusion
		p_bias = p_bias + (phi_f - phi) / parameters[KALMAN_C2];  // how noisy is the data? changes rate of update 
	}
	else if (filter == PITCH) {
		q       = sq       - q_bias;									  // remove the bias
		theta_f = theta_f  + q * P2PHI;									  // weight of increments
		theta_f = theta_f  - (theta_f - theta) / parameters[KALMAN_C1];   // sensor fusion
		q_bias  = q_bias   + (theta_f - theta) / parameters[KALMAN_C2];   // how noisy is the data? changes rate of update 
	}

	/*--------------------------------------------------------------------------------------
	 * filter_yaw: 	run 2nd order butterworth filter on the yaw controller (fixed point implementation)
	 * reference: 	http://www-users.cs.york.ac.uk/~fisher/cgi-bin/mkfscript
	 *--------------------------------------------------------------------------------------
	 */
	else if (filter == YAW) {
		static float xv[3], yv[3];

		xv[0] = xv[1]; 
		xv[1] = xv[2]; 
		xv[2] = sr / BUTTERWORTH_GAIN;
		yv[0] = yv[1]; 
		yv[1] = yv[2]; 
		yv[2] = (xv[0] + xv[2]) + 2 * xv[1] + (-0.4128015981 * yv[0]) + (1.1429805025 * yv[1]);
		sr_f = yv[2];
	}

}


/*--------------------------------------------------------------------------------------
 * raw_control: use filters and then close the loop for raw_control	
 * Parameters: 	bool yaw mode only or full control
 * Return:   	populated struct of filtered phi theta psi, sp sq sr
 * Author:    	Nilay
 * Date:    	5-6-2018
 *--------------------------------------------------------------------------------------
 */
void raw_control(bool yaw_only)
{	
	int32_t yaw_output, pitch_output, roll_output;
	get_raw_sensor_data();

	run_filter(YAW);
	yaw_output = (((int32_t) SetPoint.yaw << 4) - sr_f) * parameters[P_P_YAW];
	
	if (yaw_only) {

		pitch_output = 0;
		roll_output  = 0;	

	}
	else {

		run_filter(PITCH);
		pitch_output = parameters[P_P1] * (((int32_t) SetPoint.pitch * 11) - (theta_f << 1)) - parameters[P_P2] * sq;

		run_filter(ROLL);
		roll_output  = parameters[P_P1] * (((int32_t) SetPoint.roll * 17)  - (phi_f << 1))   - parameters[P_P2] * sp;
	}

	scale_thrust(yaw_output, pitch_output, roll_output);
}
