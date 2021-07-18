#include "bsp.h"
#include "sx1278-Hal.h"
#include "sx1278.h"
#include "tx1278-LoRa.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define TXLoRa_FREQENCY                              500000000

//�����ֵ��������RSSI���ź�ǿ�ȣ�
#define TX_RSSI_OFFSET_LF                              -155.0
#define TX_RSSI_OFFSET_HF                              -150.0

#define TX_NOISE_ABSOLUTE_ZERO                         -174.0
#define TX_NOISE_FIGURE_LF                                4.0
#define TX_NOISE_FIGURE_HF                                6.0 

//Ԥ�ȼ�����źŴ��������ڼ���RSSIֵ
const double TXSignalBwLog[] =
{
    3.8927900303521316335038277369285,  // 7.8 kHz
    4.0177301567005500940384239336392,  // 10.4 kHz
    4.193820026016112828717566631653,   // 15.6 kHz
    4.31875866931372901183597627752391, // 20.8 kHz
    4.4948500216800940239313055263775,  // 31.2 kHz
    4.6197891057238405255051280399961,  // 41.6 kHz
    4.795880017344075219145044421102,   // 62.5 kHz
    5.0969100130080564143587833158265,  // 125 kHz
    5.397940008672037609572522210551,   // 250 kHz
    5.6989700043360188047862611052755   // 500 kHz
};

//��Щֵ��Ҫ������
const double TXRssiOffsetLF[] =
{   
    -155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,
};

