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


	// YAW: P-controller for yaw
	// make setpoint and sensor comparable by scaling them to a common unit

	yaw_output = ((- (int32_t) SetPoint.yaw << 6) - sr) * GET_PARA_16(P_P_YAW); 

	if (yaw_only) {
		pitch_output = 0;
		roll_output = 0;	
	}
	else {

		// ROLL: P controller for roll
		// 48 and shift by 1 are remmiscents from old code (but not changing it on d-day)
		roll_output = 	GET_PARA_16(P_P1) * (	((int32_t) SetPoint.roll * 48) - (phi << 1)) - 	GET_PARA_16(P_P2) * sp;

		// PITCH: P controller for roll
		// 48 and shift by 1 are remmiscents from old code (but not changing it on d-day)
		pitch_output = 	-1 * GET_PARA_16(P_P1) * (	((int32_t) SetPoint.pitch * 48) - (theta << 1)) - GET_PARA_16(P_P2) * sq;
	}

	//LIFT: We use fixed point precions of 1 = 4096. We're mapping 255 (max value) to 512 (more then enough to take off) (512 times the 4096 fixed point precions whichs gives us 2097152/256 = 8192 = 2^13)
	//Pitch and roll are  scaled in the P controller.

	tempMotor[0] =  ((int32_t) SetPoint.lift  << 13) + yaw_output - pitch_output;
	tempMotor[1] =  ((int32_t) SetPoint.lift  << 13) - yaw_output - roll_output;
	tempMotor[2] =  ((int32_t) SetPoint.lift  << 13) + yaw_output + pitch_output;
	tempMotor[3] =  ((int32_t) SetPoint.lift  << 13) - yaw_output + roll_output;


	// Scale values back to range from by
	tempMotor[0] = tempMotor[0] >> 12;
	tempMotor[1] = tempMotor[1] >> 12;
	tempMotor[2] = tempMotor[2] >> 12;
	tempMotor[3] = tempMotor[3] >> 12;

	
	// Maximum RPM guard:
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
 * panic: 		This will put the drone in panic mode, in panic mode the motors we be put at a steady of 400
 				or the average of the motors if this is lower then 400. after ~s.5 seconds the motors will 
 				rev down slowely and than the state will change to safemode.
 				incomming messages are blocked in uart.c
 * Parameters: 	void
 * Return:   	void
 * Author:    	David Enthoven
 * Date:    	13-5-2018
 *--------------------------------------------------------------------------------------
 */
