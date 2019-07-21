/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context.h"
//#include "ourRtos.h"

// Global Variables
#define STACKSIZE 1024
uint32_t msTicks = 0;
uint32_t timeSlice = 200;
bool setNewRunning = false;
bool print = false;
char message[] = "Unchanged";


int runningTaskID;    // Implement so scheduler constantly updates this

typedef void (*rtosTaskFunc_t)(void *args); // Typedef to define pointer type. rtosTaskFunc_t is the type name for a pointer to a function that takes a void * parameter and returns void.

// Enums
typedef enum{
	Ready,
	Running,
	Blocked,
	Inactive,
	Terminated
}state_t;

typedef enum{
	Idle = 0, // We might need and idle task. Main can be the default task
	Low,
	BelowNormal,
	Normal,
	AboveNormal,
	Realtime,
}priority_t;

// STRUCTS
typedef struct{
    uint32_t data[STACKSIZE];
}stackHelper;

typedef struct tcb_s{
    int taskId;
    uint32_t stackPointer;
    uint32_t stackBaseAddress;
    rtosTaskFunc_t taskFunction;
    state_t taskState;
    priority_t taskPriority;
    struct tcb_s *next;
    int time;
}tcb_t;

//declare TCBs, need to be global variables to be part of data section???
tcb_t tcb[6];
tcb_t *readyList[6];
tcb_t *blockList;
tcb_t *running;
priority_t runningPriority;
/* CONCERNS
* Do I need to use malloc for these pointers to initialize them?
*/
/* CONCERNS
* Is this check 'current == tcb' as good as 'current->taskId == tcb->taskId'
* Is 	tcb_t *current = blockList pass by reference or by value?
* Did I make sure to make the element in readyList Null if needed after a remove?
* Every time you do add_to_readyList(running) make sure it does not need to be blocked
*/


/* TEST SUITE
	CONTEXT SWITCHING
	- Demonstrate that a different function runs

	FPP SCHEDULING
	- Main task is normal, make a task with lower priority (Should never switch to lower task)
	- Main task is normal, make a task with higher priority (Should never switch to back to main task)
	- Main task is normal, make a task with same priority (Switches between evenly)

	BLOCKING SEMAPHORES
	
	MUTEXT WITH OWNER TEST AND RELEASE AND
*/

// FPP shit

// ADD TO READYLIST 
bool add_to_readyList(tcb_t *tcb){
		priority_t priority = tcb->taskPriority;
    // Make sure the task is removed from the blocked list
    tcb->taskState = Ready;
    if(readyList[priority] == NULL) {
				tcb->next = NULL;
        readyList[priority] = tcb;
    } 
    else {
        tcb_t *current = readyList[priority];
        while(current->next != NULL){
            current = current->next;
        }
        current->next = tcb;
				tcb->next = NULL;
    }
    if (print){printf("...adding to readyList: Task '%d', Priority '%d'\n",tcb->taskId, tcb->taskPriority);}
		if(tcb->next != NULL){
			printf("^FAIL\n");
			return 0;
		}
    return 1;
}

// REMOVE FROM READYLIST
bool remove_from_readyList(tcb_t *tcb) {
	/* CHECKING FOR
	* Case 1: Is list Empty
	* Case 2: tcb is first node
	* Case 3: tcb is middle node
	* Case 4: tcb is leaf node
	* CASE 5 NOT DONE: tcb is the second node
	*/
	
	if(readyList[tcb->taskPriority] == NULL){ // Should not be empty
		printf("REMOVE READYLIST: FAILED b/c list empty. Task '%d', Priority '%d'",tcb->taskId, tcb->taskPriority);
		return 0;
	}
	else if(readyList[tcb->taskPriority]->taskId == tcb->taskId){ // Is the tcb the first element in list?
		readyList[tcb->taskPriority] = tcb->next;
	}
	else{
		tcb_t *current = readyList[tcb->taskPriority];
		while(current->next->taskId != tcb->taskId){ // Does not confirm that the tcb is in the list
			current = current->next;
		}
		current->next = tcb->next;
	}
	tcb->next = NULL;
  if (print){printf("...removing from readyList: Task '%d', Priority '%d'\n",tcb->taskId, tcb->taskPriority);}
	return 1;
}


