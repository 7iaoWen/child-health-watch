/********************************************************************************
* @file		:onchip_conf.h
* @brief	:Ƭ�������ͨ��ͷ�ļ���ʹ��/ʧ�ܸ����������ü�
* @Author	:Gwen9
* @Date		:2024/08/01
* @Version	:V1.0 ��ʼ�汾
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ONCHIP_CONF_H
#define __ONCHIP_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif
 
/* Dependency file ------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
	 
#include "stm32f1xx.h"
#include "stm32f1xx_it.h"
#include "stm32f1xx_hal.h"
#include "stm32_hal_legacy.h"
/* Macros  -------------------------------------------------------------------*/
#define MY_MALLOC	my_heap.malloc
#define MY_FREE		my_heap.free

#define R_OK			(0)
#define R_ERROR         (1)
#define R_BUSY         	(2)
#define R_TIMEOUT       (3)

#define R_PARAM         (11)
#define R_NULL          (12)
#define R_OUT_OF_MEMORY (13)

//��ʱ��α���� ��ʵ��
#define DelayUs(n)		my_systick.delay_us(n)
#define DelayMs(n)		my_systick.delay_ms(n)
/* Exported constants --------------------------------------------------------*/
#define ONCHIP_MY_LOG_ENABLED

#define ONCHIP_MY_HEAP_ENABLED

#define ONCHIP_MY_GPIO_ENABLED

#define ONCHIP_MY_EXTI_ENABLED

#define ONCHIP_MY_ADC_ENABLED

#define ONCHIP_MY_TIMER_ENABLED

#define ONCHIP_MY_FLASH_ENABLED

#define ONCHIP_MY_IIC_ENABLED

#define ONCHIP_MY_SPI_ENABLED

#define ONCHIP_MY_UART_ENABLED

#define ONCHIP_MY_SYSTICK_ENABLED

#define ONCHIP_MY_EXTI_ENABLED
/* Typedef -------------------------------------------------------------------*/
#ifdef ONCHIP_MY_UART_ENABLED
	/*UART 接收缓存定义*/
	#define	UART1_RX_BUF_SIZE		512
	#define	UART2_RX_BUF_SIZE		512
	#define	UART3_RX_BUF_SIZE		512
	#define	UART_RX_BUF_MAX_SIZE	512
#endif	

#ifdef ONCHIP_MY_TIMER_ENABLED
	/*STM32定时器ID*/
	typedef enum
	{
		TIM_ID1 = 0,
		TIM_ID2,
		TIM_ID3,
		TIM_ID4,
		TIM_ID_MAX,
	}my_timer_id;
	
	#define MY_TIMER_MAX	TIM_ID_MAX
	
#endif 

#ifdef ONCHIP_MY_SYSTICK_ENABLED
	#define _TICK_US 	72			
	#define MAX_TIMERS 	10			
#endif
	
/* Includes ------------------------------------------------------------------*/                                  
/**
  * @brief Include module's header file
  */
#ifdef ONCHIP_MY_LOG_ENABLED
  #include "my_log.h" 
#endif 

#ifdef ONCHIP_MY_HEAP_ENABLED
  #include "my_heap.h" 
#endif 

#ifdef ONCHIP_MY_GPIO_ENABLED
  #include "my_gpio.h" 
#endif 

#ifdef ONCHIP_MY_EXTI_ENABLED
  #include "my_exti.h" 
#endif 

#ifdef ONCHIP_MY_ADC_ENABLED
  #include "my_adc.h" 
#endif 

#ifdef ONCHIP_MY_TIMER_ENABLED
  #include "my_timer.h" 
#endif 

#ifdef ONCHIP_MY_FLASH_ENABLED
  #include "my_flash.h" 
#endif 

#ifdef ONCHIP_MY_IIC_ENABLED
  #include "my_iic.h" 
#endif 

#ifdef ONCHIP_MY_SPI_ENABLED
  #include "my_spi.h" 
#endif

#ifdef ONCHIP_MY_UART_ENABLED
  #include "my_uart.h" 
#endif

#ifdef ONCHIP_MY_SYSTICK_ENABLED
  #include "my_systick.h" 
#endif

#ifdef ONCHIP_MY_EXTI_ENABLED
  #include "my_exti.h" 
#endif
/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __ONCHIP_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
