#include "bsp.h"

#define RXLoRa_FREQENCY                             500000000

//�����ֵ��������RSSI���ź�ǿ�ȣ�
#define RX_RSSI_OFFSET_LF                              -155.0
#define RX_RSSI_OFFSET_HF                              -150.0

#define RX_NOISE_ABSOLUTE_ZERO                         -174.0
#define RX_NOISE_FIGURE_LF                                4.0
#define RX_NOISE_FIGURE_HF                                6.0 

//Ԥ�ȼ�����źŴ������ڼ���RSSIֵ
const double RXSignalBwLog[] =
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
const double RXRssiOffsetLF[] =
{   
    -155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,
};

static const uint8_t RXLoRa_DataRate_Table[][3] =
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
tLoRaSettings rxLoRaSettings =
{
    RXLoRa_FREQENCY , // Ƶ��
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


rxSX1278LR* RX1278REG;       //RX1278 LoRa�Ĵ�������
extern uint8_t RX1278Regs[0x70];        //SX1278�Ĵ������� ����
static uint8_t RXLORABuffer[RF_BUFFER_SIZE];  //���������Ƿ��������õ�����Ҳ�ǽ���ʱ�õ�����

uint8_t RXLORAState = RXLORA_STATE_IDLE;  //����ģʽ

/*********** ����֡��ʱ��� ***********/
volatile uint32_t MgtTimeout = 0;
volatile uint32_t BusiTimeout = 0;
extern LoRaManagementPara lora_mgt;
extern uint32_t TimeoutCount;		//��ʱʱ��
//ҵ��֡��ʱ��¼
static uint32_t TimeoutTick10S = 0;
static uint32_t TimeoutTick20S = 0;
static uint32_t TimeoutTick30S = 0;
static uint32_t TimeoutTick40S = 0;
/**************************************/

static uint16_t RxPacketSize = 0;     //�������ݳ���
static int8_t RxPacketSnrEstimate;    //������������ȱ���
static double RxPacketRssiValue;      //��������ʱ�ź�ǿ�ȱ���
static uint8_t RxGain = 1;            //�źŷŴ�����
static uint32_t RxTimeoutTimer = 0;   //����ʱ��
static uint32_t TimeoutTimer = 0;   	//����ʱ��
static uint32_t PacketTimeout;        //������ʱ��
static uint16_t TxPacketSize = 0;     //�������ݳ���

//��ʼ��SX1278LoRaģʽ
void RX1278LoRaInit( void )
{
	RXLORAState = RXLORA_STATE_IDLE;                                     //����Ϊ����ģʽ

	RX1278LoRaSetRFFrequency( rxLoRaSettings.RFFrequency );            //����Ƶ��
	RX1278LoRaSetSpreadingFactor( rxLoRaSettings.SpreadingFactor );    //SF6ֻ����ʽ��ͷģʽ�¹���  Ĭ��7
	RX1278LoRaSetErrorCoding( rxLoRaSettings.ErrorCoding );            //������
	RX1278LoRaSetPacketCrcOn( rxLoRaSettings.CrcOn );                  //CRCЧ�鿪��
	RX1278LoRaSetSignalBandwidth( rxLoRaSettings.SignalBw );           //����    Ĭ��9
	RX1278LoRaSetImplicitHeaderOn( rxLoRaSettings.ImplicitHeaderOn );  //����ͷ����Ϣ����  Ĭ�Ϲ�
	RX1278LoRaSetPreambleLength(8);
	
	RX1278LoRaSetSymbTimeout(0x3FF);                                 //���ճ�ʱʱ�����1024
	RX1278LoRaSetPayloadLength( rxLoRaSettings.PayloadLength );        //�������ݳ���Ϊ128����������ͷ���������ã�
	if(rxLoRaSettings.SpreadingFactor < 11)
	{
		RX1278LoRaSetLowDatarateOptimize( false );
	}
	else
		RX1278LoRaSetLowDatarateOptimize( true );                        //���������ճ�ʱʱ��Ϊ2^8+1 16ms

	SX1278Write( REG_LR_OCP, 0x3f ,RX_LORA);																	 //���ù�������Ϊ240mA
	RX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 					 //ѡ��PA_BOOST�ܽ�����ź�
	RX1278LoRaSetPa20dBm( true );                           					 //����ʿɴﵽ+20dMb 
	rxLoRaSettings.Power = 20;                                 					 //���书��Ϊ20
	RX1278LoRaSetRFPower( rxLoRaSettings.Power );              					 //���÷��书��        
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );	              				 // ���豸����Ϊ����ģʽ
}