// ADD TO BLOCKLIST
bool add_to_blockList(tcb_t *tcb){
	tcb->taskState = Blocked;
	if(blockList == NULL){
		blockList = tcb;
		blockList->next = NULL;
	} 
	else {
		tcb_t *current = blockList;
		while(current->next != NULL){
				current = current->next;
		}
		current->next = tcb;
	}
    printf("ADD BLOCKLIST: Task '%d', Priority '%d'\n",tcb->taskId, tcb->taskPriority);
		if(tcb->next != NULL){
			printf("^FAIL\n");
			return 0;
		}
    return 1;
}


// REMOVE FROM BLOCKLIST 
bool remove_from_blockList(tcb_t *tcb){
	// List shouldn't be empty
	if(blockList == NULL){
		printf("REMOVE BLOCKLIST: FAILED b/c list empty. Task '%d', Priority '%d'",tcb->taskId, tcb->taskPriority);
		return 0;
	}
	else if( blockList->taskId == tcb->taskId ){
		blockList = tcb->next;
	}
	else {
		tcb_t *current = blockList;
		while(current->next->taskId != tcb->taskId){ // Does not confirm that the tcb is in the list
			current = current->next;
		}
		current->next = tcb->next;
	}
	printf("REMOVE BLOCKLIST: Task '%d', Priority '%d'\n",tcb->taskId, tcb->taskPriority);
	return 1;
}


// SET CURRENT RUNNING TCB
bool set_running(tcb_t *tcb){
	tcb->taskState = Running;
	running = tcb;
	runningPriority = tcb->taskPriority;
	if(running->taskId != tcb->taskId || runningPriority != tcb->taskPriority)
		printf("^FAILED\n");
	return 1;
}



// Semaphore shit
typedef struct{
	int32_t s;
	tcb_t* waitList;
}sem_t;

sem_t newSem(int initialCount){
	sem_t sem;
	sem.s = initialCount;
	sem.waitList = NULL;
	return sem;
}

void wait(sem_t* sem){
	__disable_irq();
	sem->s-=1;
	if(sem->s < 0){
		// Block task that is trying to call wait (running task)
		// first find tail of semaphore wait list
		if(sem->waitList == NULL){
			sem->waitList = running;
		}
		else{
			tcb_t * curr = sem->waitList;
			while(curr->next != NULL){
				curr = curr->next;
			}
			curr->next = running;
		}
		running->next = NULL;
		running->taskState=Blocked;
		
		// running task is now in the waitlist, but still technically running. Set setNewRunning so that during the next systick a new task will run.
		setNewRunning = true;
	}
	__enable_irq();
}

void signal(sem_t * sem){
	__disable_irq();
	sem->s+=1;
	if(sem->s <= 0){
		// Unblock a task
		if(sem->waitList == NULL)	printf("no tasks to unblock");	// This should never happen prob?
		else{
			tcb_t * temp = sem->waitList;
			sem->waitList = sem->waitList->next;
			add_to_readyList(temp);
		}		
	}
	__enable_irq();
}

// Mutex shit
typedef struct{
	int32_t s;
	tcb_t* waitlist;
	tcb_t* owner;
}mutex_t;

