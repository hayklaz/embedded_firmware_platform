/* 
 * File:   NonVolatileMemory.c
 * Author: Hayk Lazaryan
 *
 * Created on March 10, 2026, 11:53pm
 */

// Microchip Libraries
#include <xc.h>
#include <sys/attribs.h>

// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "NonVolatileMemory.h"
#include "I2C.h"
#include "MessageIDs.h"
#include "BOARD.h"
#include "Uart.h"
#include "Protocol2.h"

/*******************************************************************************
 * Defines
 ******************************************************************************/

//24LC256 EEPROM I2C 7-bit address with all address pins tied low
#define EEPROM_ADDR_7BIT    ((unsigned char)0x50u)

//Total EEPROM capacity in bytes
#define EEPROM_SIZE_BYTES   32768u

//Page size in bytes
#define PAGE_SIZE 64u

//Total number of pages in EEPROM
#define EEPROM_NUM_PAGES    (EEPROM_SIZE_BYTES / PAGE_SIZE)

/*******************************************************************************
 * Helpers
 ******************************************************************************/

//Clamps a byte address into the valid EEPROM address range [0, 32767]
static int NormalizeByteAddress(int address)
{
    if (address < 0) {
        address = 0;
    }
    return address % (int)EEPROM_SIZE_BYTES;
}

//Converts a page index into its starting byte address
static int NormalizePageAddress(int pageIndex)
{
    if (pageIndex < 0) {
        pageIndex = 0;
    }
    pageIndex = pageIndex % (int)EEPROM_NUM_PAGES;
    return pageIndex * (int)PAGE_SIZE;
}

/*******************************************************************************
 * Functions
 ******************************************************************************/

//Initializes the NonVolatileMemory module
int NonVolatileMemory_Init(void)
{
    I2C_Init(I2C_DEFAULT_RATE);
    return SUCCESS;
}

//Reads one byte from a byte address in EEPROM
//Returns 0xFF if the I2C transaction fails
unsigned char NonVolatileMemory_ReadByte(int address)
{
    unsigned char data = 0xFFu;

    //Force requested address into valid EEPROM range
    address = NormalizeByteAddress(address);

    //Use I2C byte-read function to read one EEPROM byte
    if (I2C_ByteReadReg(EEPROM_ADDR_7BIT, (uint16_t)address, &data) == ERROR) {
        return 0xFFu;
    }

    return data;
}

//Writes one byte to a byte address in EEPROM
char NonVolatileMemory_WriteByte(int address, unsigned char data)
{
    //Force requested address into valid EEPROM range
    address = NormalizeByteAddress(address);

    //Use I2C byte-write function to write one EEPROM byte
    if (I2C_ByteWriteReg(EEPROM_ADDR_7BIT, (uint16_t)address, data) == ERROR) {
        return (char)ERROR;
    }

    return (char)SUCCESS;
}

//Reads up to 64 bytes from memory starting at the beginning of a page
int NonVolatileMemory_ReadPage(int page, char length, unsigned char data[])
{
    int byteAddress;

    //Validate output buffer and requested length
    if ((data == 0) || (length <= 0) || (length > (char)PAGE_SIZE)) {
        return ERROR;
    }

    //Convert page index into byte address of that page's first byte
    byteAddress = NormalizePageAddress(page);

    //Use page-read I2C function to fill caller's buffer
    if (I2C_PageReadReg(EEPROM_ADDR_7BIT, (uint16_t)byteAddress, data, (uint8_t)length) == ERROR) {
        return ERROR;
    }

    return SUCCESS;
}

//Writes up to 64 bytes into memory starting at the beginning of a page
int NonVolatileMemory_WritePage(int page, char length, unsigned char data[])
{
    int byteAddress;

    //Validate input buffer and requested length
    if ((data == 0) || (length <= 0) || (length > (char)PAGE_SIZE)) {
        return ERROR;
    }

    //Convert page index into byte address of that page's first byte
    byteAddress = NormalizePageAddress(page);

    //Use page-write I2C function to send caller's buffer to EEPROM
    if (I2C_PageWriteReg(EEPROM_ADDR_7BIT,  (uint16_t)byteAddress, data, (uint8_t)length) == ERROR) {
        return ERROR;
    }

    return SUCCESS;
}


