#ifndef __SX1278_HAL_H__
#define __SX1278_HAL_H__
#include "stdint.h"

#define RX_LORA				0
#define TX_LORA				1

//��ƵоƬ�շ��л���1��TX��0��RX�����ú궨����Ӽ���˴��묩
//#define TX1278RXTX( txEnable )                            TX1278WriteRxTx( txEnable )

typedef enum
{
	RADIO_RESET_OFF,                             
	RADIO_RESET_ON,                              
}tRadioResetState;


void SX1278_Init_IO(void);                      //��ʼ��IO�ӿ�

void SX1278SetReset( uint8_t state);             //����loraģ�����λ�͸�λ

void SX1278Write( uint8_t addr, uint8_t data,uint8_t module );   //��ָ���Ĵ�����ַ������д������

void SX1278Read( uint8_t addr, uint8_t *data,uint8_t module );   //��ȡ�Ĵ���ָ����ַ������

void SX1278WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size,uint8_t module );  //���Ĵ����ĵ�ַ������д�뼸������

void SX1278ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size,uint8_t module );   //�ڼĴ����ĵ�ַ��������ȡ��������

void SX1278WriteFifo( uint8_t *buffer, uint8_t size,uint8_t module );     //������д��SX1278��fifo��

void SX1278ReadFifo( uint8_t *buffer, uint8_t size,uint8_t module );      //��SX1278��fifo�ж�ȡ����

//void RX1278WriteRxTx( uint8_t txEnable );  				//��ƵоƬ�շ��л���1��TX��0��RX��

//void TX1278WriteRxTx( uint8_t txEnable );  				//��ƵоƬ�շ��л���1��TX��0��RX��

//void Set_RF_Switch_TX(uint8_t module);            //����Ƶ����оƬ�л��ɽ���״̬

#endif 
