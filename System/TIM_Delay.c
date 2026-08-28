#include "stm32f10x.h"                  // Device header


void TIM_Delay_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_TimeBaseInitTypeDef tim_cfg;
    tim_cfg.TIM_Prescaler = 71; // 72M /72 = 1Mhz 1us
    tim_cfg.TIM_CounterMode = TIM_CounterMode_Up;
    tim_cfg.TIM_Period = 0xFFFF;
    tim_cfg.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM4, &tim_cfg);
    TIM_Cmd(TIM4, ENABLE);
}

void Delay_us(uint32_t us)
{
    TIM_SetCounter(TIM4, 0);
    while(TIM_GetCounter(TIM4) < us);
}
