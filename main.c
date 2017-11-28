/*
 * main.c
 *
 * Created: 26.11.2017 15:08:19
 *  Author: DiGun
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include "tm1637.h"

int main(void)
{
uint8_t i = 0;

/* setup */
TM1637_init();

/* loop */
while (1) {
	TM1637_display_digit(TM1637_SET_ADR_00H, i % 0x10);
	TM1637_display_digit(TM1637_SET_ADR_01H, (i + 1) % 0x10);
	TM1637_display_digit(TM1637_SET_ADR_02H, (i + 2) % 0x10);
	TM1637_display_digit(TM1637_SET_ADR_03H, (i + 3) % 0x10);
	TM1637_display_colon(true);
	_delay_ms(200);
	TM1637_display_colon(false);
	_delay_ms(200);
	i++;
}
}