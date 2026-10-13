/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "onchip_conf.h"
#include "driver_conf.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
#define USER_UART3		2										//0-无APP 1-蓝牙通信 2-WIFI通信

/* Private variables ---------------------------------------------------------*/
/*object*/
c_my_iic iic_oled;                      //定义 oled 用到的 iic 结构体
c_oled oled;                            //定义 oled 结构体变量
c_ds18b20 ds18b20;                  //定义 ds18b20 结构体变量
c_ds1302 ds1302;                    //定义 ds1302 结构体变量
c_my_iic iic_max30102;                      //定义 max30102 用到的 iic 结构体
c_max30102 max30102;                        //定义 max30102 结构体变量
c_my_iic iic_mpu6050;                       //定义 mpu6050 用到的 iic 结构体
c_mpu6050 mpu6050;                          //定义 mpu6050 结构体变量
c_my_gpio led;                  //创建 LED灯 控制结构体
c_my_gpio beep;                 //创建 蜂鸣器 控制结构体
c_esp8266 esp8266;							//创建 ESP8266 控制结构体

/*variables*/
static sys_time_t time;      //定义变量用于保存 ds1302 获取到的系统时间
static uint32_t red_buf[BUFFER_SIZE];				//定义变量用于用于缓存从max30102直接读取到的数据
static uint32_t ir_buf[BUFFER_SIZE];				//定义变量用于用于缓存从max30102直接读取到的数据
static uint32_t heartrate, o2;              //定义变量用于保存 max30102 获取到的 心率，血氧浓度
static float angle[3];                      //定义变量用于保存 mpu6050 获取到的角度值
static uint8_t value[3]={150,70,36};			//定义数组用于保存阈值,心率、血氧、体温
static uint8_t sta;					//定义变量用于区分跌倒和步数
static uint16_t step;				//定义变量用于保存步数
static float cal;						//定义变量用于保存卡路里
static float temp;					//定义变量用于保存温度
static uint8_t last_flag=0;		//定义变量用于暂存当前的显示页数
static uint8_t value_flag=0;	//定义标志位用于控制阈值加减的对象
static uint8_t fall_flag=0;		//定义变量用于保存跌倒状态
static uint8_t page_flag=0;		//定义标志位用于控制OLED显示的内容
static char rec_buf[64];			//定义数组用来接受串口数据
static char tx_buf[64];	//定义数组用于保存待发送的内容

