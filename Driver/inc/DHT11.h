#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

// 引脚配置
#define DHT11_PIN    GPIO_Pin_14
#define DHT11_PORT   GPIOB
#define DHT11_RCC    RCC_APB2Periph_GPIOB

// 函数声明
void DHT11_Init(void);               // DHT11初始化
uint8_t DHT11_ReadData(uint8_t *temp, uint8_t *humi);  // 读取温湿度

#endif
