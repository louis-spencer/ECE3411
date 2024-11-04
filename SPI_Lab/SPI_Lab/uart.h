#ifndef UART_H_
#define UART_H_

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART declarations
 *
 * $Id: uart.h,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 */
#include <stdint.h>
#include <stdio.h>

/*
 * Perform UART startup initialization.
 */
FILE*	uart_init(uint8_t usart_num, uint32_t baud_rate, FILE* stream);

/*
 * Send one character to the UART.
 */
int	uart_putchar(char c, FILE *stream);

/*
 * Receive one character from the UART.  The actual reception is line-buffered,
 * and one character is returned from the buffer at each invocation.
 */
int	uart_getchar(FILE *stream);

#ifdef __XC8__
#define _FDEV_EOF -2
#define _FDEV_ERR -1
#endif

#endif /* UART_H_ */