static const uint8_t TXLoRa_DataRate_Table[][3] =
{
	{ 7, 12, 1 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/5,
	{ 7, 11, 1 },   //0.5kbps,		 	 BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/5, 1��
	{ 7, 10, 1 },   //1.0kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/5,
	{ 7, 9 , 1 },   //1.7kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/5,	3��
	{ 7, 8 , 1 },   //3.1kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/5,
	{ 7, 7 , 1 },   //5.5kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/5,
	{ 7, 12, 2 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/6,
	{ 7, 11, 2 },   //0.5kbps,		 	 BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/6,
	{ 7, 10, 2 },   //0.8kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/6,
	{ 7, 9 , 2 },   //1.5kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/6, 
	{ 7, 8 , 2 },   //2.6kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/6,	10��
	{ 7, 7 , 2 },   //4.5kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/6,
	{ 7, 10, 3 },   //0.7kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/7,
	{ 7, 9 , 3 },   //1.2kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/7,
	{ 7, 8 , 3 },   //2.2kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/7,	
	{ 7, 7 , 3 }    //3.9kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/7,
};

//Ĭ������
tLoRaSettings txLoRaSettings =
{
    TXLoRa_FREQENCY , // Ƶ��
    20,               // ���书��
    7,                // �ź�Ƶ�� [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,           
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    9,                // ��Ƶ����[6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    1,                // ������ [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,            	// CRCЧ�鿪��[0: OFF, 1: ON]
    false,            // ����ͷ����Ϣ���� [0: OFF, 1: ON]
    0,                // ���յ���ģʽ\����ģʽ���� [0: Continuous, 1 Single]
    0,                // ��Ƶģʽ���� [0: OFF, 1: ON]              //��Ƶ����
    4,                // ��Ƶ֮������ڳ���
    100,              // �����ʱ��
    100,              // ������ʱ��
    128,              // ���ݳ��� (������ʽͷģʽ)
};


txSX1278LR* TX1278REG;       //SX1276 LoRa�Ĵ�������
extern uint8_t TX1278Regs[0x70];        //SX1278�Ĵ������� ����
static uint8_t TXLORABuffer[RF_BUFFER_SIZE];  //���������Ƿ��������õ�����Ҳ�ǽ���ʱ�õ�����

uint8_t TXLORAState = TXLORA_STATE_IDLE;  //����ģʽ
/**************************************/
static uint16_t RxPacketSize = 0;     //�������ݳ���
static int8_t RxPacketSnrEstimate;    //������������ȱ���
static double RxPacketRssiValue;      //��������ʱ�ź�ǿ�ȱ���
static uint8_t RxGain = 1;            //�źŷŴ�����
static uint32_t PacketTimeout;        //������ʱ��
static uint16_t TxPacketSize = 0;     //�������ݳ���

//��ʼ��SX1276LoRaģʽ
void TX1278LoRaInit( void )
{
	TXLORAState = TXLORA_STATE_IDLE;                                   //����Ϊ����ģʽ

	TX1278LoRaSetRFFrequency( txLoRaSettings.RFFrequency );            //����Ƶ��
	TX1278LoRaSetSpreadingFactor( txLoRaSettings.SpreadingFactor );    //SF6ֻ����ʽ��ͷģʽ�¹���  Ĭ��7
	TX1278LoRaSetErrorCoding( txLoRaSettings.ErrorCoding );            //������
	TX1278LoRaSetPacketCrcOn( txLoRaSettings.CrcOn );                  //CRCЧ�鿪��
	TX1278LoRaSetSignalBandwidth( txLoRaSettings.SignalBw );           //����    Ĭ��9
	TX1278LoRaSetImplicitHeaderOn( txLoRaSettings.ImplicitHeaderOn );  //����ͷ����Ϣ����  Ĭ�Ϲ�
	TX1278LoRaSetPreambleLength(8);
	
	TX1278LoRaSetSymbTimeout(0x3FF);                                 	 //���ճ�ʱʱ�����1024
	TX1278LoRaSetPayloadLength( txLoRaSettings.PayloadLength );        //�������ݳ���Ϊ128����������ͷ���������ã�
	if(txLoRaSettings.SpreadingFactor < 11)
	{
		TX1278LoRaSetLowDatarateOptimize( false );
	}
	else
		TX1278LoRaSetLowDatarateOptimize( true );                        //���������ճ�ʱʱ��Ϊ2^8+1 16ms

	SX1278Write( REG_LR_OCP, 0x3f ,TX_LORA);													 //���ù�������Ϊ240mA
	TX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 					 //ѡ��PA_BOOST�ܽ�����ź�
	TX1278LoRaSetPa20dBm( true );                           					 //����ʿɴﵽ+20dMb 
	txLoRaSettings.Power = 20;                                 				 //���书��Ϊ20
	TX1278LoRaSetRFPower( txLoRaSettings.Power );              				 //���÷��书��        
	
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );	              				 // ���豸����Ϊ����ģʽ
}

//���ð汾��
void TX1278LoRaSetDefaults( void )
{
   SX1278Read( REG_LR_VERSION, &TX1278REG->RegVersion ,TX_LORA);
}

/*****************************************************************
* ��������������LoRa����ģʽ
* ��ڲ�����RFLR_OPMODE_SLEEP		        		 --	˯��ģʽ
						RFLR_OPMODE_STANDBY		         	 --	����ģʽ
            RFLR_OPMODE_SYNTHESIZER_TX       -- Ƶ�ʺϳɷ���ģʽ
						RFLR_OPMODE_TRANSMITTER	         --	����ģʽ
            RFLR_OPMODE_SYNTHESIZER_RX       -- Ƶ�ʺϳɽ���ģʽ
						RFLR_OPMODE_RECEIVER	        	 --	��������ģʽ
            RFLR_OPMODE_RECEIVER_SINGLE      -- ���ν���ģʽ
            RFLR_OPMODE_CAD                  -- �źŻ���ģʽ
* ����ֵ��	��
*******************************************************************/
void TX1278LoRaSetOpMode( uint8_t opMode )
{
	SX1278Read( REG_LR_OPMODE, &TX1278REG->RegOpMode ,TX_LORA);	
	TX1278REG->RegOpMode = ( TX1278REG->RegOpMode & RFLR_OPMODE_MASK ) | opMode;
	//REG_LR_OPMODE�Ĵ����鿴�����ֲ�111ҳ
	SX1278Write( REG_LR_OPMODE, TX1278REG->RegOpMode ,TX_LORA);   
}

//��ȡ��ǰLORA��ģʽ 
uint8_t TX1278LoRaGetOpMode( void )                       
{
	SX1278Read( REG_LR_OPMODE, &TX1278REG->RegOpMode ,TX_LORA);
	
	return TX1278REG->RegOpMode & ~RFLR_OPMODE_MASK;
}

//��ȡLNA���źŷŴ����棨001=������棬010=�������-6dB��011=�������-12dB..........��
uint8_t TX1278LoRaReadRxGain( void )                    
{
	//REG_LR_LNA�Ĵ����鿴�����ֲ�114ҳ
	SX1278Read( REG_LR_LNA, &TX1278REG->RegLna ,TX_LORA);
	return( TX1278REG->RegLna >> 5 ) & 0x07;
}

//��ȡ�ź�ǿ��
double TX1278LoRaReadRssi( void )
{  
	SX1278Read( REG_LR_RSSIVALUE, &TX1278REG->RegRssiValue ,TX_LORA);

	if( txLoRaSettings.RFFrequency < 860000000 )  
	{
		return TXRssiOffsetLF[txLoRaSettings.SignalBw] + ( double )TX1278REG->RegRssiValue;
	}
	else
	{
//        return RssiOffsetHF[txLoRaSettings.SignalBw] + ( double )TX1278REG->RegRssiValue;
		return 0;
	}
}

//��ȡ����ʱ������ֵ
uint8_t TX1278LoRaGetPacketRxGain( void )
{
  return RxGain;
}

//��ȡ����ʱ�������ֵ���źź������ı�ֵ�������Խ�ߣ�˵���źŸ���ԽС��
int8_t TX1278LoRaGetPacketSnr( void )
{
  return RxPacketSnrEstimate;
}

//��ȡ����ʱ�������ź�ǿ��
double TX1278LoRaGetPacketRssi( void )
{
  return RxPacketRssiValue;
}

//��ʼ����
void TX1278LoRaStartRx( void )
{
  TX1278LoRaSetRFState( TXLORA_STATE_RX_INIT );
}

//��������
void TX1278LoRaGetRxPacket( void *buffer, uint16_t *size )
{
	*size = RxPacketSize;
	RxPacketSize = 0;
	memcpy( (void*)buffer, (void*)TXLORABuffer, (size_t)*size );
}

//��������
void TX1278LoRaSetTxPacket( const void *buffer, uint16_t size )
{
	if( txLoRaSettings.FreqHopOn == false )    //Ĭ���ǹ��˵�
	{
		TxPacketSize = size;
	}
	else
	{
		TxPacketSize = 255;
	}
	memset( TXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );               //���RFBuffer
	memcpy( ( void * )TXLORABuffer, buffer, ( size_t )TxPacketSize );  //RFBuffer�Ǹ�װ�������ݵ����飬��buffer����Ҫ���͵����ݵ�����

	TXLORAState = TXLORA_STATE_TX_INIT;                                //����״̬Ϊ���ͳ�ʼ��   
}

//�õ�RFLRState״̬
uint8_t TX1278LoRaGetRFState( void )
{
    return TXLORAState;
}

//����RFLRState״̬��RFLRState��ֵ����������ĺ���������һ���Ĵ���
void TX1278LoRaSetRFState( uint8_t state )
{
    TXLORAState = state;
}

/********************************************************************
	* ����������	��ȡ���ñ�����ǰ���ӵ��ǰ����ݵ������
	* ��ڲ�����	��
	* ����ֵ��		����ȣ������и�
*********************************************************************/
int8_t TX1278GetLoRaSNR (void)                                             
{
	uint8_t rxSnrEstimate;
	//ȡ���һ�����ݵ������         																	//��¼����ȹ�ֵ
	SX1278Read(REG_LR_PKTSNRVALUE, &rxSnrEstimate ,TX_LORA);					//�����
	//The SNR sign bit is 1																						//�����С��0�����������
	if( rxSnrEstimate & 0x80 )
	{
		RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;		//Invert and divide by 4
		RxPacketSnrEstimate = -RxPacketSnrEstimate;
	}
	else
	{
		//����ȴ���0�����������
		RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;						//Divide by 4
	}
	return RxPacketSnrEstimate;
}

//�õ��ź����棬��LNA
uint8_t TX1278GetLoRaLNA()
{
    return TX1278LoRaReadRxGain();
}

/*******************************************************************************
	* ��������:     ��ȡ���ñ�����ǰ���ӵ��ǰ����ݵ��ź�ǿ��
	* ��ڲ���:     ��
	* ����ֵ:       �ź�ǿ�ȣ������и�
********************************************************************************/
double TX1278GetPackLoRaRSSI(void)                                          
{
	if( TX1278GetLoRaSNR() < 0 )
	{
		/***************************************************************************************
			*���ʣ�P=-174��dBm�� + BW(dB) + NF(dB) + SNR(dB);
			*���źű�������û��������ô˹�ʽ����RSSI��ǰ����������������,���һ���������
		****************************************************************************************/
		//�źű�������û
		RxPacketRssiValue = TX_NOISE_ABSOLUTE_ZERO + 10.0 * TXSignalBwLog[txLoRaSettings.SignalBw] + \
		                    TX_NOISE_FIGURE_LF     + (double)RxPacketSnrEstimate;
	}
	else
	{
		//�ź�ǿ������
		SX1278Read( REG_LR_PKTRSSIVALUE, &TX1278REG->RegPktRssiValue ,TX_LORA);
		RxPacketRssiValue = TXRssiOffsetLF[txLoRaSettings.SignalBw] + (double)TX1278REG->RegPktRssiValue;
	}
	return RxPacketRssiValue;
}

/*********************************************************************************************************
	* ��������:      �������״̬
	* ��ڲ���:      ��
	* ����ֵ:        ��
*********************************************************************************************************/
void TX1278RxStateEnter (void)
{
	//1.---->�������ģʽ
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );		//�����Ĵ���ֻ������Standby,sleep or FSTX ģʽ

	//2.---->���ճ�ʼ������������
	TX1278REG->RegIrqFlagsMask	= 	RFLR_IRQFLAGS_RXTIMEOUT          |	//��ʱ�ж�����
	                              //RFLR_IRQFLAGS_RXDONE             |	//�����ݰ���������ж�
	                              //RFLR_IRQFLAGS_PAYLOADCRCERROR    |	//����CRC�����ж�����
	                                RFLR_IRQFLAGS_VALIDHEADER        |	//Rxģʽ�½��յ�����Ч��ͷ����
	                                RFLR_IRQFLAGS_TXDONE             |	//FIFO���ط�������ж�����
	                                RFLR_IRQFLAGS_CADDONE            |	//CAD����ж�����
	                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |	//FHSS�ı��ŵ��ж�����
	                                RFLR_IRQFLAGS_CADDETECTED;			//��⵽CAD�ж�����
	SX1278Write( REG_LR_IRQFLAGSMASK, TX1278REG->RegIrqFlagsMask ,TX_LORA);		//������Ҫ���ε��ж�(��ע�͵����жϼ���)

	TX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, TX1278REG->RegHopPeriod ,TX_LORA);			//Ƶ������֮��ķ�������:255
	//3.---->���ý���ģʽ(����/����ģʽ)
	if( txLoRaSettings.RxSingleOn == true )
	{
		//����Ϊ���ν���
		TX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
	}
	else
	{
		//����Ϊ��������ģʽ
		TX1278REG->RegFifoAddrPtr = TX1278REG->RegFifoRxBaseAddr;		  				//����:0x00
		SX1278Write( REG_LR_FIFOADDRPTR, TX1278REG->RegFifoAddrPtr ,TX_LORA);	//SPI����FIFO�ĵ�ַ
		TX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER );           								//�������״̬
	}
	memset( TXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //���RFBuffer
	PacketTimeout = txLoRaSettings.RxPacketTimeout;                      		//����������ʱ��Ϊ100
	TXLORAState = TXLORA_STATE_RX_RUNNING;
}

