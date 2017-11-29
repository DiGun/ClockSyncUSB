/*
 * main.c
 *
 * Created: 26.11.2017 15:08:19
 *  Author: DiGun
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "tm1637.h"
#include "rtc.h"
#include "uart.h"

int main(void)
{
  uint8_t i = 0;
  uint8_t h,m,s;
//  rtc_date date;
//  struct rtc_time time;

/* setup */
  uart_init();
  sei();
// RTC configuration
  twi_init_master();
  rtc_init();
  TM1637_init();

/* loop */
  while (1) 
  {
	rtc_get_time_s(&h,&m,&s);
	TM1637_setTime(m,s);
	TM1637_display_colon(true);
	_delay_ms(200);
	TM1637_display_colon(false);
	_delay_ms(200);
	i++;
  }
}