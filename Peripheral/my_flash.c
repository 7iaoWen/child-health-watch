/********************************************************************************
* @file		:my_flash.c
* @brief	:对 STM32 HAL 库中 FLASH 的二次封装，提供 FLASH 操作的所有操作接口
* @Author	:Gwen9
* @Date		:2024/08/03
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_MY_FLASH_ENABLED

/* Private Typedef  -------------------------------------------------------------------*/

	
/* Private Functions Declare ----------------------------------------------------------*/
static int my_flash_init(uint32_t xPageCnt);
static int my_flash_erase_pages(uint32_t xStartAddr, uint32_t xPageCnt);
static int my_flash_write_bytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);
static int my_flash_read_bytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);

/* Private Variables -------------------------------------------------------------------*/
//参数定义:	ADC单例模型，扩展到外部使用定义
const c_my_flash my_flash = {my_flash_init, my_flash_erase_pages, my_flash_write_bytes, my_flash_read_bytes};
//参数定义:	FLASH 初始化状态
static bool _flash_init_state = false;
//参数定义:	FLASH 初始化状态
static uint32_t my_flash_start;

/* Private Functions -------------------------------------------------------------------*/
/** 
*@brief 设置初始化单片机内部 Flash
 */
static int my_flash_init(uint32_t xPageCnt)
{
	if(!_flash_init_state){
		my_flash_start = MY_FLASH_END - (xPageCnt * FLASH_PAGE_SIZE);
		if(my_flash_start <= MY_FLASH_END){
			return R_ERROR;
		}else{
			_flash_init_state = true;
		}
	}
	return R_OK;
}

static int my_flash_save_param(uint32_t xId, void *pNode, uint32_t xNodeSize)
{
	

}
	

/** 
*@brief 单片机内部Flash擦除页
 */
static int my_flash_erase_pages(uint32_t xStartAddr, uint32_t xPageCnt)
{
    uint32_t SECTORError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
	
	/*检查地址范围有效性*/
    if(IS_FLASH_PROGRAM_ADDRESS(xStartAddr) == 0){
        return R_PARAM;   
    }   
	
    /*检查地址是否按照 FLASH_PAGE_SIZE 对齐*/
    if((xStartAddr%FLASH_PAGE_SIZE) != 0){
        return R_PARAM;   
    }
	
    /*擦除数据*/
    HAL_FLASH_Unlock();   
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_PAGES;          /*擦除方式：大量擦除/按页擦除*/
    EraseInitStruct.Banks        = FLASH_BANK_1;                   /*选择BANK, STM32F103C8 只有 BANK1*/
    EraseInitStruct.NbPages      = xPageCnt;                       /*需要擦除的页数 STM32F103C8 每页1K*/ 
    EraseInitStruct.PageAddress  = xStartAddr;                     /*擦除起始页*/    
    if(HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK){
        HAL_FLASH_Lock();
        return R_ERROR;   
    }
    HAL_FLASH_Lock();
	
    return R_OK;
}

/** 
*@brief 单片机内部Flash写入数据
 */
static int my_flash_write_bytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize)
{
	uint32_t xWrAddr = xStartAddr;
    uint32_t *pWrDataBuf = (uint32_t*)pBuffer;
    uint32_t xWrDataTemp;
	
    /*检查地址范围有效性*/
    if(IS_FLASH_PROGRAM_ADDRESS(xStartAddr) == 0){
        return R_PARAM;    
    }
	
    /*写入大小必须4字节对齐*/
    if((xSize%4) != 0){
        return R_PARAM;   
    }
	
    /*写入数据, FLASH_TYPEPROGRAM_WORD 即为按 字 写入，所以写入大小必须按照4字节对齐*/
    HAL_FLASH_Unlock(); 
    while((xWrAddr - xStartAddr) < xSize){
        xWrDataTemp = *(pWrDataBuf);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, xWrAddr, xWrDataTemp) != HAL_OK){
            HAL_FLASH_Lock();
            return R_ERROR;
        }
        xWrAddr += 4;
        pWrDataBuf++;
    }
    HAL_FLASH_Lock();
	
    return R_OK;
}


/** 
*@brief 单片机内部Flash读取数据
 */