/**************************************************************
	* ����������  ��ȡ���յ�����
	* ��ڲ�����	��
	* ����ֵ��		��
***************************************************************/
void TX1278RxDataRead(void)
{
  SX1278Read( REG_LR_IRQFLAGS, &TX1278REG->RegIrqFlags ,TX_LORA);			//��ȡ�ж�״̬��־λ    
	//���CRC��־������֤���ݰ��������� 	
	if(( TX1278REG->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR)
	{
		SX1278Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR,TX_LORA);	//ִ��д��������������жϱ�־
		if( txLoRaSettings.RxSingleOn == true )                              //���ν���ģʽ
		{
			TXLORAState = TXLORA_STATE_RX_INIT;                                
		}
		else
		{ 
			TXLORAState = TXLORA_STATE_RX_RUNNING;
		}
	 return;
	}  
	if( txLoRaSettings.RxSingleOn == true )                                   //���ν���ģʽ��Ĭ�ϳ���ģʽ
	{
		TX1278REG->RegFifoAddrPtr = TX1278REG->RegFifoRxBaseAddr;       
		SX1278Write( REG_LR_FIFOADDRPTR, TX1278REG->RegFifoAddrPtr ,TX_LORA);          //��ȡ��������
		if( txLoRaSettings.ImplicitHeaderOn == true )                           //Ĭ��false
		{
			RxPacketSize = TX1278REG->RegPayloadLength;
			SX1278ReadFifo( TXLORABuffer, TX1278REG->RegPayloadLength ,TX_LORA);           	//��Lora�ж�ȡ���յ�������
		}
		else
		{
			//REG_LR_NBRXBYTES�Ĵ����鿴���ݼĴ���115ҳ  ���һ�ν��յ������ݰ��ĸ����ֽ���
			SX1278Read( REG_LR_NBRXBYTES, &TX1278REG->RegNbRxBytes ,TX_LORA);
			RxPacketSize = TX1278REG->RegNbRxBytes;
			SX1278ReadFifo( TXLORABuffer, TX1278REG->RegNbRxBytes ,TX_LORA);
		}
	 }
	 else                                                                     //��������ģʽ
	 {
		 //REG_LR_FIFORXCURRENTADDR�Ĵ����鿴���ݼĴ���115ҳ   ���յ����һ�����ݰ�����ʼ��ַ�����ݻ������У�
		 SX1278Read( REG_LR_FIFORXCURRENTADDR, &TX1278REG->RegFifoRxCurrentAddr ,TX_LORA);
		 if( txLoRaSettings.ImplicitHeaderOn == true )
		 {
			 RxPacketSize = TX1278REG->RegPayloadLength;
			 TX1278REG->RegFifoAddrPtr = TX1278REG->RegFifoRxCurrentAddr;
			 SX1278Write( REG_LR_FIFOADDRPTR, TX1278REG->RegFifoAddrPtr ,TX_LORA);
			 SX1278ReadFifo( TXLORABuffer, TX1278REG->RegPayloadLength ,TX_LORA);
		 }
		 else
		 {
			 SX1278Read( REG_LR_NBRXBYTES, &TX1278REG->RegNbRxBytes ,TX_LORA);
			 RxPacketSize = TX1278REG->RegNbRxBytes;
			 TX1278REG->RegFifoAddrPtr = TX1278REG->RegFifoRxCurrentAddr;
			 SX1278Write( REG_LR_FIFOADDRPTR, TX1278REG->RegFifoAddrPtr ,TX_LORA);
			 SX1278ReadFifo( TXLORABuffer, TX1278REG->RegNbRxBytes ,TX_LORA);
		 }
	 }  
	 if(txLoRaSettings.RxSingleOn == true)                                       //���ν���ģʽ
	 { 
		 TXLORAState = TXLORA_STATE_RX_INIT;
	 }
	 else                                                                      //��������ģʽ
	 { 
		 TXLORAState = TXLORA_STATE_RX_RUNNING;
	 }
}	

