#include <DS3231_BUZZER.h>

int Alarm_flag = 1;
int Alarm_ONOFF = 0;


// ���� ��ȯ �Լ�
/* USER CODE BEGIN PFP */
int decTobcd(uint8_t dec)
{
	return ((dec/10)*16)+(dec%10);
}
int bcdTodec(uint8_t bcd)
{
	return (bcd/16*10)+(bcd%16);
}

void I2C_Configure(void)
  {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        
        AFIO->MAPR = 0x00000002;
        GPIOB->CRH |= 0x000000FF;
        
        I2C_DeInit(I2C1);
        
        I2C1->CR1 = 0x0000;
        I2C1->CR2 = 0x0008;
        I2C1->CCR = 40;
        I2C1->TRISE = 0x0009;
        
        I2C_Cmd(I2C1, DISABLE);
        I2C_Cmd(I2C1, ENABLE);
}

uint8_t HW_I2C_Read(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t RegisterAddr, uint16_t NumByteToRead, uint8_t* pBuffer)
{
  __IO uint32_t UTIL_Timeout = LONG_TIMEOUT;
  __IO uint32_t temp;

  (void)(temp);

restart:

  UTIL_Timeout = LONG_TIMEOUT;
/* Send START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  /* Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if (UTIL_Timeout-- == 0)
      return ERROR;
  }

  UTIL_Timeout = LONG_TIMEOUT;
  /* Send slave address for read */
  I2C_Send7bitAddress(I2Cx, DeviceAddr, I2C_Direction_Transmitter);

  while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if (UTIL_Timeout-- == 0)
    {
      I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY|I2C_FLAG_AF);
      goto restart;
    }
  }
  /* Clear EV6 by setting again the PE bit */
  I2C_Cmd(I2Cx, ENABLE);

  I2C_SendData(I2Cx, RegisterAddr);

  /* Test on EV8 and clear it */
  UTIL_Timeout = LONG_TIMEOUT;
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if (UTIL_Timeout-- == 0)
     return ERROR;
  }

  if (NumByteToRead == 0x01)
  {
    restart3:
    /* Send START condition */
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
    /* Send Slave address for read */
    I2C_Send7bitAddress(I2Cx, DeviceAddr, I2C_Direction_Receiver);
    /* Wait until ADDR is set */
    UTIL_Timeout = LONG_TIMEOUT;
    while (!I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR))
    {
      if (UTIL_Timeout-- == 0)
      {
        I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY|I2C_FLAG_AF);
        goto restart3;
      }
    }
    /* Clear ACK */
    I2C_AcknowledgeConfig(I2Cx, DISABLE);
    __disable_irq();
    /* Clear ADDR flag */
    temp = I2Cx->SR2;
    /* Program the STOP */
    I2C_GenerateSTOP(I2Cx, ENABLE);
    __enable_irq();
    while ((I2C_GetLastEvent(I2Cx) & 0x0040) != 0x000040); /* Poll on RxNE */
    /* Read the data */
    *pBuffer = I2C_ReceiveData(I2Cx);
    /* Make sure that the STOP bit is cleared by Hardware before CR1 write access */
    while ((I2Cx->CR1&0x200) == 0x200);
    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(I2Cx, ENABLE);

    return SUCCESS;
  }
  else
    if(NumByteToRead == 0x02)
    {
      restart4:
      /* Send START condition */
      I2C_GenerateSTART(I2Cx, ENABLE);
      while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
      /* Send EEPROM address for read */
      I2C_Send7bitAddress(I2Cx, DeviceAddr, I2C_Direction_Receiver);
      I2Cx->CR1 = 0xC01; /* ACK=1; POS =1 */
      UTIL_Timeout = LONG_TIMEOUT;
      while (!I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR))
      {
        if (UTIL_Timeout-- == 0)
        {
          I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY|I2C_FLAG_AF);
          goto restart4;
        }
      }
      __disable_irq();
      /* Clear ADDR */
      temp = I2Cx->SR2;
      /* Disable ACK */
      I2C_AcknowledgeConfig(I2Cx, DISABLE);
      __enable_irq();
      while ((I2C_GetLastEvent(I2Cx) & 0x0004) != 0x00004); /* Poll on BTF */

       __disable_irq();
      /* Program the STOP */
      I2C_GenerateSTOP(I2Cx, ENABLE);
      /* Read first data */
      *pBuffer = I2Cx->DR;
      pBuffer++;
      /* Read second data */
      *pBuffer = I2Cx->DR;
      __enable_irq();
      I2Cx->CR1 = 0x0401; /* POS = 0, ACK = 1, PE = 1 */

      return SUCCESS;
    }
  else
  {
restart2:
    UTIL_Timeout = LONG_TIMEOUT;
    /* Send START condition */
    I2C_GenerateSTART(I2Cx, ENABLE);
    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
    {
      if (UTIL_Timeout-- == 0) return ERROR;
    }
    UTIL_Timeout = LONG_TIMEOUT;
    /* Send slave address for read */
    I2C_Send7bitAddress(I2Cx,  DeviceAddr, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {

      if (UTIL_Timeout-- == 0)
      {
        I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY|I2C_FLAG_AF);
        goto restart2;
      }
    }

    /* While there is data to be read; here the safe procedure is implemented */
    while (NumByteToRead)
    {

      if (NumByteToRead != 3) /* Receive bytes from first byte until byte N-3 */
      {
        while ((I2C_GetLastEvent(I2Cx) & 0x00004) != 0x000004); /* Poll on BTF */
        /* Read data */
        *pBuffer = I2C_ReceiveData(I2Cx);
        pBuffer++;
        /* Decrement the read bytes counter */
        NumByteToRead--;
      }

      if (NumByteToRead == 3)  /* it remains to read three data: data N-2, data N-1, Data N */
      {

        /* Data N-2 in DR and data N -1 in shift register */
        while ((I2C_GetLastEvent(I2Cx) & 0x000004) != 0x0000004); /* Poll on BTF */
        /* Clear ACK */
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        __disable_irq();
        /* Read Data N-2 */
        *pBuffer = I2C_ReceiveData(I2Cx);
        pBuffer++;
        /* Program the STOP */
        I2C_GenerateSTOP(I2Cx, ENABLE);
        /* Read DataN-1 */
        *pBuffer = I2C_ReceiveData(I2Cx);
        __enable_irq();
        pBuffer++;
        while ((I2C_GetLastEvent(I2Cx) & 0x00000040) != 0x0000040); /* Poll on RxNE */
        /* Read DataN */
        *pBuffer = I2Cx->DR;
        /* Reset the number of bytes to be read by master */
        NumByteToRead = 0;
      }
    }
    /* Make sure that the STOP bit is cleared by Hardware before CR1 write access */
    while ((I2Cx->CR1&0x200) == 0x200);
    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(I2Cx, ENABLE);

    return SUCCESS;
  }
}

