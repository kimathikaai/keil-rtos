/*
 * Default main.c for rtos lab
 * @author Kimathi, William, 2018
 */

#include "ourRtos.h"
#include "testSuite.h"

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
		//test_three();
		test_four();
}
