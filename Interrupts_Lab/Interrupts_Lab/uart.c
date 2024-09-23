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
#define TX_BUFSIZE 64 /* Size transmit buffer used when TX interrupts are on */

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
void* usart_init(uint8_t, uint32_t, bool*);
void usart_enable_interrupt(void*);
void usart_disable_interrupt(void*);
void usart_transmit_data(void*, char);
void usart_wait_until_transmit_ready(void*);
int usart_receive_data(void*);
bool in_isr(void*);

#if USE_TX_INTERRUPTS
  typedef struct {
	void* usart;
	int t_put_index;	   // buffer position at which you take out data
	volatile int t_get_index;  // buffer position at which you insert data
	char t_buffer[TX_BUFSIZE]; // circular buffer used to store tx data
  } USART_TXINT_t;

  USART_TXINT_t** txints = NULL;
  int num_txints = 0;

  /* the MCU specific code should implement the TX ISR
     which will then call this function */
  void handle_tx_isr(int usartnum)
  {
      USART_TXINT_t* txint = txints[usartnum];
      // check if the buffer is not empty
      if (txint->t_get_index != txint->t_put_index) {
          // transmit the byte at the top of the buffer
          usart_transmit_data(txint->usart, txint->t_buffer[txint->t_get_index]);
          // increment buffer pointer
          txint->t_get_index = (txint->t_get_index + 1) % TX_BUFSIZE;
          // if the buffer is empty, disable the interrupt
          if (txint->t_get_index == txint->t_put_index) {
              usart_disable_interrupt(txint->usart);
          }
      }
  }
#endif

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

    bool use_tx_intr;
    void* usart = usart_init(usartnum, baud_rate, &use_tx_intr);
    fdev_set_udata(stream, usart);

#if USE_TX_INTERRUPTS
    if (use_tx_intr) {
        USART_TXINT_t* txint = (USART_TXINT_t*)malloc(sizeof(USART_TXINT_t));
        txint->t_get_index = txint->t_put_index = 0;
        txint->usart = usart;
        if (usartnum >= num_txints) {
            num_txints = usartnum + 1;
            txints = (USART_TXINT_t**)realloc(txints, num_txints * sizeof(USART_TXINT_t*));
        }
        txints[usartnum] = txint;
        txint = (void*)(((uint16_t)txint) | 0x8000); /* hack to use unused upper bit of address to indicate use of tx intr */
        fdev_set_udata(stream, txint);
    }
#endif

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

    void* udata = fdev_get_udata(stream);
#if USE_TX_INTERRUPTS
    if(((uint16_t)udata) & 0x8000) {
        USART_TXINT_t* txint = (USART_TXINT_t*)(((uint16_t)udata) & 0x7FFF);
        // increment ring buffer index
        int new_put_index = (txint->t_put_index + 1) % TX_BUFSIZE;
        if (new_put_index == txint->t_get_index && in_isr(udata)) {
            // if we wrapped around, and we are in an interrupt handler 
            // the tx_isr won't get called and the buffer won't get emptied.
            // so don't do anything and just drop the character
        } else {
            // put character in buffer
            txint->t_buffer[txint->t_put_index] = c;

            // if we wrapped around, wait till the buffer gets emptied
            // and get_index increments
            while (new_put_index == txint->t_get_index);
            txint->t_put_index = new_put_index;
		
            // enable interrupt and the character will be sent in the ISR
            usart_enable_interrupt(txint->usart);
        }
    } else 
#endif
    {
        usart_wait_until_transmit_ready(udata);
        usart_transmit_data(udata, c);
    }

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
            void* udata = fdev_get_udata(stream);
#if USE_TX_INTERRUPTS
            if((uint16_t)udata & 0x8000) {
                USART_TXINT_t* txint = (USART_TXINT_t*)(((uint16_t)udata) & 0x7FFF);
                udata = txint->usart;
            }
#endif
            c = usart_receive_data(udata);

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

