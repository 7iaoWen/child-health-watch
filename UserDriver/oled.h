/********************************************************************************
* @file		:oled.h
* @brief	:oled.c头文件，取模方式：阴码，逆向，列行式
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/
#ifndef __OLED_H_
#define __OLED_H_

/* Public Typedef  -------------------------------------------------------------------*/
#define OLED_DEFAULT_IIC_ADDR  	0x3C 	/*显示屏 地址*/

/* Public Typedef  -------------------------------------------------------------------*/
/** 
  * @brief OLED类，结构体定义 
  */
typedef struct _OLED c_oled;
typedef struct _OLED
{
	/************************************************* 
    * Description: 空指针，指向该对象的私有成员
    *************************************************/  
    void* this;

	/************************************************* 
    * Description: OLED 清屏函数
    * Input : <p_obj> OLED 对象;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = oled.clear(&oled);
    *************************************************/ 
	 int(*clear)(const c_oled* p_obj);

	/************************************************* 
    * Description: 让OLED休眠, 休眠模式下,OLED功耗不到10uA
    * Input : <p_obj> OLED 对象;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = oled.display_off(&oled);
    *************************************************/ 
	 int(*display_off)(const c_oled* p_obj);

	/************************************************* 
    * Description: 将OLED从休眠中唤醒
    * Input : <p_obj> OLED 对象;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = oled.display_on(&oled);
    *************************************************/ 
	 int(*display_on)(const c_oled* p_obj);

	/************************************************* 
    * Description: OLED  显示内容，支持中英文显示，中文 GB2312 编码, 所以 以中文参数调用该函数时，此文件 必须为 GB2312 编码，文件转为gb2312编码步骤见 页末
    * Input : <p_obj> OLED 对象;
			  <x> OLED 对象;
			  <y> OLED 对象;
			  <size> 字体大小 16/12可选;
			  <fmt> 格式化字符串，可变参数;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = oled.show(&oled, 0, 0, 16, "temp:%.1f", temp);
			ret = oled.show(&oled, 0, 0, 16, "温度:%.1f", temp);
    *************************************************/ 
	 int(*show)(const c_oled* p_obj, uint8_t x, uint8_t y, uint8_t size, const char* fmt, ...);

    /************************************************* 
    * Description: OLED  显示 BMP 图像, 矩形图像
    * Input : <p_obj> OLED 对象;
    *         <x0> <y0> 矩形左上角顶点;
    *         <x1> <y1> 矩形右下角顶点;
    *         <BMP> 图像数据;
    * Return: <R_OK> 操作成功; <R_ERROR> 操作失败
    * Demo  : 
			ret = oled.show_dmp(&oled, 0, 0, 128, 64, pic);
    *************************************************/ 
	 int(*show_dmp)(const c_oled* p_obj, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);
}c_oled;

/************************************************* 
* Description: 创建一个 OLED 对象
* Input : <gpio> OLED 对象;
		  <pin> 用于保存获取到的温度数据;
* Output: 无
* Return: <R_OK> 操作成功; <R_ERROR> 操作失败
* Demo  :
	c_my_iic iic = my_iic_create(GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1);
	c_oled oled = oled_create(&iic);
*************************************************/ 
c_oled oled_create(c_my_iic *iic);

#endif 

/********************************************************************************
文件转为gb2312编码步骤：
1.通过 notepad++ 打开文件
2.菜单栏：编码->编码字符集->中文->GB2312(简体中文)
3.拷贝原文件内容进来
4.菜单栏：文件->另存为->保存（确认覆盖）
*******************************************************************************/
