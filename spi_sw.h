/*
	Copyright 2015 Benjamin Vedder	benjamin@vedder.se

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef SPI_SW_H_
#define SPI_SW_H_

// Pin map
#define PORT_CE		GPIOA
#define PIN_CE		11
#define PORT_CSN	GPIOA
#define PIN_CSN		4
#define PORT_SCK	GPIOA
#define PIN_SCK		5
#define PORT_MOSI	GPIOA
#define PIN_MOSI	7
#define PORT_MISO	GPIOA
#define PIN_MISO	6

// Functions
void spi_sw_init(void);
void spi_sw_transfer(char *in_buf, const char *out_buf, int length);
void spi_sw_begin(void);
void spi_sw_end(void);
void spi_sw_write_ce(int state);

#endif /* SPI_SW_H_ */