void panic(void){
	static bool 	new_panic = true;
	static uint8_t 	counter = 255;

	//first invoaction set to static dropping value for 2.5 seconds
	if (new_panic){
		// variation 2 average
		uint32_t average = ae[0]+ae[1]+ae[2]+ae[3];
		average = average >> 2;

		if (average > PANIC_LIFT){
			average = PANIC_LIFT;
		}
		ae[0] = average;
		ae[1] = average;
		ae[2] = average;
		ae[3] = average;	
		new_panic = false;
	}
	else if (counter>0){
		counter--;
	}
	else if (counter == 0){
		//rev down motors
		if ( ae[0] || ae[1] || ae[2] || ae[3]){
			for ( uint8_t i = 0; i<4; i++) {
			//zero-check and revdown
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

/*--------------------------------------------------------------------------------------
 * run_filter: 		This function uses the raw data, and uses kallman filtering and
 					butterworth filtering to gain usefull data. It writes to the same
 					variables as the dmp, so the same controlloop can be used
 * Parameters: 	bit wise filter to slect pitch roll yaw or any combination
 * Return:   	void
 * Author:    	Nilay Sheth
 * Date:    	20-6-2018
 *--------------------------------------------------------------------------------------
 */

void run_filter(char filter)
{	
	/*
	 * keep the initializations to static, since the filters use the last saved values
	 */
	static int16_t p_bias, q_bias;
	static int16_t p, q;
	static int16_t p2phi = 133;

	/*--------------------------------------------------------------------------------------
	 * filter roll and pitch using a filter resembling kalman filter
	 * reference: Arjan J.C. van Gemund (IN4073: QR controller theory)
	 * todo: 1) check if the signs of gyro and accelerometer are changing together
	 *       2) matlab does it as p_bias(i) = p_bias(i-1) + (phi_error(i)/p2phi) / C2;
	 * 
	 *--------------------------------------------------------------------------------------
	 */
	if ((filter & ROLL_FILTER)  > 0) {
		p      = raw_sp     - p_bias;							      	 // remove the bias
		phi  = phi  + MUL_SCALED1(p, p2phi, 14);			     // weight of increments
		phi  = phi  - ((phi - raw_say) >> 3);                      // maybe 2^8
		p_bias = p_bias + ((phi - raw_say) >> 10);                     // maybe 2^14
		// phi  = phi  - (phi - raw_phi) / parameters[KALMAN_C1];  // sensor fusion
		// p_bias = p_bias + (phi - raw_phi) / parameters[KALMAN_C2];  // how noisy is the data? changes rate of update 
	}
	if ((filter & PITCH_FILTER)  > 0) {
		q       = raw_sq       - q_bias;									  // remove the bias
		theta = theta  + MUL_SCALED1(q, p2phi, 14);				      // weight of increments
		theta = theta  - ((theta - raw_sax) >> 3);					  // maybe 2^8
		q_bias  = q_bias   + ((theta - raw_sax) >> 10);					  // maybe 2^14
		// theta = theta  - (theta - raw_sax) / parameters[KALMAN_C1];  // sensor fusion
		// q_bias  = q_bias   + (theta - raw_sax) / parameters[KALMAN_C2];  // how noisy is the data? changes rate of update 
	}


	if ((filter & YAW_FILTER)  > 0) {

		/*--------------------------------------------------------------------------------------
		 * fixed point implementation for 2nd order 
		 * filter co-efficients when fs = 100 Hz, fc = 10Hz. 
		 * use [a,b] = butter(2, 10/(100/2), 'low')
		 *
		 * reference: CS4140ES Resources page > butterworth filters implementation and octave-online.net
		 * 100 Hz: 
		 * use [a,b] = butter(2, 10/(100/2), 'low')
		 * a = [0.067455273889071881709966760354291  0.13491054777814376341993352070858  0.067455273889071881709966760354291]
		 * b = [1.0  -1.1429805025399009110742554184981  0.41280159809618865995872738494654]
		 * 14 bit left shift is then
		 * a = [1105, 2210, 1105]
		 * b = [16384, -18727, 6763]
		 *
		 * 1000 Hz:
		 * use [a,b] = butter(2, 10/(1000/2), 'low')
		 * a = [0.00094469184384015074829737956818576  0.0018893836876803014965947591363715 0.00094469184384015074829737956818576] 
		 * b = [1.0  -1.911197067426073203932901378721  0.91497583480143385159522040339652]
		 * 14 bit left shift is then
		 * a = [15, 31, 15]
		 * b = [16384, -31313, 14991]
		 * 
		 * 500 Hz: 
		 * use [a,b] = butter(2, 10/(500/2), 'low')
		 * a = [0.0036216815149286412517382061082571  0.0072433630298572825034764122165143  0.0036216815149286412517382061082571] 
		 * b = [1.0  -1.8226949251963078246774330182234  0.83718165125602250764558220907929]
		 * 14 bit left shift is then
		 * a = [59, 119, 59]
		 * b = [16384, -29863, 13716]
		 *
		 * round(a*(2^14))
		 * round(b*(2^14))
         * --------------------------------------------------------------------------------------
		*/
		static int16_t a0 = 15;    
		static int16_t a1 = 31;    
		static int16_t a2 = 15;   
		//static int16_t b0_l =  16384;  
		static int16_t b1 = -31313;
		static int16_t b2 =  14991;

		static int16_t x0 = 0; 
		static int16_t x1 = 0;
		static int16_t x2 = 0;
		static int16_t y0 = 0;
		static int16_t y1 = 0;
		static int16_t y2 = 0;

		x0 = raw_sr;
		/* 
		 * 2nd order butterworth implementation
		 * b0*y0 = (a0*x0 + a1*x1 + a2*x2 - b1*y1 - b2*y2)
		 * y0 = ( a0*(2^14) * x0 ) >> 14 ...... and so on
		 */
		y0 = (    MUL_SCALED1(a0, x0, 14) 
				+ MUL_SCALED1(a1, x1, 14)
				+ MUL_SCALED1(a2, x2, 14) 
				- MUL_SCALED1(b1, y1, 14) 
				- MUL_SCALED1(b2, y2, 14) 
		       );
		sr = y0;

		//updating values in the array.
		x2 = x1;		
		y2 = y1;
		x1 = x0;
		y1 = y0;
	}

	if ((filter & ALT_FILTER)  > 0) {

		static int16_t a0_l =  1105;    
		static int16_t a1_l =  2210;    
		static int16_t a2_l =  1105;   
		//static int16_t b0_l =  16384;  
		static int16_t b1_l =  -18727;
		static int16_t b2_l =  6763;

		static int16_t x0_l = 0; 
		static int16_t x1_l = 0;
		static int16_t x2_l = 0;

		static int16_t y0_l = 0;
		static int16_t y1_l = 0;
		static int16_t y2_l = 0;

		x0_l = raw_saz;

		y0_l = (  MUL_SCALED(a0_l, x0_l, 14) 
				+ MUL_SCALED(a1_l, x1_l, 14)
				+ MUL_SCALED(a2_l, x2_l, 14) 
				- MUL_SCALED(b1_l, y1_l, 14) 
				- MUL_SCALED(b2_l, y2_l, 14) 
		       );
		saz = y0_l;

		x2_l = x1_l;
		y2_l = y1_l;

		x1_l = x0_l;
		y1_l = y0_l;
	}


}