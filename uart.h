#ifndef UART_H
#define UART_H

#ifndef F_CPU
#define F_CPU 14745600UL
//  #define F_CPU 16000000UL
// DiGun
//#error "F_CPU is not defined"
#endif

#include <stdint.h>
#include <avr/pgmspace.h>

#ifndef BAUD
#define BAUD 9600
#endif

#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__) || defined(__AVR_ATmega32__)
    #define RX_COMPL_INT USART_RXC_vect
    #define TX_REG_EMPTY_INT USART_UDRE_vect
    
#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATmega8535__)
    #define RX_COMPL_INT USART_RX_vect
    #define TX_REG_EMPTY_INT USART_UDRE_vect

#endif


#define BUFFERSIZE 64

struct ringBuffer   {
    volatile uint8_t *start;
    volatile uint8_t *end;
    uint8_t buffer[BUFFERSIZE];
} rxBuffer, txBuffer;

volatile uint8_t readyToExchange;

/* prototypes */
void uart_init();
int8_t uart_putc(char);
void uart_putc_w(char c);

uint8_t uart_puts(char*);
uint8_t uart_puts_P(const char* PROGMEM s);
uint8_t uart_puts_sP(unsigned char spaces, const char* PROGMEM s);
int8_t uart_getc(char*);
void uart_putln(void);
void uart_stop_receve(void);
#endif
