/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI

  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_SysTick.h"
#include "bsp_hc05_usart.h"
#include "bsp_as608.h"
#include "rx_data_queue.h"
#include "queue.h"
#include "semphr.h"

ReceiveData BLT_USART_ReceiveData;
ReceiveData DEBUG_USART_ReceiveData;
extern QueueHandle_t semaphore_handle1;
extern QueueHandle_t semaphore_handle2;
extern volatile uint16_t uart_p;
extern uint8_t uart_buff[UART_BUFF_SIZE];
uint8_t detection = 0;
uint16_t s;

/** @addtogroup STM32F10x_StdPeriph_Template
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
// void SVC_Handler(void)
//{
// }

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */

///**
//  * @brief  This function handles SysTick Handler.
//  * @param  None
//  * @retval None
//  */
extern void xPortSysTickHandler(void);
// systick中断服务函数
void SysTick_Handler(void)
{
  TimingDelay_Decrement();
#if (INCLUDE_xTaskGetSchedulerState == 1)
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
    xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1)
  }
#endif /* INCLUDE_xTaskGetSchedulerState */
}


void BLT_USART_IRQHandler(void)
{
  uint8_t ucCh;
  if (USART_GetITStatus(BLT_USARTx, USART_IT_RXNE) != RESET)
  {
    BLT_USART_ReceiveData.receive_data_flag = 1;
    ucCh = USART_ReceiveData(BLT_USARTx);
    if (BLT_USART_ReceiveData.datanum < UART_BUFF_SIZE)
    {
      if ((ucCh != 0x0a) && (ucCh != 0x0d))
      {
        BLT_USART_ReceiveData.uart_buff[BLT_USART_ReceiveData.datanum] = ucCh; // 不接收换行回车
        BLT_USART_ReceiveData.datanum++;
      }
    }
  }
}

void AS608_TouchOut_IRQHandler(void)
{
  /*确保是否产生了EXTI Line中断*/
  if (EXTI_GetITStatus(AS608_TouchOut_INT_EXTI_LINE) != RESET)
  {
    s = taskENTER_CRITICAL_FROM_ISR();
    /*LED反转*/
    LED1_TOGGLE;
    detection = 1;
    taskEXIT_CRITICAL_FROM_ISR(s);

    EXTI_ClearITPendingBit(AS608_TouchOut_INT_EXTI_LINE);
  }
}

void AS608_USART_IRQHandler(void)
{
  uint8_t ucCh;
  QUEUE_DATA_TYPE *data_p;

  if (USART_GetITStatus(AS608_USART, USART_IT_RXNE) != RESET)
  {
    ucCh = USART_ReceiveData(AS608_USART);

    /*获取写缓冲区指针，准备写入新数据*/
    data_p = cbWrite(&rx_queue);

    if (data_p != NULL) /*若缓冲队列未满，开始传输*/
    {

      /*往缓冲区写入数据，如使用串口接收、dma写入等方式*/
      *(data_p->head + data_p->len) = ucCh;

      if (++data_p->len >= QUEUE_NODE_DATA_LEN)
      {
        cbWriteFinish(&rx_queue);
      }
    }
    else
      return;
  }

  if (USART_GetITStatus(AS608_USART, USART_IT_IDLE) == SET) /*数据帧接收完毕*/
  {
    /*写入缓冲区完毕*/
    cbWriteFinish(&rx_queue);
    ucCh = USART_ReceiveData(AS608_USART); /*由软件序列清除中断标志位(先读USART_SR，然后读USART_DR)*/
  }
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
{
}*/

/**
 * @}
 */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