//���ð汾��
void RX1278LoRaSetDefaults( void )
{
   SX1278Read( REG_LR_VERSION, &RX1278REG->RegVersion ,RX_LORA);
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
void RX1278LoRaSetOpMode( uint8_t opMode )
{
	SX1278Read( REG_LR_OPMODE, &RX1278REG->RegOpMode ,RX_LORA);
	RX1278REG->RegOpMode = ( RX1278REG->RegOpMode & RFLR_OPMODE_MASK ) | opMode;
	//REG_LR_OPMODE�Ĵ����鿴�����ֲ�111ҳ
	SX1278Write( REG_LR_OPMODE, RX1278REG->RegOpMode ,RX_LORA);        
}

//��ȡ��ǰLORA��ģʽ 
uint8_t RX1278LoRaGetOpMode( void )                       
{
	SX1278Read( REG_LR_OPMODE, &RX1278REG->RegOpMode ,RX_LORA);	
	return RX1278REG->RegOpMode & ~RFLR_OPMODE_MASK;
}

//��ȡLNA���źŷŴ����棨001=������棬010=�������-6dB��011=�������-12dB..........��
uint8_t RX1278LoRaReadRxGain( void )                    
{
	//REG_LR_LNA�Ĵ����鿴�����ֲ�114ҳ
	SX1278Read( REG_LR_LNA, &RX1278REG->RegLna ,RX_LORA);
	return( RX1278REG->RegLna >> 5 ) & 0x07;
}

//��ȡ�ź�ǿ��
double RX1278LoRaReadRssi( void )
{  
	SX1278Read( REG_LR_RSSIVALUE, &RX1278REG->RegRssiValue ,RX_LORA);

	if( rxLoRaSettings.RFFrequency < 860000000 )  
	{
		return RXRssiOffsetLF[rxLoRaSettings.SignalBw] + ( double )RX1278REG->RegRssiValue;
	}
	else
	{
//    return RssiOffsetHF[rxLoRaSettings.SignalBw] + ( double )RX1278REG->RegRssiValue;
		return 0;
	}
}

//��ȡ����ʱ������ֵ
uint8_t RX1278LoRaGetPacketRxGain( void )
{
  return RxGain;
}

//��ȡ����ʱ�������ֵ���źź������ı�ֵ�������Խ�ߣ�˵���źŸ���ԽС��
int8_t RX1278LoRaGetPacketSnr( void )
{
  return RxPacketSnrEstimate;
}

//��ȡ����ʱ�������ź�ǿ��
double RX1278LoRaGetPacketRssi( void )
{
  return RxPacketRssiValue;
}

//��ʼ����
void RX1278LoRaStartRx( void )
{
  RX1278LoRaSetRFState( RXLORA_STATE_RX_INIT );
}

//��������
void RX1278LoRaGetRxPacket( void *buffer, uint16_t *size )
{
	*size = RxPacketSize;
	RxPacketSize = 0;
	memcpy( (void*)buffer, (void*)RXLORABuffer, (size_t)*size );
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //���RFBuffer
}

//��������
void RX1278LoRaSetTxPacket( const void *buffer, uint16_t size )
{
	if( rxLoRaSettings.FreqHopOn == false )    //Ĭ���ǹ��˵�
	{
		TxPacketSize = size;
	}
	else
	{
		TxPacketSize = 255;
	}
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //���RFBuffer
	memcpy( ( void * )RXLORABuffer, buffer, ( size_t )TxPacketSize );  //RFBuffer�Ǹ�װ�������ݵ����飬��buffer����Ҫ���͵����ݵ�����

	RXLORAState = RXLORA_STATE_TX_INIT;                                //����״̬Ϊ���ͳ�ʼ��   
}

//�õ�RFLRState״̬
uint8_t RX1278LoRaGetRFState( void )
{
  return RXLORAState;
}

//����RFLRState״̬��RFLRState��ֵ����������ĺ���������һ���Ĵ���
void RX1278LoRaSetRFState( uint8_t state )
{
  RXLORAState = state;
}

/********************************************************************
	* ����������	��ȡ���ñ�����ǰ���ӵ��ǰ����ݵ������
	* ��ڲ�����	��
	* ����ֵ��		����ȣ������и�
*********************************************************************/
int8_t RX1278GetLoRaSNR (void)                                                //512��526�����ⲿ�ִ���
{
	u8 rxSnrEstimate;
	//ȡ���һ�����ݵ������         									//��¼����ȹ�ֵ
	SX1278Read(REG_LR_PKTSNRVALUE, &rxSnrEstimate ,RX_LORA);					//�����
	//The SNR sign bit is 1												//�����С��0�����������
	if( rxSnrEstimate & 0x80 )
	{
		RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;	//Invert and divide by 4
		RxPacketSnrEstimate = -RxPacketSnrEstimate;
	}
	else
	{
		//����ȴ���0�����������
		RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;			//Divide by 4
	}
	return RxPacketSnrEstimate;
}

//�õ��ź����棬��LNA
uint8_t RX1278GetLoRaLNA()
{
  return RX1278LoRaReadRxGain();
}

/*******************************************************************************
	* ��������:     ��ȡ���ñ�����ǰ���ӵ��ǰ����ݵ��ź�ǿ��
	* ��ڲ���:     ��
	* ����ֵ:       �ź�ǿ�ȣ������и�
********************************************************************************/
double RX1278GetPackLoRaRSSI(void)                                             //527�е�551��
{
	if( RX1278GetLoRaSNR() < 0 )
	{
		/***************************************************************************************
			*���ʣ�P=-174��dBm�� + BW(dB) + NF(dB) + SNR(dB);
			*���źű�������û��������ô˹�ʽ����RSSI��ǰ����������������,���һ���������
		****************************************************************************************/
		//�źű�������û
		RxPacketRssiValue = RX_NOISE_ABSOLUTE_ZERO + 10.0 * RXSignalBwLog[rxLoRaSettings.SignalBw] + \
		                    RX_NOISE_FIGURE_LF     + (double)RxPacketSnrEstimate;
	}
	else
	{
		//�ź�ǿ������
		SX1278Read( REG_LR_PKTRSSIVALUE, &RX1278REG->RegPktRssiValue ,RX_LORA);
		RxPacketRssiValue = RXRssiOffsetLF[rxLoRaSettings.SignalBw] + (double)RX1278REG->RegPktRssiValue;
	}
	return RxPacketRssiValue;
}

/*********************************************************************************************************
	* ��������:      �������״̬
	* ��ڲ���:      ��
	* ����ֵ:        ��
*********************************************************************************************************/
void RX1278RxStateEnter (void)
{
	//1.---->�������ģʽ
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );		//�����Ĵ���ֻ������Standby,sleep or FSTX ģʽ

	//2.---->���ճ�ʼ������������
	RX1278REG->RegIrqFlagsMask	= 	RFLR_IRQFLAGS_RXTIMEOUT          |	//��ʱ�ж�����
	                              //RFLR_IRQFLAGS_RXDONE             |	//�����ݰ���������ж�
	                              //RFLR_IRQFLAGS_PAYLOADCRCERROR    |	//����CRC�����ж�����
	                                RFLR_IRQFLAGS_VALIDHEADER        |	//Rxģʽ�½��յ�����Ч��ͷ����
	                                RFLR_IRQFLAGS_TXDONE             |	//FIFO���ط�������ж�����
	                                RFLR_IRQFLAGS_CADDONE            |	//CAD����ж�����
	                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |	//FHSS�ı��ŵ��ж�����
	                                RFLR_IRQFLAGS_CADDETECTED;			//��⵽CAD�ж�����
	SX1278Write( REG_LR_IRQFLAGSMASK, RX1278REG->RegIrqFlagsMask ,RX_LORA);		//������Ҫ���ε��ж�(��ע�͵����жϼ���)

	RX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, RX1278REG->RegHopPeriod ,RX_LORA);			//Ƶ������֮��ķ�������:255
	//3.---->���ý���ģʽ(����/����ģʽ)
	if( rxLoRaSettings.RxSingleOn == true )
	{
		//����Ϊ���ν���
		RX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
	}
	else
	{
		//����Ϊ��������ģʽ
		RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxBaseAddr;		  	//����:0x00
		SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);	//SPI����FIFO�ĵ�ַ
		RX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER );           				//�������״̬
		RX1278RXTX(false);
	}
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //���RFBuffer
	PacketTimeout = rxLoRaSettings.RxPacketTimeout;                       //����������ʱ��Ϊ100
	RxTimeoutTimer = GET_TICK_COUNT( );
	RXLORAState = RXLORA_STATE_RX_RUNNING;
}

