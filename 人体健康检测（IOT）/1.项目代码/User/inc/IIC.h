#ifndef __IIC_H
#define __IIC_H

#include "stm32f10x.h"


#define IIC_SCL_GPIO_PORT               GPIOB
#define IIC_SCL_GPIO_PIN                GPIO_Pin_6
#define IIC_SCL_GPIO_CLK_ENABLE()       do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); }while(0)   /* PB口时钟使能 */

#define IIC_SDA_GPIO_PORT               GPIOB
#define IIC_SDA_GPIO_PIN                GPIO_Pin_7
#define IIC_SDA_GPIO_CLK_ENABLE()       do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); }while(0)   /* PB口时钟使能 */

#define IIC_SCL(x)        do{ x ? \
                              GPIO_SetBits(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN) : \
                              GPIO_ResetBits(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN); \
                          }while(0)       /* SCL */

#define IIC_SDA(x)        do{ x ? \
                              GPIO_SetBits(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN) : \
                              GPIO_ResetBits(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN); \
                          }while(0)       /* SDA */    

#define IIC_READ_SDA     GPIO_ReadInputDataBit(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)

static unsigned short RETRY_IN_MLSEC  = 55;													
#define ST_Sensors_I2C_WriteRegister  Soft_DMP_I2C_Write
#define ST_Sensors_I2C_ReadRegister Soft_DMP_I2C_Read
void iic_init(void);                /* 初始化IIC的IO口 */
void iic_start(void);               /* 发送IIC开始信号 */
void iic_stop(void);                /* 发送IIC停止信号 */
void iic_ack(void);                 /* IIC发送ACK信号 */
void iic_nack(void);                /* IIC不发送ACK信号 */
uint8_t iic_wait_ack(void);         /* IIC等待ACK信号 */
void iic_send_byte(uint8_t data);   /* IIC发送一个字节 */
uint8_t iic_read_byte(uint8_t ack);/* IIC读取一个字节 */
u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf);
u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf);
#endif		
			
													