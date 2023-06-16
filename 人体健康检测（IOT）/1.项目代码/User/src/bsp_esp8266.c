#include "bsp_esp8266.h"
#include "bsp_usart.h"
#include "bsp_led.h"
#include "core_delay.h"
#include <stdio.h>  
#include <string.h>  
#include <stdbool.h>

static void ESP8266_GPIO_Config(void);
static void ESP8266_USART_Config(void);
static void ESP8266_USART_NVIC_Configuration(void);

struct  STRUCT_USARTx_Fram strEsp8266_Fram_Record = { 0 };
struct  STRUCT_USARTx_Fram strUSART_Fram_Record = { 0 };

char ptr[250];
char ptr1[250];
char ptr2[250];



#define LED_CMD_NUMBER   8
char *ledCmd[8] = { "LED_RED","LED_GREEN","LED_BLUE","LED_YELLOW","LED_PURPLE","LED_CYAN","LED_WHITE","LED_RGBOFF" };          
volatile uint8_t ucTcpClosedFlag = 0;

void ESP8266_Init(void)
{
    ESP8266_GPIO_Config();

    ESP8266_USART_Config();

    macESP8266_RST_HIGH_LEVEL();
    macESP8266_CH_DISABLE();
}

static void ESP8266_GPIO_Config(void)
{
    /*定义一个GPIO_InitTypeDef类型的结构体*/
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 配置 CH_PD 引脚*/
    macESP8266_CH_PD_APBxClock_FUN(macESP8266_CH_PD_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = macESP8266_CH_PD_PIN;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(macESP8266_CH_PD_PORT, &GPIO_InitStructure);

    /* 配置 RST 引脚*/
    macESP8266_RST_APBxClock_FUN(macESP8266_RST_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = macESP8266_RST_PIN;

    GPIO_Init(macESP8266_RST_PORT, &GPIO_InitStructure);
}

static void ESP8266_USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* config USART clock */
    macESP8266_USART_APBxClock_FUN(macESP8266_USART_CLK, ENABLE);
    macESP8266_USART_GPIO_APBxClock_FUN(macESP8266_USART_GPIO_CLK, ENABLE);

    /* USART GPIO config */
    /* Configure USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = macESP8266_USART_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(macESP8266_USART_TX_PORT, &GPIO_InitStructure);

    /* Configure USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = macESP8266_USART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(macESP8266_USART_RX_PORT, &GPIO_InitStructure);

    /* USART1 mode config */
    USART_InitStructure.USART_BaudRate = macESP8266_USART_BAUD_RATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(macESP8266_USARTx, &USART_InitStructure);

    /* 中断配置 */
    USART_ITConfig(macESP8266_USARTx, USART_IT_RXNE, ENABLE); // 使能串口接收中断
    USART_ITConfig(macESP8266_USARTx, USART_IT_IDLE, ENABLE); // 使能串口总线空闲中断

    ESP8266_USART_NVIC_Configuration();

    USART_Cmd(macESP8266_USARTx, ENABLE);
}

static void ESP8266_USART_NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* Enable the USART2 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = macESP8266_USART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void ESP8266_Rst(void)
{
    macESP8266_RST_LOW_LEVEL();
    Delay_ms(500);
    macESP8266_RST_HIGH_LEVEL();
}

/*
 * 函数名：ESP8266_Cmd
 * 描述  ：对WF-ESP8266模块发送AT指令
 * 输入  ：cmd，待发送的指令
 *         reply1，reply2，期待的响应，为NULL表不需响应，两者为或逻辑关系
 *         waittime，等待响应的时间
 * 返回  : 1，指令发送成功
 *         0，指令发送失败
 * 调用  ：被外部调用
 */
bool ESP8266_Cmd ( char * cmd, char * reply1, char * reply2, u32 waittime )
{    
	strEsp8266_Fram_Record .InfBit .FramLength = 0;               //从新开始接收新的数据包

	macESP8266_Usart ( "%s\r\n", cmd );

	if ( ( reply1 == 0 ) && ( reply2 == 0 ) )                      //不需要接收数据
		return true;
	
	Delay_ms ( waittime );                 //延时
	
	strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ]  = '\0';

	macPC_Usart ( "%s", strEsp8266_Fram_Record .Data_RX_BUF );
  strEsp8266_Fram_Record .InfBit .FramLength = 0;                             //清除接收标志
	strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;                             
	if ( ( reply1 != 0 ) && ( reply2 != 0 ) )
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply1 ) || 
						 ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply2 ) ); 
 	
	else if ( reply1 != 0 )
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply1 ) );
	
	else
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply2 ) );	
}

