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

// Variables
static Mutex rf_mutex;

void rfhelp_init(void) {
	chMtxInit(&rf_mutex);
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
 * 0: Probably OK.
 * -1: Max RT.
 * -2: Timeout
 */
int rfhelp_send_data(char *data, int len) {
	int timeout = 100;

	chMtxLock(&rf_mutex);
	rf_mode_tx();
	rf_flush_all();
	rf_clear_irq();
	rf_write_tx_payload(data, len);

	int s = rf_status();
	while (!(NRF_STATUS_GET_TX_DS(s) || NRF_STATUS_GET_MAX_RT(s)) && timeout) {
		chThdSleepMilliseconds(1);
		s = rf_status();
		timeout--;
	}
	rf_mode_rx();
	chMtxUnlock();

	if (NRF_STATUS_GET_MAX_RT(s)) {
		return -1;
	} else if (timeout == 0) {
		return -2;
	}

	return 0;
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
 * 0: Read OK
 * -1: No RX data
 * -2: RX fifo full
 */
int rfhelp_read_rx_data(char *data, int *len, int *pipe) {
	int retval = -1;

	chMtxLock(&rf_mutex);

	int s = rf_status();
	int sf = rf_fifo_status();
	int pipe_n = NRF_STATUS_GET_RX_P_NO(s);

	if (pipe_n != 7) {
		*len = rf_get_payload_width();
		if (pipe) {
			*pipe = pipe_n;
		}
		rf_read_rx_payload(data, *len);
		rf_clear_rx_irq();
		retval = 0;
	}

	if (sf & NRF_FIFO_RX_FULL) {
		rf_flush_rx();

		if (retval != 0) {
			retval = -2;
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
