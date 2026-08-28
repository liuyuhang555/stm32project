#ifndef __IWDG_H
#define __IWDG_H
#include "stm32f10x.h"


void IWDG_Config(uint8_t prescaler, uint16_t reload);
void IWDG_Feed(void);

#endif