/**************************************************************
	* ����������  ��ȡ����״̬
	* ��ڲ�����	��
	* ����ֵ��		��
***************************************************************/
void RX1278RxStatusRead(void)
{
	uint8_t regValue = 0;
	uint32_t time = 0;
	SX1278Read(REG_LR_IRQFLAGS,&regValue,RX_LORA);                                  //��ȡ��ʱ��������Щ�ж�  
																																					
	if(regValue & RFLR_IRQFLAGS_RXDONE)                                      //�������
	{                                                  											                   
//		printf("lora_receive_success!!\r\n");
		SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE,RX_LORA);							//ִ��д��������������жϱ�־
		RXLORAState = RXLORA_STATE_RX_DONE;
		if(lora_mgt.frame_type_flag)
		{
//			lora_mgt.first_rec_flag = false;							//��һ�ν�������ʱ���˱�־λ��λ��תΪҵ��֡��λ
			BusiTimeout = GET_TICK_COUNT();
			MgtTimeout = 0;
		}
		else 
		{
//			lora_mgt.first_rec_flag = true;
			TimeoutCount = 0;	//ÿ�ν��յ����ݺ�ʱ��������
			lora_mgt.rx_timeout_flag = false;
			BusiTimeout = 0;
			MgtTimeout = GET_TICK_COUNT();	
		}
	}	 
	else
	{
//		if(!lora_mgt.frame_type_flag && lora_mgt.first_rec_flag) 	//�ӽ��յ����ݺ�ʼ���㳬ʱ
//		{
//			if(SelfAddJudge(GET_TICK_COUNT(),MgtTimeout,LORA_TIMEOUT,&lora_mgt.configed_timeout))     //�����Ƿ�ʱ
//			{
//				lora_mgt.rx_timeout_flag = true;
//			}
//		}
		if(lora_mgt.frame_type_flag)
		{
			if(SelfAddJudge(GET_TICK_COUNT(),BusiTimeout,5000,&time))
			{
				if(((time >= 10000) && (time <20000) &&(time % 10000) == 0))
				{
					TimeoutTick10S++;
					printf("��ʱ10s���� %d\r\n",TimeoutTick10S);
					printf("��ʱʱ�� %d\r\n",time);
				}
				if(((time >= 20000) && (time <30000) &&(time % 20000) == 0))
				{
					TimeoutTick20S++;
					printf("��ʱ20s���� %d\r\n",TimeoutTick20S);
					printf("��ʱʱ�� %d\r\n",time);
				}
				if(((time >= 30000) && (time <40000) &&(time % 300000) == 0))
				{
					TimeoutTick30S++;
					printf("��ʱ30s���� %d\r\n",TimeoutTick30S);
					printf("��ʱʱ�� %d\r\n",time);
				}
				if((time >= 40000) && ((time % 400000) == 0))
				{
					TimeoutTick40S++;
					printf("��ʱ40s���� %d\r\n",TimeoutTick40S);
					printf("��ʱʱ�� %d\r\n",time);
				}
			}
		}
		else
		{
			if(SelfAddJudge(GET_TICK_COUNT(),MgtTimeout,LORA_TIMEOUT,&lora_mgt.configed_timeout))     //�����Ƿ�ʱ
			{
				lora_mgt.rx_timeout_flag = true;
			}
		}
	}
	if( rxLoRaSettings.RxSingleOn == true )                                 //���ν���ģʽ��Ĭ�ϳ�������ģʽ
	{
		if(SelfAddJudge(GET_TICK_COUNT(),RxTimeoutTimer,PacketTimeout,&TimeoutTimer))     //�����Ƿ�ʱ
		{
			RXLORAState = RXLORA_STATE_RX_INIT;                           		 //���ճ�ʱ 
		}
	}  
}	

