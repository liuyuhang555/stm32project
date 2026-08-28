#include "rtos_components.h"

/*======== 真实变量定义，仅此一处 =========*/
QueueHandle_t        xDhtDataQueue    = NULL;
EventGroupHandle_t   xSysEventGroup   = NULL;
SemaphoreHandle_t    xOledMutex      = NULL;
SemaphoreHandle_t    xUartMutex      = NULL;

uint8_t g_flashSaveReq    = 0;
uint8_t g_last_temp       = 0;
uint8_t g_last_hum        = 0;
uint8_t g_dht_fault_cnt   = 0;

/**
 * @brief 创建队列、事件组、互斥锁
 */
void rtos_components_init(void)
{
    /* DHT数据队列 */
    xDhtDataQueue = xQueueCreate(2, sizeof(DHT_Data_t));

    /* 事件标志组 */
    xSysEventGroup = xEventGroupCreate();

    /* 互斥锁 */
    xOledMutex = xSemaphoreCreateMutex();
    xUartMutex = xSemaphoreCreateMutex();
}
