#include "bsp.h"

#define RXLoRa_FREQENCY                             500000000

//下面的值用来计算RSSI（信号强度）
#define RX_RSSI_OFFSET_LF                              -155.0
#define RX_RSSI_OFFSET_HF                              -150.0

#define RX_NOISE_ABSOLUTE_ZERO                         -174.0
#define RX_NOISE_FIGURE_LF                                4.0
#define RX_NOISE_FIGURE_HF                                6.0 

//预先计算的信号带宽，用于计算RSSI值
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

//这些值需要试验测得
const double RXRssiOffsetLF[] =
{   
    -155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,-155.0,
};

static const uint8_t RXLoRa_DataRate_Table[][3] =
{
	{ 7, 12, 1 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/5,
	{ 7, 11, 1 },   //0.5kbps,		 	 BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/5, 1√
	{ 7, 10, 1 },   //1.0kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/5,
	{ 7, 9 , 1 },   //1.7kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/5,	3√
	{ 7, 8 , 1 },   //3.1kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/5,
	{ 7, 7 , 1 },   //5.5kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/5,
	{ 7, 12, 2 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/6,
	{ 7, 11, 2 },   //0.5kbps,		 	 BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/6,
	{ 7, 10, 2 },   //0.8kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/6,
	{ 7, 9 , 2 },   //1.5kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/6, 
	{ 7, 8 , 2 },   //2.6kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/6,	10√
	{ 7, 7 , 2 },   //4.5kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/6,
	{ 7, 10, 3 },   //0.7kbps, 	     BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/7,
	{ 7, 9 , 3 },   //1.2kbps, 		 	 BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/7,
	{ 7, 8 , 3 },   //2.2kbps, 		 	 BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/7,	
	{ 7, 7 , 3 }    //3.9kbps        BW: 125KHz, Spreading Factor: 7,	 Error Coding value: 4/7,
};

//默认设置
tLoRaSettings rxLoRaSettings =
{
    RXLoRa_FREQENCY , // 频率
    20,               // 发射功率
    7,                // 信号频宽 [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,           
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    9,                // 扩频因子[6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    1,                // 纠错码 [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,            	// CRC效验开关[0: OFF, 1: ON]
    false,            // 隐藏头部信息开关 [0: OFF, 1: ON]
    0,                // 接收单次模式\连续模式配置 [0: Continuous, 1 Single]
    0,                // 跳频模式开关 [0: OFF, 1: ON]              //跳频技术
    4,                // 跳频之间的周期长度
    100,              // 最大发送时间
    100,              // 最大接收时间
    128,              // 数据长度 (用于隐式头模式)
};


rxSX1278LR* RX1278REG;       //RX1278 LoRa寄存器变量
extern uint8_t RX1278Regs[0x70];        //SX1278寄存器数组 接收
static uint8_t RXLORABuffer[RF_BUFFER_SIZE];  //这个数组既是发送数据用的数组也是接收时用的数组

uint8_t RXLORAState = RXLORA_STATE_IDLE;  //空闲模式

/*********** 数据帧超时相关 ***********/
volatile uint32_t MgtTimeout = 0;
volatile uint32_t BusiTimeout = 0;
extern LoRaManagementPara lora_mgt;
extern uint32_t TimeoutCount;		//超时时间
//业务帧超时记录
static uint32_t TimeoutTick10S = 0;
static uint32_t TimeoutTick20S = 0;
static uint32_t TimeoutTick30S = 0;
static uint32_t TimeoutTick40S = 0;
/**************************************/

static uint16_t RxPacketSize = 0;     //接收数据长度
static int8_t RxPacketSnrEstimate;    //接收数据信噪比变量
static double RxPacketRssiValue;      //接收数据时信号强度变量
static uint8_t RxGain = 1;            //信号放大增益
static uint32_t RxTimeoutTimer = 0;   //接收时间
static uint32_t TimeoutTimer = 0;   	//接收时间
static uint32_t PacketTimeout;        //最大接收时间
static uint16_t TxPacketSize = 0;     //发送数据长度

//初始化SX1278LoRa模式
void RX1278LoRaInit( void )
{
	RXLORAState = RXLORA_STATE_IDLE;                                     //设置为空闲模式

	RX1278LoRaSetRFFrequency( rxLoRaSettings.RFFrequency );            //设置频率
	RX1278LoRaSetSpreadingFactor( rxLoRaSettings.SpreadingFactor );    //SF6只在隐式报头模式下工作  默认7
	RX1278LoRaSetErrorCoding( rxLoRaSettings.ErrorCoding );            //纠错码
	RX1278LoRaSetPacketCrcOn( rxLoRaSettings.CrcOn );                  //CRC效验开关
	RX1278LoRaSetSignalBandwidth( rxLoRaSettings.SignalBw );           //带宽    默认9
	RX1278LoRaSetImplicitHeaderOn( rxLoRaSettings.ImplicitHeaderOn );  //隐藏头部信息开关  默认关
	RX1278LoRaSetPreambleLength(8);
	
	RX1278LoRaSetSymbTimeout(0x3FF);                                 //接收超时时间最大1024
	RX1278LoRaSetPayloadLength( rxLoRaSettings.PayloadLength );        //设置数据长度为128（仅在隐藏头部开启有用）
	if(rxLoRaSettings.SpreadingFactor < 11)
	{
		RX1278LoRaSetLowDatarateOptimize( false );
	}
	else
		RX1278LoRaSetLowDatarateOptimize( true );                        //设置最大接收超时时间为2^8+1 16ms

	SX1278Write( REG_LR_OCP, 0x3f ,RX_LORA);																	 //设置过流保护为240mA
	RX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 					 //选择PA_BOOST管脚输出信号
	RX1278LoRaSetPa20dBm( true );                           					 //最大功率可达到+20dMb 
	rxLoRaSettings.Power = 20;                                 					 //发射功率为20
	RX1278LoRaSetRFPower( rxLoRaSettings.Power );              					 //设置发射功率        
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );	              				 // 将设备设置为待机模式
}

//设置版本号
void RX1278LoRaSetDefaults( void )
{
   SX1278Read( REG_LR_VERSION, &RX1278REG->RegVersion ,RX_LORA);
}

/*****************************************************************
* 功能描述：设置LoRa工作模式
* 入口参数：RFLR_OPMODE_SLEEP		        		 --	睡眠模式
						RFLR_OPMODE_STANDBY		         	 --	待机模式
            RFLR_OPMODE_SYNTHESIZER_TX       -- 频率合成发送模式
						RFLR_OPMODE_TRANSMITTER	         --	发送模式
            RFLR_OPMODE_SYNTHESIZER_RX       -- 频率合成接收模式
						RFLR_OPMODE_RECEIVER	        	 --	持续接收模式
            RFLR_OPMODE_RECEIVER_SINGLE      -- 单次接收模式
            RFLR_OPMODE_CAD                  -- 信号活动检测模式
* 返回值：	无
*******************************************************************/
void RX1278LoRaSetOpMode( uint8_t opMode )
{
	SX1278Read( REG_LR_OPMODE, &RX1278REG->RegOpMode ,RX_LORA);
	RX1278REG->RegOpMode = ( RX1278REG->RegOpMode & RFLR_OPMODE_MASK ) | opMode;
	//REG_LR_OPMODE寄存器查看数据手册111页
	SX1278Write( REG_LR_OPMODE, RX1278REG->RegOpMode ,RX_LORA);        
}

//获取当前LORA的模式 
uint8_t RX1278LoRaGetOpMode( void )                       
{
	SX1278Read( REG_LR_OPMODE, &RX1278REG->RegOpMode ,RX_LORA);	
	return RX1278REG->RegOpMode & ~RFLR_OPMODE_MASK;
}

//获取LNA（信号放大）增益（001=最大增益，010=最大增益-6dB，011=最大增益-12dB..........）
uint8_t RX1278LoRaReadRxGain( void )                    
{
	//REG_LR_LNA寄存器查看数据手册114页
	SX1278Read( REG_LR_LNA, &RX1278REG->RegLna ,RX_LORA);
	return( RX1278REG->RegLna >> 5 ) & 0x07;
}

//获取信号强度
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

//获取数据时的增益值
uint8_t RX1278LoRaGetPacketRxGain( void )
{
  return RxGain;
}

//获取数据时的信噪比值，信号和噪声的比值，信噪比越高，说明信号干扰越小。
int8_t RX1278LoRaGetPacketSnr( void )
{
  return RxPacketSnrEstimate;
}

//获取数据时的无线信号强度
double RX1278LoRaGetPacketRssi( void )
{
  return RxPacketRssiValue;
}

//开始接收
void RX1278LoRaStartRx( void )
{
  RX1278LoRaSetRFState( RXLORA_STATE_RX_INIT );
}

//接收数据
void RX1278LoRaGetRxPacket( void *buffer, uint16_t *size )
{
	*size = RxPacketSize;
	RxPacketSize = 0;
	memcpy( (void*)buffer, (void*)RXLORABuffer, (size_t)*size );
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //清空RFBuffer
}

//发送数据
void RX1278LoRaSetTxPacket( const void *buffer, uint16_t size )
{
	if( rxLoRaSettings.FreqHopOn == false )    //默认是关了的
	{
		TxPacketSize = size;
	}
	else
	{
		TxPacketSize = 255;
	}
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //清空RFBuffer
	memcpy( ( void * )RXLORABuffer, buffer, ( size_t )TxPacketSize );  //RFBuffer是个装发送数据的数组，而buffer是你要发送的数据的数组

	RXLORAState = RXLORA_STATE_TX_INIT;                                //设置状态为发送初始化   
}

//得到RFLRState状态
uint8_t RX1278LoRaGetRFState( void )
{
  return RXLORAState;
}

//设置RFLRState状态，RFLRState的值决定了下面的函数处理哪一步的代码
void RX1278LoRaSetRFState( uint8_t state )
{
  RXLORAState = state;
}

/********************************************************************
	* 功能描述：	获取调用本函数前最后接到那包数据的信噪比
	* 入口参数：	无
	* 返回值：		信噪比，有正有负
*********************************************************************/
int8_t RX1278GetLoRaSNR (void)                                                //512到526行有这部分代码
{
	u8 rxSnrEstimate;
	//取最后一包数据的信噪比         									//记录信噪比估值
	SX1278Read(REG_LR_PKTSNRVALUE, &rxSnrEstimate ,RX_LORA);					//信噪比
	//The SNR sign bit is 1												//信噪比小于0的情况，负数
	if( rxSnrEstimate & 0x80 )
	{
		RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;	//Invert and divide by 4
		RxPacketSnrEstimate = -RxPacketSnrEstimate;
	}
	else
	{
		//信噪比大于0的情况，正数
		RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;			//Divide by 4
	}
	return RxPacketSnrEstimate;
}

//得到信号增益，即LNA
uint8_t RX1278GetLoRaLNA()
{
  return RX1278LoRaReadRxGain();
}

/*******************************************************************************
	* 功能描述:     获取调用本函数前最后接到那包数据的信号强度
	* 入口参数:     无
	* 返回值:       信号强度，有正有负
********************************************************************************/
double RX1278GetPackLoRaRSSI(void)                                             //527行到551行
{
	if( RX1278GetLoRaSNR() < 0 )
	{
		/***************************************************************************************
			*功率：P=-174（dBm） + BW(dB) + NF(dB) + SNR(dB);
			*在信号被噪声淹没的情况下用此公式反推RSSI，前三项是热噪声功率,最后一项是信噪比
		****************************************************************************************/
		//信号被噪声淹没
		RxPacketRssiValue = RX_NOISE_ABSOLUTE_ZERO + 10.0 * RXSignalBwLog[rxLoRaSettings.SignalBw] + \
		                    RX_NOISE_FIGURE_LF     + (double)RxPacketSnrEstimate;
	}
	else
	{
		//信号强于噪声
		SX1278Read( REG_LR_PKTRSSIVALUE, &RX1278REG->RegPktRssiValue ,RX_LORA);
		RxPacketRssiValue = RXRssiOffsetLF[rxLoRaSettings.SignalBw] + (double)RX1278REG->RegPktRssiValue;
	}
	return RxPacketRssiValue;
}

/*********************************************************************************************************
	* 功能描述:      进入接收状态
	* 入口参数:      无
	* 返回值:        无
*********************************************************************************************************/
void RX1278RxStateEnter (void)
{
	//1.---->进入待机模式
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );		//操作寄存器只能在在Standby,sleep or FSTX 模式

	//2.---->接收初始化，参数设置
	RX1278REG->RegIrqFlagsMask	= 	RFLR_IRQFLAGS_RXTIMEOUT          |	//超时中断屏蔽
	                              //RFLR_IRQFLAGS_RXDONE             |	//打开数据包接收完成中断
	                              //RFLR_IRQFLAGS_PAYLOADCRCERROR    |	//负载CRC错误中断屏蔽
	                                RFLR_IRQFLAGS_VALIDHEADER        |	//Rx模式下接收到的有效报头屏蔽
	                                RFLR_IRQFLAGS_TXDONE             |	//FIFO负载发送完成中断屏蔽
	                                RFLR_IRQFLAGS_CADDONE            |	//CAD完成中断屏蔽
	                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |	//FHSS改变信道中断屏蔽
	                                RFLR_IRQFLAGS_CADDETECTED;			//检测到CAD中断屏蔽
	SX1278Write( REG_LR_IRQFLAGSMASK, RX1278REG->RegIrqFlagsMask ,RX_LORA);		//设置需要屏蔽的中断(被注释掉的中断即打开)

	RX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, RX1278REG->RegHopPeriod ,RX_LORA);			//频率跳变之间的符号周期:255
	//3.---->设置接收模式(单次/持续模式)
	if( rxLoRaSettings.RxSingleOn == true )
	{
		//设置为单次接收
		RX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
	}
	else
	{
		//设置为持续接收模式
		RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxBaseAddr;		  	//内容:0x00
		SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);	//SPI访问FIFO的地址
		RX1278LoRaSetOpMode( RFLR_OPMODE_RECEIVER );           				//进入接收状态
		RX1278RXTX(false);
	}
	memset( RXLORABuffer, 0, ( size_t )RF_BUFFER_SIZE );                    //清空RFBuffer
	PacketTimeout = rxLoRaSettings.RxPacketTimeout;                       //设置最大接收时间为100
	RxTimeoutTimer = GET_TICK_COUNT( );
	RXLORAState = RXLORA_STATE_RX_RUNNING;
}

