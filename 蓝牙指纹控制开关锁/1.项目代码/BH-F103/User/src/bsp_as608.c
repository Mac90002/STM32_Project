#include "bsp_as608.h"
#include "bsp_usart.h"
#include "bsp_ili9341_lcd.h"
#include "rx_data_queue.h"
#include "bsp_SysTick.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

extern QueueHandle_t semaphore_handle1 ;

static uint16_t PS_Connect(uint32_t *PS_Addr);
static void AS608_SendData(uint8_t data);
static void AS608_PackHead(void);
static void NVIC_Configuration(void);
static void SendLength(uint16_t length);
static void SendFlag(uint8_t flag);
static void Sendcmd(uint8_t cmd);
static void SendCheck(uint16_t check);
static uint16_t ReturnFlag(uint16_t *i);
static void ShowErrMessage(uint16_t ensure);
static void Add_FR(void);

uint32_t AS608_Addr = 0xFFFFFFFF;
uint16_t ID ;

void AS608_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  /*开启串口GPIO口的时钟*/
  RCC_APB2PeriphClockCmd(AS608_TouchOut_INT_GPIO_CLK, ENABLE);
  AS608_USART_GPIO_APBxClkCmd(AS608_USART_GPIO_CLK, ENABLE);
  /*打开串口外设的时钟*/
  AS608_USART_APBxClkCmd(AS608_USART_CLK, ENABLE);

  /* 配置 NVIC 中断*/
  NVIC_Configuration();

  /* TouchOut线用到的GPIO */
  GPIO_InitStructure.GPIO_Pin = AS608_TouchOut_INT_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(AS608_TouchOut_INT_GPIO_PORT, &GPIO_InitStructure);

  /* 选择EXTI的信号源 */
  GPIO_EXTILineConfig(AS608_TouchOut_INT_EXTI_PORTSOURCE, AS608_TouchOut_INT_EXTI_PINSOURCE);
  EXTI_InitStructure.EXTI_Line = AS608_TouchOut_INT_EXTI_LINE;

  /* EXTI为中断模式 */
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /*将USART Tx的GPIO配置为推挽复用模式*/
  GPIO_InitStructure.GPIO_Pin = AS608_USART_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(AS608_USART_TX_GPIO_PORT, &GPIO_InitStructure);

  /*将USART Rx的GPIO配置为浮空输入模式*/
  GPIO_InitStructure.GPIO_Pin = AS608_USART_RX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(AS608_USART_RX_GPIO_PORT, &GPIO_InitStructure);

  /*配置串口的工作参数*/
  USART_InitStructure.USART_BaudRate = AS608_USART_BAUDRATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(AS608_USART, &USART_InitStructure);

  USART_ITConfig(AS608_USART, USART_IT_RXNE, ENABLE);
  USART_ITConfig(AS608_USART, USART_IT_IDLE, ENABLE); // 使能串口总线空闲中断

  USART_Cmd(AS608_USART, ENABLE);
}

static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* 嵌套向量中断控制器组选择 */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  /* 配置USART为中断源 */
  NVIC_InitStructure.NVIC_IRQChannel = AS608_USART_IRQ;
  /* 抢断优先级*/
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  /* 子优先级 */
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  /* 使能中断 */
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  /* 初始化配置NVIC */
  NVIC_Init(&NVIC_InitStructure);
  /* 配置中断源：TouchOut线 */
  NVIC_InitStructure.NVIC_IRQChannel = AS608_TouchOut_INT_EXTI_IRQ;
  /* 抢断优先级*/
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  /* 配置子优先级 */
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  /* 初始化配置NVIC */
  NVIC_Init(&NVIC_InitStructure);
}

static void AS608_SendData(uint8_t data)
{
  USART_SendData(AS608_USART, data);
  while (USART_GetFlagStatus(AS608_USART, USART_FLAG_TXE) == RESET)
    ;
}

