/*
 * main.c
 *
 * Created: 26.11.2017 15:08:19
 *  Author: DiGun
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#define BAUD 9600
#include "tm1637.h"
#include "rtc.h"
#include "uart.h"


// TIMER0 with prescaler clkI/O/256
#define TIMER0_PRESCALER (1 << CS02)

uint32_t time;
volatile uint8_t refresh;
uint32_t number;
uint8_t	mode;

uint32_t eedata[10] EEMEM;
typedef struct 
{
	uint8_t h;	
	uint8_t m;
}eetime;

typedef struct 
{
	eetime on;
	eetime off;	
}eeonoff;

union 
{
	eeonoff ee;
	uint32_t mm;
}led_rule;

void Init()
{
	// init Timer0
	
	TIMSK |= (1 << TOIE0);        // interrupt enable - here overflow
	TCCR0 |= TIMER0_PRESCALER;    // use defined prescaler value
}

void NumbToUART(uint32_t number)
{
	char s[12];
	char* ch;
	ch=ultoa(number,s,10);
	while(ch[0])
	{
		uart_putc_w(ch[0]);
		ch++;
	}
}


uint32_t cmd_get_num;//полученный номер
uint8_t cmd_get_param;//параметр до N
uint8_t cmd_mode; //текущий режим
uint8_t cmd_type; //тип команды
uint8_t cmd_status;//статус текущей команды


inline void func_hello(void)
{
	uart_putc_w('A');
}

void func_error(void)
{
	uart_putc_w('E');
	cmd_mode=0;
	cmd_status =0;
	cmd_type=0;
}

inline void func_ok(void)
{
	cmd_mode=1;
	cmd_status =0;
	cmd_type=0;
}



inline uint8_t func_type(char c)
{
	switch (c)
	{
		case 'S':	//set
		case 'G':	//get
		cmd_type=c;
		break;
		default:
		cmd_type=0;
		func_error();
	}
	return cmd_type;
}

void func_get(char c)
{
	switch (c)
	{
		case 'T':	//timestamp
		uart_putc_w('A');
		uart_putc_w('T');
		NumbToUART(time);
//		uart_putc_w('D');
//		NumbToUART(time);
		uart_putln();
		func_ok();
		break;

		case 'M':	//mode
		uart_putc_w('A');
		uart_putc_w('M');
		NumbToUART(mode);
//		uart_putc_w('D');
//		NumbToUART(mode);
		uart_putln();
		func_ok();
		break;
		
		case 'I':	//intens
		uart_putc_w('A');
		uart_putc_w('I');
		NumbToUART(TM1637_brightness);
//		uart_putc_w('D');
//		NumbToUART(TM1637_brightness);
		uart_putln();
		func_ok();
		break;
				
		case 'A':	//get
		func_error();
		//		cmd_type=c;
		break;
		default:
		func_error();
	}
	//	cmd_type=0;
}

int8_t get_str_num(char c)
{
	static char buf[12];//текстовы числовой параметр 11
	static uint8_t len; //длина
	static uint8_t dup; //дибликат
	if (c>'9' || c<'0')
	{
		if (len!=0)
		{
			buf[len]=0;
			len=0;

			uint32_t t_num=strtoul(buf,NULL,10);;
			if (dup)
			{
				if (t_num!=cmd_get_num)
				{
					dup=0;
					return -1;
					
				}
			}
			cmd_get_num=t_num;
		}
		switch (c)
		{
			case 'N': //next
			case 13:
			case 10:
			dup=0;
			return 1;
			case 'D': //duplicate
			dup=1;
			break;
			default:
			len=0;
			dup=0;
			return -1;
		}
	}
	else
	{
		if (len!=10)
		{
			buf[len]=c;
			len++;
		}
		else
		{
			//			PORTC|=1<<PC5;
			len=0;
			dup=0;
			func_error();
		}
	}
	return 0;
}

void func_set(char c)
{
	if (cmd_status)
	{
		switch (get_str_num(c))
		{
			case 0:
			break;
			case 1:
			switch (cmd_status)
			{
				case 'T':
				time=cmd_get_num;
				rtc_Unix2Time(cmd_get_num,&_tm);
				rtc_set_time(&_tm);
				break;
				case 'M':
				mode=cmd_get_num;
				break;
				case 'I':
				TM1637_set_brightness(cmd_get_num);//Яркость
				break;
				case 'R':
				eeprom_update_dword(&eedata[cmd_get_param], cmd_get_num);
				break;
/*				
				case 'L':
				if(cmd_get_num)
				{
					PORTC|=(1<<PC5);
				}
				else
				{
					PORTC&=~(1<<PC5);
				}
				break;
*/				
//				case 'N':
//				number=cmd_get_num;
//				break;
			}

			uart_putln();
			uart_putc_w('R');
			NumbToUART(cmd_get_num);
			uart_putln();
			func_ok();
			break;
			case -1:
			func_error();
			break;
			case 2:
			cmd_get_param=(uint8_t)cmd_get_num;
		}
	}
	else
	{
		switch (c)
		{
			case 'T':	//timestamp
			case 'M':	//mode
			case 'I':	//intensivity
//			case 'L':	//led
			case 'R':	//rule
//			case 'N':	//number
			cmd_status = c;
			break;

			case 'A':	//get
			func_error();
			//		cmd_type=c;
			break;
			default:
			func_error();
		}
	}
}

