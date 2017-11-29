
#include "uart.h"

#include <avr/interrupt.h>
#include <util/setbaud.h>

void uart_init()
{
    /* set baudrate */
    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;

    /* double transmission speed? */
    #if USE_2X
    UCSRA |= (1<<U2X);
    #else
    UCSRA &= ~(1<<U2X);
    #endif

    /* frame format: asynchronous, 8 data bits, no parity, 1 stop bit */
    #ifdef URSEL
    UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
    #else
    UCSRC = (1<<UCSZ1) | (1<<UCSZ0);
    #endif

    /* enable receiver, transmitter and receive complete interrupt */
    UCSRB |= (1<<RXCIE) | (1<<RXEN) | (1<<TXEN);

    /* init ring buffer pointers */
    rxBuffer.start = rxBuffer.buffer;
    rxBuffer.end = rxBuffer.buffer;
    txBuffer.start = txBuffer.buffer;
    txBuffer.end = txBuffer.buffer;
	readyToExchange=1;
}

/* sends single character
 * returns -1 on error
 */
int8_t uart_putc(char c)
{
    /* is the buffer full? */
    if((txBuffer.end == &txBuffer.buffer[BUFFERSIZE-1] &&
        txBuffer.start == txBuffer.buffer) ||
        txBuffer.end == txBuffer.start-1) {
//	asm("nop;");
        return -1;
    }

    /* add element to buffer */
    *txBuffer.end = c;

    /* set pointer to next field */
    if(txBuffer.end == &txBuffer.buffer[BUFFERSIZE-1])
        txBuffer.end = txBuffer.buffer;
    else
        txBuffer.end++;

    /* enable tx register empty interrupt */
    UCSRB |= (1<<UDRIE);
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
    UCSRB |= (1 << RXCIE);
    if(rxBuffer.start == rxBuffer.end)  
	{

        return -1;
    }

    /* get element from buffer */
    *dest = *rxBuffer.start;

    /* set pointer to next field */
    if(rxBuffer.start == &rxBuffer.buffer[BUFFERSIZE-1])
        rxBuffer.start = rxBuffer.buffer;
    else
        rxBuffer.start++;
		
    return 0;
}

inline void uart_stop_receve(void)
{
	    UCSRB &= ~(1 << RXCIE);
	    readyToExchange = 1;	
}
/* uart receive complete interrupt */
ISR(USART__RXC_vect)
{
    uint8_t rc;
    rc = UDR;

    /* store in buffer */
    if((rxBuffer.end == &rxBuffer.buffer[BUFFERSIZE-1] &&
        rxBuffer.start == rxBuffer.buffer) ||
        rxBuffer.end == rxBuffer.start-1)   
	{
//		uart_stop_receve();
        return;
    }
    else    
	{
        *rxBuffer.end = rc;

        if(rxBuffer.end == &rxBuffer.buffer[BUFFERSIZE-1])
            rxBuffer.end = rxBuffer.buffer;
        else
            rxBuffer.end++;
    }
}

/* uart transmit register empty interrupt */
ISR(USART__UDRE_vect)
{
    /* send byte */
    UDR = *txBuffer.start;

    /* set pointer to next field */
    if(txBuffer.start == &txBuffer.buffer[BUFFERSIZE-1])
        txBuffer.start = txBuffer.buffer;
    else
        txBuffer.start++;

    /* buffer empty? disable interrupt */
    if(txBuffer.start == txBuffer.end)
	{
        UCSRB &= ~(1<<UDRIE);
		readyToExchange = 1;
	}		
}

