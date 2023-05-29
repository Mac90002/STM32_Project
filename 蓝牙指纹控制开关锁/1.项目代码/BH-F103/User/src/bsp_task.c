#include "bsp_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>
#include "bsp_led.h"
#include "bsp_key.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "bsp_SysTick.h"
#include "bsp_ili9341_lcd.h"
#include "bsp_hc05.h"
#include "bsp_usart.h"
#include "timers.h"
#include "bsp_hc05_usart.h"
#include "bsp_as608.h"

extern volatile uint16_t uart_p;
extern uint8_t uart_buff[UART_BUFF_SIZE];
extern uint16_t ID;
void timer2Callback(TimerHandle_t pxTimer);

char hc05_nameCMD[40];
char disp_buff[200];
char hc05_name[30] = "HC05_Module";

uint8_t Clear_Message = 0;
TaskHandle_t start_task_handler;
QueueHandle_t semaphore_handle;
QueueHandle_t semaphore_handle1;
QueueHandle_t semaphore_handle2;
TimerHandle_t tim2_handle = 0; /*周期定时器*/

void start_task(void *pvParameters);

TaskHandle_t task1_handler;
void task1(void *pvParameters);

TaskHandle_t task2_handler;
void task2(void *pvParameters);

TaskHandle_t task3_handler;
void task3(void *pvParameters);

TaskHandle_t task4_handler;
void task4(void *pvParameters);

TaskHandle_t task5_handler;
void task5(void *pvParameters);

uint32_t i = 0;
static uint32_t timer = 0;

void OS_Task(void)
{
  ILI9341_Clear(0, 0, 240, 320);

  if (HC05_Init() == 0)
  {
    printf("HC05 Succeed Connect\r\n");
    ILI9341_DispString_EN(20, 0, "HC05 Succeed Connect");
  }
  else
  {
    printf("HC05 Failed Connect\r\n");
    ILI9341_DispString_EN(20, 0, "HC05 Failed Connect");
    ILI9341_DispString_EN(20, 20, "Please Cheak Connect");
  }

  HC05_Send_CMD("AT+INIT\r\n", 1);        // 初始化SPP库
  HC05_Send_CMD("AT+CLASS=0\r\n", 1);     // 查询各种蓝牙设备
  HC05_Send_CMD("AT+INQM=1,9,48\r\n", 1); // 超过9个蓝牙设备响应则中止查询

  sprintf(hc05_nameCMD, "AT+NAME=%s\r\n", hc05_name);
  HC05_Send_CMD(hc05_nameCMD, 1);

  sprintf(disp_buff, "Device name:%s", hc05_name);
  ILI9341_DispString_EN(20, 20, disp_buff);
  
  xTaskCreate((TaskFunction_t)start_task,
              (char *)"start_task",
              (uint16_t)START_TASK_STACK_SIZE,
              (void *)NULL,
              (UBaseType_t)START_TASK_PRIO,
              (TaskHandle_t *)&start_task_handler);
  vTaskStartScheduler();
}

void start_task(void *pvParameters)
{
  AS608_Connect_Test();
  semaphore_handle = xSemaphoreCreateBinary();
  semaphore_handle1 = xSemaphoreCreateBinary();
  semaphore_handle2 = xSemaphoreCreateBinary();
  if (semaphore_handle != NULL)
  {
    printf("binary create susceed\n");
  }
  if (semaphore_handle1 != NULL)
  {
    printf("binary1 create susceed\n");
  }
  if (semaphore_handle2 != NULL)
  {
    printf("binary2 create susceed\n");
  }
  tim2_handle = xTimerCreate("tim2", 1000, pdTRUE, (void *)2, timer2Callback);
  xTimerStart(tim2_handle, portMAX_DELAY);

  taskENTER_CRITICAL(); // 进入临界区
  xTaskCreate((TaskFunction_t)task1,
              (char *)"task1",
              (uint16_t)TASK1_STACK_SIZE,
              (void *)NULL,
              (UBaseType_t)TASK1_PRIO,
              (TaskHandle_t *)&task1_handler);

  xTaskCreate((TaskFunction_t)task2,
              (char *)"task2",
              (uint16_t)TASK2_STACK_SIZE,
              (void *)NULL,
              (UBaseType_t)TASK2_PRIO,
              (TaskHandle_t *)&task2_handler);

  xTaskCreate((TaskFunction_t)task3,
              (char *)"task3",
              (uint16_t)TASK3_STACK_SIZE,
              (void *)NULL,
              (UBaseType_t)TASK3_PRIO,
              (TaskHandle_t *)&task3_handler);

  xTaskCreate((TaskFunction_t)task4,
              (char *)"task4",
              (uint16_t)TASK4_STACK_SIZE,
              (void *)NULL,
              (UBaseType_t)TASK4_PRIO,
              (TaskHandle_t *)&task4_handler);

  //   xTaskCreate((TaskFunction_t)task5,
  //              (char *)"task5",
  //              (uint16_t)TASK5_STACK_SIZE,
  //              (void *)NULL,
  //              (UBaseType_t)TASK5_PRIO,
  //              (TaskHandle_t *)&task5_handler);

  vTaskDelete(NULL);
  taskEXIT_CRITICAL(); // 退出临界区
}

void task1(void *pvParameters)
{
  BaseType_t err;

  while (1)
  {
    err = xSemaphoreTake(semaphore_handle, 1000); /*获取信号量并等待1000*/
    if (err == pdTRUE)
    {
      err = pdFAIL;
      if (!IS_HC05_CONNECTED())
      {
        HC05_Send_CMD("AT+INQ\r\n", 1); // 模块在查询状态，才能容易被其它设备搜索到
        ILI9341_DispString_EN(20, 40, "Please connect Bluetooth");
        printf("蓝牙尚未连接。请用手机打开蓝牙调试助手搜索连接蓝牙\r\n");
      }
      else
      {
        if (Clear_Message == 0)
        {
          ILI9341_Clear(0, 20, 240, 50);
        }

        Clear_Message = 1;
        ILI9341_DispString_EN(20, 20, "HC05 connection successful");
        ILI9341_DispString_EN(20, 40, "Enter your password to open the door");

        printf("蓝牙已连接。发送“RED_LED”可控制翻转LED灯\r\n");
      }
    }
    TransData_CtrlLED_Test();
    vTaskDelay(1000);
  }
}

void task2(void *pvParameters)
{
  while (1)
  {
    if (timer == 4)
    {
      if (semaphore_handle != NULL)
      {
        xSemaphoreGive(semaphore_handle);
      }
    }
    vTaskDelay(100);
  }
}

void task3(void *pvParameters)
{

  while (1)
  {
    ILI9341_DispString_EN(20, 120, "Please Press Key1 To Input  Fingerprint");
    ILI9341_DispString_EN(20, 160, "Please Press Key2 To delete Fingerprint");
    AS608_TASK();
    vTaskDelay(1000);
  }
}

void task4(void *pvParameters)
{
  while (1)
  {
    if (Key_Scan(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == KEY_ON)
    {
      xSemaphoreGive(semaphore_handle1);
    }

    if (Key_Scan(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == KEY_ON)
    {
      xSemaphoreGive(semaphore_handle2);
    }
    vTaskDelay(100);
  }
}

void timer2Callback(TimerHandle_t pxTimer)
{

  // printf("timer2 running frequency : %d \r\n",++timer);
  ++timer;
  if (timer == 5)
  {
    timer = 0;
  }
}
