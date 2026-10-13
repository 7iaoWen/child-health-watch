/********************************************************************************
* @file		:ds1302.h
* @brief	:ds1302.c 头文件
* @Author	:Gwen9
* @Date		:2024/08/06
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __DS1302_H__
#define __DS1302_H__

/* Public Macros  -------------------------------------------------------------------*/
#define DS1302_TIME_VAILID      0xA5		//重置时间前需要修改此宏定义为其他任意不等值

//系统默认时间
#define DS1302_DEF_YEAR     	2024	//0-99
#define DS1302_DEF_MON      	9		//1-12
#define DS1302_DEF_DATE     	1		//1-31	
#define DS1302_DEF_HOUR     	12		//1-12/0-23
#define DS1302_DEF_MIN      	30		//0-59
#define DS1302_DEF_SEC       	45		//0-59
#define DS1302_DEF_WEEK      	7		//1-7

/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief 系统时间结构体定义 
  */
typedef struct _SYS_TIME_T
{
	unsigned short year;
	unsigned char mon;
	unsigned char date;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char weekday;
}sys_time_t;

/** 
  * @brief DS1302 类，结构体定义 
  */
typedef struct _DS1302 c_ds1302;
typedef struct _DS1302
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;
	
	/************************************************* 
    * Description: 获取 DS1302 的时间
    * Param : <p_obj> DS1302 对象;
			  <pTime> 用于保存获取到的时间;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		sys_time_t time;
		ret = ds1302.get_time(&ds1302, &time);
    *************************************************/ 	
    int(*get_time)(const c_ds1302* p_obj, sys_time_t *time);
	
	/************************************************* 
    * Description: 设置 DS1302 的时间
    * Param : <p_obj> DS1302 对象;
			  <pTime> 待设置的时间;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		sys_time_t time;
		ret = ds1302.set_time(&ds1302, &time);
    *************************************************/ 	
    int(*set_time)(const c_ds1302* p_obj, sys_time_t *time);
	
	/************************************************* 
    * Description: 获取DS1302 RAM 区数据
    * Param : <p_obj> DS1302 对象;
			  <addr> 地址;
			  <data> 数据;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t data;
		ret = ds1302.get_ram(&ds1302, 0x01, &data);
    *************************************************/ 	
    int(*get_ram)(const c_ds1302* p_obj, uint8_t addr, uint8_t *data);
	
	/************************************************* 
    * Description: 设置 DS1302 RAM区数据
    * Param : <p_obj> DS1302 对象;
			  <addr> 地址;
			  <data> 数据;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t data;
		ret = ds1302.set_ram(&ds1302, 0x01, data);
    *************************************************/ 
    int(*set_ram)(const c_ds1302* p_obj, uint8_t addr, uint8_t data);
}c_ds1302;


/* Public Interface  -------------------------------------------------------------------*/
/************************************************* 
* Description: 创建一个 DS1302 对象
* Param : <clk_gpio> 	DS1302 时钟线 GPIO;
		  <clk_pin> 	DS1302 时钟线 引脚;
		  <dat_gpio> 	DS1302 数据线 GPIO ;
		  <dat_pin> 	DS1302 数据线 引脚;
		  <rst_gpio> 	DS1302 复位引脚 GPIO ;
		  <rst_pin> 	DS1302 复位引脚 引脚;
* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
* Demo  : 
	c_ds1302 ds1302 = ds1302_create(GPIOA, GPIO_PIN_8, GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10);
*************************************************/ 
c_ds1302 ds1302_create(GPIO_TypeDef* clk_gpio, uint16_t clk_pin, 
					   GPIO_TypeDef* dat_gpio, uint16_t dat_pin,
					   GPIO_TypeDef* rst_gpio, uint16_t rst_pin);

#endif 

