#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "lcd_lib.h"
#include "uart.h"

#define F_CPU 16000000UL
#define BUF_SZ 16
#define EEPROM_SZ_ADDR (uint8_t*)0
#define EEPROM_PWD_ADDR (uint8_t*)1

//---------------------Clock---------------------
void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

//---------------------WDT---------------------
void init_WDT(void) {
	wdt_reset();
	wdt_enable(WDT_PERIOD_8KCLK_gc);
}


uint8_t no_match_flag = 1;

void write_buf_eeprom(char buf[], uint8_t len) {
	eeprom_write_byte(EEPROM_SZ_ADDR, len);
	eeprom_write_block(buf, EEPROM_PWD_ADDR, len);
}

uint8_t read_buf_eeprom(char buf[]) {
	uint8_t len = eeprom_read_byte(EEPROM_SZ_ADDR);
	if (buf != NULL) eeprom_read_block(buf, EEPROM_PWD_ADDR, len);
	return len;
}

void printf_raw(char buf[], uint8_t sz) {
	uint8_t len;
	if (sz == NULL) len = strlen(buf);
	else len = sz;
	
	
	for (int i = 0; i < len; i++) {
		char c = buf[i];
		if (c == '\0') printf("\\0");
		else if (c == '\n') printf("\\n");
		else printf("%c", c);
	}
}

uint8_t str_input(char buf[]) {
	while ( fgets(buf, BUF_SZ, stdin) != NULL) {
		break;
	}
	
	uint8_t len = 0;
	do {
		len++;
	} while (buf[len] != '\n' && len < BUF_SZ-1);
	for (int i = len; i < BUF_SZ; i++) {
		buf[i] = '\0';
	}
	
	//printf("%s\t%d\n", buf, len);
	//printf_raw(buf, BUF_SZ);
	
	return len;
}

void pwd_to_eeprom() {
	printf("enter pwd:");
	char buf[16];
	uint8_t len = str_input(buf);
	putchar('\n');
	write_buf_eeprom(buf, len);
}

void print_pwd_eeprom() {
	char buf[16];
	uint8_t len = read_buf_eeprom(buf);
	
	printf("current pwd: ");
	printf_raw(buf, NULL);
	putchar('\n');
}

char random_char(int index) {
	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	return charset[index];
}

int main(void)
{
    wdt_reset();
	wdt_disable();
	init_clock();
	uart_init(3, 9600, NULL);

	sei();
	
	if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm) {
		printf("\npassword reset\n");
		char buf_eeprom[BUF_SZ];
		for (int i = 0; i < BUF_SZ; i++) {
			buf_eeprom[i] = random_char( rand()%62 );
		}
		buf_eeprom[BUF_SZ-1] = '\0';
		
		write_buf_eeprom(buf_eeprom, BUF_SZ);
		RSTCTRL.RSTFR |= RSTCTRL_WDRF_bm;		
	}
	
	char eeprom_buf[BUF_SZ];
	uint8_t eeprom_len = read_buf_eeprom(eeprom_buf);
	
	if (eeprom_len == 0xff) {
		printf("\n--set password--\n");
		pwd_to_eeprom();
	} else {
		printf("\n--guess password--\n");
		//print_pwd_eeprom();
		init_WDT();
		char cmp_buf[BUF_SZ];
		
		while (no_match_flag != 0) {
			printf("enter password: ");
			uint8_t cmp_len = str_input(cmp_buf);
			putchar('\n');
			no_match_flag = (memcmp(eeprom_buf, cmp_buf, eeprom_len) || !(eeprom_len == cmp_len));
			if (no_match_flag == 0) break;
			printf("wrong password\n");
			memset(cmp_buf, 0, BUF_SZ);
			
		}
		
		wdt_disable();
	}
	
	VPORTB.DIR = 0xff;
	VPORTB.OUT = 0x00;	
	
	while (1) 
    {

    }
	
	return 0;
}

