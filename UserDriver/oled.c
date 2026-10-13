/********************************************************************************
* @file		:oled.c
* @brief	:oled 液晶显示屏驱动 基于SSD1306显示芯片，取模方式：阴码，逆向，列行式
* @Author	:Gwen9
* @Date		:2024/08/06
* @Version	:V1.0 初始版本
*******************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_OLED_ENABLED
#include "oledfont.h"

/* Private Macros  -------------------------------------------------------------------*/
#define  OLED_WIDTH			128
#define  OLED_HIGH			64
#define  OLED_PAGE_CNT		8

#define  OLED_CMD	0x00
#define  OLED_DATA	0x40

/* GB2312 编码中汉字的有效范围 */

#define GB2312_BASIC_1_MIN 	0xB0	//第一字节，基本区范围：0xB0 - 0xF7
#define GB2312_BASIC_1_MAX 	0xF7

#define GB2312_EXTEN_1_MIN 	0xA1	//第一字节，扩展区区范围：0xA1 - 0xA9
#define GB2312_EXTEN_1_MAX 	0xA9

#define GB2312_2_MIN 		0xA1	//第二字节范围：0xA1 - 0xFE
#define GB2312_2_MAX 		0xFE
/* Private Typedef  -------------------------------------------------------------------*/
typedef struct _S_OLED
{
    const c_my_iic*    iic          ;  /*iic对象*/
    uint8_t            iic_addr     ;  /*iic地址*/
}s_oled;

/* Private Functions Declare ----------------------------------------------------------*/
static int oled_display_on(const c_oled* p_obj);
static int oled_display_off(const c_oled* p_obj);
static int oled_clear(const c_oled* p_obj);

static int oled_show_string(const c_oled* p_obj, uint8_t x, uint8_t y, uint8_t size, const char* fmt, ...);
static int oled_show_dmp(const c_oled* p_obj, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);

/* Private Variables -------------------------------------------------------------------*/
static uint8_t oled_init_list[] = 
{
	0xAE,		// 关闭显示，进入低功耗模式。
	0x02,		// 设置页面地址模式下的列起始地址的低4位
	0x10,		// 设置页面地址模式下的列起始地址的高4位。
	0x40,		// 设置显示起始行地址，0x40表示起始行地址为0。
	0xB0,		// 设置页面地址模式下的页面起始地址，0xB0表示页面起始地址为0。
	0x81,		// <对比度> 设置
	0xFF,		// <对比度> 数据，0xFF表示最大对比度。
	0xA1,		// 设置段重映射 0xA0 表示正常映射，0xA1 表示水平翻转（镜像）
	0xA6,		// 设置显示模式 0xA6 表示正常显示（白底黑字）, 0xA7 表示反相显示(黑底白字) 即是 正常显示: 数据为1时点亮像素（通常是白色）
	0xA8,		// <多路复用比> 设置
	0x3F,		// <多路复用比> 数据，0x3F表示1/64占空比（64MUX）。
	0xC8,		// 设置COM输出扫描方向。0xC0 表示正常扫描，0xC8 表示垂直翻转（镜像）
	0xD3,		// <显示偏移> 设置
	0x00,		// <显示偏移> 数据，0x00表示无偏移。
	0xD5,		// <显示时钟分频比/振荡器频率> 设置
	0x80,		// <显示时钟分频比/振荡器频率> 数据，0x80表示默认设置。
	0xD8,		// <区域颜色模式开/关和低功耗显示模式> 设置
	0x05,		// <区域颜色模式开/关和低功耗显示模式> 0x05表示默认设置。
	0xD9,		// <预充电周期> 设置
	0xF1,		// <预充电周期> 数据，0xF1表示预充电周期为 15 DCLKs。
	0xDA,		// <COM引脚硬件配置> 设置
	0x12,		// <COM引脚硬件配置> 数据， 0x12表示默认设置。
	0xDB,		// <VCOMH去选电平> 设置
	0x30,		// <VCOMH去选电平> 数据，0x30表示 0.83*Vcc。
	0x8D,		// <充电泵> 设置 
	0x14,		// <充电泵> 数据，0x14表示使能充电泵。
	0xAF,		// 打开显示，退出低功耗模式。
};

