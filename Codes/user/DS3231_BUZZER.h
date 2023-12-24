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
#define ds3231_sec_addr 0x00		//DS3231 �� ��(sec) �������� �ּ�
#define ds3231_min_addr 0x01	//��(min) �������� �ּ�
#define ds3231_hour_addr 0x02	//�ð�
#define ds3231_day_addr 0x03	//����
#define ds3231_date_addr 0x04	//��¥
#define ds3231_month_addr 0x05	//��
#define ds3231_year_addr 0x06	//�⵵

#define ds3231_AM 0		//�ð� ����ü�� �ð� �κ��� AM���� ������ �� ���
#define ds3231_PM 1		//�ð� ����ü�� �ð� �κ��� PM���� ������ �� ���
#define ds3231_24_hour 2	//DS3231�� �� ����ü�� �ð� �κ��� 24�ð����� ������ �� ���
#define ds3231_day 1	//�ð� �˶� ����ü�� ��¥/���� ���� �κ��� ���Ϸ� ������ �� ���
#define ds3231_date 0	//�ð� �˶� ����ü�� ��¥/���� ���� �κ��� ��¥�� ������ �� ���
#define ds3231_alarm1_sec_addr 0x07	//DS3231 �˶�1 sec �������� �ּ�
#define ds3231_alarm1_min_addr 0x08	//DS3231 �˶�1 min �������� �ּ�
#define ds3231_alarm1_hour_addr 0x09	//DS3231 �˶�1 hour �������� �ּ�
#define ds3231_alarm1_day_date_addr 0x0A	//DS3231 �˶�1 day & date �������� �ּ�
#define ds3231_alarm2_addr 0x0B	//DS3231 �˶�2 �������� �ּ�
#define ds3231_ctrl_addr 0x0E	//DS3231 ��Ʈ�� �������� �ּ�
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
�ð� �������Ϳ��� AM/PM or 24�ð� ǥ�� ������ ���
*/
typedef struct
{
	//12�ð� ǥ�⸦ ����� ��쿣 ds3231_AM or define ds3231_PM, 24�ð� ǥ���� ��쿣 ds3231_24_hour �Է�
	uint8_t am_pm_24;	//AM or PM or 24�ð� ǥ�� ��� ����
	uint8_t hour;	//12, 24�ð� ǥ�⿡ ������� �Է��� �ð�(Hour) ����
}am_pm_24;

/*
DS3231�κ��� �ð��� �о���ų� �ð� �����͸� �� �� ����� �ð� ����ü
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
�˶����� ���� or ��¥�� ������ ���� ���� ����ü
*/
typedef struct
{
	uint8_t day_or_date;	//ds3231_day or ds3231_day �� �Է��� ��¥ or ���� ����
	uint8_t value;	//��¥ or ���ϰ� ����
}day_date;

/*
�˶�1�� �о���ų� ������ �� ����� ����ü
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