static int my_flash_read_bytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize)     
{
    /*读取数据*/
    for(uint32_t xRdAddr = xStartAddr; xRdAddr < (xStartAddr+xSize); xRdAddr++)
    {
        *pBuffer = *((uint8_t*)xRdAddr); 
        pBuffer++;
    }
    return R_OK;
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
note:
在参考手册 RM0394 中第3节我们可以看到 STM32L4 系列的 Flsah 介绍
在3.3.1节中查看 L4系列的Flash主要有三种 128K ,256K, 512K,
2K为一页.页为Flash擦除和写入的最小单位
（STM32还有以扇区分的，而且扇区大小不一定相同，此时擦除写入的最小单位就是扇区）

可以根据芯片的名称快速知道 STM32 的 Flash 容量
STM32L4CCT6 第一个C表示引脚数为48脚，第二个C表示 Flash 容量为 256K
另外也可以在.icf连接文件中找到：
define symbol __ICFEDIT_region_ROM_start__    = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__      = 0x0803FFFF;
可以看到 0x08000000 - 0x0803FFFF的 256K 内容就是STM32的Flash大小

note:STM32内部 FLASH介绍
STM32内部 FLASH 包含
1.主存储器
一般说STM32内部 FLASH 时都是指这个主存储器区域，它是存储用户应用程序的空间.
2.系统存储器
系统存储区是用户不能访问的区域，它在芯片出厂时已经固化了启动代码，它负
责实现串口、 USB 以及 CAN 等 ISP 烧录功能。
3.OTP区
OTP(One Time Program)，指的是只能写入一次的存储区域，容量为 512 字节，写
入后数据就无法再更改， OTP 常用于存储应用程序的加密密钥。
4.选项字节区
选项字节用于配置 FLASH 的读写保护、电源管理中的 BOR 级别、软件/硬件看门
狗等功能，这部分共 32 字节。可以通过修改 FLASH 的选项控制寄存器修改。

note:STM32内部FLASH的写入过程：
1. 解锁
芯片复位后默认会结FLASH上锁，这时不允许设置FLASH的控制寄存器，
并且不能修改FLASH中的内容。解锁的操作步骤如下：
(1) 往 Flash 密钥寄存器 FLASH_KEYR 中写入     KEY1 = 0x45670123
(2) 再往 Flash 密钥寄存器 FLASH_KEYR 中写入   KEY2 = 0xCDEF89AB

2. 数据操作位数
在内部FLASH进行擦除及写入操作时，电源电压会影响数据的最大操作位数，该电
源电压可通过配置FLASH_CR寄存器中的PSIZE位改变，
电压范围        2.7 - 3.6 V(使用外部 Vpp)  2.7 - 3.6 V  2.1 – 2.7 V  1.8 – 2.1 V
位数              64                          32              16          8
PSIZE(1:0)配置    11b                         10b             01b         00b
最大操作位数会影响擦除和写入的速度，64位宽度的操作除了配置寄存器位外，
还需要在Vpp引脚外加一个8-9V的电压源，且其供电时间不得超过一小时，否则FLASH
可能损坏，所以64位宽度的操作一般是在量产时对FLASH写入应用程序时才使用，
大部分应用场合都是用32位的宽度

3. 擦除扇区
在写入新的数据前，需要先擦除存储区域， STM32提供了扇区擦除指令和整个
FLASH擦除(批量擦除)的指令，批量擦除指令仅针对主存储区。
扇区擦除的过程如下：
(1) 检查FLASH_SR寄存器中的“忙碌寄存器位 BSY”，确认当前未执行任何Flash操作；
(2) 在FLASH_CR寄存器中，将“激活扇区擦除寄存器位SER”置 1，并设置“扇区编号寄存器位 SNB”，选择要擦除的扇区；
(3) 将 FLASH_CR 寄存器中的“开始擦除寄存器位 STRT ”置 1，开始擦除；
(4) 等待 BSY 位被清零时，表示擦除完成。

4. 写入数据
擦除完毕后即可写入数据，写入数据的过程并不是仅仅使用指针向地址赋值，赋值前
还还需要配置一系列的寄存器，步骤如下：
(1) 检查FLASH_SR中的BSY位，以确认当前未执行任何其它的内部Flash操作；
(2) 将FLASH_CR寄存器中的 “激活编程寄存器位 PG” 置 1；
(3) 针对所需存储器地址（主存储器块或 OTP 区域内）执行数据写入操作；
(4) 等待BSY位被清零时，表示写入完成。

note:查看工程的空间分布
在使用内部FLASH存储其它数据前需要了解哪一些空间已经写入了程序代码，
存储了程序代码的扇区都不应作任何修改。通过查询.map文件可以了解程序存储到了哪些区域
*/