/**************************************************************
	* 功能描述：  读取接收状态
	* 入口参数：	无
	* 返回值：		无
***************************************************************/
void RX1278RxStatusRead(void)
{
	uint8_t regValue = 0;
	uint32_t time = 0;
	SX1278Read(REG_LR_IRQFLAGS,&regValue,RX_LORA);                                  //读取此时开启了哪些中断  
																																					
	if(regValue & RFLR_IRQFLAGS_RXDONE)                                      //接收完成
	{                                                  											                   
//		printf("lora_receive_success!!\r\n");
		SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE,RX_LORA);							//执行写操作以清除接收中断标志
		RXLORAState = RXLORA_STATE_RX_DONE;
		if(lora_mgt.frame_type_flag)
		{
//			lora_mgt.first_rec_flag = false;							//第一次接收数据时将此标志位置位，转为业务帧复位
			BusiTimeout = GET_TICK_COUNT();
			MgtTimeout = 0;
		}
		else 
		{
//			lora_mgt.first_rec_flag = true;
			TimeoutCount = 0;	//每次接收到数据后超时次数清零
			lora_mgt.rx_timeout_flag = false;
			BusiTimeout = 0;
			MgtTimeout = GET_TICK_COUNT();	
		}
	}	 
	else
	{
//		if(!lora_mgt.frame_type_flag && lora_mgt.first_rec_flag) 	//从接收到数据后开始计算超时
//		{
//			if(SelfAddJudge(GET_TICK_COUNT(),MgtTimeout,LORA_TIMEOUT,&lora_mgt.configed_timeout))     //计算是否超时
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
					printf("超时10s次数 %d\r\n",TimeoutTick10S);
					printf("超时时间 %d\r\n",time);
				}
				if(((time >= 20000) && (time <30000) &&(time % 20000) == 0))
				{
					TimeoutTick20S++;
					printf("超时20s次数 %d\r\n",TimeoutTick20S);
					printf("超时时间 %d\r\n",time);
				}
				if(((time >= 30000) && (time <40000) &&(time % 300000) == 0))
				{
					TimeoutTick30S++;
					printf("超时30s次数 %d\r\n",TimeoutTick30S);
					printf("超时时间 %d\r\n",time);
				}
				if((time >= 40000) && ((time % 400000) == 0))
				{
					TimeoutTick40S++;
					printf("超时40s次数 %d\r\n",TimeoutTick40S);
					printf("超时时间 %d\r\n",time);
				}
			}
		}
		else
		{
			if(SelfAddJudge(GET_TICK_COUNT(),MgtTimeout,LORA_TIMEOUT,&lora_mgt.configed_timeout))     //计算是否超时
			{
				lora_mgt.rx_timeout_flag = true;
			}
		}
	}
	if( rxLoRaSettings.RxSingleOn == true )                                 //单次接收模式，默认持续接受模式
	{
		if(SelfAddJudge(GET_TICK_COUNT(),RxTimeoutTimer,PacketTimeout,&TimeoutTimer))     //计算是否超时
		{
			RXLORAState = RXLORA_STATE_RX_INIT;                           		 //接收超时 
		}
	}  
}	

