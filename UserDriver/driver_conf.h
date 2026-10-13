/********************************************************************************
* @file		:driver_conf.h
* @brief	:外部驱动层通用头文件，使能/失能各组件，方便裁剪
* @Author	:Gwen9
* @Date		:2024/08/05
* @Version	:V1.0 初始版本
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRIVER_CONF_H
#define __DRIVER_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif
 
/* Dependency file ------------------------------------------------------------*/
#include "onchip_conf.h"

/* Macros  -------------------------------------------------------------------*/
//#define DRIVER_DHT11_FLOAT_ENABLED		//DHT11 ʹ�ܸ�����

/* Exported constants --------------------------------------------------------*/
/* P0 - ALL ----------------------*/    
#define DRIVER_DS18B20_ENABLED
#define DRIVER_DHT11_ENABLED
#define DRIVER_HX711_ENABLED
#define DRIVER_HCSR04_ENABLED
#define DRIVER_DS1302_ENABLED
#define DRIVER_STEPPER_ENABLED
#define DRIVER_MATRIX_KEYPAD_ENABLED

#define DRIVER_KEY_BOARD_ENABLED

/* P1 - PWM|ADC ----------------------*/   
#define DRIVER_GP2Y101_ENABLED
#define DRIVER_PH_ENABLED
#define DRIVER_SERVO_ENABLED

/* P2 - UART ----------------------*/ 
#define DRIVER_ESP8266_ENABLED
#define DRIVER_GPS_ENABLED
#define DRIVER_JQ8400_ENABLED
#define DRIVER_SIM800_ENABLED
#define DRIVER_AS608_ENABLED
#define DRIVER_EC600_ENABLED

/* P3 - IIC ----------------------*/   
#define DRIVER_BH1750_ENABLED
#define DRIVER_BMP180_ENABLED
#define DRIVER_SGP30_ENABLED
#define DRIVER_INA219_ENABLED
#define DRIVER_MAX30102_ENABLED
#define DRIVER_OLED_ENABLED
#define DRIVER_TCS34725_ENABLED
#define DRIVER_MPU6050_ENABLED
#define DRIVER_VL53L0X_ENABLED
#define DRIVER_MLX90614_ENABLED

/* P4 - SPI ----------------------*/ 
#define DRIVER_RC522_ENABLED
#define DRIVER_TFT18_ENABLED
#define DRIVER_TOUCH_ENABLED
#define DRIVER_SD_CARD_ENABLED
#define DRIVER_LCDTOUCH_ENABLED

/* Typedef -------------------------------------------------------------------*/
/* Includes ------------------------------------------------------------------*/ 
/**
  * @brief Include module's header file
  */
#ifdef DRIVER_DS18B20_ENABLED
  #include "ds18b20.h" 
#endif 

#ifdef DRIVER_DS1302_ENABLED
  #include "ds1302.h" 
#endif 

/* P2 - UART ----------------------*/ 

#ifdef DRIVER_ESP8266_ENABLED
  #include "esp8266.h" 
#endif
/* P3 - IIC ----------------------*/   
#ifdef DRIVER_OLED_ENABLED
  #include "oled.h" 
#endif 

#ifdef DRIVER_MPU6050_ENABLED
  #include "mpu6050.h" 
#endif

#ifdef DRIVER_MAX30102_ENABLED
  #include "max30102.h" 
#endif


/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __ONCHIP_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
