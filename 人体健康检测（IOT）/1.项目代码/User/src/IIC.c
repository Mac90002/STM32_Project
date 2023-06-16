#include "IIC.h"
#include "bsp_SysTick.h"

static void iic_delay(void)
{
    Delay_us(2); /* 2us的延时, 读写速度在250Khz以内 */
}

void iic_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    IIC_SCL_GPIO_CLK_ENABLE(); /* SCL引脚时钟使能 */

    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Pin = IIC_SCL_GPIO_PIN;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio_init_struct.GPIO_Pin = IIC_SDA_GPIO_PIN;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init_struct);

    iic_stop(); /* 停止总线上所有设备 */
}

void iic_start(void)
{
    IIC_SCL(1);
    IIC_SDA(1);
    iic_delay();
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

void iic_stop(void)
{
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(1);
    iic_delay();
}

void iic_ack(void)
{
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
    IIC_SDA(1); // 最后释放总线
    iic_delay();
}

void iic_nack(void)
{
    IIC_SDA(1);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}
/**
 * @brief       等待应答信号到来
 * @param       无
 * @retval      1，接收应答失败
 *              0，接收应答成功
 */
uint8_t iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1); /* 主机释放SDA线(此时外部器件可以拉低SDA线) */
    iic_delay();
    IIC_SCL(1); /* SCL=1, 此时从机可以返回ACK */
    iic_delay();

    while (IIC_READ_SDA)
    {
        waittime++;

        if (waittime > 250)
        {
            iic_stop();
            rack = 1;
            break;
        }
    }

    IIC_SCL(0); /* SCL=0, 结束ACK检查 */
    iic_delay();
    return rack;
}

void iic_send_byte(uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8; t++)
    {
        IIC_SDA((data & 0x80) >> 7); // 先发送最高位
        iic_delay();
        IIC_SCL(1);
        iic_delay();
        IIC_SCL(0); // 在SCL为0的时候 数据可以转变电平
        iic_delay();
        data <<= 1;
    }
    IIC_SDA(1); /* 发送完成, 主机释放SDA线 */
}

/// @brief
/// @param ack
/// @return 1 ACk 0 NACK
uint8_t iic_read_byte(uint8_t ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++)
    {
        receive <<= 1; /* 高位先输出,所以先收到的数据位要左移 */
        IIC_SCL(1);
        iic_delay();

        if (IIC_READ_SDA)
        {
            receive++;
        }

        IIC_SCL(0);
        iic_delay();
    }
    if (!ack)
    {
        iic_nack(); /* 发送nACK */
    }
    else
    {
        iic_ack(); /* 发送ACK */
    }

    return receive;
}

u8 MPU_Write_Len(u8 addr, u8 reg, u8 len, u8 *buf)
{
    u8 i;
    iic_start();
    iic_send_byte((addr << 1) | 0); // 发送器件地址+写命令
    if (iic_wait_ack())             // 等待应答
    {
        iic_stop();
        return 1;
    }
    iic_send_byte(reg); // 写寄存器地址
    iic_wait_ack();     // 等待应答
    for (i = 0; i < len; i++)
    {
        iic_send_byte(buf[i]); // 发送数据
        if (iic_wait_ack())    // 等待ACK
        {
            iic_stop();
            return 1;
        }
    }
    iic_stop();
    return 0;
}

u8 MPU_Read_Len(u8 addr, u8 reg, u8 len, u8 *buf)
{
    iic_start();
    iic_send_byte((addr << 1) | 0); // 发送器件地址+写命令
    if (iic_wait_ack())             // 等待应答
    {
        iic_stop();
        return 1;
    }
    iic_send_byte(reg); // 写寄存器地址
    iic_wait_ack();     // 等待应答
    iic_start();
    iic_send_byte((addr << 1) | 1); // 发送器件地址+读命令
    iic_wait_ack();                 // 等待应答
    while (len)
    {
        if (len == 1)
            *buf = iic_read_byte(0); // 读数据,发送nACK
        else
            *buf = iic_read_byte(1); // 读数据,发送ACK
        len--;
        buf++;
    }
    iic_stop(); // 产生一个停止条件
    return 0;
}
