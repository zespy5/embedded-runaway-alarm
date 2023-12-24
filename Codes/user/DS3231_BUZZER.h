#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_i2c.h"
#include "lcd.h"
#include "touch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLAG_TIMEOUT ((uint32_t)0x1000)
#define LONG_TIMEOUT ((uint32_t)(10 * FLAG_TIMEOUT))

#define ds3231_addr 0xD0
#define ds3231_sec_addr 0x00		//DS3231 내 초(sec) 레지스터 주소
#define ds3231_min_addr 0x01	//분(min) 레지스터 주소
#define ds3231_hour_addr 0x02	//시간
#define ds3231_day_addr 0x03	//요일
#define ds3231_date_addr 0x04	//날짜
#define ds3231_month_addr 0x05	//달
#define ds3231_year_addr 0x06	//년도

#define ds3231_AM 0		//시간 구조체의 시간 부분을 AM으로 설정할 때 사용
#define ds3231_PM 1		//시간 구조체의 시간 부분을 PM으로 설정할 때 사용
#define ds3231_24_hour 2	//DS3231에 쓸 구조체의 시간 부분을 24시간으로 설정할 때 사용
#define ds3231_day 1	//시간 알람 구조체의 날짜/요일 설정 부분을 요일로 설정할 때 사용
#define ds3231_date 0	//시간 알람 구조체의 날짜/요일 설정 부분을 날짜로 설정할 때 사용
#define ds3231_alarm1_sec_addr 0x07	//DS3231 알람1 sec 레지스터 주소
#define ds3231_alarm1_min_addr 0x08	//DS3231 알람1 min 레지스터 주소
#define ds3231_alarm1_hour_addr 0x09	//DS3231 알람1 hour 레지스터 주소
#define ds3231_alarm1_day_date_addr 0x0A	//DS3231 알람1 day & date 레지스터 주소
#define ds3231_alarm2_addr 0x0B	//DS3231 알람2 레지스터 주소
#define ds3231_ctrl_addr 0x0E	//DS3231 컨트롤 레지스터 주소
#define ds3231_temp_addr 0x11

void Delay_little(void);

void RCC_Configure_ds3231(void);
void GPIO_Configure_ds3231(void);

int decTobcd(uint8_t dec);
int bcdTodec(uint8_t bcd);

void I2C_Configure(void);
uint8_t HW_I2C_Read(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t RegisterAddr, uint16_t NumByteToRead, uint8_t* pBuffer);
uint8_t HW_I2C_Write(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t RegisterAddr, uint16_t NumByteToWrite, uint8_t* pBuffer);

/*
시간 레지스터에서 AM/PM or 24시간 표기 설정에 사용
*/
typedef struct
{
	//12시간 표기를 사용할 경우엔 ds3231_AM or define ds3231_PM, 24시간 표기일 경우엔 ds3231_24_hour 입력
	uint8_t am_pm_24;	//AM or PM or 24시간 표기 사용 설정
	uint8_t hour;	//12, 24시간 표기에 상관없이 입력할 시간(Hour) 설정
}am_pm_24;

/*
DS3231로부터 시간을 읽어오거나 시간 데이터를 쓸 때 사용할 시간 구조체
*/
typedef struct
{
	uint8_t	sec;
	uint8_t min;
	am_pm_24 hour_select;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
}ds3231_time;

/*
알람에서 요일 or 날짜를 쓸건지 구분 지을 구조체
*/
typedef struct
{
	uint8_t day_or_date;	//ds3231_day or ds3231_day 를 입력해 날짜 or 요일 설정
	uint8_t value;	//날짜 or 요일값 설정
}day_date;

/*
알람1을 읽어오거나 전송할 때 사용할 구조체
*/
typedef struct
{
	uint8_t sec;
	uint8_t min;
	am_pm_24 hour_select;
	day_date day_date_select;
}ds3231_Alarm1;

void ds3231_read_time(ds3231_time *current_time); // O
void ds3231_write_time(ds3231_time *ds3231_write_time_struct); // O

void ds3231_set_alarm1(ds3231_Alarm1 *alarm1_data); // O
void ds3231_read_alarm1(ds3231_Alarm1 *current_alarm1); // O

void DS3231_Alarm_Init(); // O
int alarm_check(ds3231_time *current_time, ds3231_Alarm1 *alarm1_data); // O