bool ESP8266_AT_Test ( void )
{
	char count=0;
	
	macESP8266_RST_HIGH_LEVEL();	
  printf("\r\nAT测试.....\r\n");
	Delay_ms ( 2000 );
	while ( count < 10 )
	{
    printf("\r\nAT测试次数 %d......\r\n", count);
		if( ESP8266_Cmd ( "AT", "OK", NULL, 500 ) )
    {
      printf("\r\nAT测试启动成功 %d......\r\n", count);
      return 1;
    }
		ESP8266_Rst();
		++ count;
	}
  return 0;
}


/*
 * 函数名：ESP8266_Net_Mode_Choose
 * 描述  ：选择WF-ESP8266模块的工作模式
 * 输入  ：enumMode，工作模式
 * 返回  : 1，选择成功
 *         0，选择失败
 * 调用  ：被外部调用
 */
bool ESP8266_Net_Mode_Choose ( ENUM_Net_ModeTypeDef enumMode )
{
	switch ( enumMode )
	{
		case STA:
			return ESP8266_Cmd ( "AT+CWMODE=1", "OK", "no change", 2500 ); 
		
	  case AP:
		  return ESP8266_Cmd ( "AT+CWMODE=2", "OK", "no change", 2500 ); 
		
		case STA_AP:
		  return ESP8266_Cmd ( "AT+CWMODE=3", "OK", "no change", 2500 ); 
		
	  default:
		  return false;
  }
	
}


/*
 * 函数名：ESP8266_JoinAP
 * 描述  ：WF-ESP8266模块连接外部WiFi
 * 输入  ：pSSID，WiFi名称字符串
 *       ：pPassWord，WiFi密码字符串
 * 返回  : 1，连接成功
 *         0，连接失败
 * 调用  ：被外部调用
 */
bool ESP8266_JoinAP ( char * pSSID, char * pPassWord )
{
	char cCmd [120];

	sprintf ( cCmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassWord );
	
	return ESP8266_Cmd ( cCmd, "OK", NULL, 5000 );
	
}


/*
 * 函数名：ESP8266_BuildAP
 * 描述  ：WF-ESP8266模块创建WiFi热点
 * 输入  ：pSSID，WiFi名称字符串
 *       ：pPassWord，WiFi密码字符串
 *       ：enunPsdMode，WiFi加密方式代号字符串
 * 返回  : 1，创建成功
 *         0，创建失败
 * 调用  ：被外部调用
 */
bool ESP8266_BuildAP ( char * pSSID, char * pPassWord, ENUM_AP_PsdMode_TypeDef enunPsdMode )
{
	char cCmd [120];

	sprintf ( cCmd, "AT+CWSAP=\"%s\",\"%s\",1,%d", pSSID, pPassWord, enunPsdMode );
	
	return ESP8266_Cmd ( cCmd, "OK", 0, 1000 );
	
}


/*
 * 函数名：ESP8266_Enable_MultipleId
 * 描述  ：WF-ESP8266模块启动多连接
 * 输入  ：enumEnUnvarnishTx，配置是否多连接
 * 返回  : 1，配置成功
 *         0，配置失败
 * 调用  ：被外部调用
 */
bool ESP8266_Enable_MultipleId ( FunctionalState enumEnUnvarnishTx )
{
	char cStr [20];
	
	sprintf ( cStr, "AT+CIPMUX=%d", ( enumEnUnvarnishTx ? 1 : 0 ) );
	
	return ESP8266_Cmd ( cStr, "OK", 0, 500 );
	
}


/*
 * 函数名：ESP8266_Link_Server
 * 描述  ：WF-ESP8266模块连接外部服务器
 * 输入  ：enumE，网络协议
 *       ：ip，服务器IP字符串
 *       ：ComNum，服务器端口字符串
 *       ：id，模块连接服务器的ID
 * 返回  : 1，连接成功
 *         0，连接失败
 * 调用  ：被外部调用
 */
bool ESP8266_Link_Server ( ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum, ENUM_ID_NO_TypeDef id)
{
	char cStr [100] = { 0 }, cCmd [120];

  switch (  enumE )
  {
		case enumTCP:
		  sprintf ( cStr, "\"%s\",\"%s\",%s", "TCP", ip, ComNum );
		  break;
		
		case enumUDP:
		  sprintf ( cStr, "\"%s\",\"%s\",%s", "UDP", ip, ComNum );
		  break;
		
		default:
			break;
  }

  if ( id < 5 )
    sprintf ( cCmd, "AT+CIPSTART=%d,%s", id, cStr);

  else
	  sprintf ( cCmd, "AT+CIPSTART=%s", cStr );

	return ESP8266_Cmd ( cCmd, "OK", "ALREAY CONNECT", 4000 );
	
}


