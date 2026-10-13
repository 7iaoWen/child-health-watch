/********************************************************************************
* @file		:my_iic.h
* @brief	:my_iic.c头文件
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_IIC_H__
#define __MY_IIC_H__

#define IIC_CMD_WR			0
#define IIC_CMD_RD			1

/** 
  * @brief  模拟 IIC 类
  */
typedef struct _MY_IIC c_my_iic;
typedef struct _MY_IIC
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
	void* this;  
	
	/************************************************* 
    * Description: MCU通过模拟IIC, 向IIC从机的寄存器连续写入数据
    * Input : <p_obj> 指向模拟IIC对象;
			  <dev_addr> IIC从机的设备地址，7bits;
			  <reg_addr> 待写入的IIC从机寄存器起始地址;
			  <data> 待写入数据缓存;
			  <len> 待写入数据长度;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.write_regs(&iic, BMP180_ADDR, 0x01, buf, 1);
    *************************************************/  
	int(*write_regs)(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint8_t len);
	
	/************************************************* 
    * Description: MCU通过模拟IIC, 从IIC从机寄存器中连续读取数据
    * Input : <p_obj> 指向模拟IIC对象;
			  <dev_addr> IIC从机的设备地址，7bits;
			  <reg_addr> 待读取的IIC从机寄存器起始地址;
			  <data> 待读取数据缓存;
			  <len> 待读取数据长度;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.read_regs(&iic, BMP180_ADDR, 0x01, buf, 1);
    *************************************************/  
	int(*read_regs)(const c_my_iic *p_obj, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t len);
	
	/************************************************* 
    * Description: 模拟IIC 重设参数
    * Input : <p_obj> 指向模拟IIC对象;
			  <speed> 速度，即IIC延时时间，值越大IIC通信速度越慢
			  <ack_check> 主机发送数据后 是否检测从机的ack相应;
			  <ack_wait_time> 等待ack相应的超时时间;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.set_param(&iic, 10, ture, 80);
    *************************************************/  
	int(*set_param)(const c_my_iic *p_obj, uint32_t speed, bool ack_check, uint32_t ack_wait_time);

	/************************************************* 
    * Description: 模拟IIC，向总线发送 START 信号
    * Input : <p_obj> 指向模拟IIC对象;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.start(&iic);
    *************************************************/  
	int(*start)(const c_my_iic *p_obj);

	/************************************************* 
    * Description: 模拟IIC，向总线发送 STOP 信号
    * Input : <p_obj> 指向模拟IIC对象;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.stop(&iic);
    *************************************************/  
	int(*stop)(const c_my_iic *p_obj);

	/************************************************* 
    * Description: 模拟IIC, 向总线发送一个字节的数据，带ACK检测
    * Input : <p_obj> 指向模拟IIC对象;
			  <data> 发送的数据;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = iic.send_byte(&iic, 0xa5);
    *************************************************/  
	int(*send_byte)(const c_my_iic *p_obj, uint8_t data);

	/************************************************* 
    * Description: 模拟IIC, 从总线上读取一个字节的数据，可发送ACK
    * Input : <p_obj> 指向模拟IIC对象;
			  <ack> ture:接收完一个直接后向总线发送 ACK，表示需要继续接收， false:接收完一个直接后向总线发送 NACK ， 表示中断接收
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t data = iic.recive_byte(&iic, true);
    *************************************************/  
	uint8_t(*recive_byte)(const c_my_iic *p_obj, bool ack);
}c_my_iic;

/************************************************* 
* Description: 新建一个模拟 IIC 对象
* Input : <scl_gpio> 模拟IIC 时钟线端口号; 
		  <scl_pin>  模拟IIC 时钟线引脚; 
		  <sda_gpio> 模拟IIC 数据线端口号; 
		  <sda_gpio> 模拟IIC 数据线引脚; 
* Return: <E_OK> 操作成功; <E_ERROR> 操作失败
* Demo  : 
	c_my_iic iic = my_iic_create(GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1);
*************************************************/  
c_my_iic my_iic_create(GPIO_TypeDef* scl_gpio, uint16_t scl_pin, GPIO_TypeDef* sda_gpio, uint16_t sda_pin);

#endif 