// RTOS INITIALIZATION
void RtosInit(){
		printf("INITIALIZING RTOS...\n");
	
    // Initialize each TCB with the base address of its stack.
    uint32_t *mainStackBaseAddress = 0x0;
    uint32_t stackStart = *mainStackBaseAddress - STACKSIZE*7; // Remember that stack base address should be higher or equal to the stack pointer
    uint32_t stackAddresses[6]; // Array of the stack pointers [0-5]
    for(int i = 0; i < 6; i++){
        stackAddresses[i] = stackStart+STACKSIZE*i;
    }
    
    // Initialize TCBs
    for(int i = 0; i < 6; i++){
        tcb[i].taskId = i;
        tcb[i].stackPointer = stackAddresses[i];
        tcb[i].stackBaseAddress = stackAddresses[i];
        tcb[i].taskState = Inactive;
        tcb[i].taskPriority = Normal;
				tcb[i].time = 0;
    }
    
    // Copy the Main stack contents to the Process stack of the new main() task and set the MSP to the Main stack base address
    // Which task should we use for the main() task? assume task 5 for now
    uint32_t mainStackSize = *mainStackBaseAddress - __get_MSP();
    if(mainStackSize <= STACKSIZE)
        memcpy((void*)(tcb[5].stackBaseAddress - mainStackSize), (void*)__get_MSP(), mainStackSize);
    else
        memcpy((void*)(tcb[5].stackBaseAddress - mainStackSize), (void*)__get_MSP(), STACKSIZE);
    
    __set_MSP((uint32_t)*mainStackBaseAddress);
		//__set_MSP(0x10002100);
    
    
    // Switch from using the MSP to the PSP
//		CONTROL_Type control;
//		control.w = __get_CONTROL();
//		control.b.SPSEL = 1;
//		__set_CONTROL(control.w);
    __set_CONTROL(__get_CONTROL() | 0x02);
    __set_PSP((uint32_t)tcb[5].stackBaseAddress-mainStackSize); //PSP address to top of main() task stack 
		
		// Initialize Main Task
		set_running(&tcb[5]);
		tcb[5].taskState = Running;
		
		printf("COMPLETED INITIALIZING RTOS\n");
}

// CREATE TASK
int CreateTask(rtosTaskFunc_t taskFunc, priority_t priority, void* funcArg){
		printf("CREATING TASK... ");
    // assign new task to a tcb. How to tell if tcb is not already in use? check the tcb.taskState maybe
    for(int i=0; i<6; i++){
        if(tcb[i].taskState == Inactive){
						printf("%d\n",i);
						// Assign task to tcb
            tcb[i].taskFunction = taskFunc;
            tcb[i].taskPriority = priority;
						add_to_readyList(&tcb[i]); // Add task to the readyList
					
            /*
            when you create a new task, you need to initialize its stack according to Table 2.  The only parts that matter are:
            ?R0 – argument to task function
            ?PC – address of task fuction
            ?PSR – default value is 0x01000000
            However the other parts (R4-R11, R1-R3,R12,LR) must exist so that the context pop works correctly
            */
            for(int j = 0; j<16; j++){
								tcb[i].stackPointer-=4;    
								// Stack pointer is at base address, decrement by four and leave the base address untouched (populate the 4 bytes on top of it)
								// Remember, pointer needs to point to the very top of the 4 byte register, since when a four byte value is stored at that address the next bytes are stored in higher (increasing) memory locations
                if(j==0){    // PSR
                    *(uint32_t*)(tcb[i].stackPointer) = 0x01000000;            
                }
                else if(j==1){    // PC
                    *(rtosTaskFunc_t*)(tcb[i].stackPointer) = taskFunc;
                }
                else if(j==7){    // R0
                    *(void**)(tcb[i].stackPointer) = funcArg;    // assume we pass the address of the argument, not the argument itself
                }
                else{
                    *(uint32_t*)(tcb[i].stackPointer) = 0;    // initialize other registers as zero i guess
                }
            }
            
            // Successful, return 0
						//runnn the schedulerrr!!! set pendsv bit to 1. edit, 
						printf("COMPLETED CREATING NEW TASK\n");
            return 0;
        }
    }
    // No tcbs available, unable to create task	
		printf("FAILED CREATING NEW TASK\n");
    return -1;
}
// IS THE TIMESLICE COMPLETE?
bool timeSliceDone(tcb_t *tcb){
	return tcb->time >= timeSlice;
}

