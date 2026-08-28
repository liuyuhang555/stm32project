#ifndef __STORE_H
#define __STORE_H

#include "stm32f10x.h"

/* Flash配置页 F103C8T6 64KB Flash最后一页2KB */
#define STORE_CFG_PAGE_ADDR      0x0800F800U
#define STORE_MAGIC_WORD         0xA55AA55AU

/* 保存的配置结构体：温度、湿度报警阈值 */
typedef struct
{
	uint32_t magic;
	uint8_t temp_alarm_th;
	uint8_t hum_alarm_th;
	uint16_t reserve;
}SysConfig_t;

extern SysConfig_t g_sys_cfg;

/* 上电从Flash加载配置到RAM */
void Store_LoadConfig(void);
/* 将RAM配置写入Flash，??耗时20ms，仅低优先级任务调用 */
uint8_t Store_SaveConfig(SysConfig_t *pCfg);

#endif
