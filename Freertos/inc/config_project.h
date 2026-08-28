#ifndef __CONFIG_PROJECT_H
#define __CONFIG_PROJECT_H

//====================业务参数====================
#define TEMP_ALARM_MAX      35U
#define TEMP_ALARM_MIN      20U
#define HUMI_ALARM_MAX      75U
#define HUMI_ALARM_MIN      30U

//F103C8T6 片内Flash参数页地址，最后1K页面
#define FLASH_PARAM_PAGE_ADDR       0x0800FC00

//====================FreeRTOS任务栈(单位：字，1字=4字节) & 优先级====================
#define SENSOR_TASK_STACK_SIZE      512
#define SENSOR_TASK_PRIORITY        2

#define DISPLAY_TASK_STACK_SIZE     320
#define DISPLAY_TASK_PRIORITY       2

#define KEY_TASK_STACK_SIZE         320
#define KEY_TASK_PRIORITY           2

#define WATCHDOG_TASK_STACK_SIZE    256
#define WATCHDOG_TASK_PRIORITY      3

#define ESP_TASK_STACK_SIZE         600
#define ESP_TASK_PRIORITY           2

//====================队列配置====================
#define SENSOR_QUEUE_LEN            4

#endif