/* Public Functions -------------------------------------------------------------------*/
c_oled oled_create(c_my_iic *iic)
{
	c_oled new = {0};	//新建一个空的GPIO对象		
	int ret;
    /*参数检查*/
    if(NULL == iic || NULL == iic->this){
        return new;
    }
	/*为 OLED 新对象的私有成员申请内存 同时清空内存*/
	new.this = MY_MALLOC(sizeof(s_oled));
	if(NULL == new.this){
		return new;
	}
	memset(new.this,0,sizeof(s_oled));

	/*初始化 OLED 对象的私有成员*/
	((s_oled*)new.this)->iic 			= iic;
	((s_oled*)new.this)->iic_addr 		= OLED_DEFAULT_IIC_ADDR;

	/*重设IIC通信参数，通信速度：0（最快）， 是否等待从设备IIC相应：true(是)， 超时等待时间: 80us*/
	ret = iic->set_param(iic, 0, true, 80);

	/*初始化 OLED 对象的公有接口*/
	new.display_on 	= oled_display_on;	
	new.display_off	= oled_display_off;	
	new.clear		= oled_clear;
	new.show		= oled_show_string;
	new.show_dmp	= oled_show_dmp;

	/*发送 OLED 初始化命令*/
	for(uint8_t i = 0; i < sizeof(oled_init_list); i++){
		ret = iic->write_regs(iic, OLED_DEFAULT_IIC_ADDR, 0x00, &oled_init_list[i], 1);
		if(ret != R_OK){
			new.this = NULL;
			return new;
		}
	}

	return new;
}

/* Private Functions -------------------------------------------------------------------*/
static int oled_display_on(const c_oled* p_obj)
{
	s_oled* this = NULL;
	int ret;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this){
		return R_NULL;
	}
	this = p_obj->this;
	/*向 OLED 发送配置 命令*/
	uint8_t data[3] = {	0x8D, 	// <充电泵> 设置 
						0x14, 	// <充电泵> 数据，0x14 表示使能充电泵。
						0xAF	// 打开显示，退出低功耗模式。
						};
	for(uint8_t i = 0; i < sizeof(data); i++){
		ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_CMD, &data[i], 1);
		if(ret != R_OK)	return R_ERROR;
	}
	return R_OK;	
}

static int oled_display_off(const c_oled* p_obj)
{
	s_oled* this = NULL;
	int ret;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this){
		return R_NULL;
	}
	this = p_obj->this;
	/*向 OLED 发送配置 命令*/
	uint8_t data[3] = {	0x8D, 	// <充电泵> 设置 
						0x10, 	// <充电泵> 数据，0x10 表示关闭充电泵。
						0xAF	// 打开显示，退出低功耗模式。
						};
	for(uint8_t i = 0; i < sizeof(data); i++){
		ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_CMD, &data[i], 1);
		if(ret != R_OK)	return R_ERROR;
	}
	return R_OK;	
}

static int oled_clear(const c_oled* p_obj)
{
	s_oled* this = NULL;
	int ret;

	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this){
		return R_NULL;
	}
	this = p_obj->this;

	uint8_t data[3] = {	0xB0, 	// 设置页面地址模式下的 <页面起始地址>（0~7），0xB0表示页面起始地址为0。 
						0x00, 	// 设置页面地址模式下的 <列起始地址> 的低4位, 0x00 表示低4位为0
						0x10	// 设置页面地址模式下的 <列起始地址> 的高4位, 0x10 表示高4位为0
						}; 
	uint8_t point_val = 0;		//全部写0 表示清屏

	for(uint8_t j = 0; j < OLED_PAGE_CNT; j++){
		/*页地址 0-7 切换，列地址从0开始*/
		for(uint8_t i = 0; i < sizeof(data); i++){
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_CMD, &data[i], 1);
			if(ret != R_OK)	return R_ERROR;
		}
		/*SEG 0-127 全部写0*/
		for(uint8_t i = 0; i < OLED_WIDTH; i++){
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &point_val, 1);
			if(ret != R_OK)		return R_ERROR;
		}
		data[0]++;	//页地址加1
	}
	return R_OK;	
}

