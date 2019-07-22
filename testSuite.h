#ifndef __TESTSUITE_H
#define __TESTSUITE_H

#include "ourRtos.h"

extern sem_t semOne;

/*****************************************************************************
**                           TASK FUNCTIONS
******************************************************************************/
void function_one(void *arg);
void function_two(void *arg);

/*****************************************************************************
**                           TASK INITIALIZATION
******************************************************************************/
void test_default(void);
void test_one(void);
void test_two(void);
void test_three(void);

#endif /* end __TESTSUITE_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