/**************************************************************
	* ����������  ��ȡ���յ�����
	* ��ڲ�����	��
	* ����ֵ��		��
***************************************************************/
void RX1278RxDataRead(void)
{
  SX1278Read( REG_LR_IRQFLAGS, &RX1278REG->RegIrqFlags ,RX_LORA);			//��ȡ�ж�״̬��־λ    
	//���CRC��־������֤���ݰ��������� 	
	if(( RX1278REG->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR)
	{
		SX1278Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR,RX_LORA);	//ִ��д��������������жϱ�־
		if( rxLoRaSettings.RxSingleOn == true )                              //���ν���ģʽ
		{
			RXLORAState = RXLORA_STATE_RX_INIT;                                
		}
		else
		{ 
			RXLORAState = RXLORA_STATE_RX_RUNNING;
		}
	 return;
	}  
	if( rxLoRaSettings.RxSingleOn == true )                                   //���ν���ģʽ��Ĭ�ϳ���ģʽ
	{
		RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxBaseAddr;       
		SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);          //��ȡ��������
		if( rxLoRaSettings.ImplicitHeaderOn == true )                           //Ĭ��false
		{
			RxPacketSize = RX1278REG->RegPayloadLength;
			SX1278ReadFifo( RXLORABuffer, RX1278REG->RegPayloadLength ,RX_LORA);           	//��Lora�ж�ȡ���յ�������
		}
		else
		{
			//REG_LR_NBRXBYTES�Ĵ����鿴���ݼĴ���115ҳ  ���һ�ν��յ������ݰ��ĸ����ֽ���
			SX1278Read( REG_LR_NBRXBYTES, &RX1278REG->RegNbRxBytes ,RX_LORA);
			RxPacketSize = RX1278REG->RegNbRxBytes;
			SX1278ReadFifo( RXLORABuffer, RX1278REG->RegNbRxBytes ,RX_LORA);
		}
	 }
	 else                                                                     //��������ģʽ
	 {
		 //REG_LR_FIFORXCURRENTADDR�Ĵ����鿴���ݼĴ���115ҳ   ���յ����һ�����ݰ�����ʼ��ַ�����ݻ������У�
		 SX1278Read( REG_LR_FIFORXCURRENTADDR, &RX1278REG->RegFifoRxCurrentAddr ,RX_LORA);
		 if( rxLoRaSettings.ImplicitHeaderOn == true )
		 {
			 RxPacketSize = RX1278REG->RegPayloadLength;
			 RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxCurrentAddr;
			 SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);
			 SX1278ReadFifo( RXLORABuffer, RX1278REG->RegPayloadLength ,RX_LORA);
		 }
		 else
		 {
			 SX1278Read( REG_LR_NBRXBYTES, &RX1278REG->RegNbRxBytes ,RX_LORA);
			 RxPacketSize = RX1278REG->RegNbRxBytes;
			 RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxCurrentAddr;
			 SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);
			 SX1278ReadFifo( RXLORABuffer, RX1278REG->RegNbRxBytes ,RX_LORA);
		 }
	 }  
	 if(rxLoRaSettings.RxSingleOn == true)                                       //���ν���ģʽ
	 { 
		 RXLORAState = RXLORA_STATE_RX_INIT;
	 }
	 else                                                                      //��������ģʽ
	 { 
		 RXLORAState = RXLORA_STATE_RX_RUNNING;
	 }
}	

