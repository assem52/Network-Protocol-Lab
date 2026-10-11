/*
 * main.c - SPI Slave (ATmega32)
 *
 * ATmega32 = SPI Slave
 *
 * Flow:
 *   1. Wait for the Arduino master to clock in 7 bytes of RTC data.
 *   2. Receive: seconds, minutes, hours, day, date, month, year.
 *   3. Display the time/date on PORTA (LEDs) cycling through each byte,
 *      and also print it over USART so you can see it on a serial terminal.
 *
 * SPI pins on ATmega32 (PORTB):
 *   PB4 = SS   (input - driven LOW by master to select this slave)
 *   PB5 = MOSI (input - master sends data here)
 *   PB6 = MISO (output- slave sends reply here)
 *   PB7 = SCK  (input - clock from master)
 *
 * Make sure SLAVE_MODE is #defined in SPI.h (already done by default).
 */

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "MCAL/inc/SPI.h"
#include "MCAL/inc/USART.h"

/* ---- UART config ---- */
UART_Config_t uartCfg = {
    .baudRate    = 9600,
    .DoubleSpeed = 0
};

/* ---- Helper: receive one SPI byte from master ---- */
uint8 spiReceive() {
    /*
     * Pre-load SPDR with 0x00 (dummy reply) then wait for master to clock.
     * SPI_SendRecieveData() handles the SPIF polling.
     */
    return SPI_SendRecieveData(0x00);
}

/* ---- Print 2-digit number over USART (no stdlib printf needed) ---- */
static void printByte(uint8 val) {
    USART_send('0' + (val / 10));
    USART_send('0' + (val % 10));
}

int main(void) {

    /* PORTA -> output (LEDs show current byte being displayed) */
    DDRA = 0xFF;
    PORTA = 0x00;

    /* Init SPI as Slave */
    SPI_Init();

    /* Init USART for serial printing */
    USART_init(&uartCfg);

    USART_sendString((uint8 *)"ATmega32 SPI Slave Ready");

    while (1) {

        /* Wait for SS to go LOW (master selects us),
         * then receive 7 bytes in order. */

        uint8 sec   = spiReceive();
        uint8 min_  = spiReceive();
        uint8 hour  = spiReceive();
        uint8 day   = spiReceive();
        uint8 date  = spiReceive();
        uint8 month = spiReceive();
        uint8 year  = spiReceive();

        /* Show on PORTA LEDs (cycles through each byte) */
        PORTA = hour;   _delay_ms(300);
        PORTA = min_;   _delay_ms(300);
        PORTA = sec;    _delay_ms(300);

        /* Print over USART: HH:MM:SS  DD/MM/YY */
        USART_sendString((uint8 *)"Time: ");
        printByte(hour);  USART_send(':');
        printByte(min_);  USART_send(':');
        printByte(sec);

        USART_sendString((uint8 *)"  Date: ");
        printByte(date);  USART_send('/');
        printByte(month); USART_send('/');
        USART_send('2'); USART_send('0');
        printByte(year);
        USART_send('\n');
    }
}
