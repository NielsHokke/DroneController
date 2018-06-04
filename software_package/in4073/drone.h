
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

#define GLOBALSTATE parameters[P_MODE]


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

void motors_off();
void manual_control();
void dmp_control(bool yaw_only);

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
#define P_MODE 7
#define P_P_YAW 11
#define PARAMETER_ARRAY_SIZE 12
uint8_t parameters[PARAMETER_ARRAY_SIZE];



#endif // DRONE_H__