/**************************************************************
	* 功能描述：  读取接收到数据
	* 入口参数：	无
	* 返回值：		无
***************************************************************/
void RX1278RxDataRead(void)
{
  SX1278Read( REG_LR_IRQFLAGS, &RX1278REG->RegIrqFlags ,RX_LORA);			//读取中断状态标志位    
	//检测CRC标志，以验证数据包的完整性 	
	if(( RX1278REG->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR)
	{
		SX1278Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR,RX_LORA);	//执行写操作以清除接收中断标志
		if( rxLoRaSettings.RxSingleOn == true )                              //单次接收模式
		{
			RXLORAState = RXLORA_STATE_RX_INIT;                                
		}
		else
		{ 
			RXLORAState = RXLORA_STATE_RX_RUNNING;
		}
	 return;
	}  
	if( rxLoRaSettings.RxSingleOn == true )                                   //单次接收模式，默认持续模式
	{
		RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoRxBaseAddr;       
		SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);          //读取接收数据
		if( rxLoRaSettings.ImplicitHeaderOn == true )                           //默认false
		{
			RxPacketSize = RX1278REG->RegPayloadLength;
			SX1278ReadFifo( RXLORABuffer, RX1278REG->RegPayloadLength ,RX_LORA);           	//在Lora中读取接收到的数据
		}
		else
		{
			//REG_LR_NBRXBYTES寄存器查看数据寄存器115页  最近一次接收到的数据包的负载字节数
			SX1278Read( REG_LR_NBRXBYTES, &RX1278REG->RegNbRxBytes ,RX_LORA);
			RxPacketSize = RX1278REG->RegNbRxBytes;
			SX1278ReadFifo( RXLORABuffer, RX1278REG->RegNbRxBytes ,RX_LORA);
		}
	 }
	 else                                                                     //接续接收模式
	 {
		 //REG_LR_FIFORXCURRENTADDR寄存器查看数据寄存器115页   接收到最后一个数据包的起始地址（数据缓冲区中）
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
	 if(rxLoRaSettings.RxSingleOn == true)                                       //单次接收模式
	 { 
		 RXLORAState = RXLORA_STATE_RX_INIT;
	 }
	 else                                                                      //接续接收模式
	 { 
		 RXLORAState = RXLORA_STATE_RX_RUNNING;
	 }
}	

