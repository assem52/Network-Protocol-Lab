/*
 * SPI.c
 *
 *  Created on: 22 Feb 2023
 *      Author: Alaa Wahba
 */

#include "inc/SPI.h"

void SPI_Init() {
#ifdef MASTER_MODE
	/* GPIO pins Configuration:
	 *   MOSI (PB5) -> Output
	 *   SCLK (PB7) -> Output
	 *   SS   (PB4) -> Output
	 *   MISO (PB6) -> Input  (driven by slave, no change needed)
	 */
	DIO_initPin(MOSI, SPI_PORT, OUTPUT);
	DIO_initPin(SCLK, SPI_PORT, OUTPUT);
	DIO_initPin(SS,   SPI_PORT, OUTPUT);
	DIO_initPin(MISO, SPI_PORT, INPUT);

	/* Master Configuration:
	 * SPE  = 1  -> Enable SPI
	 * MSTR = 1  -> Set as Master
	 * SPR0 = 1  -> Clock rate = Fosc / 16
	 * SPR1 = 0  |
	 * CPOL = 0  -> Clock polarity: idle low
	 * CPHA = 0  -> Data sampled on leading (rising) edge
	 * DORD = 0  -> MSB transmitted first
	 */
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

#endif /* MASTER_MODE */

#ifdef SLAVE_MODE
	/* GPIO pins Configuration:
	 *   MISO (PB6) -> Output (slave drives MISO back to master)
	 *   MOSI (PB5) -> Input
	 *   SCLK (PB7) -> Input
	 *   SS   (PB4) -> Input
	 */
	DIO_initPin(MISO, SPI_PORT, OUTPUT);
	DIO_initPin(MOSI, SPI_PORT, INPUT);
	DIO_initPin(SCLK, SPI_PORT, INPUT);
	DIO_initPin(SS,   SPI_PORT, INPUT);

	/* Slave Configuration:
	 * SPE  = 1  -> Enable SPI
	 * MSTR = 0  -> Set as Slave (default, cleared already)
	 */
	SPCR = (1 << SPE);

#endif /* SLAVE_MODE */
}

/*
 * SPI_SendRecieveData:
 *   - Write Data into SPDR to start the transfer (Master) or pre-load
 *     the reply byte (Slave).
 *   - Poll SPIF (SPI Interrupt Flag) until the 8-bit shift is complete.
 *   - Reading SPDR clears SPIF.
 *   - Returns the byte clocked in from the other device.
 */
uint8 SPI_SendRecieveData(uint8 Data) {
	/* Load data into SPI Data Register -> starts transmission */
	SPDR = Data;
	/* Wait until transmission complete (SPIF set) */
	while (!GET(SPSR, SPIF))
		;
	/* Return the byte received from the other end */
	return SPDR;
}