// 发送包头和芯片地址
static void AS608_PackHead(void)
{
  // 包头
  AS608_SendData(0xEF);
  AS608_SendData(0x01);

  AS608_SendData(AS608_Addr >> 24);
  AS608_SendData(AS608_Addr >> 16);
  AS608_SendData(AS608_Addr >> 8);
  AS608_SendData(AS608_Addr);
}

// 发送包标识
static void SendFlag(uint8_t flag)
{
  AS608_SendData(flag);
}

// 发送包长度
static void SendLength(uint16_t length)
{
  AS608_SendData(length >> 8);
  AS608_SendData(length);
}

// 发送指令
static void Sendcmd(uint8_t cmd)
{
  AS608_SendData(cmd);
}

// 发送校验
static void SendCheck(uint16_t check)
{
  AS608_SendData(check >> 8);
  AS608_SendData(check);
}

// 返回确认码
static uint16_t ReturnFlag(uint16_t *i)
{
  QUEUE_DATA_TYPE *rx_data;

  rx_data = cbRead(&rx_queue); /*从缓冲区读取数据，进行处理*/

  if (rx_data != NULL) /*缓冲队列非空*/
  {
    /*打印环行接收到的数据*/
    QUEUE_DEBUG_ARRAY((uint8_t *)rx_data->head, rx_data->len);

    *i = ((uint16_t)(*(rx_data->head + 9))); /*确认码*/

    cbReadFinish(&rx_queue); /*使用完数据必须调用cbReadFinish更新读指针*/

    return *i;
  }
  else
  {
    *i = 0xff;

    return *i;
  }
}

// 录入图像
uint16_t PS_GetImage(void)
{
  uint16_t temp;
  uint16_t sure, p = 0;

  AS608_DELAY_MS(3000); /*给指纹输入动作预留时间*/

  AS608_PackHead();
  SendFlag(0x01); /*命令包标识*/
  SendLength(0x03);
  Sendcmd(0x01); /*录指纹指令*/
  temp = 0x01 + 0x03 + 0x01;
  SendCheck(temp);

  AS608_DELAY_MS(500); /*等待指纹识别模块处理数据*/

  sure = ReturnFlag(&p);

  return sure;
}

uint16_t PS_GenChar(uint8_t BufferID)
{
  uint16_t temp;
  uint16_t sure, p = 0;

  AS608_PackHead();
  SendFlag(0x01);
  SendLength(0x04);
  Sendcmd(0x02); /*生成特征指令*/
  AS608_SendData(BufferID);
  temp = 0x01 + 0x04 + 0x02 + BufferID;
  SendCheck(temp);

  AS608_DELAY_MS(600);

  sure = ReturnFlag(&p);

  return sure;
}

/// @brief 以 CharBuffer1 或 CharBuffer2 中的特征文件高速搜索整个或部分指纹库。若搜索到，则返回页码。
/// @param BufferID 缓冲区 CharBuffer1、CharBuffer2 的 BufferID 分别为 1h 和 2h
/// @param StartPage 起始页
/// @param PageNum   页数 -> 有多少页
/// @param p 页码
/// @return 确认码=00H 表示搜索到；
uint16_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, uint16_t *p)
{
  uint16_t temp;
  uint16_t ensure;
  QUEUE_DATA_TYPE *rx_data;

  AS608_PackHead();
  SendFlag(0x01);
  SendLength(0x08);
  Sendcmd(0x1b); /*高速搜索指纹库指令*/
  AS608_SendData(BufferID);
  AS608_SendData(StartPage >> 8);
  AS608_SendData(StartPage);
  AS608_SendData(PageNum >> 8);
  AS608_SendData(PageNum);
  temp = 0x01 + 0x08 + 0x1b + BufferID + (StartPage >> 8) + (uint8_t)StartPage + (PageNum >> 8) + (uint8_t)PageNum;
  SendCheck(temp);

  AS608_DELAY_MS(500);

  rx_data = cbRead(&rx_queue); /*从缓冲区读取数据，进行处理*/
  if (rx_data != NULL)         /*缓冲队列非空*/
  {
    /*打印环行接收到的数据*/
    QUEUE_DEBUG_ARRAY((uint8_t *)rx_data->head, rx_data->len);

    ensure = ((uint16_t)(*(rx_data->head + 9))); /*确认码*/

    /*返回页码（相匹配的指纹模板ID）*/
    *p = ((*(rx_data->head + 10)) << 8) + (*(rx_data->head + 11));

    cbReadFinish(&rx_queue); /*使用完数据必须调用cbReadFinish更新读指针*/

    return ensure;
  }
  else
  {
    ensure = 0xff;
    return ensure;
  }
}

