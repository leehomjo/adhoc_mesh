#include "bsp.h"
#include "sx1278-Hal.h"

extern tLoRaSettings txLoRaSettings;

//����Ƶ��
void TX1278LoRaSetRFFrequency(uint32_t freq)    
{
    txLoRaSettings.RFFrequency = freq;

    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    TX1278REG->RegFrfMsb = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    TX1278REG->RegFrfMid = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    TX1278REG->RegFrfLsb = ( uint8_t )( freq & 0xFF );
    SX1278WriteBuffer( REG_LR_FRFMSB, &TX1278REG->RegFrfMsb, 3 ,TX_LORA);
}

//�õ�Ƶ��
uint32_t TX1278LoRaGetRFFrequency( void )
{
    SX1278ReadBuffer( REG_LR_FRFMSB, &TX1278REG->RegFrfMsb, 3 ,TX_LORA);
    txLoRaSettings.RFFrequency = ( ( uint32_t )TX1278REG->RegFrfMsb << 16 ) | ( ( uint32_t )TX1278REG->RegFrfMid << 8 ) | ( ( uint32_t )TX1278REG->RegFrfLsb );
    txLoRaSettings.RFFrequency = ( uint32_t )( ( double )txLoRaSettings.RFFrequency * ( double )FREQ_STEP );

    return txLoRaSettings.RFFrequency;
}

