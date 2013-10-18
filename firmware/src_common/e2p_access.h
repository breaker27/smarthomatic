#include <stdint.h>

#ifdef UNITTEST

uint8_t eeprom_read_byte (const uint8_t *__p);
void signal_error_state(void);

#endif

uint8_t  eeprom_read_UIntValue8 (uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval);
uint16_t eeprom_read_UIntValue16(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval);
uint32_t eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval);