/**************************************************************
	* 功能描述：  发送数据
	* 入口参数：	无
	* 返回值：		无
***************************************************************/
void RX1278TxData(void)
{
	uint8_t regValue=0;
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                              //待机模式
	
	//REG_LR_IRQFLAGSMASK寄存器看数据手册115
	RX1278REG->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT          |
									 RFLR_IRQFLAGS_RXDONE             |
									 RFLR_IRQFLAGS_PAYLOADCRCERROR    |
									 RFLR_IRQFLAGS_VALIDHEADER        |
									 //RFLR_IRQFLAGS_TXDONE             |      //开启发送完成中断
									 RFLR_IRQFLAGS_CADDONE            |
									 RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
									 RFLR_IRQFLAGS_CADDETECTED;             
	RX1278REG->RegHopPeriod = 255;
	SX1278Write( REG_LR_HOPPERIOD, RX1278REG->RegHopPeriod ,RX_LORA);                //设置跳频周期，默认为0
	SX1278Write( REG_LR_IRQFLAGSMASK, RX1278REG->RegIrqFlagsMask ,RX_LORA);      

	RX1278REG->RegPayloadLength = TxPacketSize;                              //初始化负载大小
	SX1278Write( REG_LR_PAYLOADLENGTH, RX1278REG->RegPayloadLength ,RX_LORA);

	RX1278REG->RegFifoTxBaseAddr = 0x00;                                     
	SX1278Write( REG_LR_FIFOTXBASEADDR, RX1278REG->RegFifoTxBaseAddr ,RX_LORA);

	RX1278REG->RegFifoAddrPtr = RX1278REG->RegFifoTxBaseAddr;
	SX1278Write( REG_LR_FIFOADDRPTR, RX1278REG->RegFifoAddrPtr ,RX_LORA);

	SX1278WriteFifo( RXLORABuffer, RX1278REG->RegPayloadLength ,RX_LORA);                //将要发送的数组写到LORA的fifo中
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );			//发送模式
	
	SX1278Read(REG_LR_IRQFLAGS,&regValue,RX_LORA);                                  //读取此时开启了哪些中断  
																																					
	if(regValue & RFLR_IRQFLAGS_TXDONE)                                      //发送完成
	{
		printf("rxlora_send_success!!\r\n");
		SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE ,RX_LORA);          				//清除标志位
		RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );                           //待机模式
		RXLORAState = RXLORA_STATE_TX_DONE;  
	}
}