//���÷��书��
//ѡ�����������ʣ�Pmax=10.8+0.6 * power  20/17/14
void TX1278LoRaSetRFPower( int8_t power )
{
    SX1278Read( REG_LR_PACONFIG, &TX1278REG->RegPaConfig ,TX_LORA);
    SX1278Read( REG_LR_PADAC, &TX1278REG->RegPaDac ,TX_LORA);
    
    if( ( TX1278REG->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )  //���ѡ����PA_BOOST�������ʲ�����+20dBm
    {
        if( ( TX1278REG->RegPaDac & 0x87 ) == 0x87 )         //��OutputPower=1111 ʱ����PA_BOOST ��Ϊ+20dBm
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else                                               //���Ĭ��ֵ
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else                                                  //���ѡ����RFO�������ʲ�����14dBm
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
        TX1278REG->RegPaConfig = ( TX1278REG->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1278Write( REG_LR_PACONFIG, TX1278REG->RegPaConfig ,TX_LORA);
    txLoRaSettings.Power = power;
}


//�õ����书��
int8_t TX1278LoRaGetRFPower( void )
{
    SX1278Read( REG_LR_PACONFIG, &TX1278REG->RegPaConfig ,TX_LORA);
    SX1278Read( REG_LR_PADAC, &TX1278REG->RegPaDac ,TX_LORA);

    if( ( TX1278REG->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( TX1278REG->RegPaDac & 0x07 ) == 0x07 )
        {
            txLoRaSettings.Power = 5 + ( TX1278REG->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
        else
        {
            txLoRaSettings.Power = 2 + ( TX1278REG->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
    }
    else
    {
        txLoRaSettings.Power = -1 + ( TX1278REG->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
    }
    return txLoRaSettings.Power;
}

//���ô���
void TX1278LoRaSetSignalBandwidth( uint8_t bw )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    TX1278REG->RegModemConfig1 = ( TX1278REG->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
    SX1278Write( REG_LR_MODEMCONFIG1, TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.SignalBw = bw;
}

//�õ�����
uint8_t TX1278LoRaGetSignalBandwidth( void )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.SignalBw = ( TX1278REG->RegModemConfig1 & ~RFLR_MODEMCONFIG1_BW_MASK ) >> 4;
    return txLoRaSettings.SignalBw;
}

//������Ƶ����
void TX1278LoRaSetSpreadingFactor( uint8_t factor )
{

    if( factor > 12 )
    {
        factor = 12;
    }
    else if( factor < 6 )
    {
        factor = 6;
    }

    if( factor == 6 )
    {
        TX1278LoRaSetNbTrigPeaks( 5 );
    }
    else
    {
        TX1278LoRaSetNbTrigPeaks( 3 );
    }

    SX1278Read( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2 ,TX_LORA);    
    TX1278REG->RegModemConfig2 = ( TX1278REG->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
    SX1278Write( REG_LR_MODEMCONFIG2, TX1278REG->RegModemConfig2 ,TX_LORA);    
    txLoRaSettings.SpreadingFactor = factor;
}

//�õ���Ƶ����
uint8_t TX1278LoRaGetSpreadingFactor( void )
{
    SX1278Read( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2 ,TX_LORA);   
    txLoRaSettings.SpreadingFactor = ( TX1278REG->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SF_MASK ) >> 4;
    return txLoRaSettings.SpreadingFactor;
}

void TX1278LoRaSetErrorCoding( uint8_t value )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    TX1278REG->RegModemConfig1 = ( TX1278REG->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
    SX1278Write( REG_LR_MODEMCONFIG1, TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.ErrorCoding = value;
}

uint8_t TX1278LoRaGetErrorCoding( void )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.ErrorCoding = ( TX1278REG->RegModemConfig1 & ~RFLR_MODEMCONFIG1_CODINGRATE_MASK ) >> 1;
    return txLoRaSettings.ErrorCoding;
}

//CRCУ��ʹ��
void TX1278LoRaSetPacketCrcOn( bool enable )
{
    SX1278Read( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2 ,TX_LORA);
    TX1278REG->RegModemConfig2 = ( TX1278REG->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
    SX1278Write( REG_LR_MODEMCONFIG2, TX1278REG->RegModemConfig2 ,TX_LORA);
    txLoRaSettings.CrcOn = enable;
}

//����ǰ���볤��
void TX1278LoRaSetPreambleLength( uint16_t value )
{
    SX1278ReadBuffer( REG_LR_PREAMBLEMSB, &TX1278REG->RegPreambleMsb, 2 ,TX_LORA);

    TX1278REG->RegPreambleMsb = ( value >> 8 ) & 0x00FF;
    TX1278REG->RegPreambleLsb = value & 0xFF;
    SX1278WriteBuffer( REG_LR_PREAMBLEMSB, &TX1278REG->RegPreambleMsb, 2 ,TX_LORA);
}

//�õ�ǰ���볤��
uint16_t TX1278LoRaGetPreambleLength( void )
{
    SX1278ReadBuffer( REG_LR_PREAMBLEMSB, &TX1278REG->RegPreambleMsb, 2 ,TX_LORA);
    return ( ( TX1278REG->RegPreambleMsb & 0x00FF ) << 8 ) | TX1278REG->RegPreambleLsb;
}

//�õ�CRCУ���Ƿ���
bool TX1278LoRaGetPacketCrcOn( void )
{
    SX1278Read( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2 ,TX_LORA);
    txLoRaSettings.CrcOn = ( TX1278REG->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1;
    return txLoRaSettings.CrcOn;
}

//����ͷ����Ϣ����  Ĭ�Ϲ�
void TX1278LoRaSetImplicitHeaderOn( bool enable )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    TX1278REG->RegModemConfig1 = ( TX1278REG->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
    SX1278Write( REG_LR_MODEMCONFIG1, TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.ImplicitHeaderOn = enable;
}

//�õ�����ͷ����Ϣ
bool TX1278LoRaGetImplicitHeaderOn( void )
{
    SX1278Read( REG_LR_MODEMCONFIG1, &TX1278REG->RegModemConfig1 ,TX_LORA);
    txLoRaSettings.ImplicitHeaderOn = ( TX1278REG->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
    return txLoRaSettings.ImplicitHeaderOn;
}

//ʹ�ܵ��ν���ģʽ
void TX1278LoRaSetRxSingleOn( bool enable )
{
    txLoRaSettings.RxSingleOn = enable;
}

//�õ��Ƿ�ʼ���ν���ģʽ
bool TX1278LoRaGetRxSingleOn( void )
{
    return txLoRaSettings.RxSingleOn;
}

//ʹ����Ƶ
void TX1278LoRaSetFreqHopOn( bool enable )
{
    txLoRaSettings.FreqHopOn = enable;
}

//�õ��Ƿ�ʼ��Ƶ
bool TX1278LoRaGetFreqHopOn( void )
{
    return txLoRaSettings.FreqHopOn;
}

//������Ƶ����
void TX1278LoRaSetHopPeriod( uint8_t value )
{
    TX1278REG->RegHopPeriod = value;
    SX1278Write( REG_LR_HOPPERIOD, TX1278REG->RegHopPeriod ,TX_LORA);
    txLoRaSettings.HopPeriod = value;
}

//�õ���Ƶ����
uint8_t TX1278LoRaGetHopPeriod( void )
{
    SX1278Read( REG_LR_HOPPERIOD, &TX1278REG->RegHopPeriod ,TX_LORA);
    txLoRaSettings.HopPeriod = TX1278REG->RegHopPeriod;
    return txLoRaSettings.HopPeriod;
}

//���÷������ʱʱ��
void TX1278LoRaSetTxPacketTimeout( uint32_t value )
{
    txLoRaSettings.TxPacketTimeout = value;
}

//�õ��������ʱʱ��
uint32_t TX1278LoRaGetTxPacketTimeout( void )
{
    return txLoRaSettings.TxPacketTimeout;
}

//���ý������ʱʱ��
void TX1278LoRaSetRxPacketTimeout( uint32_t value )
{
    txLoRaSettings.RxPacketTimeout = value;
}

//�õ��������ʱʱ��
uint32_t TX1278LoRaGetRxPacketTimeout( void )
{
    return txLoRaSettings.RxPacketTimeout;
}

//���ø��صĳ��ȣ���������ͷ���������ã�
void TX1278LoRaSetPayloadLength( uint8_t value )
{
    TX1278REG->RegPayloadLength = value;
    SX1278Write( REG_LR_PAYLOADLENGTH, TX1278REG->RegPayloadLength ,TX_LORA);
    txLoRaSettings.PayloadLength = value;
}

//�õ����س���
uint8_t TX1278LoRaGetPayloadLength( void )
{
    SX1278Read( REG_LR_PAYLOADLENGTH, &TX1278REG->RegPayloadLength ,TX_LORA);
    txLoRaSettings.PayloadLength = TX1278REG->RegPayloadLength;
    return txLoRaSettings.PayloadLength;
}

//�����Ƿ�����PA���������+20dBm
void TX1278LoRaSetPa20dBm( bool enale )
{
    SX1278Read( REG_LR_PADAC, &TX1278REG->RegPaDac ,TX_LORA);    
    SX1278Read( REG_LR_PACONFIG, &TX1278REG->RegPaConfig ,TX_LORA);
    //���ѡ��PA�������ΪRFO��RegPaConfig����Ĵ������λΪ0����������ʲ��ó���+14dBm
	//���ѡ��PA�������ΪPA_BOOST ��RegPaConfig����Ĵ������λΪ1����������ʲ�����+20dBm 
    //REG_LR_PADAC���������ֲ�108ҳ������Ĵ����������� PA_BOOST ����������+20dBm ѡ��
   if( ( TX1278REG->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST ) //���ѡ����PA_BOOST
   {    
        if( enale == true )
        {
            TX1278REG->RegPaDac = 0x87;     //��OutputPower=1111 ʱ����PA_BOOST ��Ϊ+20dBm
        }
    }
    else   //���ѡ����RFO
    {
        TX1278REG->RegPaDac = 0x84;         //Ĭ��ֵ
    }
    SX1278Write( REG_LR_PADAC, TX1278REG->RegPaDac ,TX_LORA);
}

//�õ��Ƿ���ʹ��PA+20dBm
bool TX1278LoRaGetPa20dBm( void )
{
    SX1278Read( REG_LR_PADAC, &TX1278REG->RegPaDac ,TX_LORA);
    
    return ( ( TX1278REG->RegPaDac & 0x07 ) == 0x07 ) ? true : false;
}

//PAѡ���������ʿ���
void TX1278LoRaSetPAOutput( uint8_t outputPin )
{
    SX1278Read( REG_LR_PACONFIG, &TX1278REG->RegPaConfig ,TX_LORA);
    TX1278REG->RegPaConfig = (TX1278REG->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | outputPin;
    SX1278Write( REG_LR_PACONFIG, TX1278REG->RegPaConfig ,TX_LORA);
}

//�õ���ʱ������
uint8_t TX1278LoRaGetPAOutput( void )
{
    SX1278Read( REG_LR_PACONFIG, &TX1278REG->RegPaConfig ,TX_LORA);
    return TX1278REG->RegPaConfig & ~RFLR_PACONFIG_PASELECT_MASK;
}

//����б��б����ʱ��
void TX1278LoRaSetPaRamp( uint8_t value )
{
    SX1278Read( REG_LR_PARAMP, &TX1278REG->RegPaRamp ,TX_LORA);
    TX1278REG->RegPaRamp = ( TX1278REG->RegPaRamp & RFLR_PARAMP_MASK ) | ( value & ~RFLR_PARAMP_MASK );
    SX1278Write( REG_LR_PARAMP, TX1278REG->RegPaRamp ,TX_LORA);
}

//�õ�б��б����ʱ��
uint8_t TX1278LoRaGetPaRamp( void )
{
    SX1278Read( REG_LR_PARAMP, &TX1278REG->RegPaRamp ,TX_LORA);
    return TX1278REG->RegPaRamp & ~RFLR_PARAMP_MASK;
}

//���ý��ճ�ʱʱ��
void TX1278LoRaSetSymbTimeout( uint16_t value )
{
    SX1278ReadBuffer( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2, 2 ,TX_LORA);

    TX1278REG->RegModemConfig2 = ( TX1278REG->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    TX1278REG->RegSymbTimeoutLsb = value & 0xFF;
    SX1278WriteBuffer( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2, 2 ,TX_LORA);
}

//�õ����õĽ��ճ�ʱʱ��
uint16_t TX1278LoRaGetSymbTimeout( void )
{
    SX1278ReadBuffer( REG_LR_MODEMCONFIG2, &TX1278REG->RegModemConfig2, 2 ,TX_LORA);
    return ( ( TX1278REG->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) << 8 ) | TX1278REG->RegSymbTimeoutLsb;
}

void TX1278LoRaSetLowDatarateOptimize( bool enable )
{
    SX1278Read( REG_LR_MODEMCONFIG3, &TX1278REG->RegModemConfig3 ,TX_LORA);
    TX1278REG->RegModemConfig3 = ( TX1278REG->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( enable << 3 );
    SX1278Write( REG_LR_MODEMCONFIG3, TX1278REG->RegModemConfig3 ,TX_LORA);
}

bool TX1278LoRaGetLowDatarateOptimize( void )
{
    SX1278Read( REG_LR_MODEMCONFIG3, &TX1278REG->RegModemConfig3 ,TX_LORA);
    return ( ( TX1278REG->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 );
}

//�������ݰ�����
void TX1278LoRaSetNbTrigPeaks( uint8_t value )
{
    SX1278Read( 0x31, &TX1278REG->RegTestReserved31 ,TX_LORA);
    TX1278REG->RegTestReserved31 = ( TX1278REG->RegTestReserved31 & 0xF8 ) | value;//���ݰ����������Чλ 0x31 bit2 1 0
    SX1278Write( 0x31, TX1278REG->RegTestReserved31 ,TX_LORA);
}

//�õ����ݰ�����
uint8_t TX1278LoRaGetNbTrigPeaks( void )
{
    SX1278Read( 0x31, &TX1278REG->RegTestReserved31 ,TX_LORA);
    return ( TX1278REG->RegTestReserved31 & 0x07 );
}