static int oled_set_position(const c_oled* p_obj, uint8_t x, uint8_t y)
{
	s_oled* this = NULL;
	int ret;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this || x >= OLED_WIDTH || y >= OLED_PAGE_CNT){
		/*x 为列地址，即横坐标 取值 0-127， y为页面地址，即纵坐标，取值0-7 */
		return R_ERROR;
	}
	this = p_obj->this;
	/*向 OLED 发送配置 命令*/
	uint8_t data[3] = {	0xB0 + y, 					//设置页面地址模式下的 <页面起始地址>（0~7），0xB0 表示页面起始地址为0。
						0x10 | ((x & 0xF0) >> 4), 	//设置页面地址模式下的 <列起始地址> 的高4位, 0x10 表示高4位为0
						0x00 |  (x & 0x0F)			//设置页面地址模式下的 <列起始地址> 的低4位, 0x10 表示低4位为0
						};
	for(uint8_t i = 0; i < sizeof(data); i++){
		ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_CMD, &data[i], 1);
		if(ret != R_OK)	return R_ERROR;
	}
	return R_OK;	
}

static int oled_show_char(const c_oled* p_obj, uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
	s_oled* this = NULL;
	int ret;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this || y >= OLED_PAGE_CNT){
		return R_ERROR;
	}
	this = p_obj->this;
//	if(x >= OLED_WIDTH)	{x = 0; y = y + 2;}		//横坐标超出屏幕宽度时，自动移动到下一行	
	if(x >= OLED_WIDTH)	{return R_ERROR;}		//横坐标超出屏幕宽度时不显示

	unsigned char c = 0, i = 0;
	c = chr - ' ';						//得到偏移后的值

	if(size == 12){ //字符点阵 6*8， 即 6 SEG * 1 PAGE	
		oled_set_position(p_obj, x, y);
		/*写第一页（行）中的 6列 SEG*/
		for(i = 0; i < 6; i++){
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &F6x8[c][i], 1);
			if(ret != R_OK)	return R_ERROR;
		}
	}else if(size == 16){		//字符点阵 8*16， 即 8 SEG * 2 PAGE
		/*写第一页（行）中的 8列 SEG*/
		oled_set_position(p_obj, x, y);	
		for(i = 0; i < 8; i++){
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &F8X16[c*16+i], 1);
			if(ret != R_OK)	return R_ERROR;
		}
		/*写第二页（行）中的 8列 SEG*/
		oled_set_position(p_obj, x, y+1);
		for(i = 0; i < 8; i++){
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &F8X16[c*16+i+8], 1);
			if(ret != R_OK)	return R_ERROR;
		}
	} 
	
	return R_OK;	
}

