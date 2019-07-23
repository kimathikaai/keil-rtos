#include "ourRtos.h"

// VARIABLE DECLARATION
tcb_t *blockList;
tcb_t *running;
uint32_t msTicks = 0;
bool setNewRunning = false;
bool print = false;
tcb_t *readyList[6];
tcb_t tcb[6];

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
		printf("REMOVE READYLIST: FAILED b/c list empty. Task '%d', Priority '%d'\n",tcb->taskId, tcb->taskPriority);
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

// SET CURRENT RUNNING TCB
bool set_running(tcb_t *tcb){
	tcb->taskState = Running;
	running = tcb;
	if(running->taskId != tcb->taskId)
		printf("^FAILED\n");
	return 1;
}

// SEMAPHORE CREATE
sem_t newSem(int initialCount){
	sem_t sem;
	sem.s = initialCount;
	sem.waitList = NULL;
	return sem;
}

// SEMAPHORE WAIT
void wait(sem_t* sem){
	__disable_irq();
	sem->s-=1;
	if(sem->s < 0){
		// Block task that is trying to call wait (running task)
		printf("Blocking task %d\n", running->taskId);
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

// SEMAPHORE SIGNAL
void signal(sem_t * sem){
	__disable_irq();
	sem->s+=1;
	if(sem->s <= 0){
		// Unblock a task
		if(sem->waitList == NULL)	printf("no tasks to unblock\n");	// This should never happen prob?
		else{
			tcb_t * temp = sem->waitList;
			sem->waitList = sem->waitList->next;
			add_to_readyList(temp);
		}		
	}
	__enable_irq();
}

// MUTEX CREATE
mutex_t newMutex(void){
	mutex_t mutex;
	mutex.count = 1;
	mutex.waitList = NULL;
	mutex.ownerId = -1;	// default value for ownerless mutex
	return mutex;
}

// MUTEX AQUIRE
void mutexAcquire(mutex_t *mutex){	// Maybe make return type int so we know when errors occur, instead of just print statements?
	__disable_irq();
	mutex->count-=1;
	if(mutex->count == 0)
		mutex->ownerId = running->taskId;
	else if(mutex->count < 0){
		// Block task that is trying to call wait (running task)
		printf("Blocking task %d\n", running->taskId);
		// first find tail of semaphore wait list
		if(mutex->waitList == NULL){
			mutex->waitList = running;
		}
		else{
			tcb_t * curr = mutex->waitList;
			while(curr->next != NULL){
				curr = curr->next;
			}
			curr->next = running;
		}
		running->next = NULL;
		running->taskState=Blocked;
		
		// Implement Priority inheritance here: set the task that has the mutex to the same priority of this task. After setting setNewRunning, the mutex owner will hopefully run and release the mutex.
		if(mutex->ownerId == running->taskId)	printf("ERROR: Mutex owner is trying to aquire mutex again");	// Sanity check, this shouldn't happen
		if(running->taskPriority > tcb[mutex->ownerId].taskPriority){
			// Upgrade the priority of the mutex owner TEMPORARILY (to the priority of the task currently being blocked or the priority of the highest priority task on the waitList? look at class notes)
			printf("Upgrade Priority of task %d\n", mutex->ownerId);
		}
		else
			printf("Possible error: running task has lower priority than than the mutex-owning task");	// Probably shouldn't happen?
		
		// running task is now in the waitlist, but still technically running. Set setNewRunning so that during the next systick a new task will run.
		setNewRunning = true;
	}
	else
		printf("ERROR: mutex count was larger than one before decrementing\n");
	__enable_irq();
}

// MUTEX RELEASE
int mutexRelease(mutex_t *mutex){
	__disable_irq();
	if(running->taskId != mutex->ownerId){
		printf("Running task is not the mutex owner, unable to release!\n");
		__enable_irq();
		return -1;	// Use this return value in application code somehow?
	}
		
	mutex->count+=1;
	if(mutex->count <= 0){
		// Unblock a task
		if(mutex->waitList == NULL)	printf("no tasks to unblock");	// This should never happen prob?
		else{
			tcb_t * temp = mutex->waitList;
			mutex->waitList = mutex->waitList->next;
			add_to_readyList(temp);
			// Set the newly unblocked task as the mutex owner
			mutex->ownerId = temp->taskId;
		}		
	}
	else if(mutex->count == 1){
		// mutex is now ownerless
		mutex->ownerId = -1;
	}
	else{
		printf("ERROR:mutex count is greater than 1");
	}
	__enable_irq();
	return 0;
}

// RTOS INITIALIZATION
void RtosInit(void){
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
		__disable_irq();
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
						__enable_irq();
            return 0;
        }
    }
    // No tcbs available, unable to create task	
		printf("FAILED CREATING NEW TASK\n");
		__enable_irq();
    return -1;
}

// IS THE TIMESLICE COMPLETE?
bool timeSliceDone(tcb_t *tcb){
	return tcb->time >= TIMESLICE;
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
