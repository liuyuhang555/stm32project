#include "esp8266_at.h"
#include <string.h>

#define ESP_RX_BUF_LEN 512U
static uint8_t esp_rx_buf[ESP_RX_BUF_LEN];
static uint16_t rx_wr = 0;
static uint16_t rx_rd = 0;

static __INLINE uint16_t ESP_RxGetCount(void)
{
    if(rx_wr >= rx_rd)
        return rx_wr - rx_rd;
    return ESP_RX_BUF_LEN - rx_rd + rx_wr;
}

static __INLINE uint8_t ESP_RxReadByte(void)
{
    uint8_t ch = esp_rx_buf[rx_rd];
    rx_rd = (rx_rd + 1) % ESP_RX_BUF_LEN;
    return ch;
}

 void ESP_ClearRxBuf(void)
{
    rx_wr = 0;
    rx_rd = 0;
    memset(esp_rx_buf,0,ESP_RX_BUF_LEN);
}

void ESP8266_USART2_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA,&GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate = baud;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ESP_USARTx,&USART_InitStruct);

    USART_ITConfig(ESP_USARTx,USART_IT_RXNE,ENABLE);
    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    USART_Cmd(ESP_USARTx,ENABLE);
}

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(ESP_USARTx,USART_IT_RXNE)!=RESET)
    {
        uint8_t ch = USART_ReceiveData(ESP_USARTx);
        uint16_t next = (rx_wr +1) % ESP_RX_BUF_LEN;
        if(next != rx_rd)
        {
            esp_rx_buf[rx_wr] = ch;
            rx_wr = next;
        }
    }
}

void ESP_SendString(const char *str)
{
    while(*str != '\0')
    {
        while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TXE)==RESET);
        USART_SendData(ESP_USARTx,*str++);
    }
    while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TC)==RESET);
}