/*
 * 函数名：ESP8266_StartOrShutServer
 * 描述  ：WF-ESP8266模块开启或关闭服务器模式
 * 输入  ：enumMode，开启/关闭
 *       ：pPortNum，服务器端口号字符串
 *       ：pTimeOver，服务器超时时间字符串，单位：秒
 * 返回  : 1，操作成功
 *         0，操作失败
 * 调用  ：被外部调用
 */
bool ESP8266_StartOrShutServer ( FunctionalState enumMode, char * pPortNum, char * pTimeOver )
{
	char cCmd1 [120], cCmd2 [120];

	if ( enumMode )
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 1, pPortNum );
		
		sprintf ( cCmd2, "AT+CIPSTO=%s", pTimeOver );

		return ( ESP8266_Cmd ( cCmd1, "OK", 0, 500 ) &&
						 ESP8266_Cmd ( cCmd2, "OK", 0, 500 ) );
	}
	
	else
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 0, pPortNum );

		return ESP8266_Cmd ( cCmd1, "OK", 0, 500 );
	}
	
}


/*
 * 函数名：ESP8266_Get_LinkStatus
 * 描述  ：获取 WF-ESP8266 的连接状态，较适合单端口时使用
 * 输入  ：无
 * 返回  : 2，获得ip
 *         3，建立连接
 *         3，失去连接
 *         0，获取状态失败
 * 调用  ：被外部调用
 */
uint8_t ESP8266_Get_LinkStatus ( void )
{
	if ( ESP8266_Cmd ( "AT+CIPSTATUS", "OK", 0, 500 ) )
	{
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "STATUS:2\r\n" ) )
			return 2;
		
		else if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "STATUS:3\r\n" ) )
			return 3;
		
		else if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "STATUS:4\r\n" ) )
			return 4;		

	}
	
	return 0;
	
}


/*
 * 函数名：ESP8266_Get_IdLinkStatus
 * 描述  ：获取 WF-ESP8266 的端口（Id）连接状态，较适合多端口时使用
 * 输入  ：无
 * 返回  : 端口（Id）的连接状态，低5位为有效位，分别对应Id5~0，某位若置1表该Id建立了连接，若被清0表该Id未建立连接
 * 调用  ：被外部调用
 */
uint8_t ESP8266_Get_IdLinkStatus ( void )
{
	uint8_t ucIdLinkStatus = 0x00;
	
	
	if ( ESP8266_Cmd ( "AT+CIPSTATUS", "OK", 0, 500 ) )
	{
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+CIPSTATUS:0," ) )
			ucIdLinkStatus |= 0x01;
		else 
			ucIdLinkStatus &= ~ 0x01;
		
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+CIPSTATUS:1," ) )
			ucIdLinkStatus |= 0x02;
		else 
			ucIdLinkStatus &= ~ 0x02;
		
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+CIPSTATUS:2," ) )
			ucIdLinkStatus |= 0x04;
		else 
			ucIdLinkStatus &= ~ 0x04;
		
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+CIPSTATUS:3," ) )
			ucIdLinkStatus |= 0x08;
		else 
			ucIdLinkStatus &= ~ 0x08;
		
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+CIPSTATUS:4," ) )
			ucIdLinkStatus |= 0x10;
		else 
			ucIdLinkStatus &= ~ 0x10;	

	}
	
	return ucIdLinkStatus;
	
}


/*
 * 函数名：ESP8266_Inquire_ApIp
 * 描述  ：获取 F-ESP8266 的 AP IP
 * 输入  ：pApIp，存放 AP IP 的数组的首地址
 *         ucArrayLength，存放 AP IP 的数组的长度
 * 返回  : 0，获取失败
 *         1，获取成功
 * 调用  ：被外部调用
 */
uint8_t ESP8266_Inquire_ApIp ( char * pApIp, uint8_t ucArrayLength )
{
	char uc;
	
	char * pCh;
	
	
  ESP8266_Cmd ( "AT+CIFSR", "OK", 0, 500 );
	
	pCh = strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "APIP,\"" );
	
	if ( pCh )
		pCh += 6;
	
	else
		return 0;
	
	for ( uc = 0; uc < ucArrayLength; uc ++ )
	{
		pApIp [ uc ] = * ( pCh + uc);
		
		if ( pApIp [ uc ] == '\"' )
		{
			pApIp [ uc ] = '\0';
			break;
		}
		
	}
	
	return 1;
	
}


