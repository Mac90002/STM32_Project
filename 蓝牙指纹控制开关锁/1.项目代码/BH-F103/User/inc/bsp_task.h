#ifndef __BSP_TASK_H
#define __BSP_TASK_H

#define START_TASK_PRIO         1
#define START_TASK_STACK_SIZE   128

/* TASK1 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */#define TASK1_PRIO       2
#define TASK1_STACK_SIZE   128


/* TASK2 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK2_PRIO         3
#define TASK2_STACK_SIZE   128

/* TASK3 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK3_PRIO         4
#define TASK3_STACK_SIZE   128



void OS_Task(void);

#endif /* __BSP_TASK_H */



