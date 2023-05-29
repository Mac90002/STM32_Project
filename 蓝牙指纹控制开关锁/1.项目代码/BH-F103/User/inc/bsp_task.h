#ifndef __BSP_TASK_H
#define __BSP_TASK_H

#define START_TASK_PRIO         1
#define START_TASK_STACK_SIZE   128

#define TASK1_PRIO         2
#define TASK1_STACK_SIZE   128

#define TASK2_PRIO         3
#define TASK2_STACK_SIZE   128

#define TASK3_PRIO         4
#define TASK3_STACK_SIZE   128

#define TASK4_PRIO         5
#define TASK4_STACK_SIZE   128

#define TASK5_PRIO         6
#define TASK5_STACK_SIZE   128



void OS_Task(void);

#endif /* __BSP_TASK_H */