/*
 * 函数名：ESP8266_UnvarnishSend
 * 描述  ：配置WF-ESP8266模块进入透传发送
 * 输入  ：无
 * 返回  : 1，配置成功
 *         0，配置失败
 * 调用  ：被外部调用
 */
bool ESP8266_UnvarnishSend ( void )
{
	if ( ! ESP8266_Cmd ( "AT+CIPMODE=1", "OK", 0, 500 ) )
		return false;
	
	return 
	  ESP8266_Cmd ( "AT+CIPSEND", "OK", ">", 500 );
	
}


/*
 * 函数名：ESP8266_ExitUnvarnishSend
 * 描述  ：配置WF-ESP8266模块退出透传模式
 * 输入  ：无
 * 返回  : 无
 * 调用  ：被外部调用
 */
void ESP8266_ExitUnvarnishSend ( void )
{
	Delay_ms ( 1000 );
	
	macESP8266_Usart ( "+++" );
	
	Delay_ms ( 500 ); 
	
}


/*
 * 函数名：ESP8266_SendString
 * 描述  ：WF-ESP8266模块发送字符串
 * 输入  ：enumEnUnvarnishTx，声明是否已使能了透传模式
 *       ：pStr，要发送的字符串
 *       ：ulStrLength，要发送的字符串的字节数
 *       ：ucId，哪个ID发送的字符串
 * 返回  : 1，发送成功
 *         0，发送失败
 * 调用  ：被外部调用
 */
bool ESP8266_SendString ( FunctionalState enumEnUnvarnishTx, char * pStr, u32 ulStrLength, ENUM_ID_NO_TypeDef ucId )
{
	char cStr [20];
	bool bRet = false;
	
		
	if ( enumEnUnvarnishTx )
	{
		macESP8266_Usart ( "%s", pStr );
		
		bRet = true;
		
	}

	else
	{
		if ( ucId < 5 )
			sprintf ( cStr, "AT+CIPSEND=%d,%d", ucId, ulStrLength + 2 );

		else
			sprintf ( cStr, "AT+CIPSEND=%d", ulStrLength + 2 );
		
		ESP8266_Cmd ( cStr, "> ", 0, 100 );

		bRet = ESP8266_Cmd ( pStr, "SEND OK", 0, 500 );
  }
	
	return bRet;

}


/*
 * 函数名：ESP8266_ReceiveString
 * 描述  ：WF-ESP8266模块接收字符串
 * 输入  ：enumEnUnvarnishTx，声明是否已使能了透传模式
 * 返回  : 接收到的字符串首地址
 * 调用  ：被外部调用
 */
char * ESP8266_ReceiveString ( FunctionalState enumEnUnvarnishTx )
{
	char * pRecStr = 0;
	
	
	strEsp8266_Fram_Record .InfBit .FramLength = 0;
	strEsp8266_Fram_Record .InfBit .FramFinishFlag = 0;
	
	while ( ! strEsp8266_Fram_Record .InfBit .FramFinishFlag );
	strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ] = '\0';
	
	if ( enumEnUnvarnishTx )
		pRecStr = strEsp8266_Fram_Record .Data_RX_BUF;
	
	else 
	{
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+IPD" ) )
			pRecStr = strEsp8266_Fram_Record .Data_RX_BUF;

	}

	return pRecStr;
	
}




/**
  * @brief  获取网络调试助手和串口调试助手发来的信息
  * @param  无
  * @retval 无
  */
void Get_ESP82666_Cmd( char * cmd)
{
	uint8_t i;
	for(i = 0;i < LED_CMD_NUMBER; i++)
	{
     if(( bool ) strstr ( cmd, ledCmd[i] ))
		 break;
	}
	switch(i)
    {
      case 0:
        LED_RED;
      break;
      case 1:
        LED_GREEN;
      break;
      case 2:
        LED_BLUE;
      break;
      case 3:
        LED_YELLOW;
      break;
      case 4:
        LED_PURPLE;
      break;
      case 5:
        LED_CYAN;
      break;
      case 6:
        LED_WHITE;
      break;
      case 7:
        LED_RGBOFF;
      break;
      default:
        
        break;      
    }   
}


/**
  * @brief  ESP8266 StaTcpClient Unvarnish 配置测试函数
  * @param  无
  * @retval 无
  */