// NEXT TASK
tcb_t * next_task(void){
	for(int i = 5; i >=0; i--){
		if( readyList[i] != NULL ){
			if (print){printf("...next task = %d\n", readyList[i]->taskId);}
			return readyList[i];
		}
	}
	if (print){printf("...next task = %d\n", running->taskId);}
	return running;
}

// UPDATE
void update(void){
	// Will need to update the block list and ready list
	// update the time of the running task
	running->time += 1;
	if(print){printf("...running: Tid = %d, t = %d, P = %d\n",running->taskId, running->time, running->taskPriority);}
}

// SYSTICK_HANDLER
void SysTick_Handler(void) {
    msTicks++;
		update();
		// set the PENDSVSET bit of the ISCR
		SCB->ICSR |= 1<<28;		
}

// PENDSV_HANDLER
void PendSV_Handler(void){
		//__disable_irq();
		// when the Cortex-M3 enters an exception handler it pushes these eight registers on the stack R0-R3,R12,LR,PC,PSR
		// when the Cortex-M3 exits the exception handler it pops from the stack into those registers (R0-R3,R12,LR,PC,PSR)
		if(running == NULL)	return;	// if running is null, still have not completed initialization, return
		//printf("%d",running->taskId);
		running->stackPointer = storeContext();
		//printf("\nstoreContext");
	
		
		// START OF SCHEDULER ---------------------------------------------------------------------------------------------
		__disable_irq();
	
		tcb_t *nextTask = next_task();
		
		if(setNewRunning){	// Running task was blocked, need to set a new running task no matter what priority
			running->time = 0;
			set_running(nextTask);	// Haven't dealt with the edge case where the running task that was blocked is the only task, and nextTask returns the original running task
			remove_from_readyList(nextTask);
			setNewRunning = false;
		}
		else{
			if(running->taskPriority < nextTask->taskPriority){ //  No changes if running->taskPriority >= findHighestPriority()
				running->time = 0;
				add_to_readyList(running); // What if we want to block it -> Do this at this point
				set_running(nextTask);
				remove_from_readyList(nextTask);
				//	Kimathi should we Set running->next to null?
			}
			else if (timeSliceDone(running)){
				running->time = 0;
				if(nextTask != running && nextTask->taskPriority >= running->taskPriority){
					add_to_readyList(running); // What if we want to block it -> Do this at this point
					set_running(nextTask);
					remove_from_readyList(nextTask);
				}
			}
		}		
		
		// for testing hard coded context switch to testtaskone:
		//running = &tcb[0];
		
		__enable_irq();
		// END OF SCHEDULER ---------------------------------------------------------------------------------------------

	  __set_PSP(running->stackPointer); // Running task is updated 
    restoreContext(__get_PSP());
		SCB->ICSR &= ~(1<<28); // reset the PENDSVSET bit
		//__enable_irq();

}


// Global variable used for test_three
sem_t semOne;
// TEST TASK FUNCTIONS
void functionOne(void *arg){
	uint32_t i = (uint32_t)arg;
	while(true){
		printf("FUNCTION OUTPUT: Task '%d'\n",i);
	}
}
//void function_two(void *arg){
//	char 
//	while(true){
//		printf("FUNCTION OUTPUT: Task '%d'\n",i);
//	}
//}

void function_semaphore_one_aquire(void *arg){
	uint32_t i = (uint32_t)arg;
	printf("try to aqcuire semaphore, %d", i);
	wait(&semOne);
	while(1){
		functionOne((void*)0);
	}	
}

