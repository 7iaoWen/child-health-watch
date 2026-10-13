/********************************************************************************
* @file		:my_heap.c
* @brief	:对STM32 HAL 库中 ADC 的二次封装，提供 ADC 的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/02
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_HEAP_ENABLED

/* Private Macros  -------------------------------------------------------------------*/
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) ) 	// 最小块大小
#define heapBITS_PER_BYTE		( ( size_t ) 8 ) 							// 每字节的位数

/* Private Typedef  -------------------------------------------------------------------*/
// 定义块链接结构体
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< 指向下一个空闲块 */
    size_t xBlockSize;						/*<< 空闲块的大小 */
} BlockLink_t;

/* Private Functions Declare ----------------------------------------------------------*/
static void *my_heap_malloc(uint32_t xWantedSize);
static void my_heap_free(void *pv);
static uint32_t my_heap_get_free_heapsize(void);
static uint32_t my_heap_get_min_ever_free_heapsize(void);
static void my_heap_init(void);
static void my_heap_insert_block_into_freelist(BlockLink_t *pxBlockToInsert);

/* Private Variables -------------------------------------------------------------------*/
/*参数定义:	HEAP 单例模型，扩展到外部使用定义*/
const c_my_heap my_heap = {my_heap_malloc, my_heap_free, my_heap_get_free_heapsize, my_heap_get_min_ever_free_heapsize}; 
/*堆内存区域*/ 
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ]; 
/* 块头的大小必须正确字节对齐。 */
static const uint32_t xHeapStructSize = (sizeof(BlockLink_t) + ((uint32_t)(portBYTE_ALIGNMENT-1))) & ~ ((uint32_t)portBYTE_ALIGNMENT-1);
/* 创建链表头和链表尾指针 */
static BlockLink_t xStart, *pxEnd = NULL;
/* 记录剩余可用字节数，但不能显示内存碎片*/
static uint32_t xFreeBytesRemaining = 0U;
static uint32_t xMinimumEverFreeBytesRemaining = 0U;

/* 获取 uint32_t 类型的最高位。
当该位在 BlockLink_t 的 xBlockSize 成员中被设置时，块属于应用程序。
当该位未设置时，该块仍然是自由堆空间的一部分。 */
static uint32_t xBlockAllocatedBit = 0;
/* Public Functions -------------------------------------------------------------------*/
void *my_heap_malloc( uint32_t xWantedSize )
{
    BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
    void *pvReturn = NULL;

	/* 如果这是第一次调用 malloc，则需要通过初始化来设置空闲块列表。 */
	if( pxEnd == NULL )
	{
		my_heap_init();
	}

	/* 检查请求的块大小是否过大，最高位应设置为 0。 */
	if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
	{
		/* 请求的块大小增加以适应 BlockLink_t 结构体和所需字节 */
		if( xWantedSize > 0 )
		{
			xWantedSize += xHeapStructSize;

			/* 确保块始终按要求的字节数对齐。 */
			if( ( xWantedSize & ( portBYTE_ALIGNMENT - 1 ) ) != 0 )
			{
				xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & ( portBYTE_ALIGNMENT - 1 ) ) );
			}
		}

		if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
		{
			/* 从低地址块开始遍历链表，直到找到合适大小的块。 */
			pxPreviousBlock = &xStart;
			pxBlock = xStart.pxNextFreeBlock;
			while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
			{
				pxPreviousBlock = pxBlock;
				pxBlock = pxBlock->pxNextFreeBlock;
			}

			/* 如果到达了结束标志，则未找到合适大小的块。 */
			if( pxBlock != pxEnd )
			{
				/* 返回指向的内存空间，跳过 BlockLink_t 的结构体。 */
				pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

				/* 此块正在被返回使用，因此必须将其从空闲块列表中移除。 */
				pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

				/* 如果该块大于请求的大小，则可以将其拆分为两个块。 */
				if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
				{
					/* 该块要拆分为两个块。创建在请求的字节之后的新块。 */
					pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

					/* 计算从单个块中拆分出的两个块的大小。 */
					pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
					pxBlock->xBlockSize = xWantedSize;

					/* 将新块插入空闲块列表中。 */
					my_heap_insert_block_into_freelist( pxNewBlockLink );
				}

				/* 更新剩余字节数 */
				xFreeBytesRemaining -= pxBlock->xBlockSize;

				if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
				{
					xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
				}

				/* 此块正在被返回 - 它已分配并归应用程序所有，并且没有“下一个”块。 */
				pxBlock->xBlockSize |= xBlockAllocatedBit;
				pxBlock->pxNextFreeBlock = NULL; // 设置下一个块指针为 NULL
			}
		}
	}
    return pvReturn;
}

