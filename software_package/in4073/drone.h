
#ifndef DRONE_H__
#define DRONE_H__

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"

#include <inttypes.h> // for the macros


// Global Variables

#define S_SAFE 			0
#define S_MANUAL 		1
#define S_CALIBRATION	2
#define S_YAW_CONTROL	3
#define S_FULL_CONTROLL	4
#define S_RAW_MODE_1	5
#define S_RAW_MODE_2	6
#define S_RAW_MODE_3	7
#define S_GlobalState	8
#define S_PANIC 		255

#define GLOBALSTATE parameters[3]


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

#define P_MODE 3
#define PARAMETER_ARRAY_SIZE 4
uint8_t parameters[PARAMETER_ARRAY_SIZE];



#endif // DRONE_H__