/* 
 * File:   I2C.c
 * Author: Hayk Lazaryan
 *
 * Created on March 10, 2026, 10:39pm
 */

// Microchip Libraries
#include <xc.h>
#include <sys/attribs.h>
#include <stdint.h>

#include "I2C.h"
#include "BOARD.h"
#include "Uart.h"

#define PAGE_SIZE 64u

/*******************************************************************************
 * Helpers
 ******************************************************************************/

//Waits until the I2C hardware is completely idle before starting a new step
static int I2C_WaitIdle(void)
{
    unsigned int timeout = I2C_DEFAULT_RATE;

    // Wait while any lower control bits are active or transmit is still happening
    while (((I2C1CON & 0x1Fu) != 0u) || I2C1STATbits.TRSTAT) {
        if (--timeout == 0u) {
            return ERROR;
        }
    }
    return SUCCESS;
}

//Starts an I2C transaction
static int I2C_StartTransfer(int restart)
{
    unsigned int timeout = I2C_DEFAULT_RATE;

    // Make sure previous I2C activity is done first
    if (I2C_WaitIdle() == ERROR) {
        return ERROR;
    }

    if (restart) {
        //Generate repeated start
        I2C1CONbits.RSEN = 1;
        while (I2C1CONbits.RSEN) {
            if (--timeout == 0u) {
                return ERROR;
            }
        }
    } else {
        //Generate normal start
        I2C1CONbits.SEN = 1;
        while (I2C1CONbits.SEN) {
            if (--timeout == 0u) {
                return ERROR;
            }
        }
    }
    return SUCCESS;
}

//Ends an I2C transaction by generating a STOP condition.
static int I2C_StopTransfer(void)
{
    unsigned int timeout = I2C_DEFAULT_RATE;

    //Wait for module to become idle before issuing stop
    if (I2C_WaitIdle() == ERROR) {
        return ERROR;
    }

    I2C1CONbits.PEN = 1;
    //Wait until PEN goes low
    while (I2C1CONbits.PEN) {
        if (--timeout == 0u) {
            return ERROR;
        }
    }
    return SUCCESS;
}

//Transmits one byte on the I2C bus
static int TransmitOneByte(unsigned char data)
{
    unsigned int timeout = I2C_DEFAULT_RATE;

    //Wait until hardware is ready for a new transmit byte
    if (I2C_WaitIdle() == ERROR) {
        return ERROR;
    }

    //Clear old collision flags before attempting transmit
    I2C1STATbits.IWCOL = 0;
    I2C1STATbits.BCL = 0;

    //Load transmit register; hardware will shift this byte onto SDA
    I2C1TRN = data;

    //Wait until hardware finishes sending the byte
    while (I2C1STATbits.TRSTAT) {
        if (--timeout == 0u) {
            return ERROR;
        }
    }

    //If a collision happened during transmit, fail
    if (I2C1STATbits.IWCOL || I2C1STATbits.BCL) {
        I2C1STATbits.IWCOL = 0;
        I2C1STATbits.BCL = 0;
        return ERROR;
    }

    //ACKSTAT = 1 means the slave did not acknowledge the byte
    return I2C1STATbits.ACKSTAT ? ERROR : SUCCESS;
}

//Receives one byte from the I2C bus
static int ReceiveOneByte(unsigned char *data, int nack)
{
    unsigned int timeout = I2C_DEFAULT_RATE;

    if (data == 0) {
        return ERROR;
    }

    //Wait until module is ready to begin receive
    if (I2C_WaitIdle() == ERROR) {
        return ERROR;
    }

    //Enable receive mode
    I2C1CONbits.RCEN = 1;

    //Wait until a byte actually arrives in the receive buffer
    while (!I2C1STATbits.RBF) {
        if (--timeout == 0u) {
            return ERROR;
        }
    }

    //Read the received byte out of the hardware buffer
    *data = I2C1RCV;

    //Wait until hardware is ready for ACK/NACK sequence
    if (I2C_WaitIdle() == ERROR) {
        return ERROR;
    }

    //Choose whether to ACK or NACK this received byte
    I2C1CONbits.ACKDT = nack ? 1 : 0;
    I2C1CONbits.ACKEN = 1;

    timeout = I2C_DEFAULT_RATE;
    while (I2C1CONbits.ACKEN) {
        if (--timeout == 0u) {
            return ERROR;
        }
    }

    return SUCCESS;
}

/*******************************************************************************
 * Functions
 ******************************************************************************/

//Initializes I2C1 for master mode at the requested clock rate
unsigned int I2C_Init(unsigned int Rate)
{
    static unsigned char initialized = 0;

    // Prevent reinitializing the module over and over
    if (initialized) {
        return Rate;
    }

    if (Rate == 0) {
        Rate = I2C_DEFAULT_RATE;
    }

    //Clear control and status before setup
    I2C1CON  = 0;
    I2C1STAT = 0;

    //Baud rate generator value
    I2C1BRG = (PBCLK / (2u * Rate)) - 2u;

    //Turn on the I2C module
    I2C1CONbits.ON = 1;

    initialized = 1;
    return Rate;
}

