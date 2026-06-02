/* 
 * File:   Uart.h
 * Author: Hayk Lazaryan
 * 
 * Created on February 24, 2026, 11:57pm
 */

#ifndef UART_H
#define UART_H

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

// Standard libraries
#include <stdio.h>
#include <stdlib.h>

// Define SYSCLK, PBCLK, and Baud clocks
#define SYSCLK  80000000UL
#define PBCLK   SYSCLK/2
#define BAUD    115200UL

//Define Buffer sizes for TX and RX
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

//Initialize UART, interrupts, and buffers
void UART_Init(void);

//Blocking transmit of one character over UART
void UART_WriteChar(char c);

//Blocking receive of one character over UART
char UART_ReadChar(void);


//Nonblocking functions that access the buffer
int  UART_TryWriteChar(char c);           // returns 1 if queued, 0 if TX buffer full
int  UART_ReadCharNonBlocking(char *out); // returns 1 if got char, 0 if none available
uint16_t UART_RxBytesAvailable(void);     // how many bytes waiting in RX buffer

//Allows printf() to send characters through UART
void _mon_putc(char c);

#endif // UART_H