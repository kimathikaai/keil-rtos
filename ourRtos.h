/*
typedef enum{
	Ready,
	Running,
	Blocked,
	Inactive,
	Terminated
}state_t;

typedef enum{
	Idle = 1,
	Low,
	BelowNormal,
	Normal,
	AboveNormal,
	Realtime,
}priority_t;

typedef struct{
	int taskId;
	void * stackPointer;
	state_t taskState;
	priority_t taskPriority;
	void * waitList;
	int time;
}tcb_t;

typedef void (*rtosTaskFunc_t)(void *args);

// Constants
const int stackSize = 1024;
tcb_t tcb0;

// Initialization function for RTOS
void RtosInit();
*/