uint8_t HW_I2C_Write(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t RegisterAddr,
                               uint16_t NumByteToWrite,
                               uint8_t* pBuffer)
{
  __IO uint32_t UTIL_Timeout = LONG_TIMEOUT;

 restart1:
  UTIL_Timeout = LONG_TIMEOUT;
  /* Send START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  /* Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if (UTIL_Timeout-- == 0) return ERROR;
  }
  /* Send slave address for write */
  I2C_Send7bitAddress(I2Cx, DeviceAddr, I2C_Direction_Transmitter);

  UTIL_Timeout = LONG_TIMEOUT;
  /* Test on EV6 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
   {

    if (UTIL_Timeout-- == 0)
    {
      I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY|I2C_FLAG_AF);
      goto restart1;
    }
  }
  
  UTIL_Timeout = LONG_TIMEOUT;

  /* Transmit the first address for r/w operations */
  I2C_SendData(I2Cx, RegisterAddr);

  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if (UTIL_Timeout-- == 0)
      return ERROR;
  }
  if (NumByteToWrite == 0x01)
  {
    UTIL_Timeout = LONG_TIMEOUT;
    /* Prepare the register value to be sent */
    I2C_SendData(I2Cx, *pBuffer);

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
      if (UTIL_Timeout-- == 0)
        return ERROR;
    }

    /* End the configuration sequence */
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return SUCCESS;
  }
  I2C_SendData(I2Cx, *pBuffer);
  pBuffer++;
  NumByteToWrite--;
  /* While there is data to be written */
  while (NumByteToWrite--)
  {
    while ((I2C_GetLastEvent(I2Cx) & 0x04) != 0x04);  /* Poll on BTF */
    /* Send the current byte */
    I2C_SendData(I2Cx, *pBuffer);
    /* Point to the next byte to be written */
    pBuffer++;

  }
  UTIL_Timeout = LONG_TIMEOUT;
  /* Test on EV8_2 and clear it, BTF = TxE = 1, DR and shift registers are
   empty */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if (UTIL_Timeout-- == 0) return ERROR;
  }
  /* Send STOP condition */
  I2C_GenerateSTOP(I2Cx, ENABLE);
  return SUCCESS;
}