/**************************************************************
	* ����������  ��������
	* ��ڲ�����	��
	* ����ֵ��		��
***************************************************************/
void RX1278TxData(void)
{
	uint8_t regValue=0;
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                              //����ģʽ
	
	//REG_LR_IRQFLAGSMASK�Ĵ����������ֲ�115
	RX1278REG->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT          |
									 RFLR_IRQFLAGS_RXDONE             |
									 RFLR_IRQFLAGS_PAYLOADCRCERROR    |
									 RFLR_IRQFLAGS_VALIDHEADER        |
									 //RFLR_IRQFLAGS_TXDONE             |      //������������ж�
									 RFLR_IRQFLAGS_CADDONE            |
									 RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
									 RFLR_IRQFLAGS_CADDETECTED;             
	RX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, RX1278REG->RegHopPeriod ,RX_LORA);                //������Ƶ���ڣ�Ĭ��Ϊ0
	SX1278Write( REG_LR_IRQFLAGSMASK, RX1278REG->RegIrqFlagsMask ,RX_LORA);      

	RX1278REG->RegPayloadLength = TxPacketSize;                              //��ʼ�����ش�С
	SX1278Write( REG_LR_PAYLOADLENGTH, RX1278REG->RegPayloadLength ,RX_LORA);

	RX1278REG->RegFifoTxBaseAddr = 0x00;                                     
	SX1278Write( REG_LR_FIFOTXBASEADDR, RX1278REG->RegFifoTxBaseAddr ,RX_LORA);

	RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoTxBaseAddr;
	SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);

	SX1278WriteFifo( RXLORABuffer, RX1278REG->RegPayloadLength ,RX_LORA);                //��Ҫ���͵�����д��LORA��fifo��
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );			//����ģʽ
	
	SX1278Read(REG_LR_IRQFLAGS,&regValue,RX_LORA);                                  //��ȡ��ʱ��������Щ�ж�  
																																					
	if(regValue & RFLR_IRQFLAGS_TXDONE)                                      //�������
	{
		printf("rxlora_send_success!!\r\n");
		SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE ,RX_LORA);          				//�����־λ
		RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                           //����ģʽ
		RXLORAState = RXLORA_STATE_TX_DONE;  
	}
}

