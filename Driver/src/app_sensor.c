#include "app_sensor.h"
#include "config_project.h"
#include "dht11.h"
#include "rtos_components.h"
#include "key.h"
#include "store.h"
#include "serial.h"

/*传感器处理数据任务*/
void vSensorTask(void *pvParameters)
{
    uint8_t tempBuf, humBuf;
    DHT_Data_t dhtPacket;
    uint8_t key_val;
    static uint8_t flashDelayCnt = 0;

    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);
    TickType_t xStackPrintTick = 0;
    xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        Task_ReportHeartbeat(TASK_IDX_SENSOR);
        key_val = KEY_Scan(0);
				/*按键修改阈值*/
        if(key_val == KEY1_PRESS)
        {
            if(g_sys_cfg.temp_alarm_th > 20)
            {
                g_sys_cfg.temp_alarm_th--;
            }
            Serial_Printf("[KEY] Key1 ShortPress, new temp th=%d\r\n", g_sys_cfg.temp_alarm_th);
            g_flashSaveReq = FLASH_SAVE_REQ;
            flashDelayCnt = 0;
        }
        else if(key_val == KEY1_LONG_PRESS)
        {
             Serial_Printf("[KEY] Key1 LongPress → Simulate SensorTask dead loop !!!\r\n");
            while(1)
            {
            }
        }
        else if(key_val == KEY2_PRESS)
        {
            if(g_sys_cfg.temp_alarm_th < 60)
            {
                g_sys_cfg.temp_alarm_th++;
            }
            Serial_Printf("[KEY] Key2 pressed, new temp th=%d\r\n", g_sys_cfg.temp_alarm_th);
            g_flashSaveReq = FLASH_SAVE_REQ;
            flashDelayCnt = 0;
        }
        if(g_flashSaveReq != 0)
        {
            flashDelayCnt ++;
            if(flashDelayCnt >= 1U)
            {
                Store_SaveConfig(&g_sys_cfg);
                Serial_Printf("[STORE] Save Th:Temp=%d Hum=%d to Flash OK\r\n",g_sys_cfg.temp_alarm_th,g_sys_cfg.hum_alarm_th);
                g_flashSaveReq = 0;
                flashDelayCnt = 0;
            }
        }
				/*DHT11读取数据*/
        if(DHT11_ReadData(&tempBuf, &humBuf) == 0)
        {
            g_dht_fault_cnt = 0;
            dhtPacket.temp = tempBuf;
            dhtPacket.hum  = humBuf;
            g_last_temp = tempBuf;
            g_last_hum  = humBuf;
            xQueueOverwrite(xDhtDataQueue, &dhtPacket);
            xEventGroupSetBits(xSysEventGroup, EVENT_BIT_SENSOR_DONE);
            xEventGroupClearBits(xSysEventGroup, EVENT_BIT_ALARM_TRIG);
            if(g_last_temp > g_sys_cfg.temp_alarm_th || g_last_hum > g_sys_cfg.hum_alarm_th)
            {
                xEventGroupSetBits(xSysEventGroup, EVENT_BIT_ALARM_TRIG);
                Serial_Printf("!!!报警触发 温度:%d 湿度:%d |阈值T:%d H:%d\r\n",g_last_temp, g_last_hum,g_sys_cfg.temp_alarm_th,g_sys_cfg.hum_alarm_th);
            }
            Serial_Printf("温度：%d ℃ | 湿度：%d %%RH\r\n", dhtPacket.temp, dhtPacket.hum);
        }
        else
        {
            if(g_dht_fault_cnt < 0xFF)
            {
                g_dht_fault_cnt++;
            }
            Serial_Printf("[SENSOR] DHT11 read fail, fault_cnt=%d\r\n",g_dht_fault_cnt);
        }

        // 每5s打印栈水位
        if((xTaskGetTickCount() - xStackPrintTick) >= pdMS_TO_TICKS(5000))
        {
            xStackPrintTick = xTaskGetTickCount();
            UBaseType_t stack_water = uxTaskGetStackHighWaterMark(NULL);
//            Serial_Printf("[Stack] SensorTask water:%d words\r\n", stack_water);
        }
    }
}