/*
DS3231�κ��� �о� �� �ð� �����͸� ����ϴ� �Լ�
�ð� ������ ����� ��(�ð� ����ü->hour_select->am_pm_24)�� �Ǵ���
AM, PM, 24�ð� ǥ��� ������ �� ���
*/
void ds3231_print_time(ds3231_time *current_time)
{
        char time[20] = "";
        char sprin_result[5] = "";
        
	printf("%d-%d-%d",current_time->year,current_time->month
			,current_time->date);
        
        sprintf(sprin_result, "%d", current_time->year);
        strcat(time, sprin_result);
        strcat(time, "-");
        sprintf(sprin_result, "%d", current_time->month);
        strcat(time, sprin_result);
        strcat(time, "-");
        sprintf(sprin_result, "%d", current_time->date);
        strcat(time, sprin_result);
        strcat(time, " ");
        
	switch (current_time->hour_select.am_pm_24)
	{
		//12�ð� ǥ���� ��� 0~4�� ��Ʈ���� ��ȿ�� �ð� ������, 5~6�� ��Ʈ�� �ð� ǥ�� ���� ��Ʈ
		case ds3231_AM :
                  printf("AM : %d:",(current_time->hour_select.hour)&0x1F);	
                  strcat(time, "AM : ");
                  sprintf(sprin_result, "%d", (current_time->hour_select.hour)&0x1F);
                  strcat(time, sprin_result);
                  strcat(time, ":");
			break;
		case ds3231_PM :
                  printf("PM : %d:",(current_time->hour_select.hour)&0x1F);
                  strcat(time, "PM : ");
                  sprintf(sprin_result, "%d", (current_time->hour_select.hour)&0x1F);
                  strcat(time, sprin_result);
                  strcat(time, ":");
			break;
		case ds3231_24_hour :
                  printf("24 : %d:",current_time->hour_select.hour);
                  strcat(time, "24 : ");
                  sprintf(sprin_result, "%d", current_time->hour_select.hour);
                  strcat(time, sprin_result);
                  strcat(time, ":");
			break;
		default :
			break;
	}
        
	printf("%d:%d\r\n",current_time->min,current_time->sec);
        
        sprintf(sprin_result, "%d", current_time->min);
        strcat(time, sprin_result);
        strcat(time, ":");
        sprintf(sprin_result, "%d", current_time->sec);
        strcat(time, sprin_result);
        
        printf("String : %s\n", time);
        
        LCD_ShowString(25, 100, time, BLACK, WHITE);

}

