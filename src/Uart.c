/* 
 * File:   Uart.c
 * Author: Hayk Lazaryan
 * 
 * Created on February 24, 2026, 11:56pm
 */

#include "Uart.h"

/*******************************************************************************
 * Buffers
 ******************************************************************************/

//RX circular buffer (filled by RX interrupt, read by main code)
static volatile uint8_t  rxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;
static volatile uint16_t rxCount = 0;

//TX circular buffer (filled by main code, drained by TX interrupt)
static volatile uint8_t  txBuffer[UART_TX_BUFFER_SIZE];
static volatile uint16_t txHead = 0;
static volatile uint16_t txTail = 0;
static volatile uint16_t txCount = 0;

/*******************************************************************************
 * Private Helper Functions
 ******************************************************************************/

//Returns the next index in a circular buffer
static inline uint16_t NextIndex(uint16_t idx, uint16_t size)
{
    idx++;
    if (idx >= size) idx = 0;
    return idx;
}

//Enable TX interrupt and force it to run so transmission starts immediately
static inline void UART_KickTx(void)
{
    IEC0bits.U1TXIE = 1;     // enable TX interrupt
    IFS0bits.U1TXIF = 1;     // trigger it ASAP
}

/*******************************************************************************
 * UART Functions
 ******************************************************************************/

//Initializes UART for 8-N-1 at the 115200Hz baud rate.
//Also initializes interrupts and circular buffers.
void UART_Init(void)
{
    //Disable interrupts while setting up
    __builtin_disable_interrupts();

    
    //Initialize buffer state
    rxHead = rxTail = rxCount = 0;
    txHead = txTail = txCount = 0;

    //Disable UART before configuring
    U1MODE = 0;
    U1STA  = 0;

    //High-speed mode (BRGH=1) gives lower baud error at 40MHz PBCLK
    U1MODEbits.BRGH = 1;

    //U1BRG = (PBCLK / (4 * BAUD)) - 1  when BRGH is 1
    U1BRG = (PBCLK / (4UL * BAUD)) - 1UL;

    //Using 8-N-1
    U1MODEbits.PDSEL = 0;   // 8-bit, no parity
    U1MODEbits.STSEL = 0;   // 1 stop bit

    //Interrupt trigger settings:
    //URXISEL = 0 -> interrupt when RX buffer has at least 1 character
    //UTXISEL = 0 -> interrupt when TX buffer becomes empty
    U1STAbits.URXISEL = 0;
    U1STAbits.UTXISEL = 0;

    //Enable UART and enable TX/RX
    U1MODEbits.ON   = 1;
    U1STAbits.URXEN = 1;
    U1STAbits.UTXEN = 1;

    //Multi-vector interrupts
    INTCONbits.MVEC = 1;

    //Set UART1 interrupt priority/subpriority
    IPC6bits.U1IP = 2;
    IPC6bits.U1IS = 0;

    //Clear interrupt flags
    IFS0bits.U1RXIF = 0;
    IFS0bits.U1TXIF = 0;

    //Enable RX interrupt always; TX interrupt will be enabled when we have data to send
    IEC0bits.U1RXIE = 1;
    IEC0bits.U1TXIE = 0;
    
    __builtin_enable_interrupts();
}

//Blocking transmit of one character over UART.
//Waits until there is space in the TX buffer, then queues the byte.
void UART_WriteChar(char c)
{
    while (!UART_TryWriteChar(c)); // wait until TX buffer has space
}

//Blocking receive of one character from UART.
//Waits until a byte is available in the RX buffer, then returns it.
char UART_ReadChar(void)
{
    char c;
    while (!UART_ReadCharNonBlocking(&c)); // wait until a non-null char is read
    return c;
}

//Nonblocking receive: returns 1 and stores byte in *out if available, else 0.
int UART_ReadCharNonBlocking(char *out)
{
    int success = 0;

    __builtin_disable_interrupts();

    if (rxCount > 0) {
        *out = (char)rxBuffer[rxHead];
        rxHead = NextIndex(rxHead, UART_RX_BUFFER_SIZE);
        rxCount--;
        success = 1;
    }

    __builtin_enable_interrupts();

    return success;
}

//Nonblocking transmit: tries to queue one byte into the TX buffer.
//Returns 1 if queued, 0 if TX buffer is full.
int UART_TryWriteChar(char c)
{
    int success = 0;

    __builtin_disable_interrupts();

    if (txCount < UART_TX_BUFFER_SIZE) {
        txBuffer[txTail] = (uint8_t)c;
        txTail = NextIndex(txTail, UART_TX_BUFFER_SIZE);
        txCount++;
        success = 1;
    }

    __builtin_enable_interrupts();

    //If we queued something, start TX interrupt so it drains the buffer
    if (success) {
        UART_KickTx();
    }

    return success;
}

//Returns number of bytes currently stored in RX buffer.
uint16_t UART_RxBytesAvailable(void)
{
    uint16_t n;

    __builtin_disable_interrupts();
    n = rxCount;
    __builtin_enable_interrupts();

    return n;
}

/*******************************************************************************
 * Interrupt Service Routine                                                  *
 ******************************************************************************/

//UART1 ISR handles both RX and TX events
void __ISR(_UART_1_VECTOR, IPL2SOFT) UART1_Handler(void)
{
    //RX interrupt, meaning data received
    if (IFS0bits.U1RXIF) {

        //If overrun error occurs, UART stops receiving until cleared
        if (U1STAbits.OERR) {
            U1STAbits.OERR = 0;
        }

        //Drain UART hardware FIFO into RX software buffer
        while (U1STAbits.URXDA) {
            uint8_t b = (uint8_t)U1RXREG;

            //Enqueue if space; if full, drop byte
            if (rxCount < UART_RX_BUFFER_SIZE) {
                rxBuffer[rxTail] = b;
                rxTail = NextIndex(rxTail, UART_RX_BUFFER_SIZE);
                rxCount++;
            }
        }

        IFS0bits.U1RXIF = 0;
    }

    //TX interrupt, meaning transmitter ready to accept bytes
    if (IFS0bits.U1TXIF) {

        //Fill UART TX register while we have bytes queued and HW FIFO isn't full
        while (!U1STAbits.UTXBF) {

            if (txCount == 0) {
                break; // nothing left to send
            }

            U1TXREG = txBuffer[txHead];
            txHead = NextIndex(txHead, UART_TX_BUFFER_SIZE);
            txCount--;
        }

        //If nothing left to send, disable TX interrupt until we queue new data
        if (txCount == 0) {
            IEC0bits.U1TXIE = 0;
        }

        IFS0bits.U1TXIF = 0;
    }
}

//Allows printf() to use UART for stdout
//Each character printed by printf() is sent through UART
void _mon_putc(char c)
{
    UART_WriteChar(c);
}

#ifdef UART_TEST_HARNESS
//Main function to check Uart.c for correctness
int main(void)
{
    UART_Init();

    printf("Hello World!\n");

    while (1)
    {
        char c = UART_ReadChar();
        printf("%c", c);
    }

    return 0;
}
#endif