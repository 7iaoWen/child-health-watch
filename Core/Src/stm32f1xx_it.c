/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
/* 	以 72MHz 主频为例：每个时钟周期的时间 = 1/72M = 13.89 ns

	整数加减运算	：执行时间 ≈ 13.89ns（1 个时钟周期）
	整数乘法		：执行时间 ≈ 13.89ns - 41.67ns（1 到 3 个时钟周期）
	整数除法		：执行时间 ≈ 27.78ns - 166.67ns（2 到 12 个时钟周期）

	浮点加减运算	：以 100 到 300 个时钟周期为例，大致估算时间为 ：1.39μs - 4.17μs
	浮点乘法		：以 200 到 500 个时钟周期为例，大致估算时间为 ：2.78μs - 6.94μs
	浮点除法		：以 500 到 1500 个时钟周期为例，大致估算时间为：6.94μs - 20.83μs

	ARM Cortex-M3 的函数调用开销 综合时间估算：
	假设一个简单的函数调用需要保存 4 个寄存器（R0-R3）和链接寄存器（LR），然后返回：
	入栈时间：PUSH {r0-r3, lr} ≈ 6 个时钟周期 ≈ 83.34ns
	出栈时间：POP {r0-r3, pc} ≈ 6 个时钟周期 ≈ 83.34ns
	跳转时间：BX LR ≈ 1 个时钟周期 ≈ 13.89ns
	总时间：83.34ns（入栈） + 83.34ns（出栈） + 13.89ns（跳转） ≈ 180.57ns

	实际应用中，函数调用的时间还会受到其他因素的影响，例如：
	编译器生成的代码：编译器可能会生成额外的代码来处理参数传递、保存和恢复寄存器。
	中断处理：如果在函数调用过程中发生中断，中断服务程序 (ISR) 的执行时间也需要考虑在内。
	堆栈使用效率：使用优化的堆栈管理技术（如寄存器重命名、内联函数等）可以减少函数调用的开销。
*/
extern void HAL_SYSTICK_Callback(void);
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  
  /* USER CODE BEGIN SysTick_IRQn 1 */
  HAL_SYSTICK_Callback();  //HAL中已定义为 __weak 类型，在 my_systick.c 重新实现				
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */
typedef struct
{
    void(*int_func)(dma_chn_num_t,void*);
    void*              param;
}dma_func_t;

static void m_dma_call_back(dma_chn_num_t dma_chn);

static dma_func_t dma_func_list[7];

void DMA1_Channel1_IRQHandler(void)
{
	m_dma_call_back(DMAChn1);
}

void DMA1_Channel2_IRQHandler(void)
{
	m_dma_call_back(DMAChn2);
}

void DMA1_Channel3_IRQHandler(void)
{
	m_dma_call_back(DMAChn3);
}

void DMA1_Channel4_IRQHandler(void)
{
	m_dma_call_back(DMAChn4);
}

void DMA1_Channel5_IRQHandler(void)
{
	m_dma_call_back(DMAChn5);
}

void DMA1_Channel6_IRQHandler(void)
{
	m_dma_call_back(DMAChn6);
}

void DMA1_Channel7_IRQHandler(void)
{
	m_dma_call_back(DMAChn7);
}

void it_register_dma(dma_chn_num_t dma_chn, void(*int_func)(dma_chn_num_t,void*), void* param)
{
	/*检查参数*/
	if(NULL == int_func){
		return;
	}
	
	/*检查是否已经被注册*/
	if(NULL != dma_func_list[dma_chn].int_func){
		return;
	}

	/*注册*/
	dma_func_list[dma_chn].int_func = int_func;
	dma_func_list[dma_chn].param = param;

	return;
}

static void m_dma_call_back(dma_chn_num_t dma_chn)
{
	/*执行对应的回调函数*/
    if(NULL != dma_func_list[dma_chn].int_func){
        dma_func_list[dma_chn].int_func(dma_chn, dma_func_list[dma_chn].param);
    }
}

/* USER CODE END 1 */