/*
���޹��� �ð� ����ü �ּҿ� DS3231�κ��� �о� �� �ð� �����͸� �����ϴ� �Լ�
*/
void ds3231_read_time(ds3231_time *current_time)
{
	uint8_t ds3231_read_time_buff[7];	
	//DS3231�κ��� �о� �� �����͸� ������ ������ ���� (��,��,�ð�,����,��¥,��,�⵵-�� 7��)

	/*
	Ư�� �޸� �ּҿ� �׼����ؼ� ���ϴ� ������ŭ �����͸� �о�ͼ� ������ ���ۿ� ����
	*/
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_sec_addr, 1, &ds3231_read_time_buff[0]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_min_addr, 1, &ds3231_read_time_buff[1]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_hour_addr, 1, &ds3231_read_time_buff[2]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_day_addr, 1, &ds3231_read_time_buff[3]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_date_addr, 1, &ds3231_read_time_buff[4]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_month_addr, 1, &ds3231_read_time_buff[5]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_year_addr, 1, &ds3231_read_time_buff[6]);
	//����� I2C, DS3231 �ּ�, ������ DS3231 �������� �ּ�, �޸� �� ũ��, �о� �� �����͸� ������ ����,
	//�о� �� ������ ����
    
		//���� �Ϸ� ������ ���
        
	/*
	DS3231�κ��� �о� �� �����͸� 10������ ��ȯ�� �� ���޹��� ����ü�� �Է�
	*/
	current_time->sec=bcdTodec(ds3231_read_time_buff[0]);
	current_time->min=bcdTodec(ds3231_read_time_buff[1]);

	/*
	�ð� ��������(0x02)�κ��� �о� �� �������� 6�� ��Ʈ(0x40), 5�� ��Ʈ(0x20)�� Ȯ���� 
	High(1)�� ���������� ��� ����ü ������ AM, PM ������ �Ѵ� (24�ð� ǥ���� ��� 6�� ��Ʈ Low(0))
	*/
	if((ds3231_read_time_buff[2]&0x40)!=0)	//BIT6!=0 �̹Ƿ� 12�ð� ǥ��
	{
		if((ds3231_read_time_buff[2]&0x20)==0)	//BIT5==0 �̹Ƿ� AM
		{
			current_time->hour_select.am_pm_24=ds3231_AM;
		}
		else	//BIT5!=0 �̹Ƿ� PM
		{
			current_time->hour_select.am_pm_24=ds3231_PM;
		}
	}
	else
	{
		//24�ð� ǥ�� ������ ��� (6�� ��Ʈ 0, 5�� ��Ʈ 2x �ð��� ���� 1)
	}
        
	current_time->hour_select.hour=bcdTodec(ds3231_read_time_buff[2]);
        
	current_time->day=bcdTodec(ds3231_read_time_buff[3]);
	current_time->date=bcdTodec(ds3231_read_time_buff[4]);
	current_time->month=bcdTodec(ds3231_read_time_buff[5]);
	current_time->year=bcdTodec(ds3231_read_time_buff[6]);

}