/******************************************************
	* ����������    RX1278���ͽ��մ�����
	* ��ڲ�����    ��
	* ����ֵ��      ��
*******************************************************/
void RX1278TxRxProcess(void)
{
	//����
	if(RXLORAState == RXLORA_STATE_RX_INIT)RX1278RxStateEnter();
	if(RXLORAState == RXLORA_STATE_RX_RUNNING)RX1278RxStatusRead();
	if(RXLORAState == RXLORA_STATE_RX_DONE)
	{
		RX1278RxDataRead();
		RXLORAReciveData();
	}
	
	if(RXLORAState == RXLORA_STATE_RX_IDLE)						//���տ���״̬
	{	
		vTaskDelay(lora_mgt.tx_delay_time);								//�������ת��������ʱ
		RX1278_SetFreq(lora_mgt.tx_freq_index);						//�л�471����
		RX1278_SetDataRate(lora_mgt.tx_datarate_index);		//�л�����
		RXLORAState = RXLORA_STATE_TX_START;									//���뷢�;���̬
	}
	
	//����
	if(RXLORAState == RXLORA_STATE_TX_START)RXLORASendData();
	if(RXLORAState == RXLORA_STATE_TX_INIT)RX1278TxData();
	if(RXLORAState == RXLORA_STATE_TX_DONE)
	{
		vTaskDelay(lora_mgt.rx_delay_time);								//�������ת��������ʱ
		RX1278_SetFreq(lora_mgt.rx_freq_index);						//�л�����
		RX1278_SetDataRate(lora_mgt.rx_datarate_index);		//�л�����
		RXLORAState = RXLORA_STATE_RX_INIT;	
	}	
}

/******************************************************
	* ����������    RX1278���մ�����
	* ��ڲ�����    ��
	* ����ֵ��      ��
*******************************************************/
void RX1278RxProcess(void)
{
	//����
	if(RXLORAState == RXLORA_STATE_RX_INIT)
	{
		RX1278RxStateEnter();															//���ճ�ʼ��
	}
	if(RXLORAState == RXLORA_STATE_RX_RUNNING)
	{
		RX1278RxStatusRead();															//һֱ��ȡ������ɱ�־λֱ�����յ���֡����	
	}
	if(RXLORAState == RXLORA_STATE_RX_DONE)							//������ɶ�ȡFIFO����������
	{
		RX1278RxDataRead();
		RXLORAReciveData();
		RXLORAState = RXLORA_STATE_RX_IDLE;
	}	
	if(RXLORAState == RXLORA_STATE_RX_IDLE)							//���տ���״̬�����ڸ��Ľ���Ƶ�������
	{
//		vTaskDelay(lora_mgt.rx_delay_time);								//�������ת��������ʱ
		RX1278_SetFreq(lora_mgt.rx_freq_index);						//�л�����
		RX1278_SetDataRate(lora_mgt.rx_datarate_index);		//�л�����
		RXLORAState = RXLORA_STATE_RX_INIT;	
	}	
}

