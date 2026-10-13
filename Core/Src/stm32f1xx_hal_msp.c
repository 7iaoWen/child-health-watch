
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f1xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */
/*�ж����ȼ�*/
typedef enum __INTERRUPT_PRO
{
    HIGHEST_PRO = 5      ,
    EXTI_0_PRO      = 5  ,
    EXTI_1_PRO      = 5  ,
    EXTI_2_PRO      = 5  ,
    EXTI_3_PRO      = 5  ,
    EXTI_4_PRO      = 5  ,
    EXTI_5_9_PRO    = 5  ,
    EXTI_10_15_PRO  = 5  ,
    DMA_1_PRO       = 6  ,
    DMA_2_PRO       = 6  ,
    DMA_3_PRO       = 6  ,
    DMA_4_PRO       = 6  ,
    DMA_5_PRO       = 6  ,
    DMA_6_PRO       = 6  ,
    DMA_7_PRO       = 6  ,
    TIM_1_UP_PRO    = 7  ,
    TIM_2_UP_PRO    = 7  ,
    TIM_3_UP_PRO    = 7  ,
    TIM_4_UP_PRO    = 7  ,
    UART_1_RECV_PRO      ,
    UART_2_RECV_PRO      ,
    UART_3_RECV_PRO      ,
    UART_1_SEND_DMA_PRO  ,
    UART_2_SEND_DMA_PRO  ,
    UART_3_SEND_DMA_PRO  ,
    
    TIME_1_UP_PRO        ,
    
    LOWEST_PRO = 15      ,
}interrupt_pro;

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
	/*外设时钟初始化*/	
//	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
	
	/* System interrupt init*/
	/*配置外部中断优先级*/
    HAL_NVIC_SetPriority (EXTI0_IRQn    ,EXTI_0_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI1_IRQn    ,EXTI_1_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI2_IRQn    ,EXTI_2_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI3_IRQn    ,EXTI_3_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI4_IRQn    ,EXTI_4_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI9_5_IRQn  ,EXTI_5_9_PRO  ,0); 
    HAL_NVIC_SetPriority (EXTI15_10_IRQn,EXTI_10_15_PRO,0); 

     /*使能所有的中断线*/
    HAL_NVIC_EnableIRQ  (EXTI0_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI1_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI2_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI3_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI4_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI9_5_IRQn   );
    HAL_NVIC_EnableIRQ  (EXTI15_10_IRQn );

	/*配置DMA中断优先等级*/
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, DMA_1_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, DMA_2_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, DMA_3_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, DMA_4_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, DMA_5_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, DMA_6_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, DMA_7_PRO, 0);	

	/*使能DMA所有的中断通道*/
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

    /*配置TIM中断优先等级*/
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, DMA_1_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, DMA_2_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, DMA_3_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, DMA_4_PRO, 0);

    /*配置TIM中断优先等级*/
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);
	HAL_NVIC_EnableIRQ(TIM4_IRQn);
	
	/** NOJTAG: JTAG-DP Disabled and SW-DP Enabled
	*/
	__HAL_AFIO_REMAP_SWJ_NOJTAG();
}