/******************************************************
	* 功能描述：    RX1278发送接收处理函数
	* 入口参数：    无
	* 返回值：      无
*******************************************************/
void RX1278TxRxProcess(void)
{
	//接收
	if(RXLORAState == RXLORA_STATE_RX_INIT)RX1278RxStateEnter();
	if(RXLORAState == RXLORA_STATE_RX_RUNNING)RX1278RxStatusRead();
	if(RXLORAState == RXLORA_STATE_RX_DONE)
	{
		RX1278RxDataRead();
		RXLORAReciveData();
	}
	
	if(RXLORAState == RXLORA_STATE_RX_IDLE)						//接收空闲状态
	{	
		vTaskDelay(lora_mgt.tx_delay_time);								//接收完毕转换发送延时
		RX1278_SetFreq(lora_mgt.tx_freq_index);						//切换471发送
		RX1278_SetDataRate(lora_mgt.tx_datarate_index);		//切换速率
		RXLORAState = RXLORA_STATE_TX_START;									//进入发送就绪态
	}
	
	//发送
	if(RXLORAState == RXLORA_STATE_TX_START)RXLORASendData();
	if(RXLORAState == RXLORA_STATE_TX_INIT)RX1278TxData();
	if(RXLORAState == RXLORA_STATE_TX_DONE)
	{
		vTaskDelay(lora_mgt.rx_delay_time);								//发送完毕转换接收延时
		RX1278_SetFreq(lora_mgt.rx_freq_index);						//切换接收
		RX1278_SetDataRate(lora_mgt.rx_datarate_index);		//切换速率
		RXLORAState = RXLORA_STATE_RX_INIT;	
	}	
}

