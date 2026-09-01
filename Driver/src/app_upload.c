#include "app_upload.h"
#include "serial.h"
#include "esp8266_at.h"
#include <string.h>
#include <stdio.h>
#include "rtos_components.h"
#include "config_project.h"



typedef enum
{
    STA_RESET_ESP,
    STA_WIFI_MODE,
    STA_WIFI_JOIN,
    STA_MQTT_CFG,
    STA_MQTT_CONNECT,
    STA_MQTT_SUB,
    STA_MQTT_ONLINE,
}Upload_State_t;

static Upload_State_t upload_state = STA_RESET_ESP;
static TickType_t last_upload_tick = 0;

void vUploadTask(void *pvParameters)
{
    int temp = 0;
    int humi = 0;
    char at_buf[256];
    char json_raw[128];
    uint16_t raw_len;

    ESP8266_USART2_Init(115200);
    vTaskDelay(pdMS_TO_TICKS(2000));

    TickType_t xStackPrintTick = 0;
    static uint8_t mqtt_upload_fail_cnt = 0;

    for(;;)
    {
        if((xTaskGetTickCount() - xStackPrintTick) >= pdMS_TO_TICKS(5000))
        {
            xStackPrintTick = xTaskGetTickCount();
        }

        switch(upload_state)
        {
        case STA_RESET_ESP:
            Serial_Printf("[UPLOAD] Reset ESP8266\r\n");
            ESP_SendAT("AT+RST","OK",1000); //由1500→1000ms
            vTaskDelay(pdMS_TO_TICKS(1000));//缩短延时
            ESP_ClearRxBuf();
            upload_state = STA_WIFI_MODE;
            break;

        case STA_WIFI_MODE:
            Serial_Printf("[UPLOAD] Set STA mode\r\n");
            if(ESP_SendAT("AT+CWMODE=1","OK",1000)==ESP_OK)//1000ms上限
            {
                upload_state = STA_WIFI_JOIN;
                vTaskDelay(pdMS_TO_TICKS(400));
            }
            else
            {
                Serial_Printf("[UPLOAD] Set mode fail\r\n");
                upload_state = STA_RESET_ESP;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;

        case STA_WIFI_JOIN:
            Serial_Printf("[UPLOAD] Connect WiFi:%s\r\n",WIFI_NAME);
            sprintf(at_buf,"AT+CWJAP=\"%s\",\"%s\"",WIFI_NAME,WIFI_PASS);
            if(ESP_SendAT(at_buf,"WIFI GOT IP",1000)==ESP_OK)//重点！6000ms改成1000ms
            {
                Serial_Printf("[UPLOAD] WiFi OK\r\n");
                upload_state = STA_MQTT_CFG;
                vTaskDelay(pdMS_TO_TICKS(600));
            }
            else
            {
                Serial_Printf("[UPLOAD] WiFi fail, retry\r\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                upload_state = STA_RESET_ESP;
            }
            break;

        case STA_MQTT_CFG:
            Serial_Printf("[UPLOAD] MQTTUSERCFG\r\n");
            sprintf(at_buf,"AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",DEVICE_NAME,PRODUCT_ID,TOKEN_STR);
            if(ESP_SendAT(at_buf,"OK",1000)==ESP_OK)//1000ms
            {
                Serial_Printf("[UPLOAD] MQTTUSERCFG OK\r\n");
                upload_state = STA_MQTT_CONNECT;
                vTaskDelay(pdMS_TO_TICKS(600));
            }
            else
            {
                upload_state = STA_RESET_ESP;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;

        case STA_MQTT_CONNECT:
            Serial_Printf("[UPLOAD] MQTT Connect OneNET\r\n");
            if(ESP_SendAT("AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1","OK",1000)==ESP_OK)//1000ms
            {
                Serial_Printf("[UPLOAD] MQTT CONN cmd OK\r\n");
                vTaskDelay(pdMS_TO_TICKS(800));
                upload_state = STA_MQTT_SUB;
            }
            else
            {
                Serial_Printf("[UPLOAD] MQTT connect cmd fail\r\n");
                upload_state = STA_RESET_ESP;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;

        case STA_MQTT_SUB:
            Serial_Printf("[UPLOAD] Subscribe reply topic\r\n");
            sprintf(at_buf,"AT+MQTTSUB=0,\"%s\",0",SUB_TOPIC);
            if(ESP_SendAT(at_buf,"OK",1000)==ESP_OK)//1000ms
            {
                Serial_Printf("[UPLOAD] MQTT online ready,start upload\r\n");
                upload_state = STA_MQTT_ONLINE;
                last_upload_tick = xTaskGetTickCount();
            }
            else
            {
                upload_state = STA_RESET_ESP;
            }
            break;

 case STA_MQTT_ONLINE:
    if(xTaskGetTickCount()-last_upload_tick >= pdMS_TO_TICKS(UPLOAD_PERIOD_MS))
    {
        last_upload_tick = xTaskGetTickCount();
        temp = g_last_temp;
        humi = g_last_hum;
        sprintf(json_raw,"{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"temp\":{\"value\":%d},\"humi\":{\"value\":%d}}}",temp,humi);
        raw_len=strlen(json_raw);
        ESP_Status_t ret=ESP_MQTT_PubRaw(PUB_TOPIC,(uint8_t *)json_raw,raw_len,4000);
        if(ret == ESP_OK)
        {
            Serial_Printf("[UPLOAD] Publish OK\r\n");
            mqtt_upload_fail_cnt = 0;
        }
        else
        {
            mqtt_upload_fail_cnt++;
            Serial_Printf("[UPLOAD] Publish fail, cnt=%d\r\n",mqtt_upload_fail_cnt);
            if(mqtt_upload_fail_cnt >=3)
            {
                Serial_Printf("[UPLOAD] 连续失败3次，重启ESP\r\n");
                upload_state = STA_RESET_ESP;
                mqtt_upload_fail_cnt=0;
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    break;


        default:
            upload_state=STA_RESET_ESP;
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
