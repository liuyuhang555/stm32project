#ifndef __ESP8266_AT_H
#define __ESP8266_AT_H
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define ESP_USARTx USART2

//AT×´Ì¬Âë
typedef enum
{
    ESP_OK = 0,
    ESP_TIMEOUT,
    ESP_ERROR
}ESP_Status_t;

void ESP8266_USART2_Init(uint32_t baud);
ESP_Status_t ESP_SendAT(const char *cmd, const char *resp_ok, uint32_t timeout_ms);
ESP_Status_t ESP_WaitRecv(const char *resp_ok, uint32_t timeout_ms); //ÐÂÔö
ESP_Status_t ESP_WifiConnect(const char *ssid,const char *pwd);
ESP_Status_t ESP_MQTT_Init(const char *mqtt_ip,uint16_t mqtt_port,const char *client_id);
ESP_Status_t ESP_MQTT_Publish(const char *topic,const char *payload);
ESP_Status_t ESP_TcpConnect(const char* host,uint16_t port,uint32_t timeout_ms);
ESP_Status_t ESP_TcpSend(const uint8_t* data,uint16_t len,uint32_t timeout_ms);

void ESP_ClearRxBuf(void);
void ESP_SendString(const char *str);
void ESP_SendAT_NoWait(const char *cmd);
ESP_Status_t ESP_WaitResp(const char *resp_ok, uint32_t timeout_ms);
ESP_Status_t ESP_MQTT_PubRaw(const char *topic,const uint8_t *payload,uint16_t pay_len,uint32_t timeout_ms);


#endif
