#ifndef __TX1278_LORA_MISC_H__
#define __TX1278_LORA_MISC_H__
#include "stdint.h"
#include "stdbool.h"
/*!
 * \brief Writes the new RF frequency value
 *
 * \param [IN] freq New RF frequency value in [Hz]
 */
void TX1278LoRaSetRFFrequency( uint32_t freq );

/*!
 * \brief Reads the current RF frequency value
 *
 * \retval freq Current RF frequency value in [Hz]
 */
uint32_t TX1278LoRaGetRFFrequency( void );

/*!
 * \brief Writes the new RF output power value
 *
 * \param [IN] power New output power value in [dBm]
 */
void TX1278LoRaSetRFPower( int8_t power );

/*!
 * \brief Reads the current RF output power value
 *
 * \retval power Current output power value in [dBm]
 */
int8_t TX1278LoRaGetRFPower( void );

/*!
 * \brief Writes the new Signal Bandwidth value
 *
 * \remark This function sets the IF frequency according to the datasheet
 *
 * \param [IN] factor New Signal Bandwidth value [0: 125 kHz, 1: 250 kHz, 2: 500 kHz]
 */
void TX1278LoRaSetSignalBandwidth( uint8_t bw );

/*!
 * \brief Reads the current Signal Bandwidth value
 *
 * \retval factor Current Signal Bandwidth value [0: 125 kHz, 1: 250 kHz, 2: 500 kHz] 
 */
uint8_t TX1278LoRaGetSignalBandwidth( void );

/*!
 * \brief Writes the new Spreading Factor value
 *
 * \param [IN] factor New Spreading Factor value [7, 8, 9, 10, 11, 12]
 */
void TX1278LoRaSetSpreadingFactor( uint8_t factor );

/*!
 * \brief Reads the current Spreading Factor value
 *
 * \retval factor Current Spreading Factor value [7, 8, 9, 10, 11, 12] 
 */
uint8_t TX1278LoRaGetSpreadingFactor( void );

/*!
 * \brief Writes the new Error Coding value
 *
 * \param [IN] value New Error Coding value [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
 */
void TX1278LoRaSetErrorCoding( uint8_t value );

/*!
 * \brief Reads the current Error Coding value
 *
 * \retval value Current Error Coding value [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
 */
uint8_t TX1278LoRaGetErrorCoding( void );

/*!
 * \brief Enables/Disables the packet CRC generation
 *
 * \param [IN] enaable [true, false]
 */
void TX1278LoRaSetPacketCrcOn( bool enable );

/*!
 * \brief Reads the current packet CRC generation status
 *
 * \retval enable [true, false]
 */
bool TX1278LoRaGetPacketCrcOn( void );

/*!
 * \brief Enables/Disables the Implicit Header mode in LoRa
 *
 * \param [IN] enable [true, false]
 */
void TX1278LoRaSetImplicitHeaderOn( bool enable );

/*!
 * \brief Check if implicit header mode in LoRa in enabled or disabled
 *
 * \retval enable [true, false]
 */
bool TX1278LoRaGetImplicitHeaderOn( void );

/*!
 * \brief Enables/Disables Rx single instead of Rx continuous
 *
 * \param [IN] enable [true, false]
 */

/*!
 * \brief Set payload length
 *
 * \param [IN] value payload length
 */
void TX1278LoRaSetPayloadLength( uint8_t value );

/*!
 * \brief Get payload length
 *
 * \retval value payload length
 */
uint8_t TX1278LoRaGetPayloadLength( void );

/*!
 * \brief Enables/Disables the 20 dBm PA
 *
 * \param [IN] enable [true, false]
 */
void TX1278LoRaSetPa20dBm( bool enale );

/*!
 * \brief Gets the current 20 dBm PA status
 *
 * \retval enable [true, false]
 */
bool TX1278LoRaGetPa20dBm( void );

/*!
 * \brief Set the RF Output pin 
 *
 * \param [IN] RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
 */
void TX1278LoRaSetPAOutput( uint8_t outputPin );

/*!
 * \brief Gets the used RF Ouptut pin
 *
 * \retval RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
 */
uint8_t TX1278LoRaGetPAOutput( void );

/*!
 * \brief Writes the new PA rise/fall time of ramp up/down value
 *
 * \param [IN] value New PaRamp value
 */
void TX1278LoRaSetPaRamp( uint8_t value );

/*!
 * \brief Reads the current PA rise/fall time of ramp up/down value
 *
 * \retval freq Current PaRamp value
 */
uint8_t TX1278LoRaGetPaRamp( void );

/*!
 * \brief Set Symbol Timeout based on symbol length
 *
 * \param [IN] value number of symbol
 */
void TX1278LoRaSetSymbTimeout( uint16_t value );

/*!
 * \brief  Get Symbol Timeout based on symbol length
 *
 * \retval value number of symbol
 */
uint16_t TX1278LoRaGetSymbTimeout( void );

/*!
 * \brief  Configure the device to optimize low datarate transfers
 *
 * \param [IN] enable Enables/Disables the low datarate optimization
 */
void TX1278LoRaSetLowDatarateOptimize( bool enable );

/*!
 * \brief  Get the status of optimize low datarate transfers
 *
 * \retval LowDatarateOptimize enable or disable
 */
bool TX1278LoRaGetLowDatarateOptimize( void );

/*!
 * \brief Get the preamble length
 *
 * \retval value preamble length
 */
uint16_t TX1278LoRaGetPreambleLength( void );

/*!
 * \brief Set the preamble length
 *
 * \param [IN] value preamble length
 */
void TX1278LoRaSetPreambleLength( uint16_t value );

/*!
 * \brief Set the number or rolling preamble symbol needed for detection
 *
 * \param [IN] value number of preamble symbol
 */
void TX1278LoRaSetNbTrigPeaks( uint8_t value );

/*!
 * \brief Get the number or rolling preamble symbol needed for detection
 *
 * \retval value number of preamble symbol
 */
uint8_t TX1278LoRaGetNbTrigPeaks( void );
#endif 