/**************************************************************
	* ����������  ��������
	* ��ڲ�����	��
	* ����ֵ��		��
***************************************************************/
void TX1278TxData(uint8_t * buff, uint8_t size)
{
	uint32_t cnt = 0;
	uint8_t regValue=0;
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                              //����ģʽ
	
	//REG_LR_IRQFLAGSMASK�Ĵ����������ֲ�115
	TX1278REG->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT          |
									 RFLR_IRQFLAGS_RXDONE             |
									 RFLR_IRQFLAGS_PAYLOADCRCERROR    |
									 RFLR_IRQFLAGS_VALIDHEADER        |
									 //RFLR_IRQFLAGS_TXDONE             |      //������������ж�
									 RFLR_IRQFLAGS_CADDONE            |
									 RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
									 RFLR_IRQFLAGS_CADDETECTED;             
	TX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, TX1278REG->RegHopPeriod ,TX_LORA);                //������Ƶ���ڣ�Ĭ��Ϊ0
	SX1278Write( REG_LR_IRQFLAGSMASK, TX1278REG->RegIrqFlagsMask ,TX_LORA);      

	TX1278REG->RegPayloadLength = size;//TxPacketSize;                              //��ʼ�����ش�С
	SX1278Write( REG_LR_PAYLOADLENGTH, TX1278REG->RegPayloadLength ,TX_LORA);

	TX1278REG->RegFifoTxBaseAddr = 0x00;                                     
	SX1278Write( REG_LR_FIFOTXBASEADDR, TX1278REG->RegFifoTxBaseAddr ,TX_LORA);

	TX1278REG->RegFifoAddrPtr = TX1278REG->RegFifoTxBaseAddr;
	SX1278Write( REG_LR_FIFOADDRPTR, TX1278REG->RegFifoAddrPtr ,TX_LORA);

	//SX1278WriteFifo( TXLORABuffer, TX1278REG->RegPayloadLength ,TX_LORA);                //��Ҫ���͵�����д��LORA��fifo��
	SX1278WriteFifo( buff, TX1278REG->RegPayloadLength ,TX_LORA);

	TX1278LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );			//����ģʽ
