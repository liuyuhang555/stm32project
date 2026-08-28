#include "stm32f10x.h"   // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "Tim_Delay.h"
#include "led.h"
#include "key.h"
#include "serial.h"
#include "oled.h"
#include "dht11.h"
#include "store.h"
#include "iwdg.h"
#include "esp8266_at.h"
#include <string.h>
#include "rtos_components.h"

#include "app_sensor.h"
#include "app_display.h"
#include "app_upload.h"
#include "app_lowpower.h"

//构建MQTT CONNECT报文
uint16_t MQTT_BuildConnect(uint8_t* buf,const char* clientId)
{
    uint16_t idx=0;
    buf[idx++] = 0x10;
    uint16_t rem_len = 14 + strlen(clientId);
    buf[idx++] = rem_len;
    buf[idx++] = 0x00;buf[idx++] =0x04;
    buf[idx++] = 'M';buf[idx++]='Q';buf[idx++]='T';buf[idx++]='T';
    buf[idx++] = 0x04;
    buf[idx++] = 0x02; //clean session
    buf[idx++] = 0x00;buf[idx++] = 0x3C; //keepalive 60
    uint16_t cli_len = strlen(clientId);
    buf[idx++] = (cli_len>>8)&0xFF;
    buf[idx++] = cli_len&0xFF;
    memcpy(buf+idx,clientId,cli_len);
    idx += cli_len;
    return idx;
}

//构建MQTT PUBLISH报文 QoS1
uint16_t MQTT_BuildPublish(uint8_t* buf,const char* topic,const char* payload)
{
    uint16_t idx=0;
    buf[idx++] = 0x32;  // DUP=0 QoS=1 RETAIN=0
    uint16_t topic_len = strlen(topic);
    uint16_t pay_len = strlen(payload);
    uint16_t rem_len = 2 + topic_len + 2 + pay_len; //+2 bytes msgid
    buf[idx++] = rem_len;
    buf[idx++] = (topic_len>>8)&0xFF;
    buf[idx++] = topic_len&0xFF;
    memcpy(buf+idx,topic,topic_len);
    idx += topic_len;
    //message id
    buf[idx++] = 0x00;
    buf[idx++] = 0x01;
    memcpy(buf+idx,payload,pay_len);
    idx += pay_len;
    return idx;
}

uint8_t g_in_stop_mode = 0;  // 1=正在STOP休眠
uint8_t g_enterStopReq = 0;

uint8_t Get_InStopMode(void)
{
    return g_in_stop_mode;
}

/* ========= 任务心跳监控结构体 只监控3个任务 ========= */
typedef struct
{
    TaskHandle_t taskHandle;
    uint32_t heartbeat;         // 任务自己递增心跳
    uint32_t lastHeartbeat;     // watchdog保存上一次心跳
    uint8_t faultCnt;           // 连续异常计数
    const char* taskName;
}TaskHeartBeat_t;

static TaskHeartBeat_t *g_TaskHeartBeatList = NULL;
#define TASK_HEARTBEAT_NUM  3U
#define HEARTBEAT_FAULT_MAX   (4U)       // 连续2次心跳不变判定卡死

/* 给外部任务用：上报心跳，每个业务任务循环调用 */
void Task_ReportHeartbeat(uint8_t idx)
{
    if(g_TaskHeartBeatList != NULL && idx < TASK_HEARTBEAT_NUM)
    {
        g_TaskHeartBeatList[idx].heartbeat++;
    }
}

/* ========= 任务句柄 ========= */
TaskHandle_t xSensorTaskHandle;
TaskHandle_t xDisplayTaskHandle;
TaskHandle_t xUploadTaskHandle;
TaskHandle_t xWatchdogTaskHandle;
TaskHandle_t xLowPowerTaskHandle;

/* ========= 互斥量句柄定义 ========= */
extern SemaphoreHandle_t xOledMutex;
extern SemaphoreHandle_t xUartMutex;

/**
 * @brief FreeRTOS空闲任务钩子，实现Tickless STOP模式
 * FreeRTOSConfig.h configUSE_IDLE_HOOK 1
 */
void vApplicationIdleHook(void)
{
    if(g_enterStopReq != 0)
    {
        EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13);
        PWR_ClearFlag(PWR_FLAG_WU);
        //进入STOP模式
        PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
        //唤醒之后，运行在HSI，禁止在这里做HSE初始化！
        EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13);
        PWR_ClearFlag(PWR_FLAG_WU);
        g_enterStopReq = 0; //清零休眠请求，通知低功耗任务
    }
}

/**
 * @brief 看门狗异常处理任务
 * 1.任务心跳卡死；2.DHT11连续读取失败；任一条件停止喂狗，IWDG复位
 */
