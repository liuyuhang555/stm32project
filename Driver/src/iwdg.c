#include "iwdg.h"

/**
 * @brief IWDG初始化，LSI=40KHz
 * @param prescaler: IWDG_Prescaler_4/8/16/32/64/128/256
 * @param reload: 0~0xFFF
 * 超时时间 ≈ (prescaler * reload) / 40000  秒
 * 示例：预分频64，reload=1250 → 64*1250/40000 = 2s，2秒不喂狗就复位
 */
void IWDG_Config(uint8_t prescaler, uint16_t reload)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(prescaler);
    IWDG_SetReload(reload);
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
    IWDG_Enable();
}

/**
 * @brief 喂狗
 */
void IWDG_Feed(void)
{
    IWDG_ReloadCounter();
}