//	TX1278RXTX(true);//��ģ�鷢������
	
	while (1)
	{
		SX1278Read(REG_LR_IRQFLAGS,&regValue,TX_LORA);                                  //��ȡ��ʱ��������Щ�ж�  

		if(regValue & RFLR_IRQFLAGS_TXDONE)                                      //�������
		{
			SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE ,TX_LORA);          				//�����־λ
			TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                           //����ģʽ
			TXLORAState = TXLORA_STATE_TX_DONE; 
			break;
		}
		else
		{
			cnt++;
			sleep(1);
			if(cnt > 10)
			{
				printf("ERR__RFLR_IRQFLAGS_TXDONE\n");//TODO:�ȴ�����
				SX1278_DeInit();
				break;
			}			
		}
	}
	

	
}

/******************************************************
	* ����������    TX1278���ʹ�������
	* ��ڲ�����    ��
	* ����ֵ��      ��
*******************************************************/
void TX1278TxProcess(uint8_t *buff, uint32_t size, uint8_t speed, uint32_t freq)
{	
	if(TXLORAState == TXLORA_STATE_TX_IDLE)							//���Ϳ���״̬
	{	
		//printf("\nfreq:%d\n",freq);
		TX1278_SetFreq(freq);						//�л�471����
		TX1278_SetDataRate(speed);		//�л�����
		TX1278TxData(buff, size);
		//TXLORAState = TXLORA_STATE_TX_START;							//���뷢�;���̬
	}
	//TXLORAState = TXLORA_STATE_TX_INIT;
	// if(TXLORAState == TXLORA_STATE_TX_START)
	// {
	// 	TX1278LoRaSetTxPacket(buff,size);																	//��������֡		
	// }
	//if(TXLORAState == TXLORA_STATE_TX_INIT)
	// {
	// 																			//�������ݵ�FIFO
	// }
}