void vWatchdogTask(void *pvParameters)
{
    uint8_t i;
    uint8_t systemAbnormal = 0;
    TickType_t xStackPrintTick = 0;
    while(1)
    {
        Task_ReportHeartbeat(TASK_IDX_WATCHDOG);
        systemAbnormal = 0;
        for(i = 0; i < TASK_HEARTBEAT_NUM; i++)
        {
            if(g_TaskHeartBeatList[i].heartbeat == g_TaskHeartBeatList[i].lastHeartbeat)
            {
                g_TaskHeartBeatList[i].faultCnt++;
                if(g_TaskHeartBeatList[i].faultCnt >= HEARTBEAT_FAULT_MAX)
                {
                    systemAbnormal = 1;
                }
            }
            else
            {
                g_TaskHeartBeatList[i].faultCnt = 0;
            }
        }
        if(g_dht_fault_cnt >= DHT_FAULT_MAX_CNT)
        {
            systemAbnormal = 1;
        }
        for(i = 0; i < TASK_HEARTBEAT_NUM; i++)
        {
            g_TaskHeartBeatList[i].lastHeartbeat = g_TaskHeartBeatList[i].heartbeat;
        }
        if(systemAbnormal == 0)
        {
            IWDG_Feed();
        }
        else
        {
            Serial_Printf("[WATCHDOG] FAULT! Wait IWDG Reset...\r\n");
        }
					//每5s打印栈水位
        if((xTaskGetTickCount() - xStackPrintTick) >= pdMS_TO_TICKS(5000))
        {
            xStackPrintTick = xTaskGetTickCount();
            UBaseType_t stack_water = uxTaskGetStackHighWaterMark(NULL);
//            Serial_Printf("[Stack] WatchdogTask water:%d words\r\n", stack_water);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    Serial_Printf("!!!STACK OVERFLOW!!! Task:%s\r\n",pcTaskName);
    while(1);
}

int main(void)
{
    SystemInit();
    Serial_Init();
    /* --------复位标志读取---------- */
    uint8_t is_iwdg_rst = (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET);
    uint8_t is_pin_rst  = (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET);
    RCC_ClearFlag(); //读完清除复位标志
    if(is_iwdg_rst)
    {
        Serial_Printf("#### SYSTEM RESET BY IWDG ####\r\n");
    }
    if(is_pin_rst)
    {
        Serial_Printf("#### SYSTEM RESET BY PIN ####\r\n");
    }
			rtos_components_init();//创建队列、事件组、互斥锁
    /* 硬件初始化 */
    led_Init();
    key_Init();
    DHT11_Init();
    OLED_Init();
    OLED_Clear();
    TIM_Delay_Init();
    IWDG_Config(IWDG_Prescaler_64,1250);
    Store_LoadConfig();
    OLED_ShowChinese(0,0,"温度：  ");
    OLED_ShowChinese(0,16,"湿度： ");
    OLED_Update();

    /* ========= 判断DHT队列是否创建 ========= */
    if(xDhtDataQueue == NULL)
    {
        //队列失败：快闪
        while(1)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            for(uint32_t i=0;i<50000;i++);
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            for(uint32_t i=0;i<50000;i++);
        }
    }
    /* ========= 判断事件标志组是否创建成功 ========= */
    if(xSysEventGroup == NULL)
    {
        //事件组失败：中速
        while(1)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            for(uint32_t i=0;i<120000;i++);
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            for(uint32_t i=0;i<120000;i++);
        }
    }
    /* ========= 判断创建互斥量是否成功 ========= */
    if(xOledMutex == NULL)
    {
        //oled互斥锁失败
        while(1)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            for(uint32_t i=0;i<180000;i++);
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            for(uint32_t i=0;i<180000;i++);
        }
    }
    if(xUartMutex == NULL)
    {
        //uart互斥锁失败
        while(1)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            for(uint32_t i=0;i<220000;i++);
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            for(uint32_t i=0;i<220000;i++);
        }
    }

    /* 分配心跳监控内存 */
    g_TaskHeartBeatList = pvPortMalloc(sizeof(TaskHeartBeat_t)*TASK_HEARTBEAT_NUM);
    if(g_TaskHeartBeatList == NULL)
    {
        //心跳数组malloc失败
        while(1)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
            for(uint32_t i=0;i<280000;i++);
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
            for(uint32_t i=0;i<280000;i++);
        }
    }
    g_TaskHeartBeatList[0] = (TaskHeartBeat_t){NULL,0,0,0,"SensorTask"};
    g_TaskHeartBeatList[1] = (TaskHeartBeat_t){NULL,0,0,0,"DisplayTask"};
    g_TaskHeartBeatList[2] = (TaskHeartBeat_t){NULL,0,0,0,"WatchdogTask"};

    /* 创建任务，upload栈调整为400减轻F103 RAM压力 */
    xTaskCreate(vSensorTask,     "Sensor_Task",    512, NULL, 2, &xSensorTaskHandle);
    xTaskCreate(vDisplayTask,    "Display_Task",   320, NULL, 2, &xDisplayTaskHandle);
    xTaskCreate(vUploadTask,     "Upload_Task",    400, NULL, 2, &xUploadTaskHandle);
    xTaskCreate(vWatchdogTask,   "Watchdog_Task",  256, NULL, 3, &xWatchdogTaskHandle);
//    xTaskCreate(vLowPowerTask,    "LowPower_Task", 256, NULL, 2, &xLowPowerTaskHandle);
		//创建句柄
    g_TaskHeartBeatList[TASK_IDX_SENSOR].taskHandle    = xSensorTaskHandle;
    g_TaskHeartBeatList[TASK_IDX_DISPLAY].taskHandle   = xDisplayTaskHandle;
    g_TaskHeartBeatList[TASK_IDX_WATCHDOG].taskHandle  = xWatchdogTaskHandle;

    vTaskStartScheduler();

    // 调度器启动失败，跑到这里
    while(1)
    {
        //修复main末尾闪烁的枚举警告
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, (BitAction)(!GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13)));
        for(uint32_t i=0;i<500000;i++);
    }
}