ESP_Status_t ESP_SendAT(const char *cmd, const char *resp_ok, uint32_t timeout_ms)
{
    ESP_ClearRxBuf();
    ESP_SendString(cmd);
    ESP_SendString("\r\n");
    uint32_t tick_start = xTaskGetTickCount();
    TickType_t timeout_tick = pdMS_TO_TICKS(timeout_ms);
    char tmp[512]={0};
    uint16_t idx = 0;
    while((xTaskGetTickCount() - tick_start) < timeout_tick)
    {
        while(ESP_RxGetCount()>0 && idx < sizeof(tmp)-1)
        {
            tmp[idx++] = ESP_RxReadByte();
        }
        if(strstr(tmp, resp_ok) != NULL)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    Serial_Printf("[ESP_SendAT] Timeout!\r\n");
    return ESP_TIMEOUT;
}

ESP_Status_t ESP_WaitRecv(const char *resp_ok, uint32_t timeout_ms)
{
    uint32_t tick_start = xTaskGetTickCount();
    TickType_t timeout_tick = pdMS_TO_TICKS(timeout_ms);
    char tmp[512]={0};
    uint16_t idx = 0;
    while((xTaskGetTickCount() - tick_start) < timeout_tick)
    {
        while(ESP_RxGetCount()>0 && idx < sizeof(tmp)-1)
        {
            tmp[idx++] = ESP_RxReadByte();
        }
        if(strstr(tmp, resp_ok) != NULL)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    Serial_Printf("[ESP_WaitRecv] Timeout!\r\n");
    return ESP_TIMEOUT;
}


ESP_Status_t ESP_WifiConnect(const char *ssid,const char *pwd)
{
    char buf[128];
    ESP_SendAT("AT+RST","OK",1000);
    vTaskDelay(pdMS_TO_TICKS(1500));
		ESP_SendAT("ATE0","OK",500);   //关闭回显！！
    ESP_SendAT("AT+CWMODE=1","OK",500);
    sprintf(buf,"AT+CWJAP=\"%s\",\"%s\"",ssid,pwd);
    return ESP_SendAT(buf,"WIFI GOT IP",8000);
}

ESP_Status_t ESP_MQTT_Init(const char *mqtt_ip,uint16_t mqtt_port,const char *client_id)
{
    static char buf[128];  //改成static，不在栈分配
    ESP_Status_t ret;

    ret = ESP_SendAT("AT+MQTTUSERCFG=0,1,\"\",\"\",\"\",0,0,\"\"","OK",2000);
    if(ret != ESP_OK)
    {
        Serial_Printf("[AT] MQTTUSERCFG fail\r\n");
        return ret;
    }
    Serial_Printf("[AT] MQTTUSERCFG OK\r\n");

    sprintf(buf,"AT+MQTTCONN=0,\"%s\",%d,\"%s\",1,60",mqtt_ip,mqtt_port,client_id);
    //打印出真正发给ESP的完整AT字符串，看拼接是否出错
    Serial_Printf("[DEBUG] send: %s\r\n",buf);

    ret = ESP_SendAT(buf,"OK",3000);
    if(ret != ESP_OK)
    {
        Serial_Printf("[AT] MQTTCONN cmd send fail\r\n");
        return ret;
    }
    Serial_Printf("[AT] MQTTCONN cmd OK,wait +MQTTCONNECTED\r\n");

    ret = ESP_WaitRecv("+MQTTCONNECTED",8000);
    if(ret != ESP_OK)
    {
        Serial_Printf("[AT] wait +MQTTCONNECTED timeout\r\n");
    }
    return ret;
}

ESP_Status_t ESP_MQTT_Publish(const char *topic,const char *payload)
{
    char buf[256];
    sprintf(buf,"AT+MQTTPUB=0,\"%s\",\"%s\",0,0",topic,payload);
    return ESP_SendAT(buf,"OK",3000);
}

ESP_Status_t ESP_TcpConnect(const char* host,uint16_t port,uint32_t timeout_ms)
{
    static char buf[128];
    ESP_ClearRxBuf();
    sprintf(buf,"AT+CIPSTART=\"TCP\",\"%s\",%d",host,port);
    return ESP_SendAT(buf,"CONNECT",timeout_ms);
}

ESP_Status_t ESP_TcpSend(const uint8_t* data,uint16_t len,uint32_t timeout_ms)
{
    static char buf[64];
    ESP_ClearRxBuf();
    sprintf(buf,"AT+CIPSEND=%d",len);
    ESP_Status_t ret = ESP_SendAT(buf,">",2000); //只等待 > 提示符
    if(ret != ESP_OK)
        return ret;

    //发送payload
    USART_ITConfig(ESP_USARTx, USART_IT_RXNE, DISABLE);
    for(uint16_t i=0;i<len;i++)
    {
        while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TXE)==RESET);
        USART_SendData(ESP_USARTx,data[i]);
    }
    while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TC)==RESET);
    USART_ITConfig(ESP_USARTx, USART_IT_RXNE, ENABLE);

    //公网环境，不等待SEND OK，网络延迟高经常超时，直接返回OK
    return ESP_OK;
}
//发送AT命令追加\r\n，**不等待应答**（ESP_SendAT_NoWait）
void ESP_SendAT_NoWait(const char *cmd)
{
    ESP_SendString(cmd);
    ESP_SendString("\r\n");
}
//别名包装，和前面代码保持名字一致
ESP_Status_t ESP_WaitResp(const char *resp_ok, uint32_t timeout_ms)
{
    return ESP_WaitRecv(resp_ok,timeout_ms);
}
ESP_Status_t ESP_MQTT_PubRaw(const char *topic,const uint8_t *payload,uint16_t pay_len,uint32_t timeout_ms)
{
    char cmd_buf[256];
    ESP_ClearRxBuf();
    //拼接AT+MQTTPUBRAW=0,"topic",length,0,0
    sprintf(cmd_buf,"AT+MQTTPUBRAW=0,\"%s\",%d,0,0",topic,pay_len);
    ESP_SendString(cmd_buf);
    ESP_SendString("\r\n");
    //第一步等待 ">" 提示符
    ESP_Status_t ret = ESP_WaitRecv(">",timeout_ms);
    if(ret != ESP_OK)
    {
        Serial_Printf("[PUBRAW] wait '>' failed\r\n");
        return ret;
    }
    //发送原始载荷，不要追加\r\n
    for(uint16_t i=0;i<pay_len;i++)
    {
        while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TXE)==RESET);
        USART_SendData(ESP_USARTx,payload[i]);
    }
    while(USART_GetFlagStatus(ESP_USARTx,USART_FLAG_TC)==RESET);

    //====重点删除下面等待SEND OK的代码====
    //ret = ESP_WaitRecv("SEND OK",timeout_ms);
    //if(ret != ESP_OK)
    //{
    //    Serial_Printf("[PUBRAW] wait SEND OK timeout\r\n");
    //}
    //=====================================

    Serial_Printf("[PUBRAW] send payload done, ignore SEND OK\r\n");
    return ESP_OK;
}
