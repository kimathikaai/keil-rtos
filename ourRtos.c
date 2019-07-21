/*
#include <stdint.h>
#include "ourRtos.h"

// Initialization function for RTOS
void RtosInit(){	
	uint32_t *mainStackPointer = 0x0;
	uint32_t stackStart = *mainStackPointer - stackSize*8;
	uint32_t stackAddresses[6]; // Array of the stack pointers [0-5]
	for(int i = 0; i < 6; i++){
		stackAddresses[i] = stackStart+stackSize*i;
	}
	
	//declare TCBs, need to be global variables to be part of data section???
	// Copy Main stack contents to the Process stack of the new main() task and set the MSP to the Main stack base address
}






	QUESTIONS:
*		Getting main stack base address? How do I access Vector 0 of the Vector Table at 0x0?
			- __get_MSP() gives the top of the main stack (stack pointer)
			- void *mainStackPointer = 0x0; zero index is the base address of the main stack
*		- What is the process stack of the new main() task? It's stack?
*		- Does main become e.g. task 0 with stack 0?
			- Yes
*		How do you decrement address based on stack size e.g. 2 kiB?\
			- uint32_t *mainStackPointer = 0x0;
			- uint32_t stackStart = *mainStackPointer - stackSize*8;
*/

/* HELPER CODE
	int big[5] = {1,2,3,4,5};
	printf("%d\n",__get_MSP());
	void *mainStackPointer = 0x0;
	uint32_t addy = *(uint32_t*)mainStackPointer;
	printf("%d\n",addy);
*/
