#include "app_lowpower.h"
#include "oled.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_exti.h"
#include "iwdg.h"
#include "serial.h"

// 这两个全局变量实现在main.c，外部引用
extern uint8_t g_in_stop_mode;
extern uint8_t g_enterStopReq;
extern TaskHandle_t xSensorTaskHandle;
extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xUploadTaskHandle;

void vLowPowerTask(void *pvParameters)
{
    EventBits_t uxBits;
    while(1)
    {
//        Task_ReportHeartbeat(TASK_IDX_LOWPOWER); 
        uxBits = xEventGroupWaitBits(
                    xSysEventGroup,
                    EVENT_BIT_ENTER_LOWPOWER,
                    pdTRUE,
                    pdFALSE,
                    pdMS_TO_TICKS(500));
        if((uxBits & EVENT_BIT_ENTER_LOWPOWER) != 0)
        {
            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
            {
                OLED_Clear();
                OLED_ShowString(0,0,"GO STOP MODE",OLED_8X16);
                OLED_Update();
                xSemaphoreGive(xOledMutex);
            }
            //等待按键松开
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_13) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            g_in_stop_mode = 1;
            //挂起业务任务，看门狗任务保留运行
            vTaskSuspend(xSensorTaskHandle);
            vTaskSuspend(xDisplayTaskHandle);
            vTaskSuspend(xUploadTaskHandle);
            g_enterStopReq = 1;
            while(g_enterStopReq == 1)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            //========唤醒回来第一步：重新初始化串口，恢复发送========
            Serial_Init();
            //恢复任务
            vTaskResume(xSensorTaskHandle);
            vTaskResume(xDisplayTaskHandle);
            vTaskResume(xUploadTaskHandle);
            g_in_stop_mode = 0;
            IWDG_Feed();
        }
    }
}
