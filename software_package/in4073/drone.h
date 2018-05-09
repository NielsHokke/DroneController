
#ifndef DRONE_H__
#define DRONE_H__

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"

#include <inttypes.h> // for the macros


// Global Variables

enum state {CALIBRATION, SAFE, PANIC, MANUAL, YAW_CONTROL, FULL_CONTROLL, RAW_MODE_1, RAW_MODE_2, RAW_MODE_3} GlobalState;


// Motor Control
#define MAN_PITCH_SCALER 10 //
#define MAN_ROLL_SCALER 10 //


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


TaskHandle_t validate_ctrl_msg_Handle;
TaskHandle_t validate_para_msg_Handle;





#endif // DRONE_H__