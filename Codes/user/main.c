#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_tim.h"

#include "lcd.h"
#include "touch.h"

#include <bluetooth.h>
#include <stdio.h>
#include <stdlib.h>
#include <moveWheel.h>
#include <DS3231_BUZZER.h>
#include "stm32f10x_exti.h"


void RCC_Configure(void);
void GPIO_Configure(void);
void is_answer(void);
void setDirection();


int directionFlag = 0;
int onOff = 1;
extern int Alarm_ONOFF;
int answer;




//---------------------------------------------------------------------------------------------------


void RCC_Configure(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    // interrupt
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);  // RCC GPIO D

  TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
}

//------------------------------------------------- blue tooth below------------

uint16_t receive_string[50]; //the buffer for a received words from UART4_IRQHandler
extern uint16_t user_answer[10];
int string_count = 0;
int day = 0;
int hour = 0;
int minute = 0;
int string_receive_offset = 0;
int start_offset = 0;
int answer_correct;
//---------------------------------------------------------------------------------------------------

void RCC_Configure_bluetooth(void)
{
   //UART 4 TX,RX clock enable
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 

   // USART4 clock enable  
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
      
   // Alternate Function IO clock enable
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configure_bluetooth(void)
{   
    GPIO_InitTypeDef GPIO_InitStructure;
   
    //TX2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    //RX2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void USART4_Init(void)
{
  //UART init
   USART_InitTypeDef USART_InitStructure;

   USART_Cmd(UART4, ENABLE);
   
   USART_InitStructure.USART_BaudRate = 9600;
   USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
   USART_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART_InitStructure.USART_Parity = USART_Parity_No;
   USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART_InitStructure.USART_Mode= USART_Mode_Rx| USART_Mode_Tx;
   USART_Init(UART4, &USART_InitStructure);
   
   USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
}

void NVIC_Configure_bluetooth(void) {
    //setting interrupt priority
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
   
    NVIC_EnableIRQ(UART4_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


void UART4_IRQHandler() {
  //uart handler can receive just one character
   uint16_t word;

    if(USART_GetITStatus(UART4,USART_IT_RXNE)!=RESET){
       // the most recent received data by the USART1 peripheral
       word = USART_ReceiveData(UART4);
       
       //while start_offset is 1
       if (start_offset == 1) {     
         // ;(ascii 0x3b) <- The end of string
         if (word == 0x3b) { //SetAlarm 7-19:20 [0x53, 0x65, 0x74, 0x41, 0x6c, 0x61, 0x72, 0x6d, 0x20, 0x37, 0x2d, 0x31, 0x39, 0x3a, 0x32, 0x30]
           
           //we can split
           string_partition();
           //and change the start_offset's value to 0
           start_offset = 0;
         }
         else {
           receive_string[string_count] = word; 
           string_count++;

           // clear 'Read data register not empty' flag
           USART_ClearITPendingBit(UART4,USART_IT_RXNE);
         }
       }
       
       /* @(ascii 0x40) <- if the board gets the first alphabet, the global variable start_offset value is changed to 1
       then handler can receive the words.*/ 
       if (word == 0x40)  start_offset = 1;
       
       
       
       //while start_offset is 2
       if (start_offset ==2){
         // ;(ascii 0x3b) <- The end of string
         if (word == 0x3b){
          // we can check input string is the answer 
           is_answer();
           start_offset = 0;
         }
         else {
           receive_string[string_count] = word;
           string_count++;
           
           USART_ClearITPendingBit(UART4,USART_IT_RXNE);
         }
       }

       //=�� �Է¹޾� ������ �Է� ����
       if (word == 0x3d) start_offset = 2;
    
    }
}

void sendDataToUART4(uint16_t data) {
  while ((UART4->SR & USART_SR_TXE) == 0);
    USART_SendData(UART4, data);
}


void is_answer(void) { //�߰���
  
  char String[10];
  int st = 10-string_count;
  
  for (int i = 0; i < 10; i++) {
    String[i] = 0x30;
  }
  
  for (int i = 0; i < string_count; i++) {
    String[st+i] = (char)receive_string[i];
    receive_string[i] = 0;
  } //�޾Ҵ� �ܾ���� String �迭�� �ű�� ���� �迭�� �� �迭�� �������
  
   st = atoi(String);

  
  if (st == answer){
    string_count = 0;
    sendDataToUART4('O');
    answer_correct = 1;
  }else{
    sendDataToUART4('X');
  }
}

void string_partition(void) {
  char String[50];
  
  for (int i = 0; i < string_count; i++) {
    String[i] = (char)receive_string[i];
    receive_string[i] = 0;
  } //�޾Ҵ� �ܾ���� String �迭�� �ű�� receive_string �迭�� �� �迭�� �������
  
  string_count = 0;
  
  char temp[50], *point;
  char* parti = " -:";
  char* partion_char[10];
  int i = 0;
  
  strcpy(temp, String);
  point = strtok(temp, parti); //parti ���ڰ� ������ �κп��� string �κ� ������
  
  while(point) {
    partion_char[i] = point;
    i++;
    point = strtok(NULL, parti);
  }
  
  if (strcmp(partion_char[0], "SetAlarm") == 0) {
    day = atoi(partion_char[1]);
    hour = atoi(partion_char[2]);
    minute = atoi(partion_char[3]);
    string_receive_offset = 1;
    
    char msg[] = "Alarm setting Complete\n";
                        
    for(int i = 0; i < 23; i++) {
         sendDataToUART4(msg[i]);
    }
  }  
}




void bluetooth_Init(void) {
  RCC_Configure_bluetooth();
  GPIO_Configure_bluetooth();
  USART4_Init();
  NVIC_Configure_bluetooth();
}

int get_day(void) {
  return day;
}

int get_hour(void) {
  return hour;
}

int get_minute(void) {
  string_receive_offset = 0;
  
  return minute;
}

int get_receive_flag(void) {
  return string_receive_offset;
}


//������ ������ ����
void setDirection(int flag){
  switch(flag){
  case 0:
    setDirectionToFront();
    break;
  case 1:
    turnToLeft();
    break;
  case 2:
    setDirectionToBack();
    break;
  case 3:
    turnToRight();
    break;
  case 4:
    turnToLeft();
    break;
  case 5:
    setDirectionToFront();
    break;
  }
}



void delay(int n){

  for (int i = 0; i < n;i++);
};

 int main() {
  
  SystemInit();
  bluetooth_Init();
  RCC_Configure();
  
  RCC_Configure_wheel_pin();
  GPIO_Configure_wheel();
  
  DS3231_Alarm_Init();
  

  LCD_Init();
  LCD_Clear(WHITE);
  
  while(1){setDirectionToBack();}
    
  ds3231_time ds_time_default;   //����ü ���� ����
  ds3231_Alarm1 alarm1_default;
  //DS3231�� ���Ӱ� �Է��� �ð� �����͵��� ���� (������ ������ ���� ���)
  ds_time_default.sec=0;
  ds_time_default.min=5;
  ds_time_default.hour_select.am_pm_24=ds3231_24_hour;
  ds_time_default.hour_select.hour=10;
  ds_time_default.day=6;
  ds_time_default.date=22;
  ds_time_default.month=12;
  ds_time_default.year=23;
  
  ds3231_write_time(&ds_time_default);   //����ü�� �̿��� DS3231�� �ð� ������ �Է�
  ds3231_read_time(&ds_time_default);
  //�ð��� ����ƴ��� Ȯ���ϱ� ���� Ÿ�� �������Ϳ� ����Ǿ��ִ� �ð� ������ �о��

  
  alarm1_default.sec=10;
  alarm1_default.min = 30;
  alarm1_default.hour_select.am_pm_24=ds3231_24_hour;
  alarm1_default.hour_select.hour = 10;
  alarm1_default.day_date_select.value = 22;
  alarm1_default.day_date_select.day_or_date=ds3231_date;
  
  ds3231_set_alarm1(&alarm1_default);
  ds3231_read_alarm1(&alarm1_default);


    

  int sec1 = ds_time_default.sec;
  int min1 = ds_time_default.min;
  int hour1 = ds_time_default.hour_select.hour;
  int hour_am_pm1 = ds_time_default.hour_select.am_pm_24;
  int day1 = ds_time_default.day;
  int date1 = ds_time_default.date;
  int month1 = ds_time_default.month;
  int year1 = ds_time_default.year;
  
  
  int alarm_sec = 0;
  int alarm_min = 0;
  int alarm_hour = 0;
  int alarm_day = 0;
  
  while(1){

    
    //RTC
    ds3231_read_time(&ds_time_default);
    ds3231_read_alarm1(&alarm1_default);
    GPIO_ResetBits(GPIOC, GPIO_Pin_9); //+ �߰��� => 0���� �˶��� ������ �ð��� �Ǵ� 1�� �����
    
    int problem1 = 0;
    int problem2 = 0;
    
    
    if(alarm_check(&ds_time_default, &alarm1_default)){

      int sec_problem = alarm1_default.sec;
      int min_problem = alarm1_default.min;
      int hour_problem = alarm1_default.hour_select.hour;
      
      
      //GPIO_SetBits(GPIOC, GPIO_Pin_9); // ���� �۵�
      GPIO_SetBits(GPIOC, GPIO_Pin_9);
      
      
      LCD_Clear(WHITE);
      
      
      while (1) {
          GPIO_SetBits(GPIOB, GPIO_Pin_12);
          GPIO_ResetBits(GPIOB, GPIO_Pin_13);
          GPIO_SetBits(GPIOA, GPIO_Pin_4);
          GPIO_ResetBits(GPIOA, GPIO_Pin_5);
        
         
         
         //sec%hour, min%hour, X
         problem1 = sec_problem + hour_problem;
         problem2 = min_problem * hour_problem;
         
        answer = problem1 * problem2;
        
        LCD_ShowString(70, 100, "Solve the problem", BLACK, WHITE);
        LCD_ShowNum(90, 150, problem1, 2, BLACK, WHITE);
        LCD_ShowString(110, 150, "X", BLACK, WHITE);
        LCD_ShowNum(130, 150, problem2, 2, BLACK, WHITE);
        LCD_ShowString(150, 150, "=", BLACK, WHITE);
        LCD_ShowString(170, 150, "?", BLACK, WHITE);
        LCD_ShowString(130, 300, "Alarm_on", 0x0000, 0xFFFF);
        

        
        if (answer_correct) { // S1 ��ư ������
          LCD_Clear(WHITE);
          GPIO_ResetBits(GPIOC, GPIO_Pin_9); // ���� ����
          Delay_little();
          LCD_ShowString(30, 300, "Alarm_off", 0x0000, 0xFFFF);
          LCD_Clear(WHITE);
          answer_correct=0;
          stopAllWheel();
          break;
        }
      }
    }
      
    
    
      
      
      if (get_receive_flag() == 1) {
        
      alarm_day = get_day();
      alarm_min = get_minute();
      alarm_hour = get_hour();
      
      alarm1_default.day_date_select.value = get_day();
      alarm1_default.hour_select.hour = get_hour();
      alarm1_default.min = get_minute();
      ds3231_set_alarm1(&alarm1_default);
      ds3231_read_alarm1(&alarm1_default);
    }
    
    // ���� �ð� ��������.
      sec1 = ds_time_default.sec;
      min1 = ds_time_default.min;
      hour1 = ds_time_default.hour_select.hour;
      hour_am_pm1 = ds_time_default.hour_select.am_pm_24;
      day1 = ds_time_default.day;
      date1 = ds_time_default.date;
      month1 = ds_time_default.month;
      year1 = ds_time_default.year;
    
      
      //����ð��� �˶� �����ð� LCD�� ǥ��.
      LCD_ShowNum(180, 60, GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9), 2, RED, WHITE); // C8 bit -> �˶��� on�̸� 1, off�� 0
      
      LCD_ShowNum(50, 100, year1, 2, BLACK, WHITE);
      LCD_ShowString(70, 100, "/", BLACK, WHITE);
      LCD_ShowNum(90, 100, month1, 2, BLACK, WHITE);
      LCD_ShowString(110, 100, "/", BLACK, WHITE);
      LCD_ShowNum(130, 100, date1, 2, BLACK, WHITE);
      LCD_ShowString(150, 100, "/", BLACK, WHITE);
      LCD_ShowNum(170, 100, day1, 1, BLACK, WHITE);
      
      LCD_ShowNum(30, 200, ds_time_default.hour_select.am_pm_24, 1, BLACK, WHITE);
      LCD_ShowNum(60, 200, hour1, 2, BLACK, WHITE);
      LCD_ShowString(90, 200, ":", BLACK, WHITE);
      LCD_ShowNum(120, 200, min1, 2, BLACK, WHITE);
      LCD_ShowString(150, 200, ":", BLACK, WHITE);
      LCD_ShowNum(180, 200, sec1, 2, BLACK, WHITE);
      
      LCD_ShowNum(60, 250, alarm_hour, 2, BLACK, WHITE);
      LCD_ShowString(90, 250, ":", BLACK, WHITE);
      LCD_ShowNum(120, 250, alarm_min, 2, BLACK, WHITE);
      LCD_ShowString(150, 250, ":", BLACK, WHITE);
      LCD_ShowNum(180, 250, alarm_sec, 2, BLACK, WHITE);
      
      
  }
}