/********************************************************************************
* @file		:my_log.h
* @brief	:my_log.c头文件
* @Author	:Gwen9
* @Date		:2024/08/02
* @Version	:V1.0 初始版本
*******************************************************************************/
  
#ifndef __MY_LOG_H__
#define __MY_LOG_H__


/** 
  * @brief  LOG 日志输出接口
  */
typedef struct _MY_LOG
{
  /************************************************* 
  * Description: LOG 日志输出
  * Input : <info> 需要显示的信息
  * Output: 无
  * Return: 无
  * Demo  : 
    ret = my_log.show("hello world!!!", 500);
  *************************************************/  
	int(*show)(char* info, uint32_t time);

  /************************************************* 
    * Description: 注册LOG显示函数
    * Input : <p_obj> IO对象;
    * Output: 无
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
	static void log_show_info(char *data, uint32_t time)
	{ 
		oled.clear(&oled);
		if(strlen(data) <= 64){
			oled.show(&oled, 0, 0, 16, data);
		}else{
			oled.show(&oled, 0, 0, 16, "OVER LENTH!!!");
		}
		DelayMs(time);
		oled.clear(&oled);
	}
    ret = my_log.register_show(log_show_info);
    *************************************************/ 
	int(*register_show)(void(*show_info)(char *, uint32_t));
}c_my_log;


/** 
  * @brief  片上ADC外设, 单例模型，扩展到外部使用
  */
extern const c_my_log my_log;

#endif 

