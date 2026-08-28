# STM32F103C8T6 FreeRTOS 智能环境监测系统
基于STM32F103C8T6 + FreeRTOS的温湿度监测项目，嵌入式学习/秋招实战项目。

## 硬件平台
- MCU：STM32F103C8T6（标准库）
- 外设：DHT11温湿度传感器、0.96寸I2C‑OLED、按键、板载LED、ESP8266‑AT WIFI模块
- 开发环境：Keil MDK‑ARM

## 引脚定义
- LED：PC13
- KEY1：PA0
- KEY2：PA1
- DHT11：PB14
- OLED I2C：PB7(SDA)，PB6(SCL)
- USART1(ESP8266‑AT)：PA9(TX)，PA10(RX)
- USART2(调试串口)：PA2(TX)，PA3(RX)

## 功能实现
### 外设层
- DHT11温湿度采集，传感器故障计数，OLED显示故障提示
- OLED显示实时温湿度、报警阈值、报警状态
- 按键修改温湿度报警阈值，短按增减阈值、长按模拟系统卡死触发看门狗
- 片内Flash存储参数，掉电保存报警阈值
- IWDG独立看门狗，任务心跳检测，任务卡死自动系统复位

### FreeRTOS 内核层
- 任务划分：SensorTask、DisplayTask、WatchdogTask、UploadTask、LowpowoerTask
- 队列传递传感器数据，实现任务解耦
- 互斥锁保护OLED共享资源，可以复现多任务访问乱屏现象
- 事件组实现报警状态、采集完成事件同步
- uxTaskGetStackHighWaterMark 栈高水位检测，开启栈溢出检测
- 理解互斥量优先级翻转、优先级继承机制

### 联网模块（待完善）
- ESP8266‑AT 可以正常连接WiFi路由器
- MQTT报文收发未调通，相关代码=编译保留，不删除驱动源码

## 工程说明
- UploadTask任务默认条件编译屏蔽，关闭WiFi反复重连打印
- 栈水位打印为调试代码，正式版本可注释关闭
- WIFI账号密码已条件注释，防止公开仓库泄露密钥


## 已知问题
1. MQTT客户端报文收发异常；受F103C8T6 RAM资源限制，AT应答时序处理复杂，留作后续迭代。
2. 低功耗模式触发printf重定向卡死，暂未启用。

## 学习要点
- FreeRTOS：任务、队列、互斥锁、事件组、任务心跳监控
- 多任务共享资源竞争，互斥锁作用，优先级翻转
- 任务栈余量评估、栈溢出检测
- STM32片内Flash参数掉电存储

