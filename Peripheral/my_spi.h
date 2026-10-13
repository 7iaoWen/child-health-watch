/********************************************************************************
* @file		:my_spi.h
* @brief	:my_spi.c头文件
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_SPI_H__
#define __MY_SPI_H__

/** 
  * @brief  模拟 SPI 类
  */
typedef struct _MY_SPI c_my_spi;
typedef struct _MY_SPI
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
	void* this;  
	
	/************************************************* 
    * Description: SPI 主机向 SPI 从机发送数据
    * Input : <p_obj> 指向模拟 SPI 对象;
			  <txData> SPI需要发送的数据缓存;
			  <len> SPI需要发送的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = spi_w25q64.send_bytes(&spi_w25q64, buf, 32);
    *************************************************/  
	int(*send_bytes)(const c_my_spi *p_obj, uint8_t* txData, uint16_t len);

	/************************************************* 
    * Description: SPI 主机从 SPI 从机读取数据
    * Input : <p_obj> 指向模拟 SPI 对象;
			  <rxData> SPI需要读取的数据缓存;
			  <len> SPI需要读取的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = spi_w25q64.read_bytes(&spi_w25q64, buf, 32);
    *************************************************/  
	int(*read_bytes)(const c_my_spi *p_obj, uint8_t* rxData, uint16_t len);
	
	/************************************************* 
    * Description: SPI 主机和 SPI 从机交换数据
    * Input : <p_obj> 指向模拟 SPI 对象;
			  <txData> SPI发送的数据缓存;
			  <rxData> SPI接收的数据缓存;
			  <len> SPI需要读取的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = spi_w25q64.transfer_bytes(&spi_w25q64, txBuf, rxBuf，32);
    *************************************************/  
	int(*transfer_bytes)(const c_my_spi *p_obj, uint8_t* txData, uint8_t* rxData, uint16_t lenn);
	
	/************************************************* 
    * Description: 模拟 SPI 重设速度
    * Input : <p_obj> 指向模拟 SPI 对象;
			  <speed> SPI 速度值;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = spi_w25q64.set_speed(&spi_w25q64, 10);
    *************************************************/  
	int(*set_speed)(const c_my_spi *p_obj, uint32_t speed);
}c_my_spi;


/************************************************* 
* Description: 新建一个模拟 SPI 对象
* Input : <cs_gpio> CS 片选 端口号; 
		  <cs_pin> CS 片选 引脚; 
		  <sck_gpio> 时钟线 端口号; 
		  <sck_pin> 时钟线 引脚; 
		  <mosi_gpio> 主机输出 引脚 端口号; 
		  <mosi_pin> 主机输出  引脚; 
		  <miso_gpio> 主机输入 引脚端口号; 
		  <miso_pin> 主机输入 引脚; 
* Output: 无
* Return: <E_OK> 操作成功; <E_ERROR> 操作失败
* Demo  : 
	c_my_spi spi = my_spi_creat(GPIOB, GPIO_PIN_15, GPIOB, GPIO_PIN_12, GPIOB, GPIO_PIN_13, GPIOB, GPIO_PIN_14);
*************************************************/  
c_my_spi my_spi_creat(GPIO_TypeDef* cs_gpio, uint16_t cs_pin, GPIO_TypeDef* sck_gpio, uint16_t sck_pin, GPIO_TypeDef* mosi_gpio, uint16_t mosi_pin, GPIO_TypeDef* miso_gpio, uint16_t miso_pin);

#endif 