void ESP8266_MQTT_Config(void)
{
  printf( "\r\n正在配置 ESP8266 ......\r\n" );
  printf( "\r\n使能 ESP8266 ......\r\n" );
	macESP8266_CH_ENABLE();
	while( ! ESP8266_AT_Test() );
  
  printf( "\r\n正在配置工作模式 STA ......\r\n" );
	while( ! ESP8266_Net_Mode_Choose ( STA ) );

  printf( "\r\n正在连接 WiFi ......\r\n" );
  while( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );	
	
  printf( "\r\n不允许多连接 ......\r\n" );
	while( ! ESP8266_Enable_MultipleId ( DISABLE ) );
	
	printf( "\r\n正在配置MQTT用户名密码 ......\r\n" );
	sprintf ( ptr, "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"",username,passwd);
	ESP8266_Cmd(ptr,"OK","ERROR",500);

	printf( "\r\n正在配置MQTT ClientID ......\r\n" );
	sprintf ( ptr1, "AT+MQTTCLIENTID=0,\"iv8n2SoV9vD.D001|securemode=2\\,signmethod=hmacsha256\\,timestamp=1686814492716|\"");
	ESP8266_Cmd(ptr1,"OK","NULL",2000);
	
	printf( "\r\n正在连接阿里云服务器 ......\r\n" );
	sprintf ( ptr2, "AT+MQTTCONN=0,\"iot-06z00hjdct1op8z.mqtt.iothub.aliyuncs.com\",1883,1");
	ESP8266_Cmd(ptr2,"OK","NULL",2000);
	
	printf( "\r\n配置 ESP8266 完毕\r\n" );
//	printf ( "\r\n开始透传......\r\n" );
  
}


/**
  * @brief  ESP8266 检查是否接收到了数据，检查连接和掉线重连
  * @param  无
  * @retval 无
  */
void ESP8266_CheckRecvDataTest(void)
{
  uint8_t ucStatus;
  uint16_t i;
  
  /* 如果接收到了串口调试助手的数据 */
  if(strUSART_Fram_Record.InfBit.FramFinishFlag == 1)
  {
    for(i = 0;i < strUSART_Fram_Record.InfBit.FramLength; i++)
    {
       USART_SendData( macESP8266_USARTx ,strUSART_Fram_Record.Data_RX_BUF[i]); //转发给ESP82636
       while(USART_GetFlagStatus(macESP8266_USARTx,USART_FLAG_TC)==RESET){}      //等待发送完成
    }
    strUSART_Fram_Record .InfBit .FramLength = 0;                                //接收数据长度置零
    strUSART_Fram_Record .InfBit .FramFinishFlag = 0;                            //接收标志置零
    Get_ESP82666_Cmd(strUSART_Fram_Record .Data_RX_BUF);                         //检查一下是不是点灯命令
  }
  
  /* 如果接收到了ESP8266的数据 */
  if(strEsp8266_Fram_Record.InfBit.FramFinishFlag)
  {                                                      
    for(i = 0;i < strEsp8266_Fram_Record .InfBit .FramLength; i++)               
    {
       USART_SendData( DEBUG_USARTx ,strEsp8266_Fram_Record .Data_RX_BUF[i]);    //转发给ESP82636
       while(USART_GetFlagStatus(DEBUG_USARTx,USART_FLAG_TC)==RESET){}
    }
     strEsp8266_Fram_Record .InfBit .FramLength = 0;                             //接收数据长度置零
     strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;                           //接收标志置零
     Get_ESP82666_Cmd(strEsp8266_Fram_Record .Data_RX_BUF);                      //检查一下是不是点灯命令
  }
  
  if ( ucTcpClosedFlag )                                             //检测是否失去连接
  {
    ESP8266_ExitUnvarnishSend ();                                    //退出透传模式
    
    do ucStatus = ESP8266_Get_LinkStatus ();                         //获取连接状态
    while ( ! ucStatus );
    
    if ( ucStatus == 4 )                                             //确认失去连接后重连
    {
      printf ( "\r\n正在重连热点和服务器 ......\r\n" );
      
      while ( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );
      
      while ( !	ESP8266_Link_Server ( enumTCP, macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, Single_ID_0 ) );
      
      printf ( "\r\n重连热点和服务器成功\r\n" );

    }
    
    while ( ! ESP8266_UnvarnishSend () );		
    
  }
}


bool Set_MQTT_User_Config(char * id , char * name , char * password){
	char cCmd [500];
	
	sprintf ( cCmd, "AT+MQTTUSERCFG=0，1，\"%s\",\"%s\",\"%s\"，0,0，""", id, name, password );
	

	printf("%s",cCmd);

	return ESP8266_Cmd ( cCmd, "OK", "ERROR", 1000 );
}



