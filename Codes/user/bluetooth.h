#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "String.h"
#include "Stdio.h"
#include "Stdlib.h"

#include "misc.h"

/* function prototype */
void RCC_Configure_bluetooth(void);
void GPIO_Configure_bluetooth(void);
void USART4_Init(void);
void NVIC_Configure_bluetooth(void);
void bluetooth_Init(void); 

int get_day(void); 
int get_hour(void); 
int get_minute(void); 
int get_receive_flag(void); 

void USART1_IRQHandler(void);
void USART4_IRQHandler(void);

void sendDataToUART1(uint16_t data);
void sendDataToUART4(uint16_t data);

void string_partition(void);