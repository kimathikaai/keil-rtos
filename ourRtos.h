#ifndef __OURTOS_H
#define __OURTOS_H

#include <LPC17xx.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "context.h"

#define STACKSIZE 1024
#define TIMESLICE 300

extern uint32_t msTicks;
extern bool setNewRunning;
extern bool print;
typedef void (*rtosTaskFunc_t)(void *args); // Typedef to define pointer type. rtosTaskFunc_t is the type name for a pointer to a function that takes a void * parameter and returns void.

/*****************************************************************************
**                            ENUMS
******************************************************************************/
typedef enum{
	Ready,
	Running,
	Blocked,
	Inactive,
	Terminated
}state_t;

typedef enum{
	Idle = 0,
	Low,
	BelowNormal,
	Normal,
	AboveNormal,
	Realtime,
}priority_t;
/*****************************************************************************
**                           STRUCTS
******************************************************************************/
typedef struct tcb_s{
    int taskId;
    uint32_t stackPointer;
    uint32_t stackBaseAddress;
    rtosTaskFunc_t taskFunction;
    state_t taskState;
    priority_t taskPriority;
		priority_t taskBasePriority;	// Used for mutex priority inheritance
    struct tcb_s *next;
    int time;
}tcb_t;
extern tcb_t *readyList[6];
extern tcb_t tcb[6];
extern tcb_t *blockList;
extern tcb_t *running;

typedef struct{
	int32_t s;
	tcb_t* waitList;
}sem_t;

typedef struct{
	int32_t count;
	tcb_t* waitList;
	int16_t ownerId;
}mutex_t;

/*****************************************************************************
**                            FUNCTION DECLARATIONS
******************************************************************************/
void RtosInit(void);
int CreateTask(rtosTaskFunc_t taskFunc, priority_t priority, void* funcArg);
bool add_to_readyList(tcb_t *tcb);
bool remove_from_readyList(tcb_t *tcb);
bool set_running(tcb_t *tcb);
sem_t newSem(int initialCount);
void wait(sem_t* sem);
void signal(sem_t * sem);
mutex_t newMutex(void);
void mutexAcquire(mutex_t *mutex);
int mutexRelease(mutex_t *mutex);
bool timeSliceDone(tcb_t *tcb);
tcb_t * next_task(void);
void update(void);
void SysTick_Handler(void);
void PendSV_Handler(void);

#endif /* end __RTOS_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
