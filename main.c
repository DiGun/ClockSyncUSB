/*
 * main.c
 *
 * Created: 26.11.2017 15:08:19
 *  Author: DiGun
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define BAUD 9600
#include "tm1637.h"
#include "rtc.h"
#include "uart.h"


// TIMER0 with prescaler clkI/O/256
#define TIMER0_PRESCALER (1 << CS02)

void Init()
{
	// init Timer0
	
	TIMSK |= (1 << TOIE0);        // interrupt enable - here overflow
	TCCR0 |= TIMER0_PRESCALER;    // use defined prescaler value
}

volatile uint8_t refresh;

#define STEP4SECUNDA	225

ISR(TIMER0_OVF_vect)
{
	static uint8_t cnt_secunda=STEP4SECUNDA;
	if (!(--cnt_secunda))
	{
		refresh=1;
		cnt_secunda=STEP4SECUNDA;
	}
	else if(cnt_secunda==175)
	{
		refresh=2;
	}
}


int main(void)
{
  uint8_t i = 0;
  uint8_t h,m,s;
  DDRD|=1<<PD7;
//  rtc_date date;
//  struct rtc_time time;

/* setup */
  Init();
  uart_init();
  sei();
// RTC configuration
  twi_init_master();
  rtc_init();
  TM1637_init();

/* loop */
  while (1) 
  {
	
	if (refresh)
	{
	  switch (refresh)
	  {
		case 1:
		rtc_get_time_s(&h,&m,&s);
		TM1637_setTime(m,s);
		TM1637_display_colon(true);
		PORTD|=1<<PD7;
//		PORTD^=1<<PD7;
		break;
		case 2:
		TM1637_display_colon(false);
		PORTD&=~(1<<PD7);
		break;
	  }
	  refresh=0;
	}
	i++;
  }
}