#ifdef NVM_TESTHARNESS

//Test harness for testing NVM
int main(void)
{
    uint8_t       pktType;
    uint8_t       pktLen;
    unsigned char msg[MAXPAYLOADLENGTH];
    char          dbg[80];
    unsigned char dummy = 0u;

    //Initialize protocol/UART stack and NVM module
    Protocol_Init(BAUD);
    NonVolatileMemory_Init();

    //Send startup debug message so host knows code is running
    sprintf(dbg, "NVM Test Harness %s %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(dbg);

    while (1) {
        //Continuously pull UART data into the protocol packet queue
        Protocol_QueuePacket();

        //Process all complete packets currently waiting
        while (Protocol_GetInPacket(&pktType, &pktLen, msg) == SUCCESS) {

            switch (pktType) {
                //Reading a byte
                case ID_NVM_READ_BYTE: {
                    //Check packet length for correctness
                    if (pktLen < 4u) {
                        break;
                    }

                    unsigned int addrBE;
                    memcpy(&addrBE, msg, 4);

                    //Convert incoming big-endian byte address into local format
                    unsigned int addrLE = convertEndian(&addrBE);

                    //Read one byte from EEPROM and send it back
                    unsigned char val = NonVolatileMemory_ReadByte((int)addrLE);
                    Protocol_SendPacket(1u, (unsigned char)ID_NVM_READ_BYTE_RESP, &val);
                    break;
                }

                //Writing a byte
                case ID_NVM_WRITE_BYTE: {
                    //Check packet length for correctness
                    if (pktLen < 5u) {
                        break;
                    }

                    unsigned int addrBE;
                    memcpy(&addrBE, msg, 4);

                    // Convert incoming big-endian byte address into local format
                    unsigned int addrLE = convertEndian(&addrBE);
                    unsigned char data = msg[4];

                    // Write requested byte and acknowledge on success
                    if (NonVolatileMemory_WriteByte((int)addrLE, data) == SUCCESS) {
                        Protocol_SendPacket(0u, (unsigned char)ID_NVM_WRITE_BYTE_ACK, &dummy);
                    } else {
                        Protocol_SendDebugMessage("WriteByte FAILED");
                    }
                    break;
                }

                //Reading a page
                case ID_NVM_READ_PAGE: {
                    //Check packet length for correctness
                    if (pktLen < 4u) {
                        break;
                    }

                    unsigned int pageBE;
                    memcpy(&pageBE, msg, 4);

                    //Convert incoming big-endian page index into local format
                    unsigned int pageLE = convertEndian(&pageBE);

                    //Prepare page buffer and fill it with read data
                    unsigned char pageData[PAGE_SIZE];
                    memset(pageData, 0xFFu, sizeof(pageData));

                    NonVolatileMemory_ReadPage((int)pageLE, (char)PAGE_SIZE, pageData);

                    //Send the full page back to host
                    Protocol_SendPacket((unsigned char)PAGE_SIZE, (unsigned char)ID_NVM_READ_PAGE_RESP, pageData);
                    break;
                }

                //Writing a page
                case ID_NVM_WRITE_PAGE: {
                    //Check packet length for correctness
                    if (pktLen < (uint8_t)(4u + PAGE_SIZE)) {
                        break;
                    }

                    unsigned int pageBE;
                    memcpy(&pageBE, msg, 4);

                    //Convert incoming big-endian page index into local format
                    unsigned int pageLE = convertEndian(&pageBE);

                    //Write provided 64-byte payload into requested EEPROM page
                    if (NonVolatileMemory_WritePage((int)pageLE, (char)PAGE_SIZE, &msg[4]) == SUCCESS) {
                        Protocol_SendPacket(0u, (unsigned char)ID_NVM_WRITE_PAGE_ACK, &dummy);
                    } else {
                        Protocol_SendDebugMessage("WritePage FAILED");
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return 0;
}

#endif /* NVM_TESTHARNESS */