static int oled_show_char_cn(const c_oled* p_obj, uint8_t x, uint8_t y, const uint8_t* ch, uint8_t size)
{
	s_oled* this = NULL;
	int ret;
	const uint8_t* data = NULL;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this || y >= OLED_PAGE_CNT){
		return R_NULL;
	}
	this = p_obj->this;

	// 查找字库中是否存在该汉字
    if (size == 12) {
        for(size_t i = 0; i < sizeof(tfont12) / sizeof(tfont12[0]); i++) {
            if(memcmp(tfont12[i].name, ch, 2) == 0) {
                data = tfont12[i].data;
                break;
            }
        }
    } else if (size == 16) {
        for(size_t i = 0; i < sizeof(tfont16) / sizeof(tfont16[0]); i++) {
            if(memcmp(tfont16[i].name, ch, 2) == 0) {
                data = tfont16[i].data;
                break;
            }
        }
    }
    // 如果字库中没有找到该汉字, 直接返回错误
    if(data == NULL) 	return R_ERROR;
    
    // 显示字库中找到的汉字点阵数据
    if(size == 12) {  // 汉字点阵 12*12，即 12 SEG * 2 PAGE
        oled_set_position(p_obj, x, y);
        for(uint8_t i = 0; i < 12; i++) {
            ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &data[i], 1);
            if(ret != R_OK) return R_ERROR;
        }
        oled_set_position(p_obj, x, y + 1);
        for(uint8_t i = 0; i < 12; i++) {
            ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &data[i + 12], 1);
            if(ret != R_OK) return R_ERROR;
        }
    } else if(size == 16) {  // 汉字点阵 16*16，即 16 SEG * 2 PAGE
        oled_set_position(p_obj, x, y);
        for(uint8_t i = 0; i < 16; i++) {
            ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &data[i], 1);
            if(ret != R_OK) return R_ERROR;
        }
        oled_set_position(p_obj, x, y + 1);
        for(uint8_t i = 0; i < 16; i++) {
            ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &data[i + 16], 1);
            if(ret != R_OK) return R_ERROR;
        }
    }
	return R_OK;	
}


// 字符查找函数，用于从字库中查找中文字符
static uint8_t* oled_find_chinese_char(const char* ch, uint8_t size) 
{
	uint8_t i;
	if(size == 12){
		for (i = 0; i < sizeof(tfont12)/sizeof(tfont12[0]); i++){
			if(memcmp(tfont12[i].name, ch, 2) == 0) return (uint8_t*)(tfont12[i].name); // 返回字模数据的指针
		}
	}else if(size == 16){
		for (i = 0; i < sizeof(tfont16)/sizeof(tfont16[0]); i++){
			if(memcmp(tfont16[i].name, ch, 2) == 0) return (uint8_t*)(tfont16[i].name); // 返回字模数据的指针
		}
	}
    return NULL; // 如果没有找到，返回NULL
}


/* 检查字符是否为 GB2312 汉字 */
static int oled_is_gb2312_chinese_char(uint8_t* p) 
{
    if ((p[0] >= GB2312_BASIC_1_MIN && p[0] <= GB2312_BASIC_1_MAX) || 
		(p[0] >= GB2312_EXTEN_1_MIN && p[0] <= GB2312_EXTEN_1_MAX)){	//判断第一字节是否在基本区/扩展区
        if(p[1] >= GB2312_2_MIN && p[1] <= GB2312_2_MAX) return 1;  // 是汉字
    }
    return 0;  // 不是汉字
}