/******************************************************
	* 功能描述：    RX1278接收处理函数
	* 入口参数：    无
	* 返回值：      无
*******************************************************/
void RX1278RxProcess(void)
{
	//接收
	if(RXLORAState == RXLORA_STATE_RX_INIT)
	{
		RX1278RxStateEnter();															//接收初始化
	}
	if(RXLORAState == RXLORA_STATE_RX_RUNNING)
	{
		RX1278RxStatusRead();															//一直读取接收完成标志位直到接收到整帧数据	
	}
	if(RXLORAState == RXLORA_STATE_RX_DONE)							//接收完成读取FIFO并处理数据
	{
		RX1278RxDataRead();
		RXLORAReciveData();
		RXLORAState = RXLORA_STATE_RX_IDLE;
	}	
	if(RXLORAState == RXLORA_STATE_RX_IDLE)							//接收空闲状态，用于更改接收频点和速率
	{
//		vTaskDelay(lora_mgt.rx_delay_time);								//发送完毕转换接收延时
		RX1278_SetFreq(lora_mgt.rx_freq_index);						//切换接收
		RX1278_SetDataRate(lora_mgt.rx_datarate_index);		//切换速率
		RXLORAState = RXLORA_STATE_RX_INIT;	
	}	
}

/******************************************************
	* 功能描述：    设置发射功率
	* 入口参数：    pwr: 功率范围5 ~ 20
	* 返回值：      无
*******************************************************/
void RX1278TxPower (uint8_t pwr)
{
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//进入待机模式
	//默认参数PA最在电流为100mA,当输出20dBm时需要120mA,所以必须配置0x0b存器
	SX1278Write( REG_LR_OCP, 0x3f ,RX_LORA);
	RX1278LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
	RX1278LoRaSetPa20dBm( true );
	RX1278LoRaSetRFPower( pwr);
}

