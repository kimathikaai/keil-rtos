#include "testSuite.h"

/*****************************************************************************
**                           TASK FUNCTIONS
******************************************************************************/
// Variables
sem_t semOne;

void function_one(void *arg){
	uint32_t i = (uint32_t)arg;
	while(true){
		printf("FUNCTION ONE OUTPUT: (Task %d)\n",i);
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
	bool task1Created = false;
	semOne = newSem(1);
	
	printf("OUTPUT: Trying to aqcuire semaphore (Task %d)\n", 5);
	wait(&semOne);
	printf("OUTPUT: Semaphore aquired (Task %d)\n", 5);	

	while(true){

		if(msTicks > 2000 && !task0Created){
			// Create task of higher priority that tries to aquire semaphore
			CreateTask(&function_two, (priority_t)4, (void *)0);
			task0Created = true;
		}
		
		if(msTicks > 4000 && !task1Created){
			printf("OUTPUT: Signal sempaphore (Task %d)\n", 5);	
			signal(&semOne);
		}
		
		printf("FUNCTION TASK OUTPUT: (Task %d)\n", 5);
	}
}

