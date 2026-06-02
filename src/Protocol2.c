/* 
 * File:   Protocol2.c
 * Author: Hayk Lazaryan
 * 
 * Created on March 5, 2026, 1:36am
 */

#include "Protocol2.h"
#include "MessageIDs.h"
#include "Uart.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

// Standard libraries
#include <stdio.h>
#include <stdlib.h>

// String library
#include <string.h>


/*******************************************************************************
 * Defines
 ******************************************************************************/

#ifndef PROTOCOL_HEAD
#define PROTOCOL_HEAD 0xCC
#endif

#ifndef PROTOCOL_TAIL
#define PROTOCOL_TAIL 0xB9
#endif

#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef ERROR
#define ERROR 0
#endif

#ifndef WAITING
#define WAITING 0
#endif

//Circular buffer of packets (queued incoming packets)
static volatile rxpT packetBuf[PACKETBUFFERSIZE];
static volatile uint8_t pktHead = 0;
static volatile uint8_t pktTail = 0;
static volatile uint8_t pktCount = 0;

//Working packet under construction (state machine output)
static rxpT buildingPacket;

/*******************************************************************************
 * Private Helper Functions
 ******************************************************************************/

//Increments idx and wraps to 0 at size to implement circular-buffer indexing
static inline uint8_t NextIndex8(uint8_t idx, uint8_t size) {
    idx++;
    if (idx >= size) idx = 0;
    return idx;
}

//Push a packet into the circular buffer. Returns 1 on success, 0 if full
static int PacketBuffer_Push(const rxpT *p)
{
    if (pktCount >= PACKETBUFFERSIZE) {
        return 0;
    }
    packetBuf[pktTail] = *p;
    pktTail = NextIndex8(pktTail, PACKETBUFFERSIZE);
    pktCount++;
    return 1;
}

//Pop a packet from the circular buffer. Returns 1 on success, 0 if empty
static int PacketBuffer_Pop(rxpT *out)
{
    if (pktCount == 0) {
        return 0;
    }
    *out = packetBuf[pktHead];
    pktHead = NextIndex8(pktHead, PACKETBUFFERSIZE);
    pktCount--;
    return 1;
}

//Peek next packet ID without popping. Returns 0 if empty
static uint8_t PacketBuffer_PeekID(void)
{
    if (pktCount == 0) {
        return 0;
    }
    return packetBuf[pktHead].ID;
}

//Write a character using UART driver
static inline void Protocol_PutChar(char c)
{
    UART_WriteChar(c);
}

/*******************************************************************************
 * Public Protocol Functions
 ******************************************************************************/

int Protocol_Init(unsigned long baudrate)
{
    (void)baudrate;
    UART_Init();
    LEDS_INIT();
    flushPacketBuffer();
    return SUCCESS;
}

unsigned char Protocol_CalcIterativeChecksum(unsigned char charIn, unsigned char curChecksum)
{
    curChecksum = (curChecksum >> 1) + (curChecksum << 7);
    curChecksum = (unsigned char)(curChecksum + charIn);
    return curChecksum;
}

//Builds a packet from incoming UART bytes. State machine output is stored
//in rxPacket when a full valid packet is received.
//Returns 1 if a complete valid packet was built into *rxPacket, else 0.
uint8_t BuildRxPacket(rxpADT rxPacket, unsigned char reset)
{
    //HEAD, LENGTH + ID, payload bytes, TAIL, CHECKSUM, \r, \n
    enum {
        WAIT_HEAD = 0,
        WAIT_LEN,
        WAIT_PAYLOAD,
        WAIT_TAIL,
        WAIT_CHECKSUM,
        WAIT_CR,
        WAIT_LF
    };

    static uint8_t state = WAIT_HEAD;
    static uint8_t payloadIndex = 0;
    static uint8_t expectedLen = 0;
    static unsigned char checksum = 0;

    if (reset) {
        state = WAIT_HEAD;
        payloadIndex = 0;
        expectedLen = 0;
        checksum = 0;
        return 0;
    }

    //Pull bytes from UART driver as they arrive. We keep processing until
    //we either complete a packet or run out of bytes
    char c;
    while (UART_ReadCharNonBlocking(&c)) {
        uint8_t b = (uint8_t)c;

        switch (state) {

            case WAIT_HEAD:
                if (b == PROTOCOL_HEAD) {
                    state = WAIT_LEN;
                }
                break;

            case WAIT_LEN:
                expectedLen = b;              // payload length including ID
                payloadIndex = 0;
                checksum = 0;

                //Length must be at in range 1 <= expectedLen <= MAXPAYLOADLENGTH
                if (expectedLen == 0 || expectedLen > MAXPAYLOADLENGTH) {
                    state = WAIT_HEAD;
                } else {
                    state = WAIT_PAYLOAD;
                }
                break;

            case WAIT_PAYLOAD:
                rxPacket->payLoad[payloadIndex] = b;
                checksum = Protocol_CalcIterativeChecksum(b, checksum);

                payloadIndex++;

                if (payloadIndex >= expectedLen) {
                    state = WAIT_TAIL;
                }
                break;

            case WAIT_TAIL:
                if (b == PROTOCOL_TAIL) {
                    state = WAIT_CHECKSUM;
                } else {
                    //Bad tail means discard packet
                    state = WAIT_HEAD;
                }
                break;

            case WAIT_CHECKSUM:
                rxPacket->checkSum = b;
                rxPacket->len = expectedLen;

                //First byte of payload is ID
                rxPacket->ID = rxPacket->payLoad[0];

                //Verify checksum
                if (b == checksum) {
                    state = WAIT_CR;
                } else {
                    state = WAIT_HEAD;
                }
                break;

            case WAIT_CR:
                if (b == '\r') {
                    state = WAIT_LF;
                } else {
                    state = WAIT_HEAD;
                }
                break;

            case WAIT_LF:
                if (b == '\n') {
                    //Packet complete and valid
                    state = WAIT_HEAD;
                    return 1;
                } else {
                    state = WAIT_HEAD;
                }
                break;

            default:
                state = WAIT_HEAD;
                break;
        }
    }

    return 0;
}