void my_heap_free( void *pv )
{
    uint8_t *puc = ( uint8_t * ) pv;
    BlockLink_t *pxLink;

    if( pv != NULL )
    {
        /* 被释放的内存会在它前面有一个 BlockLink_t 结构。 */
        puc -= xHeapStructSize;

        /* 这样做是为了防止编译器发出警告。 */
        pxLink = ( void * ) puc;

        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            /* 该块正在返回给堆 - 它不再属于已分配块。 */
            pxLink->xBlockSize &= ~xBlockAllocatedBit;

			/* 将该块添加到空闲块列表。 */
			xFreeBytesRemaining += pxLink->xBlockSize;
			my_heap_insert_block_into_freelist( ( ( BlockLink_t * ) pxLink ) );
        }
    }
}

uint32_t my_heap_get_free_heapsize( void )
{
    return xFreeBytesRemaining; // 返回剩余可用字节数
}

uint32_t my_heap_get_min_ever_free_heapsize( void )
{
    return xMinimumEverFreeBytesRemaining; // 返回历史最小可用字节数
}

/* Private Functions -------------------------------------------------------------------*/
static void my_heap_init( void )
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    uint32_t uxAddress;
    uint32_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* 确保堆以正确对齐的边界开始。 */
    uxAddress = (uint32_t)ucHeap;

    if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
    {
        uxAddress += (portBYTE_ALIGNMENT - 1);
        uxAddress &= ~( (uint32_t) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - (uint32_t)ucHeap;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* xStart 用于持有空闲块链表的第一个元素的指针。 */
    xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
    xStart.xBlockSize = (uint32_t)0;

    /* pxEnd 用于标记空闲块链表的结束，并插入在堆空间的末尾。 */
    uxAddress = ((uint32_t)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= sizeof(BlockLink_t);
    uxAddress &= ~( (uint32_t) portBYTE_ALIGNMENT_MASK );
    pxEnd = (void *)uxAddress; // 设置 pxEnd 为堆的末尾
    pxEnd->xBlockSize = 0; // 结束块的大小设置为0
    pxEnd->pxNextFreeBlock = NULL; // 下一个指针设为NULL

    /* 开始时只有一个空闲块，该块的大小为整个堆空间 */
    pxFirstFreeBlock = (void *)pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - (uint32_t)pxFirstFreeBlock; // 计算空闲块大小
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd; // 链接到结束块

    /* 仅有一个块，覆盖整个可用堆空间。 */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize; // 初始化最小可用
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize; // 初始化可用字节

    /* 计算 uint32_t 变量中顶位的位置。 */
    xBlockAllocatedBit = ( (uint32_t) 1 ) << ( (sizeof(uint32_t) * heapBITS_PER_BYTE) - 1 );
}

static void my_heap_insert_block_into_freelist(BlockLink_t *pxBlockToInsert)
{
    BlockLink_t *pxIterator;
    uint8_t *puc;

    /* 遍历链表，找到适合插入的新块位置 */
    for(pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* 可能与前一个块合并 */
    puc = (uint8_t *)pxIterator;
    if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert) 
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize; // 合并块
        pxBlockToInsert = pxIterator; // 更新插入块
    }

    /* 可能与后一个块合并 */
    puc = (uint8_t *)pxBlockToInsert;
    if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock) 
    {
        if (pxIterator->pxNextFreeBlock != pxEnd) 
        {
            /* 合并两个块 */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock; // 更新链表
        } 
        else 
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd; // 更新结束块
        }
    } 
    else 
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock; // 插入新块
    }

    // 如果未发生合并，插入块
    if (pxIterator != pxBlockToInsert) 
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
}


#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

