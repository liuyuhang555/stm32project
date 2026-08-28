#include "store.h"
#include "FreeRTOS.h"

SysConfig_t g_sys_cfg;

uint32_t Store_ReadWord(uint32_t Address)
{
	return *((__IO uint32_t *)(Address));
}

uint16_t Store_ReadHalfWord(uint32_t Address)
{
	return *((__IO uint16_t *)(Address));
}

uint8_t Store_ReadByte(uint32_t Address)
{
	return *((__IO uint8_t *)(Address));
}

void Store_ErasePage(uint32_t PageAddress)
{
	FLASH_Unlock();
	FLASH_ErasePage(PageAddress);
	FLASH_Lock();
}

void Store_ProgramWord(uint32_t Address,uint32_t Data)
{
	FLASH_Unlock();
	FLASH_ProgramWord(Address,Data);
	FLASH_Lock();
}

/**
 * @brief 上电加载配置
 */
void Store_LoadConfig(void)
{
	SysConfig_t *pFlash = (SysConfig_t *)STORE_CFG_PAGE_ADDR;
	g_sys_cfg = *pFlash;
	if(g_sys_cfg.magic != STORE_MAGIC_WORD)
	{
		/* 默认报警阈值 */
		g_sys_cfg.magic        = STORE_MAGIC_WORD;
		g_sys_cfg.temp_alarm_th = 35;
		g_sys_cfg.hum_alarm_th  = 80;
		g_sys_cfg.reserve       = 0;
	}
}

/**
 * @brief 把RAM配置写入Flash
 * @retval 1成功
 */
uint8_t Store_SaveConfig(SysConfig_t *pCfg)
{
	Store_ErasePage(STORE_CFG_PAGE_ADDR);
	Store_ProgramWord(STORE_CFG_PAGE_ADDR,      pCfg->magic);
	Store_ProgramWord(STORE_CFG_PAGE_ADDR + 4, *((uint32_t*)pCfg + 1));
	return 1;
}