// 精确比对 CharBuffer1 与 CharBuffer2 中的指纹
uint16_t PS_Match(void)
{
  uint16_t temp;
  uint16_t sure, p = 0;

  AS608_PackHead();
  SendFlag(0x01);
  SendLength(0x03);
  Sendcmd(0x03); /*精确比对指令*/
  temp = 0x01 + 0x03 + 0x03;
  SendCheck(temp);

  AS608_DELAY_MS(500);

  sure = ReturnFlag(&p);

  return sure;
}

// 将 CharBuffer1 与 CharBuffer2 中的特征文件合并生成模板，结果存于 CharBuffer1 与 CharBuffer2
uint16_t PS_RegModel(void)
{
  uint16_t temp;
  uint16_t sure, p = 0;

  AS608_PackHead();
  SendFlag(0x01);
  SendLength(0x03);
  Sendcmd(0x05); /*合并特征指令*/
  temp = 0x01 + 0x03 + 0x05;
  SendCheck(temp);

  AS608_DELAY_MS(500);

  sure = ReturnFlag(&p);

  return sure;
}

static void ShowErrMessage(uint16_t ensure)
{
  switch (ensure)
  {
  case 0x00:
    printf("OK\r\n");
    break;

  case 0x01:
    printf("数据包接收错误\r\n");
    break;

  case 0x02:
    printf("指纹模块没有检测到指纹！\r\n");
    break;

  case 0x03:
    printf("录入指纹图像失败\r\n\r\n");
    break;

  case 0x04:
    printf("指纹图像太干、太淡而生不成特征\r\n\r\n");
    break;

  case 0x05:
    printf("指纹图像太湿、太糊而生不成特征\r\n\r\n");
    break;

  case 0x06:
    printf("指纹图像太乱而生不成特征\r\n\r\n");
    break;

  case 0x07:
    printf("指纹图像正常，但特征点太少（或面积太小）而生不成特征\r\n");
    break;

  case 0x08:
    printf("指纹不匹配\r\n\r\n");
    break;

  case 0x09:
    printf("对比指纹失败，指纹库不存在此指纹！\r\n\r\n");
    break;

  case 0x0a:
    printf("特征合并失败\r\n");
    break;

  case 0x0b:
    printf("访问指纹库时地址序号超出指纹库范围\r\n");
    break;

  case 0x10:
    printf("删除模板失败\r\n");
    break;

  case 0x11:
    printf("清空指纹库失败\r\n");
    break;

  case 0x15:
    printf("缓冲区内没有有效原始图而生不成图像\r\n");
    break;

  case 0x18:
    printf("读写 FLASH 出错\r\n");
    break;

  case 0x19:
    printf("未定义错误\r\n");
    break;

  case 0x1a:
    printf("无效寄存器号\r\n");
    break;

  case 0x1b:
    printf("寄存器设定内容错误\r\n");
    break;

  case 0x1c:
    printf("记事本页码指定错误\r\n");
    break;

  case 0x1f:
    printf("指纹库满\r\n");
    break;

  case 0x20:
    printf("地址错误\r\n");
    break;

  default:
    printf("模块返回确认码有误\r\n");
    break;
  }
}
// 将 CharBuffer1 或 CharBuffer2 中的模板文件存到 PageID 号flash 数据库位置
// BufferID(缓冲区号)，PageID（指纹库位置号）
uint16_t PS_StoreChar(uint8_t BufferID, uint16_t PageID)
{
  uint16_t temp;
  uint16_t sure, p = 0;

  AS608_PackHead();
  SendFlag(0x01);
  SendLength(0x06);
  Sendcmd(0x06); /*存储模板指令*/
  AS608_SendData(BufferID);
  AS608_SendData(PageID >> 8);
  AS608_SendData(PageID);
  temp = 0x01 + 0x06 + 0x06 + BufferID + (PageID >> 8) + (uint8_t)PageID;
  SendCheck(temp);

  AS608_DELAY_MS(500);

  sure = ReturnFlag(&p);

  return sure;
}

