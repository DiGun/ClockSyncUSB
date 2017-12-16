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
uint8_t last_min;

#define MAX_ALARM 16

uint32_t eedata[MAX_ALARM] EEMEM;
typedef struct 
{
	uint8_t m;
	uint8_t h;
}eetime;

typedef struct 
{
	eetime on;
	eetime off;	
}eeonoff;

typedef struct
{
	uint16_t on;
	uint16_t off;
}eecmp;

union led_rule
{
	eeonoff ee;
	uint32_t mm;
	eecmp cc;
};

void Init()
{
	// init Timer0
	
	TIMSK0 |= (1 << TOIE0);        // interrupt enable - here overflow
	TCCR0B |= TIMER0_PRESCALER;    // use defined prescaler value
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


int8_t get_str_num(char c)
{
	static char buf[12];//текстовый числовой параметр 11
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
		else
		{
			dup=0;
			return -1;
		}
		switch (c)
		{
			case ' ': //next
			len=0;
			dup=0;
			return 2;
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
			return -1;
		}
	}
	return 0;
}

//***********************************
//* Get
//***********************************
void func_get(char c)
{
	if (cmd_status)
	{
		switch (get_str_num(c))
		{
			case 1:
			switch (cmd_status)
			{
				case 'R':
				uart_putc_w('A');
				uart_putc_w('R');
				NumbToUART(cmd_get_num);
				uart_putc_w(' ');
				NumbToUART(eeprom_read_dword(&eedata[cmd_get_num&0x0F]));
//				uart_putc_w('D');
//				NumbToUART(eeprom_read_dword(&eedata[cmd_get_num&0x0F]));
				uart_putln();
				func_ok();
				break;
			}
			break;
			case -1:
			func_error();
			break;
		}
		return;
	}
	switch (c)
	{
		case 'T':	//timestamp
		uart_putc_w('A');
		uart_putc_w('T');
		NumbToUART(time);
		uart_putc_w('D');
		NumbToUART(time);
		uart_putln();
		func_ok();
		break;

		case 'M':	//mode
		uart_putc_w('A');
		uart_putc_w('M');
		NumbToUART(mode);
		uart_putc_w('D');
		NumbToUART(mode);
		uart_putln();
		func_ok();
		break;
		
		case 'I':	//intens
		uart_putc_w('A');
		uart_putc_w('I');
		NumbToUART(TM1637_brightness);
		uart_putc_w('D');
		NumbToUART(TM1637_brightness);
		uart_putln();
		func_ok();
		break;

		case 'R':	//Alarm
		cmd_status = c;
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

//***********************************
//* Set
//***********************************
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
				eeprom_update_dword(&eedata[cmd_get_param&0x0F], cmd_get_num);
				break;
				
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
				
				case 'N':
				number=cmd_get_num;
				break;
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
			case 'L':	//led
			case 'R':	//rule
			case 'N':	//number
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

char invalid_time(eetime* ee)
{
	if ((ee->h<24)&&(ee->m<60))
	{
		return 0;
	}
	return 1;
}

inline char check_time(eeonoff* ee)
{
	if (invalid_time(&ee->on)||invalid_time(&ee->off))
    {
		return 0;	
    }
	if (ee->on.h>ee->off.h)
	{
		return 0;		
	}
	if (ee->on.h==ee->off.h)
	{
      if (ee->on.m>=ee->off.m)
	  {
		return 0;
	  }
	}
	return 1;
}
inline char between_time(eecmp* cc)
{
	int16_t minutes;
	minutes=(_tm.hour<<8)+_tm.min;
	if ((cc->on<=minutes)&&(cc->off>minutes))
	{
		return 1;
	}
	return 0;	
}

char check_alarm()
{
 uint8_t f;
 union led_rule alarm;
 for(f=0;f<MAX_ALARM-1;f++)
 {
	alarm.mm=(eeprom_read_dword(&eedata[f]));
	if (!check_time(&alarm.ee))
	{
		continue;
	}
	if (between_time(&alarm.cc))
	{
		return 1;
	}
 }
 return 0;
}
void check_intens()
{
 union led_rule alarm;
 alarm.mm=(eeprom_read_dword(&eedata[MAX_ALARM-2]));
 if (!check_time(&alarm.ee))
 {
		 return;
 }
if (between_time(&alarm.cc))
 {
	 TM1637_set_brightness(7);
 }
 else
 {
	 TM1637_set_brightness(1);
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
  last_min=254;
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
//			TM1637_display_colon(true);
			break;
			case 4: // Alarm
			{
				if(rtc_is_ds3231()) 
				{
					int16_t t;
					TM1637_display_colon(true);
					ds3231_get_temp_int(&t);
					t=(t+5)/10;
					TM1637_set2((uint8_t)(t/10),1);
					TM1637_display_digit(2,(uint8_t)(t%10));
					TM1637_display_segments(3,0x39);
				}
				
				
/*				union led_rule alarm;		
				alarm.mm=(eeprom_read_dword(&eedata[0]));
				if (time&1)
				{
					TM1637_setTime(alarm.ee.off.h,alarm.ee.off.m);
				}
				else
				{
					TM1637_display_colon(true);
					TM1637_setTime(alarm.ee.on.h,alarm.ee.on.m);
				}
				*/
;
			}
			break;
//			TM1637_display_colon(true);
			case 5:
			TM1637_enable(false);
			break;
			case 6:
			TM1637_enable(true);
			break;
			
		}
		
//		PORTD|=1<<PD7;
//		PORTD^=1<<PD7;
		break;
		case 2:
		if(mode!=2&&mode!=4)
			TM1637_display_colon(false);
//		PORTD&=~(1<<PD7);
		break;
	  }
	  if (last_min!=_tm.min)
	  {
		  last_min=_tm.min;
		  check_intens();
		  if (check_alarm())
		  {
//			PORTD|=1<<PD7;
			PORTD&=~(1<<PD7);
		  }
		  else
		  {
			  PORTD|=1<<PD7;
//			PORTD&=~(1<<PD7);
		  }
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
