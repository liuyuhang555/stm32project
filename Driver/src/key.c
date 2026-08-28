#include "key.h"
#include "stm32f10x.h"
#define KEY1_GPIO_PORT     GPIOA
#define KEY1_GPIO_PIN      GPIO_Pin_12
#define KEY1_RCC           RCC_APB2Periph_GPIOA
#define KEY2_GPIO_PORT     GPIOA
#define KEY2_GPIO_PIN      GPIO_Pin_13
#define KEY2_RCC           RCC_APB2Periph_GPIOA


#define LONG_PRESS_TIME  800   //ms 长按判定时间

static uint32_t key1_tick = 0;
static uint32_t key2_tick = 0;
static uint8_t key1_int_flag = 0;
static uint8_t key2_int_flag = 0;

/**
 * @brief KEY初始化 上拉输入 + 外部中断下降沿
 * KEY1 PA12  KEY2 PA13，关闭JTAG，否则引脚被调试占用
 */
void key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_APB2PeriphClockCmd(KEY1_RCC | RCC_APB2Periph_AFIO, ENABLE);
    // 关闭JTAG，释放PA13 PA12
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    // PA12 PA13 上拉输入
    GPIO_InitStruct.GPIO_Pin = KEY1_GPIO_PIN | KEY2_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStruct);

    // PA12 -> EXTI12 ; PA13 -> EXTI13
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource12);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource13);

    EXTI_InitStruct.EXTI_Line = EXTI_Line12 | EXTI_Line13;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    // NVIC 修改抢占优先级为15，满足FreeRTOS要求！！
    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 4;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/* EXTI10~15共用中断服务函数 PA12 PA13 */
void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line12);
        key1_int_flag = 1;
    }
    if(EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line13);
        key2_int_flag = 1;
    }
}

/**
 * @brief KEY_Scan 按键扫描，放在FreeRTOS任务中调用
 * @param mode 0:单次扫描
 * @retval KEY_NULL / KEY1_PRESS / KEY1_LONG_PRESS / KEY2_LONG_PRESS / KEY2_PRESS
 */
uint8_t KEY_Scan(uint8_t mode)
{
    static uint8_t key_state = 0;
    uint8_t ret = KEY_NULL;
    switch(key_state)
    {
        case 0:
        {
            if((key1_int_flag == 1)||(key2_int_flag ==1))
            {
                key_state = 1;
                if(key1_int_flag) key1_tick = xTaskGetTickCount();
                if(key2_int_flag) key2_tick = xTaskGetTickCount();
            }
            break;
        }
        case 1:
        {
            if(key1_int_flag != 0)
            {
                if(GPIO_ReadInputDataBit(KEY1_GPIO_PORT,KEY1_GPIO_PIN)==0)
                {
                    if((xTaskGetTickCount()-key1_tick)>=pdMS_TO_TICKS(LONG_PRESS_TIME))
                    {
                        ret = KEY1_LONG_PRESS;
                        key1_int_flag =0;
                        key_state = 2;
                    }
                }
                else
                {
                    ret = KEY1_PRESS;
                    key1_int_flag =0;
                    key_state = 2;
                }
            }
            else if(key2_int_flag !=0)
            {
                if(GPIO_ReadInputDataBit(KEY2_GPIO_PORT,KEY2_GPIO_PIN)==0)
                {
                    if((xTaskGetTickCount()-key2_tick)>=pdMS_TO_TICKS(LONG_PRESS_TIME))
                    {
                        ret = KEY2_LONG_PRESS;
                        key2_int_flag =0;
                        key_state =2;
                    }
                }
                else
                {
                    ret = KEY2_PRESS;
                    key2_int_flag =0;
                    key_state =2;
                }
            }
            break;
        }
        case 2:
        {
            if((GPIO_ReadInputDataBit(KEY1_GPIO_PORT,KEY1_GPIO_PIN)==1)&&
               (GPIO_ReadInputDataBit(KEY2_GPIO_PORT,KEY2_GPIO_PIN)==1))
            {
                key_state =0;
            }
            break;
        }
    }
    return ret;
}
