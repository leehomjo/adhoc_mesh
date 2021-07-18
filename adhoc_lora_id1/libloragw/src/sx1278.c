#include "bsp.h"
#include "sx1278-Hal.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

uint8_t RX1278Regs[0x70];        //SX1278�Ĵ������� ����
uint8_t TX1278Regs[0x70];        //SX1278�Ĵ������� ����

void TX_LoRa_Init(void)
{
	uint8_t TempReg = 0;
	
	TX1278REG = (txSX1278LR*)TX1278Regs;	//��ʼ��LoRa�Ĵ����ṹ
	
	SX1278_Init_IO();              //SX1278��IO�ڳ�ʼ��         
	SX1278_Reset();                //����SX1278

	SX1278ReadBuffer( REG_LR_OPMODE, TX1278Regs + 1, 0x70 - 1 ,TX_LORA);

	SX1278Read(0x42,&TempReg,TX_LORA);      //���ڲ���spi�Ƿ��ܶ�д���� �汾�Ĵ���
	
	while(TempReg != 0x12)
	{
		printf("Hard SPI1 Err!\r\n");
		//exit(0);
		usleep(100000);
	}

	SX1278_SetLoRaOn(TX_LORA);
	TX1278LoRaInit();       //��ʼ��LoRaģʽ   
}

void SX1278_Init( void )
{
	TX_LoRa_Init();
}

void SX1278_DeInit(void)
{
	SX1278_Reset();
	SX1278_Init();
}

 void SX1278_Reset(void)
 { 
 	SX1278SetReset(RADIO_RESET_ON);   
 	usleep(10000);   //�ȴ�10ms  
 	SX1278SetReset(RADIO_RESET_OFF);	
 	usleep(10000);   //�ȴ�10ms  
 }

//����LoRaģʽ
void SX1278_SetLoRaOn( uint8_t module )
{
	if(module == RX_LORA)
	{
		
	}
	else
	{
		TX1278LoRaSetOpMode( RFLR_OPMODE_SLEEP );   //����˯��ģʽ,�ſ����޸�OPMODE

		/*RegOpMode����Ĵ�����0x01�����λ��������LORA����FSK��1λLORA��0λFSK*/
		TX1278REG->RegOpMode = ( TX1278REG->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
		SX1278Write( REG_LR_OPMODE, TX1278REG->RegOpMode ,TX_LORA);

		TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );  //����Ϊ����ģʽ

		/*REG_LR_DIOMAPPING1��0x40������Ĵ�����������LORAģʽ��DIO0-DIO5���ж�ӳ���ϵ*/
																		// RxDone               RxTimeout                   FhssChangeChannel           CadDone
		TX1278REG->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
																		// CadDetected          ModeReady
		TX1278REG->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
		SX1278WriteBuffer( REG_LR_DIOMAPPING1, &TX1278REG->RegDioMapping1, 2 ,TX_LORA);

		SX1278ReadBuffer( REG_LR_OPMODE, TX1278Regs + 1, 0x70 - 1 ,TX_LORA);  //��REG_LR_OPMODE�Ĵ���ֵ������TX1278Regs[0x70-1]�Ĵ�������
	}
}

//��ʼ����
void SX1278StartRx( uint8_t module )
{
  if(module == RX_LORA)
	{
		
	}
	else
	{
		TX1278LoRaSetRFState( TXLORA_STATE_RX_INIT );    //LoRa �жϽ���״̬
	}
}

/******************************************************
	* ����������	Ƶ���л�������ֵ
	* ��ڲ�����  Ƶ�� ��λMHz
	* ����ֵ��    ����ֵ
*******************************************************/
uint8_t LoRaFreqIndex(float Freq)
{
	uint8_t index;
	Freq = ( Freq >= 470 ) ? Freq : 470;
	Freq = ( Freq <= 510 ) ? Freq : 510;
	index = (Freq - 470) * 5;
	
	return index;
}

uint8_t SelfAddJudge(uint32_t current_value,uint32_t before_vlaue,uint32_t threshold,uint32_t *timeout)
{
	uint8_t result = 0;
	uint32_t temp_val = 0;

	*timeout = 0;

	if(current_value >= before_vlaue)
	{
		temp_val = current_value - before_vlaue;
		if(temp_val > threshold)
		{
			*timeout = temp_val;
			result = 1;
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		temp_val = 0xffffffff - before_vlaue + current_value;
		if(temp_val > threshold)
		{
			*timeout = temp_val;
			result = 1;
		}
		else
		{
			result = 0;
		}
	}
	return result;
}
