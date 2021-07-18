#ifndef __SX1278_H__
#define __SX1278_H__

#include "stdint.h"
#include "stdbool.h"

void SX1278_Init( void );										 //��ʼ��SX1278

void SX1278_DeInit(void);

void SX1278_Reset(void);									 //����SX1278

void SX1278_SetLoRaOn(uint8_t module); 						 //����LoRa���ƽ������FSK���ƽ����

void SX1278StartRx( uint8_t module );                                   //��ʼ����

uint8_t LoRaFreqIndex(float Freq);				//Ƶ������ֵ�л�

uint8_t SelfAddJudge(uint32_t current_value,uint32_t before_vlaue,uint32_t threshold,uint32_t *timeout); //��ʱ�Լ�����ж�

#endif 
