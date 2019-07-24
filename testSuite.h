#ifndef __TESTSUITE_H
#define __TESTSUITE_H

#include "ourRtos.h"

extern sem_t semOne;

/*****************************************************************************
**                           TASK FUNCTIONS
******************************************************************************/
void function_one(void *arg);
void function_two(void *arg);
void function_three(void *arg);
void function_four(void *arg);
void function_five(void *arg);
void function_six(void *arg);
void function_seven(void *arg);
/*****************************************************************************
**                           TASK INITIALIZATION
******************************************************************************/
void test_default(void);
void test_one(void);	// context switching
void test_two(void);	// FPP scheduling
void test_three(void);	// blocking semaphores (also shows FPP)
void test_four(void);	// Mutex with owner test on release
void test_five(void);	// Mutex Priority Inheritance (Mars rover problem)

#endif /* end __TESTSUITE_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
