#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "semphr.h"


extern SemaphoreHandle_t xUartMutex;

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
uint8_t Get_InStopMode(void);

#endif