/******************************************************
	* 功能描述：	设置载波频率
	* 入口参数：  0-200 频点索引值
	* 返回值：      无
*******************************************************/
void RX1278_SetFreq(uint8_t index)
{
	float Freq;
	uint32_t lora_freq;
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//进入待机模式
	
	Freq = 470 + index * 0.2;
	Freq = ( Freq >= 470 ) ? Freq : 470;
	Freq = ( Freq <= 510 ) ? Freq : 510;
	lora_freq = (uint32_t)((Freq*1e6) + 0.5); //偏移0.5hz避免四舍五入
		
	RX1278LoRaSetRFFrequency( lora_freq );
}	

/******************************************************
	* 功能描述：	设置空中速率
	* 入口参数：  速率档位300bps~5.5kbps
	* 返回值：    无
*******************************************************/
void RX1278_SetDataRate(uint8_t rate)
{
	rate = (rate <= 15) ? rate : 15;
	
	RX1278LoRaSetOpMode( RFLR_OPMODE_STANDBY );				//进入待机模式
	
	RX1278LoRaSetSignalBandwidth( RXLoRa_DataRate_Table[rate][0] );        //带宽125KHz
	RX1278LoRaSetSpreadingFactor( RXLoRa_DataRate_Table[rate][1] );    		 //扩频因子
	RX1278LoRaSetErrorCoding( RXLoRa_DataRate_Table[rate][2] );            //纠错码
	if(RXLoRa_DataRate_Table[rate][1] < 11)
	{	
		RX1278LoRaSetLowDatarateOptimize( false );
	}
	else RX1278LoRaSetLowDatarateOptimize( true );                       //设置最大接收超时时间为2^8+1 16ms
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
	* 功能描述：	设置发送接收延时 单向延时
	* 入口参数：  速率档位300bps~5.5kbps
	* 返回值：    延时时间
*******************************************************/
uint16_t RX1278_SetTxRxDelay(uint8_t rate,uint8_t size)
{
	uint16_t tr_delay = 0;
	
	rate = (rate <= 15) ? rate : 15;
	tr_delay = RX1278_SetTxRxDelay_SF( RXLoRa_DataRate_Table[rate][0], RXLoRa_DataRate_Table[rate][1], RXLoRa_DataRate_Table[rate][2],size);
	
	return tr_delay;
}	