/******************************************************
	* ����������    ���÷��书��
	* ��ڲ�����    pwr: ���ʷ�Χ5 ~ 20
	* ����ֵ��      ��
*******************************************************/
void TX1278TxPower(int8_t pwr)
{
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	
	if((pwr >= 5) &&(pwr <= 20))
	{
		SX1278Write( REG_LR_OCP, 0x3f ,TX_LORA);													//���ù�������Ϊ240mA
		TX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 					//ѡ��PA_BOOST�ܽ�����ź�
		TX1278LoRaSetPa20dBm( true );                           					//����ʿɴﵽ+20dMb 
	}
	if((pwr >= 0) &&(pwr < 5))
	{
		TX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO ); 							//ѡ��RFO�ܽ�����ź�
		TX1278LoRaSetPa20dBm( false );                           					//����ʿɴﵽ+14dMb 
	}
	TX1278LoRaSetRFPower( pwr );              					 //���÷��书��  
}

/******************************************************
	* ����������	�����ز�Ƶ��
	* ��ڲ�����  0-200 Ƶ������ֵ
	* ����ֵ��      ��
*******************************************************/
void TX1278_SetFreq(uint32_t freq)
{
	// float Freq;
	// uint32_t lora_freq;
	//printf("freq:%d\n",(int32_t)freq);
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	
	// Freq = 470 + index * 0.2;
	// Freq = ( Freq >= 470 ) ? Freq : 470;
	// Freq = ( Freq <= 510 ) ? Freq : 510;
	// lora_freq = (uint32_t)((Freq*1e6) + 0.5); //ƫ��0.5hz������������
		
	TX1278LoRaSetRFFrequency(freq);
}	

