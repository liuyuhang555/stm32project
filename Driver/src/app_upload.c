#include "app_upload.h"
#include "serial.h"
#include "esp8266_at.h"
#include <string.h>
#include <stdio.h>   
/**
 * @brief 【数据上传任务】ESP8266 MQTT上传
 */
void vUploadTask(void *pvParameters)
{
		TickType_t xStackPrintTick = 0;
    DHT_Data_t dhtPacket;
    static char jsonBuf[128];
    uint8_t mqttBuf[256];
    uint16_t mqttLen;
    uint8_t wifi_ok = 0;
    uint8_t mqtt_tcp_ok = 0;
    ESP8266_USART2_Init(115200);
    vTaskDelay(pdMS_TO_TICKS(2000));
    for(;;)
    {
				// 每5s打印栈水位
				if((xTaskGetTickCount() - xStackPrintTick) >= pdMS_TO_TICKS(2000))
        {
            xStackPrintTick = xTaskGetTickCount();
            UBaseType_t stack_water = uxTaskGetStackHighWaterMark(NULL);
//            Serial_Printf("[Stack] UploadTask water:%d words\r\n", stack_water);
        }

        if(wifi_ok == 0)
        {
            Serial_Printf("[UPLOAD] Try connect WiFi\r\n");
            if(ESP_WifiConnect(WIFI_SSID,WIFI_PWD) == ESP_OK)
            {
                wifi_ok = 1;
                mqtt_tcp_ok = 0;
                Serial_Printf("[UPLOAD] WiFi Connected OK\r\n");
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            else
            {
                Serial_Printf("[UPLOAD] WiFi fail retry 3s\r\n");
                vTaskDelay(pdMS_TO_TICKS(3000));
                continue;
            }
        }
        if(mqtt_tcp_ok == 0)
        {
            Serial_Printf("[UPLOAD] TCP connect mqtt server\r\n");
            if(ESP_TcpConnect(MQTT_HOST,MQTT_PORT,8000) == ESP_OK)
            {
                Serial_Printf("[UPLOAD] TCP connected OK,send MQTT CONNECT\r\n");
                mqttLen = MQTT_BuildConnect(mqttBuf,MQTT_CLI_ID);
                if(ESP_TcpSend(mqttBuf,mqttLen,3000) == ESP_OK)
                {
                    mqtt_tcp_ok = 1;
                    Serial_Printf("[UPLOAD] MQTT handshake OK\r\n");
                }
                else
                {
                    Serial_Printf("[UPLOAD] MQTT connect pkt send fail\r\n");
                    mqtt_tcp_ok = 0;
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    continue;
                }
            }
            else
            {
                Serial_Printf("[UPLOAD] TCP connect fail retry 3s\r\n");
                vTaskDelay(pdMS_TO_TICKS(3000));
                continue;
            }
        }
        if(xQueueReceive(xDhtDataQueue,&dhtPacket,pdMS_TO_TICKS(1000)) == pdPASS)
        {
            sprintf(jsonBuf,"{\"temp\":%d,\"hum\":%d}",dhtPacket.temp,dhtPacket.hum);
            mqttLen = MQTT_BuildPublish(mqttBuf,MQTT_TOPIC,jsonBuf);
            Serial_Printf("[UPLOAD] Publish:%s\r\n",jsonBuf);
            if(ESP_TcpSend(mqttBuf,mqttLen,3000)!=ESP_OK)
            {
                Serial_Printf("[UPLOAD] publish send fail,reset tcp\r\n");
                mqtt_tcp_ok = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
