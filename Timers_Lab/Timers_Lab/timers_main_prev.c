#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL
#define EVENT_TIME 500
volatile int tc = 0; // timer counter

void init_clock(void) {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
    while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

void init_TCA0(void) {
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PER = 249;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    
    TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

int main(void) {
    init_clock();
    init_TCA0();
    uart_init(3, 9600, NULL);
    
    VPORTD.DIR = 0xff;
    VPORTD.OUT = 0x00;
    
    // when using on-board button on PORTB
    // make sure to &= PIN0 and PIN1
    // those pins are used for UART on AVR
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTB.DIR &= (0x00 | PIN0_bm | PIN1_bm);
    
    int r = 2000 + (int)(random() % 3000);
    char counter_f = 0;
    unsigned char btn_handled2 = 0;
    
    printf("setup complete\n");
    sei();
    
    while (1) {
        if (tc >= r && counter_f == 0) {
            VPORTD.OUT = 0xff;
            tc = 0;
            counter_f = 1;
            printf("press the damn button\n");
        }
        
        if (!(PORTB.IN & PIN2_bm) && counter_f == 1) {
            if (!btn_handled2) {
                printf("reaction time: %d milliseconds\n\n", (int)tc);
                VPORTD.OUT = 0x00;
                
                tc = 0;
                counter_f = 0;
                r = 2000 + (int)(random() % 3000);
                btn_handled2 = 1;
            }
        } else {
            btn_handled2 = 0;
        }
        
    }
}

ISR(TCA0_OVF_vect) {
    if (tc < INT_MAX) tc++;
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}