static void Add_FR(void)
{
  uint16_t i, j, sure ;

  i = j = 0;

  while (1)
  {
    switch (j)
    {
    case 0: /*执行第1步*/

      i++;
			ILI9341_Clear(0, 120, 240, 140);
      ILI9341_DispString_EN(20, 180, "Please Press Your Finger");
      sure = PS_GetImage(); /*录入图像*/
      if (sure == 0x00)
      {
        sure = PS_GenChar(CHAR_BUFFER1); /*生成特征1*/
        if (sure == 0x00)
        {
          ILI9341_Clear(0, 140, 240, 180);
          ILI9341_DispString_EN(20, 180, "fingerprint input succeed");
          SysTick_Delay_Ms(1500);
          sure = PS_HighSpeedSearch(CHAR_BUFFER1, 0, PS_MAXNUM, &ID);
          if (sure == 0x00)
          {
            ILI9341_Clear(0, 140, 240, 180);
            ILI9341_DispString_EN(20, 180, "fingerprint already exist");
            SysTick_Delay_Ms(3000);
						ILI9341_Clear(0, 120, 240, 180);
            return;
          }
          else
          {
            i = 0;
            j = 1; /*跳转到第2步*/
          }
        }
        else
        {
          ShowErrMessage(sure);
        }
      }
      else
      {
        ShowErrMessage(sure);
      }
      break;

    case 1:

      i++;
      ILI9341_Clear(0, 140, 240, 180);
      ILI9341_DispString_EN(20, 180, "Please Press Key1 to input fingerprint again");

      sure = PS_GetImage();
      if (sure == 0x00)
      {
        sure = PS_GenChar(CHAR_BUFFER2); /*生成特征2*/
        if (sure == 0x00)
        {
          ILI9341_Clear(0, 140, 240, 180);
          ILI9341_DispString_EN(20, 180, "fingerprint input succeed");
          SysTick_Delay_Ms(2000);
          i = 0;
          j = 2; /*跳转到第3步*/
        }
        else
        {
          ShowErrMessage(sure);
        }
      }
      else
      {
        ShowErrMessage(sure);
      }
      break;

    case 2:

      ILI9341_Clear(0, 140, 240, 180);
      ILI9341_DispString_EN(20, 180, "Fingerprint Compare...");
      SysTick_Delay_Ms(1000);

      sure = PS_Match(); /*精确比对两枚指纹特征*/
      if (sure == 0x00)
      {
        ILI9341_DispString_EN(20, 200, "Compare Succeed !!!");
        SysTick_Delay_Ms(2000);
        j = 3; /*跳转到第4步*/
      }
      else
      {
        ILI9341_DispString_EN(20, 200, "Compare Failed !!!");
        ShowErrMessage(sure);
        i = 0;
        j = 0;
      }

      break;

    case 3:

      ILI9341_Clear(0, 140, 240, 180);
      ILI9341_DispString_EN(20, 180, "The Fingerprint Module Is Being Generated...");

      sure = PS_RegModel(); /*合并特征（生成模板）*/
      if (sure == 0x00)
      {
        ILI9341_DispString_EN(20, 180, "The Generated Succeed !!!");
        SysTick_Delay_Ms(2000);
        j = 4; /*跳转到第5步*/
      }
      else
      {
        j = 0;
        ShowErrMessage(sure);
      }

      break;

    case 4:

      //  do
      //  {
      //    printf ("******命令：请输入存储ID，范围为0—239******\r\n");

      //    ID=GET_NUM();
      //  }while(!(ID<PS_MAXNUM));

      sure = PS_StoreChar(CHAR_BUFFER2, 1); /*储存模板*/
      if (sure == 0x00)
      {
      ILI9341_Clear(0, 140, 240, 180);
      ILI9341_DispString_EN(20, 180, "Fingerprint Entry Successful !!!");

        return;
      }
      else
      {
        j = 0;
        ShowErrMessage(sure);
      }
      break;
    }

    SysTick_Delay_Ms(1000);

    if (i == 4) /*超过4次没有按手指则退出*/
    {
      ILI9341_Clear(0, 140, 240, 180);
      ILI9341_DispString_EN(20, 180, "Fingerprint Entry Failed !!!");

      break;
    }
  }
}