/*
���޹��� ����ü �ּҸ� ���� DS3231�� ������ �ð� ������ ���۸� ä��
*/
void ds3231_write_time(ds3231_time *ds3231_write_time_struct)
{
	uint8_t write_buf[7];	//���ۿ� ����� ������ ���� �迭 ����
	
	//�Է��� 10���� �ð� �����͸� 2����ȭ
	write_buf[0]=decTobcd(ds3231_write_time_struct->sec);
	write_buf[1]=decTobcd(ds3231_write_time_struct->min);
	write_buf[2]=decTobcd(ds3231_write_time_struct->hour_select.hour);
	write_buf[3]=decTobcd(ds3231_write_time_struct->day);
	write_buf[4]=decTobcd(ds3231_write_time_struct->date);
	write_buf[5]=decTobcd(ds3231_write_time_struct->month);
	write_buf[6]=decTobcd(ds3231_write_time_struct->year);

	/*
	24�ð� ǥ�Ⱑ �ƴ� 12�ð� ǥ��(AM/PM)�� ����ϴ� ��쿣 
	DS3231 �ð�(Hour) ��������(0x02)�� ������ �����Ϳ� �߰��� 6, 5�� ��Ʈ�� ��������� �Ѵ�
	*/
	switch(ds3231_write_time_struct->hour_select.am_pm_24)
	{
		case ds3231_AM :	//AM�� ��� 6�� ��Ʈ(12/24)�� High�� �������ָ� �ȴ�
			write_buf[2]|=0x40;
			break;
		case ds3231_PM :	//PM�� ��� 6, 5�� ��Ʈ�� High�� ��������� �Ѵ�
			write_buf[2]|=0x60;
			break;
		case ds3231_24_hour :
			break;
		default :
			break;
	}

	/*
	DS3231 ��(Sec) ��������(0x00)���� 7���� 8��Ʈ ������ �迭�� �Է�
	�� Ÿ�� �������ʹ� 8��Ʈ�� ũ�⸦ �����Ƿ� 0x00���� 0x06���� ���������� 8��Ʈ ������ 7���� �Էµȴ�
	*/
        
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_sec_addr, 1, &write_buf[0]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_min_addr, 1, &write_buf[1]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_hour_addr, 1, &write_buf[2]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_day_addr, 1, &write_buf[3]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_date_addr, 1, &write_buf[4]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_month_addr, 1, &write_buf[5]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_year_addr, 1, &write_buf[6]);
        
		//�Է� �Ϸ���� ���
}

void ds3231_set_alarm1(ds3231_Alarm1 *alarm1_data)
{
	uint8_t alarm1_buff[4];	//DS3231�� ������ �˶�1 ������ ����

	alarm1_buff[0]=decTobcd(alarm1_data->sec);	//��
	alarm1_buff[1]=decTobcd(alarm1_data->min);	//��
	alarm1_buff[2]=decTobcd(alarm1_data->hour_select.hour);	//�ð�
	alarm1_buff[3]=decTobcd(alarm1_data->day_date_select.value);	//��¥ or ���ϰ�

	/*
	12/24 �ð� �� AM, PM ����
	*/
	switch(alarm1_data->hour_select.am_pm_24)
	{
		case ds3231_AM :	//���޵� ����ü�� �˶� �ð��� AM�� ���
			alarm1_buff[2]|=0x40;	//BIT6 Logic High=12�ð� ǥ��, BIT5 Logic Low=AM
			break;
		case ds3231_PM :	//���޵� ����ü�� �˶� �ð��� PM�� ���
			alarm1_buff[2]|=0x60;	//BIT6 Logic High=12�ð� ǥ��, BIT5 Logic High=PM
			break;
		case ds3231_24_hour :	//���޵� ����ü�� �˶� �ð��� 24�ð� ǥ���� ��� (���� ���� �ʿ� X)
			break;
		default :
			break;
	}

	/*
	�˶� ������ ���Ϸ� ���� �ƴϸ� ��¥�� ������ ����
	*/
	switch(alarm1_data->day_date_select.day_or_date)
	{
		case ds3231_date :	//��¥ ����, BIT6 - Low
			break;
		case ds3231_day :
			alarm1_buff[3]|=0x40;	//���� ����, BIT6 - High
			break;
		default :
			break;
	}
        
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_alarm1_sec_addr, 1, &alarm1_buff[0]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_alarm1_min_addr, 1, &alarm1_buff[1]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_alarm1_hour_addr, 1, &alarm1_buff[2]);
	HW_I2C_Write(I2C1, ds3231_addr, ds3231_alarm1_day_date_addr, 1, &alarm1_buff[3]);
        
        Alarm_flag = 0;
}

