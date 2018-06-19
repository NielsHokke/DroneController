
#ifndef DRONE_H__
#define DRONE_H__

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"

#include <inttypes.h> // for the macros


// Global Variables

#define S_SAFE 			0
#define S_PANIC 		1
#define S_MANUAL 		2
#define S_CALIBRATION	3
#define S_YAW_CONTROL	4
#define S_FULL_CONTROLL	5
#define S_RAW_MODE		6
#define S_HEIGHT_CTRL	7
#define S_WIRELESS		8


// Calibration
#define CALIBRATION_ROUNDS 255
void calibrate(bool raw);
int calibration_counter;

int16_t phi_offset;
int16_t theta_offset;
int16_t psi_offset;


// Motor Control
#define MAN_PITCH_SCALER 1
#define MAN_ROLL_SCALER 1
#define MAN_YAW_SCALER 1

void motors_off();
void manual_control();
void dmp_control(bool yaw_only);

#define PANIC_ACTIVE //the loop may go to panic mode
bool flag_gotosafemode; // a flag to prevent state changes in panic mode
void panic();


typedef struct {
	int8_t yaw;
	int8_t pitch;
	int8_t roll;
	uint8_t lift;
} setpoint;

setpoint SetPoint;


// UART
#define CTRL_DATA_LENGTH 4
#define PARA_DATA_LENGTH 5
#define PARA_MSG_QUEUE_SIZE 10

enum SerialStates{
	IDLE,
	CTRL,
	PARA
};
enum SerialStates serialstate;

void validate_ctrl_msg(void *);
void validate_para_msg(void *);

TimerHandle_t UartTimeoutHandle;



// Global parameters
#define GET_PARA_8(R) parameters[R]
#define GET_PARA_16(R) (( (uint16_t) parameters[R] << 8) + (uint16_t) parameters[R + 1])
#define GET_PARA_32(R) (( (uint32_t) parameters[R] << 24) + (uint32_t) (parameters[R + 1] << 16) + (uint32_t) (parameters[R + 2] << 8) + (uint32_t) parameters[R + 3])

#define GLOBALSTATE parameters[P_MODE]

#define P_MIN_LIFT 0
#define P_MAX_RPM 1
#define P_MODE 7

#define P_P_YAW	  10
#define P_ANGLE_MAX 12
#define P_ANGLE_MIN 13
#define P_YAW_MAX 14
#define P_YAW_MIN 15

#define P_P1 16
#define P_P2 18



#define PARAMETER_ARRAY_SIZE 36
uint8_t parameters[PARAMETER_ARRAY_SIZE];



#endif // DRONE_H__