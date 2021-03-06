#ifndef __CRC8_H__
#define __CRC8_H__
#include <stdint.h> 
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CRC_START_8     0xFF
#define CRC_XOROUT_8    0xFF

extern uint8_t crc_8(const uint8_t *input, size_t len);
extern uint8_t update_crc_8(uint8_t crc, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif /* __CRC8_H__ */
