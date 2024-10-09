/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART implementation
 *
 * $Id: uart.c,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 * moved MCU specific code into a separate file jchandy 2024/06/26
 *
 * Mod for mega644 BRL Jan2009
 */

#define RX_BUFSIZE 80 /* Size of internal line buffer used by uart_getchar() */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "uart.h"

#ifdef __XC8__
  static FILE uartFile = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, F_PERM);
  /* on gcc, we can put stream-specific udata in the FILE struct.  XC8 doesn't
     allow that, so we have to create a global udata which means that we can
     have only one stream at a time 
   */
  static void* _udata; 
  #define fdev_set_udata(stream,u) _udata=u
  #define fdev_get_udata(stream) _udata
#else
  static FILE uartFile = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
#endif

/* the following functions must be implemented by the MCU specific USART code */
void* usart_init(uint8_t, uint32_t);
void usart_transmit_data(void*, char);
void usart_wait_until_transmit_ready(void*);
int usart_receive_data(void*);

/*
 * Initialize the UART to 9600 Bd, tx/rx, 8N1.
 */
FILE*
uart_init(uint8_t usartnum, uint32_t baud_rate, FILE* stream)
{
	if (stream) {
		*stream = uartFile;
	} else {
		stdout = &uartFile;
		stdin = &uartFile;
		stderr = &uartFile;
		stream = &uartFile;
	}

	void* usart = usart_init(usartnum, baud_rate);
	fdev_set_udata(stream, usart);
	  
	return stream;
}

/*
 * Send character c down the UART Tx, wait until tx holding register
 * is empty.
 */
int
uart_putchar(char c, FILE *stream)
{
	if (c == '\a') {
		fputs("*ring*\n", stderr);
		return 0;
	}

	if (c == '\n') {
		uart_putchar('\r', stream);
	}

	void* usart = fdev_get_udata(stream);
	usart_wait_until_transmit_ready(usart);
	usart_transmit_data(usart, c);

	return 0;
}

/*
 * Receive a character from the UART Rx.
 *
 * This features a simple line-editor that allows to delete and
 * re-edit the characters entered, until either CR or NL is entered.
 * Printable characters entered will be echoed using uart_putchar().
 *
 * Editing characters:
 *
 * . \b (BS) or \177 (DEL) delete the previous character
 * . ^u kills the entire input buffer
 * . ^w deletes the previous word
 * . ^r sends a CR, and then reprints the buffer
 * . \t will be replaced by a single space
 *
 * All other control characters will be ignored.
 *
 * The internal line buffer is RX_BUFSIZE (80) characters long, which
 * includes the terminating \n (but no terminating \0).  If the buffer
 * is full (i. e., at RX_BUFSIZE-1 characters in order to keep space for
 * the trailing \n), any further input attempts will send a \a to
 * uart_putchar() (BEL character), although line editing is still
 * allowed.
 *
 * Input errors while talking to the UART will cause an immediate
 * return of -1 (error indication).  Notably, this will be caused by a
 * framing error (e. g. serial line "break" condition), by an input
 * overrun, and by a parity error (if parity was enabled and automatic
 * parity recognition is supported by hardware).
 *
 * Successive calls to uart_getchar() will be satisfied from the
 * internal buffer until that buffer is emptied again.
 */
int
uart_getchar(FILE *stream)
{
	uint8_t c;
	char *cp, *cp2;
	static char b[RX_BUFSIZE];
	static char *rxp;

	if (rxp == 0) {
		for (cp = b;;) {
			void* usart = fdev_get_udata(stream);
			c = usart_receive_data(usart);

			/* behaviour similar to Unix stty ICRNL */
			if (c == '\r')
				c = '\n';
			if (c == '\n') {
			    *cp = c;
				uart_putchar(c, stream);
			    rxp = b;
				break;
			}
			else if (c == '\t')
				c = ' ';

			if ((c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') ||
				c >= (uint8_t)'\xa0') {
			    if (cp == b + RX_BUFSIZE - 1)
					uart_putchar('\a', stream);
				else {
					*cp++ = c;
					uart_putchar(c, stream);
				}
			    continue;
			}

			switch (c) {
				case 'c' & 0x1f:
				    return -1;

				case '\b':
				case '\x7f':
				    if (cp > b) {
						uart_putchar('\b', stream);
						uart_putchar(' ', stream);
						uart_putchar('\b', stream);
						cp--;
					}
					break;

				case 'r' & 0x1f:
					uart_putchar('\r', stream);
					for (cp2 = b; cp2 < cp; cp2++)
						uart_putchar(*cp2, stream);
					break;

				case 'u' & 0x1f:
					while (cp > b) {
						uart_putchar('\b', stream);
						uart_putchar(' ', stream);
						uart_putchar('\b', stream);
						cp--;
					}
					break;

				case 'w' & 0x1f:
					while (cp > b && cp[-1] != ' ') {
						uart_putchar('\b', stream);
						uart_putchar(' ', stream);
						uart_putchar('\b', stream);
						cp--;
					}
					break;
			}
		}
	}

	c = *rxp++;
	if (c == '\n')
		rxp = 0;

	return c;
}

