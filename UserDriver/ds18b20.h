/********************************************************************************
* @file		:ds18b20.h
* @brief	:ds18b20.c头文件
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __DS18B20_H__
#define __DS18B20_H__


/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief DS18B20 类，结构体定义 
  */
typedef struct _DS18B20 c_ds18b20;
typedef struct _DS18B20
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;

	/************************************************* 
    * Description: DS18B20 获取实时温度接口
    * Input : <p_obj> DS18B20 对象;
			  <temp> 用于保存获取到的温度数据;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		float temp;
		ret = ds18b20.get(&ds18b20, &temp);
    *************************************************/ 
    int(*get)(const c_ds18b20* p_obj, float* temp);
	
	/************************************************* 
    * Description: DS18B20 获取当前DS18B20的 ROM ID
    * Input : <p_obj> DS18B20 对象;
			  <rom_id> 用于保存获取到的 ROM ID;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		uint8_t rom_id[8];
		ret = ds18b20.rom_id(&ds18b20, rom_id);
    *************************************************/ 
    int(*rom_id)(const c_ds18b20* p_obj, uint8_t *rom_id);
	
	/************************************************* 
    * Description: DS18B20 获取匹指定ROM ID的温度数据
    * Input : <p_obj> DS18B20 对象;
			  <temp> 用于保存获取到的温度数据;
			  <rom_id> 指定 DS18B20 的 ROM ID;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		float temp;
		ret = ds18b20.get_match_rom_id(&ds18b20, &temp, rom_id);
    *************************************************/ 
    int(*get_match_rom_id)(const c_ds18b20* p_obj, float *temp, uint8_t *rom_id);
}c_ds18b20;


/************************************************* 
* Description: 创建一个 DS18B20 对象
* Input : <gpio> DS18B20 对象;
		  <pin> 用于保存获取到的温度数据;
* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
* Demo  : 
	c_ds18b20 ds18b20 = ds18b20_create(GPIOA, GPIO_PIN_8);
*************************************************/ 
c_ds18b20 ds18b20_create(GPIO_TypeDef* GPIOx,uint32_t GPIO_Pin);
#endif 

