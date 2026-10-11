/*
 * USART.c
 *
 *  Created on: 13 Feb 2023
 *      Author: Alaa Wahba
 */

#include "inc/USART.h"
#include <stdio.h>
#include <string.h>

#define Default_Stop 		'\r'
static uint32 flag = 1;
static uint8 *TX_str;

void USART_init(UART_Config_t *pinConfig) {
	uint16 ubrr_value;

	/* ---- Baud rate ---- */
	// Formula (Normal Speed): UBRR = (F_CPU / (16 * BaudRate)) - 1
	// Formula (Double Speed): UBRR = (F_CPU / (8  * BaudRate)) - 1
	if (pinConfig->DoubleSpeed) {
		SET(UCSRA, U2X);                        // Enable double speed
		ubrr_value = (uint16)((F_CPU / (8UL  * pinConfig->baudRate)) - 1);
	} else {
		CLEAR(UCSRA, U2X);
		ubrr_value = (uint16)((F_CPU / (16UL * pinConfig->baudRate)) - 1);
	}
	UBRRH = (uint8)(ubrr_value >> 8);
	UBRRL = (uint8)(ubrr_value);

	/* ---- FRAME format (written to UCSRC) ----
	 * URSEL must be set to 1 when writing UCSRC
	 * (UBRRH shares the same address 0x40 — URSEL bit distinguishes them)
	 */
	// Asynchronous mode  : UMSEL = 0
	// No parity          : UPM1 = 0, UPM0 = 0
	// 1 stop bit         : USBS  = 0
	// 8-bit character    : UCSZ1 = 1, UCSZ0 = 1, UCSZ2 = 0 (in UCSRB)
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
	// UCSZ2 in UCSRB stays 0 (8-bit data, not 9-bit)
	CLEAR(UCSRB, UCSZ2);

	/* ---- Enable TX and RX ---- */
	SET(UCSRB, TXEN);
	SET(UCSRB, RXEN);
}

void USART_send(uint8 data) {
	/* The transmit buffer can only be written when UDRE Flag in UCSRA is set */
	while (!GET(UCSRA, UDRE))
		;
	UDR = data;
}

uint8 USART_recieve() {
	/*
	 * RXC flag is set when there are unread data in the receive buffer
	 * and cleared when the receive buffer is empty.
	 * Wait until a byte arrives, then read UDR.
	 */
	while (!GET(UCSRA, RXC))
		;
	return UDR;
}

void USART_sendNumber(uint32 data) {
	char str[12];
	sprintf(str, "%lu", (unsigned long)data);
	USART_sendString((uint8 *)str);
}

uint32 USART_recieveNumber() {
	uint8  buff[12];
	uint32 num = 0;
	USART_recieveString(buff);
	num = (uint32)atoi((char *)buff);
	return num;
}

void USART_sendString(uint8 *str) {
	uint8 i = 0;
	/* Iterate until the null terminator */
	while (str[i] != '\0') {
		USART_send(str[i]);
		i++;
	}
	/* Send stop / termination character so the receiver knows where the string ends */
	USART_send(Default_Stop);
}

void USART_recieveString(uint8 *Buff) {
	uint8 i = 0;
	Buff[i] = USART_recieve();
	while (Buff[i] != Default_Stop) {
		i++;
		Buff[i] = USART_recieve();
	}
	Buff[i] = '\0';
}

/* -----------------------------------------------------------------------
 *  Non-blocking / Interrupt-driven stubs
 * --------------------------------------------------------------------- */

void USART_sendNoBlock(uint8 data) {
	UDR = data;
}

uint8 USART_recieveNoBlock() {
	if (GET(UCSRA, RXC))
		return UDR;
	return 0;
}

void USART_TX_interrupt_Enable()  { SET(UCSRB,  TXCIE); }
void USART_TX_interrupt_Disable() { CLEAR(UCSRB, TXCIE); }
void USART_RX_interrupt_Enable()  { SET(UCSRB,  RXCIE); }
void USART_RX_interrupt_Disable() { CLEAR(UCSRB, RXCIE); }

uint8 USART_recievePeriodicData(uint8 *data) {
	if (GET(UCSRA, RXC)) {
		*data = UDR;
		return 1;   /* new data available */
	}
	return 0;       /* nothing yet */
}

void USART_send_string_Asynch(uint8 *str) {
	TX_str = str;
	flag   = 0;
	USART_TX_interrupt_Enable();
	USART_sendNoBlock(TX_str[0]);
}