static int oled_show_string(const c_oled* p_obj, uint8_t x, uint8_t y, uint8_t size, const char* fmt, ...)
{
	s_oled* this = NULL;
	int ret;
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this){
		return R_NULL;
	}
    /*检查坐标*/
    if(x >= OLED_WIDTH || y >= OLED_PAGE_CNT){
        return R_PARAM;
    }

	this = p_obj->this;

	char buf[64] = {0};  		// 缓冲区，用于存储格式化后的字符串
	va_list args;				// 声明一个 va_list 类型的变量 args，用于存储可变参数列表。

	memset(buf, 0, sizeof(buf));//清空静态缓存区
	va_start(args, fmt);		// 初始化 args 以存储所有传递给函数的可变参数。fmt 是最后一个确定的参数，在它之后的所有参数都被认为是可变参数。
	vsprintf(buf, fmt, args);	// 使用 vsprintf 函数将格式化字符串 fmt 和可变参数列表 args 生成的结果存储在 buf 中。
	va_end(args);				// 结束对 args 的处理。va_end 宏用于清理 va_list 变量。

	
	/*
		在 GB2312 的字符集中，真正的汉字范围是有限的。例如，GB2312 编码中的汉字主要位于以下区域：
		byte1区码：从 0xB0 到 0xF7（共 68 个区码）
		byte2位码：从 0xA1 到 0xFE（共 94 个位码）
	*/
	/*遍历缓冲区并显示每个字符*/ 
	uint8_t len = strlen(buf);
	for(uint8_t i = 0; i < len; /*i++*/) {
		if(buf[i] & 0x80){				// 如果字符的最高位为1，则认为是中文字符（UTF-8编码,GB2312d都是大于 0xA1的，这里也必须大于 0x80），
			if(i + 1 < len && oled_is_gb2312_chinese_char(&buf[i])){
				const uint8_t* chinese_char = 	oled_find_chinese_char(&buf[i], size);
				if(chinese_char != NULL) 		oled_show_char_cn(p_obj, x, y, chinese_char, size);  	// 显示中文字符
				x += size;  			// 更新 x 位置
				i += 2;  				// 跳过中文字符的两个字节
			}else{
				i++;					// 跳过无效字符,这里的无效字符一定不是英文字符，应为之前已经判断过必须大于 0x80, 所以一般是非法字符
			}
		}else{							// 否则认为是英文字符
			ret = oled_show_char(p_obj, x, y, buf[i], size);
			if(ret != R_OK)	return R_ERROR;
			x += size / 2;  									// 更新 x 位置，假设每个字符占据 size/2 像素宽度
			if(x > OLED_WIDTH - size / 2)	{y += size / 8; x = 0;}	//超长切换到下一行显示
			// if(x > OLED_WIDTH - size / 2)	{x = 0;}		//超长从当前行起始位置开始覆盖
			i++;
		}
	}
	return R_OK;	
}

static int oled_show_dmp(const c_oled* p_obj, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[])
{
	s_oled* this = NULL;
	int ret;
	
	/*检查参数*/
	if(NULL == p_obj || NULL == p_obj->this){
		return R_NULL;
	}
    /*检查坐标*/
    if(x0 >= OLED_WIDTH || y0 >= OLED_PAGE_CNT || x1 >= OLED_WIDTH || y1 >= OLED_PAGE_CNT){
        return R_PARAM;
    }

	uint32_t j = 0;
	uint8_t x, y;

	if((y1%8) == 0) 	y = y1 / 8;      
	else 				y = y1 / 8 + 1;

	for(y = y0; y < y1; y++){
		oled_set_position(p_obj, x0, y);	//设置写每页的起始地址
		for(x = x0; x < x1; x++){     		//写当前页数据
			ret = this->iic->write_regs(this->iic, this->iic_addr, OLED_DATA, &BMP[j++], 1);
			if(ret != R_OK)	return R_ERROR;    	
		}
	}
	
	return R_OK;
}


#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/********************************************************************************

SSD1306显存：

		SEG0  SEG1 ... SEG127
PAGE0   	
PAGE1
PAGE2
PAGE3
PAGE4
PAGE5
PAGE6
PAGE7

8 PAGE， 每页 128个 SEG，每个SEG为一个字节(8bits)， 显存总大小为 128 * 8 bytes
每个bit标志一个像素点的亮或者灭，一个SEG可以表示8个像素点如下：
SEG0
bit0
bit1
bit2
bit3
bit4
bit5
bit6
bit7
所以X向共128个像素，Y向共 8 * 8 = 64个像素 

SSD1306寻址模式：页地址模式，水平地址模式，竖直地址模式
页地址模式：
页面寻址模式下，读写显示RAM后，列地址指针自动增加1。如果列地址指针到达列结束地址(0 - 127)，
则将列地址指针重置为列起始地址，而不对页面地址指针进行更改（PAGE不会改变）。
用户必须设置新的页和列地址，才能访问下一页的RAM内容。

dat[0] = 0xb0+y;              		//设置页地址     （y: 0 - 7）  
dat[1] = ((x&0xf0)>>4)|0x10; 	    //设置列地址高位
dat[2] = (x&0x0f);					//设置列地址低位  (x: 0 - 8)
*******************************************************************************/


