#ifndef __KEY_H
#define __KEY_H
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

#define KEY_NULL        0
#define KEY1_PRESS      1
#define KEY1_LONG_PRESS 2
#define KEY2_PRESS      3
#define KEY2_LONG_PRESS 4

void key_Init(void);
uint8_t KEY_Scan(uint8_t mode);

#endif