// TEST SUITE CODE
void test_default(void){
	  uint32_t period = 1000; // 1s
    uint32_t prev = -period;
		while(true) {
		if((uint32_t)(msTicks - prev) >= period) {
			printf("tick ");
			prev += period;
		}
	}   
}
void test_one(void){
	/* CONTEXT SWITCHING
	* Should switch at consistent intervals b/c priorities are the same
	*/
	CreateTask(&functionOne, 3, (void *)0);
	CreateTask(&functionOne, 3, (void *)1);
	CreateTask(&functionOne, 3, (void *)2);
	CreateTask(&functionOne, 3, (void *)3);	
	CreateTask(&functionOne, 3, (void *)4);
	CreateTask(&functionOne, 3, (void *)5); // Shouldn't create this task
	while(true){
		functionOne((void*)5);
	}
}

void test_two(void){
	//uint32_t delay = 3;
	bool task0Created = false;
	bool task1Created = false;
	bool task2Created = false;
	
	/* FPP SCHEDULING
	* Lower (than main) Priority Task should never run
	* Same (than main) Priority Task should alternate with main
	* Higher (than main) priority Task should preempt main task
	*/
	
	while(true){
		if(msTicks > 2000 && !task0Created){
			CreateTask(&functionOne, 2, (void *)0);
			task0Created = true;
		}
			
		if(msTicks > 4000 && !task1Created){
			CreateTask(&functionOne, 3, (void *)1);
			task1Created = true;
		}
			
		if(msTicks > 6000 && !task2Created){
			CreateTask(&functionOne, 4, (void *)2);
			task2Created = true;
		}
		
		printf("MAIN TASK OUTPUT: Task '%d'\n", 5);
	}
}

void test_three(void){
	bool task0Created = false;
	bool task1Created = false;
	semOne = newSem(1);
	// Aquire Semaphore
	wait(&semOne);
	printf("main task acquire semaphore");
	
	while(true){
		if(msTicks > 2000 && !task0Created){
			// Create task of higher priority that tries to aquire semaphore
			CreateTask(&semaphoreTryAquire, 4, (void *)0);
			task0Created = true;
		}
		
		if(msTicks > 4000 && !task1Created){
			printf("signal semaphore");
			signal(&semOne);
		}
		
		printf("MAIN TASK OUTPUT: Task '%d'\n", 5);
	}
}
	

//  MAIN
int main(void){
	  printf("\n-------------------------------------------------\n");
    printf("------------------STARTING RTOS------------------\n");
		printf("-------------------------------------------------\n");

		RtosInit();
		SysTick_Config(SystemCoreClock/1000);
		
		// TEST SUITE (only run one at a time per reset lol)
		//test_default();
		//test_one();
		//test_two(); 
		test_three();
}

/*  QUESTIONS:
*       Getting main stack base address? How do I access Vector 0 of the Vector Table at 0x0?
            - __get_MSP() gives the top of the main stack (stack pointer)
            - void *mainStackPointer = 0x0; zero index is the base address of the main stack
*       - What is the process stack of the new main() task? It's stack?
*       - Does main become e.g. task 0 with stack 0?
            - Yes
*       How do you decrement address based on stack size e.g. 2 kiB?\
            - uint32_t *mainStackPointer = 0x0;
            - uint32_t stackStart = *mainStackPointer - stackSize*8;
         
         Should a stack pointer point to the top value of the stack or the address after the top value?
         -It seems like the top value from table 2 in the spec, however what about when the stack is empty/has not been initialized? then the stack pointer points to the base address, even thought there is nothing at the base address.
*/

/* HELPER CODE
    int big[5] = {1,2,3,4,5};
    printf("%d\n",__get_MSP());
    void *mainStackPointer = 0x0;
    uint32_t addy = *(uint32_t*)mainStackPointer;
    printf("%d\n",addy);
		
		// FIND THE HIGHES PRIORITY TASK
tcb_t * findHighestPriorityTask(void){
	tcb_t * current = running;
	
	for(int i = running->taskPriority; i < 6; i++){
		if(current->taskPriority < readyList[i]->taskPriority){
			current = readyList[i];
		}
	}
	return current;
}
*/