// cmd
//Q - start
//[S,G]	- Set,Get

void main_func(char c)
{
	switch (cmd_mode)
	{
		case 0:
		if (c=='Q')
		{
			func_hello();
			cmd_mode++;
		}
		else
		{
			func_error();
		}
		break;
		case 1:
		if (func_type(c))
		{
			cmd_mode++;
		}
		break;
		case 2:
		switch (cmd_type)
		{
			case 'G':
			func_get(c);
			break;
			case 'S':
			func_set(c);
			refresh=1;
			break;
		}
		break;
	}
}








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
  mode=1;
  cmd_mode=0;
  cmd_status=0;
  refresh=1;
  DDRD|=1<<PD7;
  
//  rtc_date date;
//  struct rtc_time time;

/* setup */

  Init();
  
	//	init_USART();
//	uart_stop_receve();
	sei();
   // RTC configuration
    twi_init_master();
    rtc_init();
   // Init LED screen
    TM1637_init();	

	uart_init();
//	uart_puts_P(PSTR("MCUR"));
  
//  uart_putln();
// rtc_get_time();
/* loop */
  while (1) 
  {
	if (refresh)
	{
	  switch (refresh)
	  {
		case 1:

		rtc_get_time();
		switch (mode)
		{
			case 0: //Secunda
			TM1637_setTime(_tm.min,_tm.sec);
			TM1637_display_colon(true);
			break;
			case 1: //Time
			TM1637_setTime(_tm.hour,_tm.min);
			TM1637_display_colon(true);
			break;
			case 2: //Date
			TM1637_setTime(_tm.mon,_tm.mday);
			TM1637_display_colon(true);
			break;
			case 3: //Year
			TM1637_setTime(_tm.year/100,_tm.year%100);
			TM1637_display_colon(true);
			break;
			case 7:
			TM1637_enable(false);
			break;
			case 8:
			TM1637_enable(true);
			break;
			
			
			
		}
		
		
		
		
//		PORTD|=1<<PD7;
//		PORTD^=1<<PD7;
		break;
		case 2:
		TM1637_display_colon(false);
//		PORTD&=~(1<<PD7);
		break;
	  }
	  time=rtc_Time2Unix(&_tm);
	  refresh=0;
	}
	{
      char c;
	  if ((uart_getc(&c))==0)
	  {
		main_func(c);
/*	DEBUG		
		mode=0;
		TM1637_display_digit(0,c>>4);
		TM1637_display_digit(1,0x0f&c);
		TM1637_display_segments(2,0);
		TM1637_display_segments(3,0);
*/		
      }
	}	
 }
}
