
#ifndef DRONE_H__
#define DRONE_H__

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"
#include "rtos_timers.h"


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


// Global parameters

#define P_MODE 3
#define PARAMETER_ARRAY_SIZE 4
uint8_t parameters[PARAMETER_ARRAY_SIZE];



#endif // DRONE_H__