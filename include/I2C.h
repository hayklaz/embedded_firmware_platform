/* 
 * File:  I2C.h
 * Original Author: Max
 * Created on February 22, 2018, 1:17 PM
 * Revised for ece121 W2023 by scp
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define I2C_DEFAULT_RATE 100000

/**
 * @Function I2C_Init(Rate)
 * @param Rate - Clock rate for the I2C system
 * @return The clock rate set for the I2C system, 0 if already inited
 * @brief  Initializes the I2C System for use with the intended peripheral
 */
unsigned int I2C_Init(unsigned int Rate);

//Read and Write functions for byte and page
unsigned char I2C_ByteWriteReg(unsigned char I2CAddress,
                               uint16_t deviceRegisterAddress,
                               uint8_t data);

unsigned char I2C_ByteReadReg(unsigned char I2CAddress,
                              uint16_t deviceRegisterAddress,
                              uint8_t *data);

unsigned char I2C_PageWriteReg(unsigned char I2CAddress,
                               uint16_t deviceRegisterAddress,
                               uint8_t *data,
                               uint8_t length);

unsigned char I2C_PageReadReg(unsigned char I2CAddress,
                              uint16_t deviceRegisterAddress,
                              uint8_t *data,
                              uint8_t length);

/**
 * @Function I2C_ReadInt(char I2CAddress, char deviceRegisterAddress, char isBigEndian)
 * @param I2CAddresss - 7-bit address of I2C device wished to interact with
 * @param deviceRegisterAddress - 8-bit lower address of register on device
 * @param isBigEndian - Boolean determining if device is big or little endian
 * @return 0 if error and 1 if success
 * @brief  Reads two sequential registers to build a 16-bit value. isBigEndian
 * whether the first bits are either the high or low bits of the value
 */
int I2C_ReadInt(char I2CAddress, char deviceRegisterAddress, char isBigEndian);

#endif	/* I2C_H */

