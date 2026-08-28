#include "DHT11.h"
#include "TIM_Delay.h"
#include "FreeRTOS.h"   //新增
#include "task.h"
// 设置DATA引脚为输出模式
static void DHT11_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}
// 设置DATA引脚为输入模式
static void DHT11_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}
// DHT11初始化
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);
    DHT11_OUT();
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
}
// 读取一个字节数据
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i, dat = 0;
    uint16_t cnt;
    for(i = 0; i < 8; i++)
    {
        dat <<= 1;
        cnt = 0;
        while(GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN)==0 && cnt<200) cnt++;
        if(cnt>=200) return 0;
        Delay_us(40);
        
        if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
            dat |= 1;
        
        cnt = 0;
        while(GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN)==1 && cnt<200) cnt++;
        if(cnt>=200) return 0;
    }
    return dat;
}
// 读取温湿度
// 返回0：成功  返回1：失败
uint8_t DHT11_ReadData(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t buf[5] = {0};
    uint8_t i;
    uint16_t cnt;
    BaseType_t xSavedInterruptStatus;

    vTaskSuspendAll();  //挂起调度器，禁止任务切换，保护DHT11时序
    
    // 主机发送起始信号
    DHT11_OUT();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(18000);
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(20);
    
    // 等待DHT11响应
    DHT11_IN();
    if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0)
    {
        cnt = 0;
        while(GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN)==0 && cnt<200) cnt++;
        if(cnt>=200)
        {
            xTaskResumeAll();   //出错也要恢复调度器！！
            return 1;
        }
        cnt = 0;
        while(GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN)==1 && cnt<200) cnt++;
        if(cnt>=200)
        {
            xTaskResumeAll();
            return 1;
        }
        
        // 连续读取5个字节
        for(i = 0; i < 5; i++)
        {
            buf[i] = DHT11_ReadByte();
        }
        
        // 数据校验
        if((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humidity = buf[0];
            *temperature = buf[2];
            xTaskResumeAll(); //恢复调度
            return 0;
        }
    }
    xTaskResumeAll(); //恢复调度器，所有出口必须调用！
    return 1;
}