void AS608_TASK()
{
  int sure , err ;

		err = xSemaphoreTake(semaphore_handle1,1000);/*获取信号量并等待1000*/
    if(err == pdTRUE){
			err = pdFAIL ;
			Add_FR();
		}
  
    ILI9341_DispString_EN(20, 200, "If your Fingerprints are there, run them directly");
    sure = PS_GetImage(); /*录入图像*/
    if(sure == 0x00){
      sure = PS_GenChar(CHAR_BUFFER1); /*生成特征1*/
      if(sure == 0x00){
        sure = PS_HighSpeedSearch(CHAR_BUFFER1, 0, PS_MAXNUM, &ID);
        if(sure == 0x00){
        ILI9341_Clear(0, 120, 240, 200);
        ILI9341_DispString_EN(20, 140, "The Fingerprint is Compare");
        ILI9341_DispString_EN(20, 160, "The Door is Open !!!");
        LED2_TOGGLE;
        SysTick_Delay_Ms(3000);
        LED2_TOGGLE;
        ILI9341_Clear(0, 120, 240, 200);
        ILI9341_DispString_EN(20, 140, "The Door is Close !!!");
				SysTick_Delay_Ms(1000);
				ILI9341_Clear(0, 120, 240, 200);
        }
				else{
					ILI9341_Clear(0, 120, 240, 200);
					ILI9341_DispString_EN(20, 140, "The Fingerprint is Illegal !!!");
					SysTick_Delay_Ms(2000);
					ILI9341_Clear(0, 120, 240, 200);
				}
      }
    }
}

uint16_t PS_Connect(uint32_t *PS_Addr)
{
  QUEUE_DATA_TYPE *rx_data;

  AS608_PackHead();
  AS608_SendData(0X01);
  AS608_SendData(0X00);
  AS608_SendData(0X00);

  AS608_DELAY_MS(1000);

  rx_data = cbRead(&rx_queue); /*从缓冲区读取数据，进行处理*/
  if (rx_data != NULL)         /*缓冲队列非空*/
  {
    /*打印环行接收到的数据*/
    QUEUE_DEBUG_ARRAY((uint8_t *)rx_data->head, rx_data->len);

    if (/*判断是不是模块返回的应答包*/
        *(rx_data->head) == 0XEF && *(rx_data->head + 1) == 0X01 && *(rx_data->head + 6) == 0X07)
    {
      printf("指纹模块的地址为:0x%x%x%x%x\r\n", *(rx_data->head + 2),
             *(rx_data->head + 3),
             *(rx_data->head + 4),
             *(rx_data->head + 5));
    }
    cbReadFinish(&rx_queue); /*使用完数据必须调用cbReadFinish更新读指针*/

    return 0;
  }

  return 1;
}

/**
 * @brief  检测与指纹模块的通信
 * @param  无
 * @retval 无
 */
void AS608_Connect_Test(void)
{
  printf("as608模块实验");

  if (PS_Connect(&AS608_Addr)) /*与AS608串口通信*/
  {
    ILI9341_DispString_EN(20, 20, "AS608 Failed Connect");
  }
  else
  {
    ILI9341_DispString_EN(20, 20, "AS608 Succeed Connect");
  }
}
