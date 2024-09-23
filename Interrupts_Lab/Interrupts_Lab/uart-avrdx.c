/*
* AVRDx specific UART code
*/

/* CPU frequency */
#define F_CPU 16000000UL

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

#ifdef __XC8__
#define _FDEV_EOF -2
#define _FDEV_ERR -1
#endif

#define USE_TX_INTERRUPTS_0 0
#define USE_TX_INTERRUPTS_1 0
#define USE_TX_INTERRUPTS_2 0
#define USE_TX_INTERRUPTS_3 1
#define USE_TX_INTERRUPTS_4 0
#define USE_TX_INTERRUPTS_5 0

#if USE_TX_INTERRUPTS
  #if !(USE_TX_INTERRUPTS_0 || USE_TX_INTERRUPTS_1 || USE_TX_INTERRUPTS_2 || USE_TX_INTERRUPTS_3 || USE_TX_INTERRUPTS_4 || USE_TX_INTERRUPTS_5)
    #error "if USE_TX_INTERRUPTS is set, set one of the USE_TX_INTERRUPTS_n flags"
  #endif

  extern void handle_tx_isr(int);

  #if USE_TX_INTERRUPTS_0
    ISR(USART0_DRE_vect) {handle_tx_isr(0);}
  #endif
  #if USE_TX_INTERRUPTS_1
    ISR(USART1_DRE_vect) {handle_tx_isr(1);}
  #endif
  #if USE_TX_INTERRUPTS_2
    ISR(USART2_DRE_vect) {handle_tx_isr(2);}
  #endif
  #ifdef USART3
    #if USE_TX_INTERRUPTS_3
      ISR(USART3_DRE_vect) {handle_tx_isr(3);}
    #endif
  #endif
  #ifdef USART4
    #if USE_TX_INTERRUPTS_4
      ISR(USART4_DRE_vect) {handle_tx_isr(4);}
    #endif
  #endif
  #ifdef USART5
    #if USE_TX_INTERRUPTS_5
      ISR(USART5_DRE_vect) {handle_tx_isr(5);}
    #endif
  #endif
#endif

void* usart_init(uint8_t usartnum, uint32_t baud_rate, bool* use_tx_intr)
{
    USART_t* usart;

    if (usartnum == 0) {
        usart = &USART0;
        *use_tx_intr = USE_TX_INTERRUPTS_0;
        // enable USART0 TX pin
        PORTA.DIRSET = PIN0_bm;
    }
    else if (usartnum == 1) {
        usart = &USART1;
        *use_tx_intr = USE_TX_INTERRUPTS_1;
        // enable USART1 TX pin
        PORTC.DIRSET = PIN0_bm;
    }
    else if (usartnum == 2) {
        usart = &USART2;
        *use_tx_intr = USE_TX_INTERRUPTS_2;
        // enable USART2 TX pin
        PORTF.DIRSET = PIN0_bm;
    }
#ifdef USART3
    else if (usartnum == 3) {
        usart = &USART3;
        *use_tx_intr = USE_TX_INTERRUPTS_3;
        // enable USART3 TX pin
        PORTB.DIRSET = PIN0_bm;
    }
#endif
#ifdef USART4
    else if (usartnum == 4) {
        usart = &USART4;
        *use_tx_intr = USE_TX_INTERRUPTS_4;
        // enable USART4 TX pin
        PORTE.DIRSET = PIN0_bm;
    }
#endif
#ifdef USART5
    else if (usartnum == 5) {
        usart = &USART5;
        *use_tx_intr = USE_TX_INTERRUPTS_5;
        // enable USART5 TX pin
        PORTG.DIRSET = PIN0_bm;
    }
#endif
    else {
        usart = NULL;
    }
	
    usart->BAUD = (4 * F_CPU) / baud_rate;
    usart->CTRLB |= (USART_RXEN_bm | USART_TXEN_bm); /* tx/rx enable */

    return usart;
}

void usart_enable_interrupt(void* ptr)
{
    USART_t* usart = (USART_t*)ptr;
    usart->CTRLA |= USART_DREIE_bm;
}

void usart_disable_interrupt(void* ptr)
{
    USART_t* usart = (USART_t*)ptr;
    usart->CTRLA &= ~USART_DREIE_bm;
}

void usart_transmit_data(void* ptr, char c)
{
    USART_t* usart = (USART_t*)ptr;
    usart->TXDATAL = c;
}

void usart_wait_until_transmit_ready(void *ptr)
{
    USART_t* usart = (USART_t*)ptr;
    loop_until_bit_is_set(usart->STATUS, USART_DREIF_bp);
}

int usart_receive_data(void* ptr)
{
    USART_t* usart = (USART_t*)ptr;

    uint8_t c;

    loop_until_bit_is_set(usart->STATUS, USART_RXCIF_bp);
    char rcv_status = usart->RXDATAH;
    if (rcv_status & USART_FERR_bm) {
        c = usart->RXDATAL; /* clear error by reading data */
        return _FDEV_EOF;
    }
    if (rcv_status & USART_BUFOVF_bm) {
        c = usart->RXDATAL; /* clear error by reading data */
        return _FDEV_ERR;
    }
    c = usart->RXDATAL;
    return c;
}

bool in_isr(void *ptr)
{
    return (CPUINT.STATUS != 0);
}
