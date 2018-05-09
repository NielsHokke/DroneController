
#ifndef DRONE_H__
#define DRONE_H__

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