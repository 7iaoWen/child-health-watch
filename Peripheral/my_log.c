/********************************************************************************
* @file		:my_log.c
* @brief	:LOG 调试日志输出
* @Author	:Gwen9
* @Date		:2024/08/02
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_LOG_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/
/* Private Functions Declare ----------------------------------------------------------*/
static int my_log_show(char * info, uint32_t time);
static int my_log_register_show(void(*show_info)(char *, uint32_t));
/* Private Variables -------------------------------------------------------------------*/
//参数定义:	LOG 日志输出单例模型
const c_my_log my_log = {my_log_show, my_log_register_show};
static void(*pv_show_info)(char *, uint32_t) = NULL;
/* Private Functions -------------------------------------------------------------------*/
/** 
 *@brief 设置初始化ADC通道的状态
 */
static int my_log_show(char * info, uint32_t time)
{
	if(pv_show_info != NULL){
		pv_show_info(info, time);
	}
}

static int my_log_register_show(void(*show_info)(char *, uint32_t))
{
	pv_show_info = show_info;
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

