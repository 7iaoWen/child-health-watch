/********************************************************************************
* @file		:my_uart.h
* @brief	:my_uart.c头文件
* @Author	:Gwen9
* @Date		:2024/08/04
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_UART_H__
#define __MY_UART_H__

/* Public Macros  -------------------------------------------------------------------*/
/*定义DMA串口 发送的超时时间*/
#define MY_UART_DMA_SEND_OVERTIME	1000


/* Public Typedef  -------------------------------------------------------------------*/
/*串口序号定义*/
typedef enum
{
	MY_UART1 = 0,
	MY_UART2,
	MY_UART3,
	UART_MAX,
	MY_UART4,
	MY_UART5,
}my_uart_id_t;

/*串口工作模式定义*/
typedef enum
{
    UART_MODE_POLL = 0, //轮询模式 （暂未启用）
    UART_MODE_ISR,      //中断模式
    UART_MODE_DMA,      //DMA模式
}uart_mode_t;

/** 
  * @brief  GPIO 初始化类型，输入/输出 
  */
typedef struct _MY_UART
{
    /************************************************* 
    * Description: UART 串口初始化程序
    * Input : <uart_id> 待初始化的串口编号;
			  <baud_rate> 波特率;
			  <mode> 工作模式 ISR/DMA;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  ret = my_uart.init(MY_UART1, 115200, UART_MODE_DMA);
    *************************************************/  
	int(*init)(my_uart_id_t uart_id, uint32_t baud_rate, uart_mode_t mode);

    /************************************************* 
    * Description: UART 串口 数据发送函数
    * Input : <uart_id> 待初始化的串口编号;
			  <data> 待发送数据缓存;
			  <len> 待发送的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  ret = my_uart.send(MY_UART1, data, 32);
    *************************************************/  
	int(*send)(my_uart_id_t uart_id, uint8_t *data, uint32_t len);
	
	/************************************************* 
    * Description: UART 串口 数据接收函数
    * Input : <uart_id> 待初始化的串口编号;
			  <data> 待发送数据缓存;
			  <len> 待发送的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			  ret = my_uart.recive(MY_UART1, data, &len);
    *************************************************/  
	int(*recive)(my_uart_id_t uart_id, uint8_t *data, uint32_t *len);

	/************************************************* 
    * Description: UART 串口 数据接收函数
    * Input : <uart_id> 待初始化的串口编号;
			  <data> 待发送数据缓存;
			  <len> 待发送的数据长度;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
        void uart3_callback(void* param, uint8_t* data, uint16_t data_len) { }
			  ret = my_uart.register_callback(MY_UART1, uart3_callback, NULL);
    *************************************************/  
	int(*register_callback)(my_uart_id_t uart_id, void(*callback)(void* , uint8_t* , uint16_t), void *param);
}c_my_uart;

/* Public Variables -----------------------------------------------------------------*/
/** 
  * @brief  片上 FLASH 外设, 单例模型，扩展到外部使用
  */
extern const c_my_uart my_uart;

#endif 