/******************************************************
	* ����������    ���÷��书��
	* ��ڲ�����    pwr: ���ʷ�Χ5 ~ 20
	* ����ֵ��      ��
*******************************************************/
void RX1278TxPower (uint8_t pwr)
{
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	//Ĭ�ϲ���PA���ڵ���Ϊ100mA,�����20dBmʱ��Ҫ120mA,���Ա�������0x0b����
	SX1278Write( REG_LR_OCP, 0x3f ,RX_LORA);
	RX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
	RX1278LoRaSetPa20dBm( true );
	RX1278LoRaSetRFPower( pwr);
}

/******************************************************
	* ����������	�����ز�Ƶ��
	* ��ڲ�����  0-200 Ƶ������ֵ
	* ����ֵ��      ��
*******************************************************/
void RX1278_SetFreq(uint8_t index)
{
	float Freq;
	uint32_t lora_freq;
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	
	Freq = 470 + index * 0.2;
	Freq = ( Freq >= 470 ) ? Freq : 470;
	Freq = ( Freq <= 510 ) ? Freq : 510;
	lora_freq = (uint32_t)((Freq*1e6) + 0.5); //ƫ��0.5hz������������
		
	RX1278LoRaSetRFFrequency( lora_freq );
}	

/******************************************************
	* ����������	���ÿ�������
	* ��ڲ�����  ���ʵ�λ300bps~5.5kbps
	* ����ֵ��    ��
*******************************************************/
void RX1278_SetDataRate(uint8_t rate)
{
	rate = (rate <= 15) ? rate : 15;
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//�������ģʽ
	
	RX1278LoRaSetSignalBandwidth( RXLoRa_DataRate_Table[rate][0] );        //����125KHz
	RX1278LoRaSetSpreadingFactor( RXLoRa_DataRate_Table[rate][1] );    		 //��Ƶ����
	RX1278LoRaSetErrorCoding( RXLoRa_DataRate_Table[rate][2] );            //������
	if(RXLoRa_DataRate_Table[rate][1] < 11)
	{	
		RX1278LoRaSetLowDatarateOptimize( false );
	}
	else RX1278LoRaSetLowDatarateOptimize( true );                       //���������ճ�ʱʱ��Ϊ2^8+1 16ms
}	

