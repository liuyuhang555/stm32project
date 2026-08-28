#include "stm32f10x.h"                  // Device header

void led_Init(void)
{   
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);  //使能GPIOC时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
	// PA0 PA1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
    GPIO_SetBits(GPIOA,GPIO_Pin_0 |GPIO_Pin_1 );

    // PC13 推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
    GPIO_SetBits(GPIOC,GPIO_Pin_13);  //默认高电平，板载LED熄灭
}

void led1_on(void)
{

GPIO_ResetBits(GPIOA,GPIO_Pin_0);

}

void led1_off(void)
{
  GPIO_SetBits(GPIOA,GPIO_Pin_0);
}

void led2_on(void)
{

GPIO_ResetBits(GPIOA,GPIO_Pin_1);

}

void led2_off(void)
{
  GPIO_SetBits(GPIOA,GPIO_Pin_1);
}


void led1_turn(void)             //led1翻转
{
  if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_0) == 0)
	{
	  GPIO_SetBits(GPIOA,GPIO_Pin_0);
	}
else 
{
  GPIO_ResetBits(GPIOA,GPIO_Pin_0);

}

}
void led2_turn(void)             //led2翻转
{
  if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1) == 0)
	{
	  GPIO_SetBits(GPIOA,GPIO_Pin_1);
	}
else 
{
  GPIO_ResetBits(GPIOA,GPIO_Pin_1);

}

}
