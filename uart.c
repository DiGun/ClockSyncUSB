
#include "uart.h"

#include <avr/interrupt.h>
#include <util/setbaud.h>
#define U2X 1

void uart_init()
{
    /* set baudrate */
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    /* double transmission speed? */
    #if USE_2X
    UCSR0A |= (1<<U2X);
    #else
    UCSR0A &= ~(1<<U2X);
    #endif

    /* frame format: asynchronous, 8 data bits, no parity, 1 stop bit */
    #ifdef URSEL
    UCSR0C = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
    #else
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
    #endif

    /* enable receiver, transmitter and receive complete interrupt */
    UCSR0B |= (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);

    /* init ring buffer pointers */
    UARTrxBuffer.start = UARTrxBuffer.buffer;
    UARTrxBuffer.end = UARTrxBuffer.buffer;
    UARTtxBuffer.start = UARTtxBuffer.buffer;
    UARTtxBuffer.end = UARTtxBuffer.buffer;
	readyToExchange=1;
}

/* sends single character
 * returns -1 on error
 */
int8_t uart_putc(char c)
{
    /* is the buffer full? */
    if((UARTtxBuffer.end == &UARTtxBuffer.buffer[BUFFERSIZE-1] &&
        UARTtxBuffer.start == UARTtxBuffer.buffer) ||
        UARTtxBuffer.end == UARTtxBuffer.start-1) {
//	asm("nop;");
        return -1;
    }

    /* add element to buffer */
    *UARTtxBuffer.end = c;

    /* set pointer to next field */
    if(UARTtxBuffer.end == &UARTtxBuffer.buffer[BUFFERSIZE-1])
        UARTtxBuffer.end = UARTtxBuffer.buffer;
    else
        UARTtxBuffer.end++;

    /* enable tx register empty interrupt */
    UCSR0B |= (1<<UDRIE0);
	readyToExchange = 0;

    return 0;
}

/* sends null-terminated character string
 * returns number of sent characters
 */
uint8_t uart_puts(char *s)
{
    uint8_t count = 0;
    while(*s)   {

        if(uart_putc(*s) != -1) {
            count++;
            s++;
        }
//        else
//            break;
    }
//	asm("nop;");
    return count;
}

void uart_putc_w(char c)
{
	while(uart_putc(c) == -1);
}


void uart_putln(void)
{
	uart_putc_w('\r');
	uart_putc_w('\n');
}

uint8_t uart_puts_sP(unsigned char spaces, const char* PROGMEM s)
{
	unsigned char f;
	for (f=0;f<spaces;f++)
	{
		uart_putc_w(' ');
	}
	return uart_puts_P(s)+spaces;
}

uint8_t uart_puts_P(const char* PROGMEM s)
{
    uint8_t count = 0;
	char ch = pgm_read_byte_near(s);
	while (ch) {
//		if (ch == '\n')
//		uart_putc('\r');       // firstly send CR
        if(uart_putc(ch) != -1) {
		            count++;
					ch = pgm_read_byte_near(++s);
		}
	}
//	asm("nop;");
    return count;
}
/* receives a character and stores it in 'dest'
 * returnes -1 if there is nothing in the buffer
 */
int8_t uart_getc(char *dest)
{
    /* is the buffer empty? */
    readyToExchange = 0;
    UCSR0B |= (1 << RXCIE0);
    if(UARTrxBuffer.start == UARTrxBuffer.end)  
	{

        return -1;
    }

    /* get element from buffer */
    *dest = *UARTrxBuffer.start;

    /* set pointer to next field */
    if(UARTrxBuffer.start == &UARTrxBuffer.buffer[BUFFERSIZE-1])
        UARTrxBuffer.start = UARTrxBuffer.buffer;
    else
        UARTrxBuffer.start++;
		
    return 0;
}

inline void uart_stop_receve(void)
{
	    UCSR0B &= ~(1 << RXCIE0);
	    readyToExchange = 1;	
}
/* uart receive complete interrupt */
ISR(USART_RX_vect)
{
    uint8_t rc;
    rc = UDR0;

    /* store in buffer */
    if((UARTrxBuffer.end == &UARTrxBuffer.buffer[BUFFERSIZE-1] &&
        UARTrxBuffer.start == UARTrxBuffer.buffer) ||
        UARTrxBuffer.end == UARTrxBuffer.start-1)   
	{
//		uart_stop_receve();
        return;
    }
    else    
	{
        *UARTrxBuffer.end = rc;

        if(UARTrxBuffer.end == &UARTrxBuffer.buffer[BUFFERSIZE-1])
            UARTrxBuffer.end = UARTrxBuffer.buffer;
        else
            UARTrxBuffer.end++;
    }
}

/* uart transmit register empty interrupt */
ISR(USART_UDRE_vect)
{
    /* send byte */
    UDR0 = *UARTtxBuffer.start;

    /* set pointer to next field */
    if(UARTtxBuffer.start == &UARTtxBuffer.buffer[BUFFERSIZE-1])
        UARTtxBuffer.start = UARTtxBuffer.buffer;
    else
        UARTtxBuffer.start++;

    /* buffer empty? disable interrupt */
    if(UARTtxBuffer.start == UARTtxBuffer.end)
	{
        UCSR0B &= ~(1<<UDRIE0);
		readyToExchange = 1;
	}		
}