uint16_t RX1278_SetTxRxDelay_SF(uint8_t bw,uint8_t sf,uint8_t cr,uint8_t size)
{
	uint16_t tr_delay = 0;
	uint16_t offset = 0;
	if(size <= 30)
	{
		if((bw == 7) && (sf == 12) && (cr == 1))tr_delay = 1800;				//	7, 12, 1    0.3kbps~1.7s
		else if((bw == 7) && (sf == 11) && (cr == 1))tr_delay = 1200;		//	7, 11, 1    0.5kbps~1s
		else if((bw == 7) && (sf == 10) && (cr == 1))tr_delay = 600;		//	7, 10, 1    1.0kbps~0.5s
		else if((bw == 7) && (sf == 9) && (cr == 1))tr_delay = 350;			//	7, 9 , 1    1.7kbps~0.3s
		else if((bw == 7) && (sf == 8) && (cr == 1))tr_delay = 250;			//	7, 8 , 1    3.1kbps~0.2s
		else if((bw == 7) && (sf == 7) && (cr == 1))tr_delay = 150;			//	7, 7 , 1    5.5kbps~0.1s
		else if((bw == 7) && (sf == 12) && (cr == 2))tr_delay = 2000;		//	7, 12, 2    0.3kbps~1.9s
		else if((bw == 7) && (sf == 11) && (cr == 2))tr_delay = 1200;		//	7, 11, 2    0.5kbps~1.1s
		else if((bw == 7) && (sf == 10) && (cr == 2))tr_delay = 600;		//	7, 10, 2    0.8kbps~0.5s
		else if((bw == 7) && (sf == 9) && (cr == 2))tr_delay = 350;			//	7, 9 , 2    1.5kbps~0.3s
		else if((bw == 7) && (sf == 8) && (cr == 2))tr_delay = 250;			//	7, 8 , 2    2.6kbps~0.2s
		else if((bw == 7) && (sf == 7) && (cr == 2))tr_delay = 150;			//	7, 7 , 2    4.5kbps~0.1s
		else if((bw == 7) && (sf == 10) && (cr == 3))tr_delay = 700;		//	7, 10, 3    0.7kbps~0.6s
		else if((bw == 7) && (sf == 9) && (cr == 3))tr_delay = 350;			//	7, 9 , 3    1.2kbps~0.3s
		else if((bw == 7) && (sf == 8) && (cr == 3))tr_delay = 250;			//	7, 8 , 3    2.2kbps~0.2s
		else if((bw == 7) && (sf == 7) && (cr == 3))tr_delay = 150;			//	7, 7 , 3    3.9kbps~0.1s
		else tr_delay = 2000;
	}
	else
	{
		size = size - 30;
		if((bw == 7) && (sf == 12) && (cr == 1))tr_delay = 60;				//	7, 12, 1    0.3kbps~1.7s
		else if((bw == 7) && (sf == 11) && (cr == 1))
		{
			tr_delay = 30;		//	7, 11, 1    0.5kbps~1s
			offset = 900;
		}
		else if((bw == 7) && (sf == 10) && (cr == 1))tr_delay = 20;		//	7, 10, 1    1.0kbps~0.5s
		else if((bw == 7) && (sf == 9) && (cr == 1))
		{
			tr_delay = 15;			//	7, 9 , 1    1.7kbps~0.3s
			offset = 350;
		}
		else if((bw == 7) && (sf == 8) && (cr == 1))tr_delay = 8;			//	7, 8 , 1    3.1kbps~0.2s
		else if((bw == 7) && (sf == 7) && (cr == 1))tr_delay = 5;			//	7, 7 , 1    5.5kbps~0.1s
		else if((bw == 7) && (sf == 12) && (cr == 2))tr_delay = 65;		//	7, 12, 2    0.3kbps~1.9s
		else if((bw == 7) && (sf == 11) && (cr == 2))tr_delay = 40;		//	7, 11, 2    0.5kbps~1.1s
		else if((bw == 7) && (sf == 10) && (cr == 2))tr_delay = 20;		//	7, 10, 2    0.8kbps~0.5s
		else if((bw == 7) && (sf == 9) && (cr == 2))tr_delay = 11;			//	7, 9 , 2    1.5kbps~0.3s
		else if((bw == 7) && (sf == 8) && (cr == 2))
		{
			tr_delay = 6;			//	7, 8 , 2    2.6kbps~0.2s
			offset = 200;
		}
		else if((bw == 7) && (sf == 7) && (cr == 2))tr_delay = 5;			//	7, 7 , 2    4.5kbps~0.1s
		else if((bw == 7) && (sf == 10) && (cr == 3))tr_delay = 23;		//	7, 10, 3    0.7kbps~0.6s
		else if((bw == 7) && (sf == 9) && (cr == 3))tr_delay = 11;			//	7, 9 , 3    1.2kbps~0.3s
		else if((bw == 7) && (sf == 8) && (cr == 3))tr_delay = 8;			//	7, 8 , 3    2.2kbps~0.2s
		else if((bw == 7) && (sf == 7) && (cr == 3))tr_delay = 5;			//	7, 7 , 3    3.9kbps~0.1s
		else tr_delay = 65;
		
		tr_delay = tr_delay * size + offset;
	}	
	return tr_delay;
}


/******************************************************
	* ����������	���÷��ͽ�����ʱ ������ʱ
	* ��ڲ�����  ���ʵ�λ300bps~5.5kbps
	* ����ֵ��    ��ʱʱ��
*******************************************************/
uint16_t RX1278_SetTxRxDelay(uint8_t rate,uint8_t size)
{
	uint16_t tr_delay = 0;
	
	rate = (rate <= 15) ? rate : 15;
	tr_delay = RX1278_SetTxRxDelay_SF( RXLoRa_DataRate_Table[rate][0], RXLoRa_DataRate_Table[rate][1], RXLoRa_DataRate_Table[rate][2],size);
	
	return tr_delay;
}	