void ds3231_read_alarm1(ds3231_Alarm1 *current_alarm1)
{
	uint8_t read_alarm1_buff[4];	//DS3231�κ��� �о� �� �˶�1 �ð��� ������ ������ ����
        
        
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_alarm1_sec_addr, 1, &read_alarm1_buff[0]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_alarm1_min_addr, 1, &read_alarm1_buff[1]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_alarm1_hour_addr, 1, &read_alarm1_buff[2]);
	HW_I2C_Read(I2C1, ds3231_addr,ds3231_alarm1_day_date_addr, 1, &read_alarm1_buff[3]);
    
	/*
	������ �����͸� 10������ ��ȯ �� ���޹��� ����ü�� �Է�
	*/
	current_alarm1->sec=bcdTodec(read_alarm1_buff[0]);
	current_alarm1->min=bcdTodec(read_alarm1_buff[1]);

	if((read_alarm1_buff[2]&0x40)!=0)	//12�ð� ǥ�⸦ ����� ���
	{
		if((read_alarm1_buff[2]&0x20)==0)	//�˶� ���� �ð��� AM�� ���
		{
			current_alarm1->hour_select.am_pm_24=ds3231_AM;
			current_alarm1->hour_select.hour=bcdTodec(read_alarm1_buff[2]&0x1F);
			//12�ð� ǥ�⸦ ����� ���, �ð� �������Ϳ��� ��ȿ�� �ð� �����ʹ� BIT 0 ~ BIT 4 ����
		}
		else	//�˶� ���� �ð��� PM�� ���
		{
			current_alarm1->hour_select.am_pm_24=ds3231_PM;
			current_alarm1->hour_select.hour=bcdTodec(read_alarm1_buff[2]&0x1F);
		}
	}
	else	//24�ð� ǥ��
	{
		current_alarm1->hour_select.hour=bcdTodec(read_alarm1_buff[2]);	
	}

	if((read_alarm1_buff[3]&0x40)!=0)	//Day�� ��� (BIT 6 - High)
	{
		current_alarm1->day_date_select.day_or_date=ds3231_day;
		current_alarm1->day_date_select.value=bcdTodec(read_alarm1_buff[3]&0x0F);
		//Day ����� ���, ��ȿ�� ��¥ �����ʹ� BIT 0 ~ BIT 3����
	}
	else	//Date�� ��� (BIT 6 - Low)
	{
		current_alarm1->day_date_select.day_or_date=ds3231_date;
		current_alarm1->day_date_select.value=bcdTodec(read_alarm1_buff[3]&0x3F);
		//Date ����� ���, ��ȿ�� ��¥ �����ʹ� BIT 0 ~ BIT 5����
	}

	//ds3231_print_alarm1(current_alarm1);	//�о� �� �˶�1 �����͸� ����ϱ� ���� �Լ� ȣ��
}

void Delay_little(void) {
   int i;

   for (i = 0; i < 200; i++) {}
}

void RCC_Configure_ds3231(void)
{
  /* 1-1. TIM Clock Enable */
  //RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  /* 1-2. enable for PB7 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);     // RCC GPIO B
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    // interrupt
  /* 2. Enable for button -> use PD11 for s1 user */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);     // RCC GPIO D
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);     // RCC GPIO D
}

void GPIO_Configure_ds3231(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   
  /* init for button PD11(s1 user) *///�ٲ� D11->C4
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &GPIO_InitStructure); 
  
  /* Configure the PB7 or BUZZER pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void DS3231_Alarm_Init() {
  SystemInit();
  RCC_Configure_ds3231();
  GPIO_Configure_ds3231();

  I2C_Configure();
  LCD_Init();
  LCD_Clear(WHITE);
}



int alarm_check(ds3231_time *current_time, ds3231_Alarm1 *alarm1_data) {
  
  
  if (current_time->date >= alarm1_data->day_date_select.value) {
    if (current_time->hour_select.am_pm_24 == alarm1_data->hour_select.am_pm_24) {
      if (current_time->hour_select.hour >= alarm1_data->hour_select.hour) {
        if (current_time->min >= alarm1_data->min) {
          if (current_time->sec >= alarm1_data->sec) {
            if (Alarm_flag == 0) { // �˶� �۵���
              Alarm_flag = 1;
              Alarm_ONOFF = 1;
            }
            else {
              Alarm_ONOFF = 0;
            }
          }
        }
      }
    }
  }
  return Alarm_ONOFF;
}


