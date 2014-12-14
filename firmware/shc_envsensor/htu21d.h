/* 
 HTU21D Humidity Sensor Library
 
 This code based on the library from https://github.com/sparkfun/HTU21D
 
 Get humidity and temperature from the HTU21D sensor.
 
 */
#include "util.h"

#define HTDU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE


  //Public Functions
  
  /*
 * Measures and reads humidity from HTU21D in %.
 * A measurement takes at most around 55 ms.
 * Returns: measured humidity in %
 * 
 */
  int16_t htu21d_readHumidity(void);
  
  /*
 * Measures and reads temperature from HTU21D.
 * Converts reading in centigrades.
 * Returns: measured temperature in centigrades
 */
  int16_t htu21d_readTemperature(void);
  
  /*
 * Sets the sensor resolution to one of four levels
 * Page 12:
 * 	0/0 = 12bit RH, 14bit Temp
 *	0/1 = 8bit RH, 12bit Temp
 *	1/0 = 10bit RH, 13bit Temp
 *  1/1 = 11bit RH, 11bit Temp
 */ 
  void htu21d_setResolution(unsigned char);
 
  /*
 * Read the user register
 */ 
  uint8_t htu21d_read_user_register(void);
 
  /*
 * Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
 * Returns: 0 if the transmission was good
 * Returns: something other than 0 if the communication was corrupted
 * From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
 * POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
 */  
  uint8_t htu21d_check_crc(uint16_t, uint8_t);