/*callback*/
int my_exti_callback(const char* name, uint8_t state)
{
	if((strstr(name, "KEY1") != NULL)&&(state==1)){		//按下按键一
		page_flag++;																		//更换OLED显示页数
		if(page_flag>2) page_flag=0;										//复位显示内容标志位，复位OLED显示内容为初始内容
	}
	if((strstr(name, "KEY2") != NULL)&&(state==1)){		//按下按键二
		value_flag++;                               		//更改阈值控制对象
		if(value_flag>2) value_flag=0;              		//复位阈值控制对象为初始对象
	}
	if((strstr(name, "KEY3") != NULL)&&(state==1)){		//按下按键三
		value[value_flag]++;														//阈值加一
	}
	if((strstr(name, "KEY4") != NULL)&&(state==1)){		//按下按键四
		value[value_flag]--;														//阈值减一
		if(value[value_flag]<0) value[value_flag]=0;		//控制阈值最低为零
	}
	return R_OK;
}
//接受数据处理函数
void User_UART3(){
	char *pxIndex;			//定义变量用于暂存分解得到的字符						
	if(strstr(rec_buf, "AT+DATA1") != NULL){		//判断数据包内是否含有包头
		pxIndex = strtok(rec_buf, "=");						//分解字符串。s为要分解的字符，delim为分隔符字符（如果传入字符串，则传入的字符串中每个字符均为分割符）。
																							//原型：char *strtok(char s[], const char *delim)；
		//接收APP下发的时间其中的年份
		pxIndex = strtok(NULL, ",");
		time.year = atoi(pxIndex);
		//接收APP下发的时间其中的月份
		pxIndex = strtok(NULL, ",");
		time.mon = atoi(pxIndex);
		//接收APP下发的时间其中的日期
		pxIndex = strtok(NULL, ",");
		time.date = atoi(pxIndex);
		//接收APP下发的时间其中的小时
		pxIndex = strtok(NULL, ",");
		time.hour = atoi(pxIndex);
		//接收APP下发的时间其中的分钟
		pxIndex = strtok(NULL, ",");
		time.min = atoi(pxIndex);
		//接收APP下发的时间其中的秒数
		pxIndex = strtok(NULL, ",");
		time.sec = atoi(pxIndex);
		//设置时间
		ds1302.set_time(&ds1302, &time);
	}
	else if(strstr(rec_buf, "AT+DATA2") != NULL){		//接收APP下发的心率阈值
		pxIndex = strtok(rec_buf, "=");
		pxIndex = strtok(NULL, ",");
		value[0] = atoi(pxIndex);
	}
	else if(strstr(rec_buf, "AT+DATA3") != NULL){		//接收APP下发的血氧阈值
		pxIndex = strtok(rec_buf, "=");
		pxIndex = strtok(NULL, ",");
		value[1] = atoi(pxIndex);
	}
	else if(strstr(rec_buf, "AT+DATA4") != NULL){		//接收APP下发的体温阈值
		pxIndex = strtok(rec_buf, "=");
		pxIndex = strtok(NULL, ",");
		value[2] = atoi(pxIndex);
	}
}
int esp8266_recive_callback(void* param, char* key, char *value)
{
	//对数据长度进行判断，超过64的认为是乱码，干扰
	if(strlen(value) < 64){
		memset(rec_buf, 0, sizeof(rec_buf));
		memcpy(rec_buf, value, strlen(value));
	}
	User_UART3();
	return R_OK;
}
void bluetooth_interrupt(void* param, uint8_t* data, uint16_t data_len)
{
	/*对数据长度进行判断，超过64的认为是乱码，干扰,不超过就保存到数组rec_buf中*/
	if(data_len < 64){
		memset(rec_buf, 0, sizeof(rec_buf));				//清空rec_buf
		memcpy(rec_buf, data, data_len);						//将data的内容拷贝到rec_buf中
	}
	char *pxIndex;			//定义变量用于暂存分解得到的字符
	if(strstr(rec_buf,"\r\n") != NULL){						//判断数据包内是否含有结束符
		User_UART3();
	}
}
/* Private user code ---------------------------------------------------------*/
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* 延时100ms 等待时钟信号稳定 */
	DelayMs(100);

	/*init*/
	iic_oled = my_iic_create(GPIOA, GPIO_PIN_11, GPIOA, GPIO_PIN_12);			//初始化 iic				
	oled = oled_create(&iic_oled);			//初始化OLED显示屏
	oled.clear(&oled);									//清屏一次
	
	ds18b20 = ds18b20_create(GPIOA, GPIO_PIN_8);//初始化 ds18b20 温度传感器

	ds1302 = ds1302_create(GPIOB, GPIO_PIN_12, GPIOB, GPIO_PIN_13, GPIOA, GPIO_PIN_4);   //初始化 ds1302 实时时钟模块

	iic_max30102= my_iic_create(GPIOB, GPIO_PIN_15, GPIOB, GPIO_PIN_14);//初始化 iic
	max30102 = max30102_create(&iic_max30102, red_buf, ir_buf);  //初始化 max30102 心率血氧检测模块

	iic_mpu6050 = my_iic_create(GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_9);  //初始化 iic
	mpu6050 = mpu6050_create(&iic_mpu6050);     //初始化 mpu6050 6轴传感器

	led = my_gpio_create(GPIOA, GPIO_PIN_5, GPIO_OUT);  //初始化 LED 用到的GPIO

	beep = my_gpio_create(GPIOB, GPIO_PIN_5, GPIO_OUT); //初始化 蜂鸣器 用到的GPIO

	exti_list_t exti_list[] = { //创建外部中断列表
	/*exti_list*/
		{"KEY1",GPIOC, GPIO_PIN_15, MY_EXTI_FALLING},    //按键1， 下降沿触发，默认为高电平
		{"KEY2",GPIOC, GPIO_PIN_14, MY_EXTI_FALLING},    //按键2， 上升沿触发，默认为低电平
		{"KEY3",GPIOC, GPIO_PIN_13, MY_EXTI_FALLING},    //按键3， 下降沿触发，默认为高电平
		{"KEY4",GPIOB, GPIO_PIN_4, MY_EXTI_FALLING},   	 //按键4， 下降沿触发，默认为高电平
	};
	my_exti.init(exti_list, sizeof(exti_list)/sizeof(exti_list_t), my_exti_callback);		//创建按键回调函数

	if(USER_UART3 == 1){
		my_uart.init(MY_UART3, 115200, UART_MODE_DMA); //初始化 蓝牙模块 用到的串口
		my_uart.register_callback(MY_UART3, bluetooth_interrupt, NULL); //注册 蓝牙模块 的串口中断接收处理函数
	}else if(USER_UART3 == 2){
		//AP-TCP 模式
		esp8266 = esp8266_creat(MY_UART3,			//选择串口3
						AP_TCP,				//工作模式: STA + MQTT + ONENET
						"",				//产品ID
						"",				//设备名
						"");	
		esp8266.set_recive_callback(&esp8266, esp8266_recive_callback, NULL);
	}

	/*step*/
	static float x_oft, y_oft;		//定义变量用于保存初始状态下MPU6050的x,y方向上的倾角
	static float x_sum, y_sum;		//定义变量用于保存初始状态下MPU6050累计十次的x,y方向上的倾角
	for(uint8_t i = 0; i < 10; i++){	//累计初始状态下十次MPU6050的x,y方向上的倾角
		mpu6050.read_angle(&mpu6050, angle);		//获取MPU6050 的倾角数据
		x_sum += angle[0];					//累计x方向上的倾角
		y_sum += angle[1];					//累计y方向上的倾角
		DelayMs(100);								//延时
	}
	x_oft = x_sum / 10.0;					//计算得到初始状态下MPU6050的x方向上的倾角
	y_oft = y_sum / 10.0;					//计算得到初始状态下MPU6050的y方向上的倾角
	
	/*Infinite loop*/
	static uint32_t sys_cnt;
	while (1)
	{	
		if((sys_cnt%50) == 0){									//每50ms执行一次
			mpu6050.read_angle(&mpu6050, angle);		//获取MPU6050的倾角
			angle[0] = angle[0] - x_oft;						//计算得到与初始状态x方向的倾角差值
			angle[1] = angle[1] - y_oft;						//计算得到与初始状态y方向的倾角差值
			
			if(fabs(angle[0]) > 50){								//倾角过大，视为跌倒
				sta = 0;
				fall_flag=1;
			}
			else if(fabs(angle[0]) > 5){						//倾角不大，视为步数走动
				sta++;
				fall_flag=0;
			}
			else{								//清除抖动情况
				sta = 0;
				fall_flag=0;
			}
			if(sta > 3){				//满足情况，步数加一
				sta = 0;
				step++;
			}
		}
		if((sys_cnt%200) == 0){								//每200ms执行一次
			sys_cnt=0;
			
			cal=65*step*0.001*1.036;									//计算得到卡路里
			ds1302.get_time(&ds1302, &time);					//获取时间
			ds18b20.get(&ds18b20, &temp);							//获取温度
			max30102.get(&max30102, &heartrate, &o2);	//获取心率血氧
			//超过阈值声光报警
			if(fall_flag || heartrate>value[0] || (o2<value[1]&&o2>0) || temp>value[2]){		
				beep.set(&beep, 1);
				led.set(&led, 1);
			}
			//恢复到正常状态关闭声光报警
			else{
				beep.set(&beep, 0);
				led.set(&led, 0);
			}
			//当页数改变时，oled清空一次
			if(page_flag!=last_flag){
				last_flag=page_flag;
				oled.clear(&oled);
			}
			//第一页显示的内容（默认页显示的内容）
			if(page_flag==0){
				oled.show(&oled, 32, 0, 16, "%02d:%02d:%02d", time.hour,time.min,time.sec);
				oled.show(&oled, 0, 2, 16, "心率:%dt/m  ", heartrate);
				oled.show(&oled, 0, 4, 16, "血氧:%d%%  ", o2);
				oled.show(&oled, 0, 6, 16, "体温:%.1f℃  ", temp);
			}
			//第二页显示的内容
			else if(page_flag==1){
				oled.show(&oled, 0, 0, 16, "步数:%d", step);
				oled.show(&oled, 0, 2, 16, "卡路里:%.2fcal", cal);
			}
			//第三页显示的内容
			else if(page_flag==2){
				oled.show(&oled, 0, 0, 16, "心率:%dt/m  ", value[0]);
				oled.show(&oled, 0, 2, 16, "血氧:%d%%  ", value[1]);
				oled.show(&oled, 0, 4, 16, "体温:%d℃  ", value[2]);
				//选定阈值加减的目标对象
				for(uint8_t i=0;i<3;i++){
					if(i==value_flag){
						oled.show(&oled, 112, i*2, 16, "<-");
					}
					else{
						oled.show(&oled, 112, i*2, 16, "  ");
						oled.show(&oled, 112, i*2, 16, "  ");
					}
				}
			}
			
			if(USER_UART3 == 1){								//通过蓝牙向APP发送数据
				memset(tx_buf, 0, sizeof(tx_buf));
				sprintf(tx_buf,"AT+DATA1=%d,%d,%.1f,%d,%.1f,\r\n",heartrate,o2,temp,step,cal);
				my_uart.send(MY_UART3,(uint8_t*)tx_buf,strlen(tx_buf));
			}
			else if(USER_UART3==2){								//通过WIFI向APP发送数据
				if(esp8266.state(&esp8266) == ESP8266_TCP_CONNECTED){
					memset(tx_buf, 0, sizeof(tx_buf));
					sprintf(tx_buf,"AT+DATA1=%d,%d,%.1f,%d,%.1f,\r\n",heartrate,o2,temp,step,cal);
					esp8266.send(&esp8266, tx_buf);   				
				}
			}
		}
		/*运行系统周期任务*/
		my_systick.period_task_running();
		sys_cnt++;
		HAL_Delay(1);
	}
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType 			= RCC_OSCILLATORTYPE_HSE;	
  RCC_OscInitStruct.HSEState 				= RCC_HSE_ON;				
  RCC_OscInitStruct.HSEPredivValue 			= RCC_HSE_PREDIV_DIV1;		
  RCC_OscInitStruct.HSIState				= RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState			= RCC_PLL_ON;				
  RCC_OscInitStruct.PLL.PLLSource			= RCC_PLLSOURCE_HSE;	
  RCC_OscInitStruct.PLL.PLLMUL				= RCC_PLL_MUL9;		
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType 		= RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource 	= RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider 	= RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider 	= RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider 	= RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}