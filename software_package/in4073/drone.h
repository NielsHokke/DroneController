
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
#define CALIBRATION_ROUNDS 100
void calibrate(bool raw);
int calibration_counter;

int16_t phi_offset;
int16_t theta_offset;
int16_t psi_offset;


// Motor Control
#define MAN_PITCH_SCALER 1
#define MAN_ROLL_SCALER 1
#define MAN_YAW_SCALER 1


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
#define GET_FROM_PARA_8(R) parameters[R]
#define GET_FROM_PARA_16(R_H, R_L) (parameters[R_H] << 8) + parameters[R_L]
#define GET_FROM_PARA_32(R_H1, R_H2, R_L1, R_L2) (parameters[R_H1] << 24) + (parameters[R_H2] << 16) + (parameters[R_L1] << 8) + parameters[R_L2]

#define GLOBALSTATE parameters[P_MODE]

#define P_MODE 7

#define P_P_YAW_H 10
#define P_P_YAW_L 11
#define P_YAW_MIN 15
#define P_YAW_MAX 19

#define P_P1_H 22
#define P_P1_L 23
#define P_P2_H 26
#define P_P2_L 27
#define P_ANGLE_MIN 31
#define P_ANGLE_MAX 34


#define PARAMETER_ARRAY_SIZE 36
uint8_t parameters[PARAMETER_ARRAY_SIZE];



#endif // DRONE_H__