//Writes one byte of data to a 16-bit device register address, including
//control byte + address high byte + address low byte + data byte
unsigned char I2C_ByteWriteReg(unsigned char I2CAddress, uint16_t deviceRegisterAddress, uint8_t data)
{
    //Start write transaction
    if (I2C_StartTransfer(0) == ERROR) {
        return 0u;
    }

    //Send device address with write bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 0u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send upper 8 bits of 16-bit register/memory address
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress >> 8)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send lower 8 bits of 16-bit register/memory address
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress & 0x00FFu)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send actual data byte to be written
    if (TransmitOneByte((unsigned char)data) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //End write transaction
    I2C_StopTransfer();
    return 1u;
}

//Reads one byte from a 16-bit device register address, including
//write control + address high + address low + repeated start + read control
unsigned char I2C_ByteReadReg(unsigned char I2CAddress, uint16_t deviceRegisterAddress, uint8_t *data)
{
    if (data == 0) {
        return 0u;
    }

    //Start by writing the address we want to read from
    if (I2C_StartTransfer(0) == ERROR) {
        return 0u;
    }

    //Send device address with write bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 0u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send upper address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress >> 8)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send lower address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress & 0x00FFu)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Repeated start switches from address-write phase to read phase
    if (I2C_StartTransfer(1) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send device address with read bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 1u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Receive exactly one byte and NACK it since this is the last byte
    if (ReceiveOneByte((unsigned char *)data, 1) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //End read transaction
    I2C_StopTransfer();
    return 1u;
}

//Writes multiple bytes starting at a 16-bit device register address.
unsigned char I2C_PageWriteReg(unsigned char I2CAddress, uint16_t deviceRegisterAddress, uint8_t *data, uint8_t length)
{
    uint8_t i;
    uint16_t pageOffset;

    if (data == 0) {
        return 0u;
    }

    if ((length == 0u) || (length > PAGE_SIZE)) {
        return 0u;
    }

    //Make sure this write will not wrap around inside a single EEPROM page
    pageOffset = (uint16_t)(deviceRegisterAddress & (PAGE_SIZE - 1u));
    if ((pageOffset + length) > PAGE_SIZE) {
        return 0u;
    }

    //Start write transaction
    if (I2C_StartTransfer(0) == ERROR) {
        return 0u;
    }

    //Send device address with write bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 0u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send upper address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress >> 8)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send lower address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress & 0x00FFu)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send each byte from the caller's buffer
    for (i = 0; i < length; i++) {
        if (TransmitOneByte((unsigned char)data[i]) == ERROR) {
            I2C_StopTransfer();
            return 0u;
        }
    }

    //End write transaction
    I2C_StopTransfer();
    return 1u;
}

//Reads multiple bytes starting at a 16-bit device register address.
unsigned char I2C_PageReadReg(unsigned char I2CAddress, uint16_t deviceRegisterAddress, uint8_t *data, uint8_t length)
{
    uint8_t i;

    if (data == 0) {
        return 0u;
    }

    if (length == 0u) {
        return 0u;
    }

    //Start by writing the address to read from
    if (I2C_StartTransfer(0) == ERROR) {
        return 0u;
    }

    //Send device address with write bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 0u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send upper address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress >> 8)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send lower address byte
    if (TransmitOneByte((unsigned char)(deviceRegisterAddress & 0x00FFu)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Repeated start begins the read phase
    if (I2C_StartTransfer(1) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Send device address with read bit
    if (TransmitOneByte((unsigned char)((I2CAddress << 1u) | 1u)) == ERROR) {
        I2C_StopTransfer();
        return 0u;
    }

    //Read bytes one at a time; ACK all but the last, NACK the last
    for (i = 0; i < length; i++) {
        int nack = (i == (uint8_t)(length - 1u)) ? 1 : 0;

        if (ReceiveOneByte((unsigned char *)&data[i], nack) == ERROR) {
            I2C_StopTransfer();
            return 0u;
        }
    }

    // End read transaction
    I2C_StopTransfer();
    return 1u;
}

//Reads a 16-bit integer from two consecutive device addresses
int I2C_ReadInt(char I2CAddress, char deviceRegisterAddress, char isBigEndian)
{
    unsigned char first = 0u;
    unsigned char second = 0u;

    //Read first byte
    if (!I2C_ByteReadReg((unsigned char)I2CAddress, (uint16_t)(unsigned char)deviceRegisterAddress, &first)) {
        return 0;
    }

    //Read second byte from the next address
    if (!I2C_ByteReadReg((unsigned char)I2CAddress, (uint16_t)(unsigned char)(deviceRegisterAddress + 1), &second)) {
        return 0;
    }

    //Combine bytes according to requested endianness
    if (isBigEndian) {
        return (((int)first) << 8) | (int)second;
    } else {
        return (((int)second) << 8) | (int)first;
    }
}