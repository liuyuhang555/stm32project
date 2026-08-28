#ifndef __RTOS_COMPONENTS_H
#define __RTOS_COMPONENTS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "config_project.h"

/* 温湿度数据包结构体，队列传递 */
typedef struct
{
    uint8_t temp;
    uint8_t hum;
}DHT_Data_t;

/* 任务索引 */
enum TASK_IDX
{
    TASK_IDX_SENSOR = 0,
    TASK_IDX_DISPLAY,
    TASK_IDX_WATCHDOG,
};

/* ========== 事件标志位定义 ========== */
#define EVENT_BIT_SENSOR_DONE     (1 << 0)   // 传感器采集完成
#define EVENT_BIT_ALARM_TRIG      (1 << 1)   // 报警触发
#define EVENT_BIT_KEY_PRESS       (1 << 2)   // 按键按下
#define EVENT_BIT_ENTER_LOWPOWER  (1 << 3)   // 请求进入低功耗STOP模式

#define FLASH_SAVE_REQ     0x01U
#define DHT_FAULT_MAX_CNT     5U      //DHT连续失败5次判定传感器故障

/* WIFI MQTT 上传配置 */
#define WIFI_SSID      "liu123"
#define WIFI_PWD       "12125418"
#define MQTT_HOST      "broker.emqx.io"
#define MQTT_PORT      1883
#define MQTT_CLI_ID    "stm32f103_sensor_001"
#define MQTT_TOPIC     "sensor/env/data"


/*************************
 * 全部使用 extern 声明，真实定义放在 rtos_components.c
 *************************/
/* 队列 */
extern QueueHandle_t xDhtDataQueue;
/* 事件标志组 */
extern EventGroupHandle_t xSysEventGroup;
/* 互斥量 */
extern SemaphoreHandle_t xOledMutex;
extern SemaphoreHandle_t xUartMutex;

/* 业务全局状态变量 */
extern uint8_t g_flashSaveReq;
extern uint8_t g_last_temp;
extern uint8_t g_last_hum;
extern uint8_t g_dht_fault_cnt;

/* 内核对象初始化函数，main调用一次 */
void rtos_components_init(void);
/* 任务心跳上报，业务任务调用 */
void Task_ReportHeartbeat(uint8_t idx);
/* MQTT报文构建函数，实现在main.c */
uint16_t MQTT_BuildConnect(uint8_t* buf,const char* clientId);
uint16_t MQTT_BuildPublish(uint8_t* buf,const char* topic,const char* payload);
/* 心跳任务索引 */
#define TASK_IDX_SENSOR        0U
#define TASK_IDX_DISPLAY       1U
#define TASK_IDX_WATCHDOG      2U


#define TASK_HEARTBEAT_NUM     3U   

#endif
