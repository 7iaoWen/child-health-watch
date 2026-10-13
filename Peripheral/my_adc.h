/********************************************************************************
* @file		:my_adc.h
* @brief	:my_adc.c头文件
* @Author	:Gwen9
* @Date		:2024/08/02
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_ADC_H__
#define __MY_ADC_H__


/** 
  * @brief  GPIO 初始化类型，输入/输出 
  */
typedef struct _MY_ADC
{
    /************************************************* 
    * Description: 片上ADC外设初始化程序
    * Input : <xChn> 需要初始化的ADC通道，@defgroup ADC_channels;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = my_adc.init(ADC_CHANNEL_0);
    *************************************************/  
	int(*init)(uint32_t xChn);

	/************************************************* 
    * Description: 翻转IO输出的电平状态
    * Input : <p_obj> IO对象;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
		float fvol;
		ret = my_adc.get(ADC_CHANNEL_0, &fvol);
    *************************************************/ 
	int(*get)(uint32_t xChn, float* fVol);
}c_my_adc;


/** 
  * @brief  片上ADC外设, 单例模型，扩展到外部使用
  */
extern const c_my_adc my_adc;


/** @defgroup ADC_channels ADC channels
  * @{
  */

/* ADC 通道可选参数说明
* STM32F103C8: ADC_CHANNEL_0 - ADC_CHANNEL_9
* STM32F103RC: ADC_CHANNEL_0 - ADC_CHANNEL_17
*/

#endif 

