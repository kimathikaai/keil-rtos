#include "testSuite.h"

/*****************************************************************************
**                           TASK FUNCTIONS
******************************************************************************/
// Variables
sem_t semOne;
mutex_t mutexOne;

void function_one(void *arg){
	uint32_t i = (uint32_t)arg;
	while(true){
		if (!print){printf("FUNCTION ONE OUTPUT: (Task %d)\n",i);}
	}
}
void function_two(void *arg){
	uint32_t i = (uint32_t)arg;
	printf("OUTPUT: Trying to aqcuire semaphore (Task %d)\n", i);
	wait(&semOne);
	while(1){
		function_one(arg);
	}	
}

void function_three(void *arg){
	bool semAcquired = false;
	bool task1Created = false;
	uint32_t i = (uint32_t)arg;
	
	while(1){
		if(msTicks > 3000 && !semAcquired){
			printf("OUTPUT: Trying to aqcuire semaphore (Task %d)\n", arg);
			wait(&semOne);
			semAcquired = true;
		}
		
		if(msTicks > 6000 && !task1Created){
			CreateTask(&function_one, (priority_t)4, (void *)1);
			task1Created = true;
		}
		
		printf("FUNCTION TASK OUTPUT: (Task %d)\n", arg);
	}	
}
void function_four(void*arg){
	bool mutexAcquired = false;
	bool task1Created = false;
	bool mutexReleased = false;
	mutexOne = newMutex();

	while(true){
		
		if(msTicks > 1000 && !mutexAcquired){
			__disable_irq();
			printf("OUTPUT: Trying to aqcuire mutex (Task %d)\n", running->taskId);
			mutexAcquire(&mutexOne);
			printf("OUTPUT: Mutex aquired (Task %d)\n", running->taskId);
			mutexAcquired = true;
			__enable_irq();
		}			

		if(msTicks > 2000 && !task1Created){
			// Create task of higher priority that tries to release mutex, and then acquire it
			CreateTask(&function_five, (priority_t)5, (void *)1);
			task1Created = true;
		}
		
		if(msTicks > 5000 && !mutexReleased){
			__disable_irq();
			printf("OUTPUT: Trying to release mutex (Task %d)\n", running->taskId);
			if(mutexRelease(&mutexOne)==0)
				printf("OUTPUT: Mutex released (Task %d)\n", running->taskId);
			else
				printf("OUTPUT: Unable to release mutex (Task %d)\n", running->taskId);
			mutexReleased = true;
			__enable_irq();
		}
		
		if(mutexOne.ownerId<0)	printf("FUNCTION TASK OUTPUT: (Task %d), mutex is unowned\n", running->taskId);
		else	printf("FUNCTION TASK OUTPUT: (Task %d), task %d is the owner of the mutex\n", running->taskId, mutexOne.ownerId);
	}
}
void function_five(void*arg){
	bool triedReleasingMutex = false;
	bool triedAcquiringMutex = false;
	
	while(true){
		
		if(msTicks > 3000 && !triedReleasingMutex){
			__disable_irq();
			printf("OUTPUT: Trying to release mutex (Task %d)\n", running->taskId);
			if(mutexRelease(&mutexOne)==0)
				printf("OUTPUT: Mutex released (Task %d)\n", running->taskId);
			else
				printf("OUTPUT: Unable to release mutex (Task %d)\n", running->taskId);
			triedReleasingMutex = true;
			__enable_irq();
		}			

		if(msTicks > 4000 && !triedAcquiringMutex){
			__disable_irq();
			printf("OUTPUT: Trying to aqcuire mutex (Task %d)\n", running->taskId);
			mutexAcquire(&mutexOne);
			printf("OUTPUT: Mutex aquired (Task %d)\n", running->taskId);
			triedAcquiringMutex = true;
			__enable_irq();
		}		
		
		if(mutexOne.ownerId<0)	printf("FUNCTION TASK OUTPUT: (Task %d), mutex is unowned\n", running->taskId);
		else	printf("FUNCTION TASK OUTPUT: (Task %d), task %d is the owner of the mutex\n", running->taskId, mutexOne.ownerId);
	}
}


/*****************************************************************************
**                           TASK INITIALIZATION
******************************************************************************/
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
	CreateTask(&function_one, (priority_t)3, (void *)0);
	CreateTask(&function_one, (priority_t)3, (void *)1);
	CreateTask(&function_one, (priority_t)3, (void *)2);
	CreateTask(&function_one, (priority_t)3, (void *)3);	
	CreateTask(&function_one, (priority_t)3, (void *)4);
	CreateTask(&function_one, (priority_t)3, (void *)5); // Shouldn't create this task
	while(true){
		function_one((void*)5);
	}
}

void test_two(void){	
	/* FPP SCHEDULING
	* Lower (than main) Priority Task should never run
	* Same (than main) Priority Task should alternate with main
	* Higher (than main) priority Task should preempt main task
	*/
	bool task0Created = false;
	bool task1Created = false;
	bool task2Created = false;
	
	while(true){
		if(msTicks > 2000 && !task0Created){
			CreateTask(&function_one, (priority_t)2, (void *)0);
			task0Created = true;
		}
			
		if(msTicks > 4000 && !task1Created){
			CreateTask(&function_one, (priority_t)3, (void *)1);
			task1Created = true;
		}
			
		if(msTicks > 6000 && !task2Created){
			CreateTask(&function_one, (priority_t)4, (void *)2);
			task2Created = true;
		}
		printf("FUNCTION TASK OUTPUT: (Task %d)\n", 5);
	}
}

void test_three(void){
	bool task0Created = false;
	bool semReleased = false;
	semOne = newSem(1);
	
	printf("OUTPUT: Trying to aqcuire semaphore (Task %d)\n", 5);
	wait(&semOne);
	printf("OUTPUT: Semaphore aquired (Task %d)\n", 5);	

	while(true){

		if(msTicks > 2000 && !task0Created){
			// Create task of higher priority that tries to aquire semaphore
			CreateTask(&function_three, (priority_t)4, (void *)0);
			task0Created = true;
		}
		
		if(msTicks > 4000 && !semReleased){
			printf("OUTPUT: Signal sempaphore (Task %d)\n", 5);	
			signal(&semOne);
			semReleased = true;
		}
		
		printf("FUNCTION TASK OUTPUT: (Task %d)\n", 5);
	}
}

void test_four(void){	// this test case doesn't really use the main task for testing
	CreateTask(&function_four, (priority_t)4, (void *)0);
	while(true){
		printf("FUNCTION TASK OUTPUT: (Task %d)\n", 5);
	}
}