/******************************************************
	* ����������	���÷��ͽ�����ʱ ������ʱ
	* ��ڲ�����  ���ʵ�λ300bps~5.5kbps
	* ����ֵ��    ��
*******************************************************/
void TX1278_SetDataRate(uint8_t rate)
{
	//printf("rate:%d\n",rate);
	rate = (rate <= 15) ? rate : 15;
	
	TX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	
	TX1278LoRaSetSignalBandwidth( TXLoRa_DataRate_Table[rate][0] );        //����125KHz
	TX1278LoRaSetSpreadingFactor( TXLoRa_DataRate_Table[rate][1] );    		 //��Ƶ����
	TX1278LoRaSetErrorCoding( TXLoRa_DataRate_Table[rate][2] );            //������
	if(TXLoRa_DataRate_Table[rate][1] < 11)
	{	
		TX1278LoRaSetLowDatarateOptimize( false );
	}
	else TX1278LoRaSetLowDatarateOptimize( true );                       //���������ճ�ʱʱ��Ϊ2^8+1 16ms
}	

void set_power(uint8_t num)
{
	uint8_t power;

    num = ( num >= 20 ) ? num : 20;
    num = ( num <= 30 ) ? num : 30;
    if(num == 30)
		power = 20;
    else if(num == 29)
		power = 18;
    else if(num == 28)
		power = 15;
    else if(num == 27)
		power = 13;
    else if(num == 26)
		power = 12;
    else 
		power = num - 15;
    TX1278TxPower(power);
}
