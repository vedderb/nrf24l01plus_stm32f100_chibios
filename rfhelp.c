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

#include "rfhelp.h"
#include "rf.h"
#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdbool.h>

// Variables
static Mutex rf_mutex;
static char pipe0_addr[5];
static char tx_addr[5];
static int address_length;
static bool tx_pipe0_addr_eq;

void rfhelp_init(void) {
	chMtxInit(&rf_mutex);

	address_length = rf_get_address_width();
	rf_read_reg(NRF_REG_RX_ADDR_P0, pipe0_addr, address_length);
	rf_read_reg(NRF_REG_TX_ADDR, tx_addr, address_length);

	if (memcmp(pipe0_addr, tx_addr, address_length) == 0) {
		tx_pipe0_addr_eq = true;
	} else {
		tx_pipe0_addr_eq = false;
	}
}

/**
 * Set TX mode, send data, wait for result, set RX mode.
 *
 * @param data
 * The data to be sent.
 *
 * @param len
 * Length of the data.
 *
 * @return
 * 0: Send OK.
 * -1: Max RT.
 * -2: Timeout
 */
int rfhelp_send_data(char *data, int len) {
	int timeout = 60;
	int retval = -1;

	chMtxLock(&rf_mutex);

	rf_mode_tx();
	rf_clear_irq();
	rf_flush_all();

	rf_write_tx_payload(data, len);

	// Set pipe0-address to the tx address for ack to work.
	if (!tx_pipe0_addr_eq) {
		rf_set_rx_addr(0, tx_addr, address_length);
	}

	for(;;) {
		int s = rf_status();

		chThdSleepMilliseconds(1);
		timeout--;

		if (NRF_STATUS_GET_TX_DS(s)) {
			retval = 0;
			break;
		} else if (NRF_STATUS_GET_MAX_RT(s)) {
			rf_clear_maxrt_irq();
			retval = -1;
			break;
		} else if (timeout == 0) {
			retval = -2;
			break;
		}
	}

	// Restore pipe0 address
	if (!tx_pipe0_addr_eq) {
		rf_set_rx_addr(0, pipe0_addr, address_length);
	}

	rf_mode_rx();

	chMtxUnlock();

	return retval;
}

/**
 * Read data from the RX fifo
 *
 * @param data
 * Pointer to the array in which to store the data.
 *
 * @param len
 * Pointer to variable storing the data length.
 *
 * @param pipe
 * Pointer to the pipe on which the data was received. Can be 0.
 *
 * @return
 * 1: Read OK, more data to read.
 * 0: Read OK
 * -1: No RX data
 */
int rfhelp_read_rx_data(char *data, int *len, int *pipe) {
	int retval = -1;

	chMtxLock(&rf_mutex);

	int s = rf_status();
	int pipe_n = NRF_STATUS_GET_RX_P_NO(s);

	if (pipe_n != 7) {
		*len = rf_get_payload_width();
		if (pipe) {
			*pipe = pipe_n;
		}
		rf_read_rx_payload(data, *len);
		rf_clear_rx_irq();

		s = rf_status();
		if (NRF_STATUS_GET_RX_P_NO(s) == 7) {
			retval = 0;
		} else {
			retval = 1;
		}
	}

	chMtxUnlock();

	return retval;
}

int rfhelp_rf_status(void) {
	chMtxLock(&rf_mutex);
	int s = rf_status();
	chMtxUnlock();

	return s;
}

void rfhelp_set_tx_addr(char *addr, int addr_len) {
	chMtxLock(&rf_mutex);
	memcpy(tx_addr, addr, addr_len);
	address_length = addr_len;

	if (memcmp(pipe0_addr, tx_addr, address_length) == 0) {
		tx_pipe0_addr_eq = true;
	} else {
		tx_pipe0_addr_eq = false;
	}

	rf_set_tx_addr(addr, addr_len);
	chMtxUnlock();
}

void rfhelp_set_rx_addr(int pipe, char *addr, int addr_len) {
	chMtxLock(&rf_mutex);
	if (pipe == 0) {
		memcpy(pipe0_addr, addr, addr_len);
	}
	address_length = addr_len;

	if (memcmp(pipe0_addr, tx_addr, address_length) == 0) {
		tx_pipe0_addr_eq = true;
	} else {
		tx_pipe0_addr_eq = false;
	}

	rf_set_rx_addr(pipe, addr, addr_len);
	chMtxUnlock();
}
