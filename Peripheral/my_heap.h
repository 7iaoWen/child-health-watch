/********************************************************************************
* @file		:my_heap.h
* @brief	:my_heap.c头文件
* @Author	:Gwen9
* @Date		:2024/09/14
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __MY_HEAP_H__
#define __MY_HEAP_H__

/* Public Macros  -------------------------------------------------------------------*/
// 定义堆的总大小
#define configTOTAL_HEAP_SIZE (1024) 
// 对齐字节数
#define portBYTE_ALIGNMENT      8 

#if portBYTE_ALIGNMENT == 32
	#define portBYTE_ALIGNMENT_MASK ( 0x001f )
#endif

#if portBYTE_ALIGNMENT == 16
	#define portBYTE_ALIGNMENT_MASK ( 0x000f )
#endif

#if portBYTE_ALIGNMENT == 8
	#define portBYTE_ALIGNMENT_MASK ( 0x0007 )
#endif

#if portBYTE_ALIGNMENT == 4
	#define portBYTE_ALIGNMENT_MASK	( 0x0003 )
#endif

#if portBYTE_ALIGNMENT == 2
	#define portBYTE_ALIGNMENT_MASK	( 0x0001 )
#endif

#if portBYTE_ALIGNMENT == 1
	#define portBYTE_ALIGNMENT_MASK	( 0x0000 )
#endif

#ifndef portBYTE_ALIGNMENT_MASK
	#error "Invalid portBYTE_ALIGNMENT definition"
#endif

/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief  MY_HEAP 单例模型使用接口
  */
typedef struct _MY_HEAP
{
    /************************************************* 
    * Description: 内存分配
    * Input : <xWantedSize> 需要初始化的ADC通道，@defgroup ADC_channels;
    * Return: 无
    * Demo  : 
        uint8_t pdata;
        pdata = my_heap.malloc(sizeof(buf));
    *************************************************/  
    void * (*malloc)(uint32_t xWantedSize);

    /************************************************* 
    * Description: 内存释放
    * Input : <p_obj> IO对象;
    * Return: 无
    * Demo  : 
        my_heap.free(pdata);
    *************************************************/ 
    void (*free)(void *pv);

    /************************************************* 
    * Description: 获取当前可用堆大小
    * Input : 无
    * Return: 当前可用堆大小
    * Demo  : 
        my_heap.get_free_heapsize();
    *************************************************/ 
    uint32_t(*get_free_heapsize)(void);

      /************************************************* 
    * Description: 获取历史最小可用堆大小
    * Input : 无
    * Return: 历史最小可用堆大小
    * Demo  : 
        my_heap.get_min_ever_free_heapsize();
    *************************************************/ 
    uint32_t(*get_min_ever_free_heapsize)(void);
}c_my_heap;

/** 
  * @brief  MY_HEAP 单例模型，扩展到外部使用
  */
extern const c_my_heap my_heap;

#endif 

