/********************************************************************************
* @file		:my_flash.h
* @brief	:my_flash.c头文件
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_FLASH_H__
#define __MY_FLASH_H__

#define MY_FLASH_START 0x0800F000UL
#define MY_FLASH_END   0x08010000UL

/** 
  * @brief  Flash储存参数队列 
  */
typedef struct _MY_FLASH_NODE
{
	unsigned char nValid;
	uint32_t xID;
	uint32_t xFlashAddr;
	struct _MY_FLASH_NODE* pNext;
}my_flash_node_t;

/** 
  * @brief  GPIO 初始化类型，输入/输出 
  */
typedef struct _MY_FLASH
{
    /************************************************* 
    * Description: 片上 Flash 外设初始化程序
    * Input : <xPageCnt> 需要初始化的 Flash 页数 STM32F103C8 系列 1页=1kb
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		`ret = my_flash.init(4);
    *************************************************/  
	int(*init)(uint32_t xPageCnt);

	/************************************************* 
    * Description: 片上 Flash 擦除函数
    * Input : <xStartAddr> 擦除的起始地址，1K 地址对齐; 
			  <xPageCnt> 擦除的页数;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		ret = my_flash.erase_pages(MY_FLASH_START, 4);
    *************************************************/ 
	int(*erase_pages)(uint32_t xStartAddr, uint32_t xPageCnt);
	
	/************************************************* 
    * Description: 片上 Flash 写入函数
    * Input : <xStartAddr> 擦除的起始地址，1K 地址对齐; 
			  <pBuffer> 带写入的数据;
			  <xSize> 写入的数据大小;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t w_buf[64];
		ret = my_flash.write_bytes(MY_FLASH_START, w_buf, 64);
    *************************************************/ 
	int(*write_bytes)(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);
	
	/************************************************* 
    * Description: 片上 Flash 写入函数
    * Input : <xStartAddr> 擦除的起始地址，1K 地址对齐; 
			  <pBuffer> 带写入的数据;
			  <xSize> 写入的数据大小;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t r_buf[64];
		ret = my_flash.read_bytes(MY_FLASH_START, r_buf, 64);
    *************************************************/ 
	int(*read_bytes)(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);
	
}c_my_flash;


/** 
  * @brief  片上 FLASH 外设, 单例模型，扩展到外部使用
  */
extern const c_my_flash my_flash;


#endif 

