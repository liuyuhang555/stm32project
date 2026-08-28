#include "app_display.h"
#include "oled.h"
#include "led.h"
#include "store.h"
#include "serial.h"
//oled显示任务
void vDisplayTask(void *pvParameters)
{
    DHT_Data_t dhtPacket;
    BaseType_t xStatus;
    static uint32_t ledTickCnt = 0;
    const uint32_t LED_BLINK_PERIOD = 200;  //闪烁间隔ms
    TickType_t xStackPrintTick = 0;

    while(1)
    {
        Task_ReportHeartbeat(TASK_IDX_DISPLAY);
        xStatus = xQueueReceive(xDhtDataQueue, &dhtPacket, pdMS_TO_TICKS(200));
        if(xStatus == pdPASS)
        {
            // 复现乱屏：注释下面if(xSemaphoreTake(...)) 和末尾 xSemaphoreGive(xOledMutex);
            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(10)) == pdPASS)
            {
                OLED_ClearArea(48, 0, 16, 16);
                OLED_ShowNum(48, 0, dhtPacket.temp, 2, OLED_8X16);
                OLED_ClearArea(48, 16, 16, 16);
                OLED_ShowNum(48, 16, dhtPacket.hum, 2, OLED_8X16);
                OLED_ShowString(0,32,"TH:",OLED_8X16);
                OLED_ShowNum(24,32,g_sys_cfg.temp_alarm_th,2,OLED_8X16);
                OLED_ShowString(48,32,"HH:",OLED_8X16);
                OLED_ShowNum(72,32,g_sys_cfg.hum_alarm_th,2,OLED_8X16);
                EventBits_t bits = xEventGroupGetBits(xSysEventGroup);
                if(bits & EVENT_BIT_ALARM_TRIG)
                {
                    OLED_ShowString(0,48,"ALARM!",OLED_8X16);
                }
                else if(g_dht_fault_cnt >= DHT_FAULT_MAX_CNT)
                {
                    OLED_ShowString(0,48,"SENSOR ERR",OLED_8X16);
                }
                else
                {
                    OLED_ClearArea(0,48,48,16);
                }
                OLED_Update();
                xSemaphoreGive(xOledMutex);
            }
        }
        /* ========= PC13报警LED：报警闪烁，平时熄灭 ========= */
        EventBits_t bitsNow = xEventGroupGetBits(xSysEventGroup);
        if(bitsNow & EVENT_BIT_ALARM_TRIG)
        {
            ledTickCnt += 200;
            if(ledTickCnt >= LED_BLINK_PERIOD)
            {
                //修复枚举混合警告，强转为BitAction
                GPIO_WriteBit(GPIOC, GPIO_Pin_13, (BitAction)(!GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13)));
                ledTickCnt = 0;
            }
        }
        else
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            ledTickCnt = 0;
        }

        // 每5s打印栈水位
        if((xTaskGetTickCount() - xStackPrintTick) >= pdMS_TO_TICKS(5000))
        {
            xStackPrintTick = xTaskGetTickCount();
            UBaseType_t stack_water = uxTaskGetStackHighWaterMark(NULL);
//            Serial_Printf("[Stack] DisplayTask water:%d words\r\n", stack_water);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