//Called repeatedly in main loop to build packets from UART stream and queue them.
//Returns 1 if packet buffer is full (could not queue a packet), else 0
uint8_t Protocol_QueuePacket(void)
{
    //Try to build packets until no complete packet exists right now
    while (BuildRxPacket(&buildingPacket, 0)) {

        //Native messages that never go on stack
        if (buildingPacket.ID == ID_LEDS_SET) {
            //payload[1] contains LED bitmask (payload length should be 2)
            if (buildingPacket.len >= 2) {
                LEDS_SET(buildingPacket.payLoad[1]);
            }
            continue;
        }

        if (buildingPacket.ID == ID_LEDS_GET) {
            //Respond immediately with LEDS_STATE message
            uint8_t leds = (uint8_t)LEDS_GET();
            (void)Protocol_SendPacket(1, ID_LEDS_STATE, &leds);
            continue;
        }

        //Non-native packets go into packet buffer
        if (!PacketBuffer_Push(&buildingPacket)) {
            //Buffer full
            return 1;
        }
    }

    return 0;
}

//Pops next queued packet. Returns SUCCESS if one was returned, WAITING otherwise.
int Protocol_GetInPacket(uint8_t *type, uint8_t *len, unsigned char *msg)
{
    rxpT p;

    if (!PacketBuffer_Pop(&p)) {
        return WAITING;
    }

    *type = p.ID;

    // p.len includes the ID byte at payLoad[0]
    if (p.len > 1) {
        *len = (uint8_t)(p.len - 1);
        memcpy(msg, &p.payLoad[1], *len);
    } else {
        *len = 0;
    }

    return SUCCESS;
}

unsigned char Protocol_ReadNextPacketID(void)
{
    return PacketBuffer_PeekID();
}

void flushPacketBuffer(void)
{
    __builtin_disable_interrupts();
    pktHead = pktTail = pktCount = 0;
    __builtin_enable_interrupts();
}

int Protocol_SendPacket(unsigned char len, unsigned char ID, void *Payload)
{
    unsigned char checksum = 0;
    unsigned char payloadLen = (unsigned char)(len + 1);
    unsigned char *p = (unsigned char *)Payload;

    if (payloadLen == 0 || payloadLen > MAXPAYLOADLENGTH) {
        return ERROR;
    }

    Protocol_PutChar((char)PROTOCOL_HEAD);
    Protocol_PutChar((char)payloadLen);

    checksum = Protocol_CalcIterativeChecksum((unsigned char)ID, checksum);
    Protocol_PutChar((char)ID);

    for (unsigned char i = 0; i < len; i++) {
        checksum = Protocol_CalcIterativeChecksum(p[i], checksum);
        Protocol_PutChar((char)p[i]);
    }

    Protocol_PutChar((char)PROTOCOL_TAIL);
    Protocol_PutChar((char)checksum);
    Protocol_PutChar('\r');
    Protocol_PutChar('\n');

    return SUCCESS;
}

int Protocol_SendDebugMessage(char *Message)
{
    unsigned char n = (unsigned char)strlen(Message);
    return Protocol_SendPacket(n, (unsigned char)ID_DEBUG, Message);
}

unsigned int convertEndian(unsigned int *x)
{
    unsigned int v = *x;
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
}

/*******************************************************************************
 * PROTOCOL TEST HARNESS
 ******************************************************************************/
#ifdef PROTOCOL_TESTHARNESS
int main(void)
{
    Protocol_Init(BAUD);

    char dbg[128];
    sprintf(dbg, "Protocol2 running: %s %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(dbg);

    while (1) {
        Protocol_QueuePacket();

        uint8_t id, len;
        unsigned char msg[MAXPAYLOADLENGTH];

        while (Protocol_GetInPacket(&id, &len, msg) == SUCCESS) {

            if (id == ID_PING) {
                //Expect 4-byte big-endian unsigned int
                if (len == 4) {
                    unsigned int netVal;
                    memcpy(&netVal, msg, 4);

                    unsigned int little = convertEndian(&netVal);
                    little >>= 1; //divide by 2

                    unsigned int outNet = convertEndian(&little);
                    Protocol_SendPacket(4, (unsigned char)ID_PONG, &outNet);
                } else {
                    Protocol_SendDebugMessage("Ping received with wrong length");
                }
            }
        }
    }
}
#endif