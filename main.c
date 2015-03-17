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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "spi_sw.h"
#include "rf.h"
#include "rfhelp.h"

static Mutex print_mutex;
static WORKING_AREA(rx_thread_wa, 256);
static WORKING_AREA(tx_thread_wa, 256);

const SerialConfig sc1_config =
{
		115200,     // baud rate
		0,          // CR1 register
		0,          // CR2 register
		0           // CR3 register
};

void printf_thd(const char* format, ...) {
	va_list arg;
	va_start(arg, format);

	chMtxLock(&print_mutex);
	chvprintf((BaseSequentialStream*)&SD1, format, arg);
	chMtxUnlock();
}

void print_rf_status(void) {
	int s = rfhelp_rf_status();

	printf_thd("RF Status Register\r\n");
	printf_thd("RX_DR  TX_DS  MAX_RT  RX_P_NO  TX_FULL\r\n");
	printf_thd("%i      %i      %i       %i        %i\r\n",
			NRF_STATUS_GET_RX_DR(s), NRF_STATUS_GET_TX_DS(s), NRF_STATUS_GET_MAX_RT(s),
			NRF_STATUS_GET_RX_P_NO(s), NRF_STATUS_GET_TX_FULL(s));
}

static msg_t tx_thread(void *arg) {
	(void)arg;

	chRegSetThreadName("TX");

	for(;;) {
		char pl[3] = {1, 4, 6};

		switch (rfhelp_send_data(pl, 3)) {
			case 0: printf_thd("Send probably OK\r\n"); break;
			case -1: printf_thd("Max RT\r\n"); break;
			case -2: printf_thd("Timeout\r\n"); break;
			default: break;
		}
		printf_thd("\r\n");
		chThdSleepMilliseconds(500);

		pl[0] = 119;
		switch (rfhelp_send_data(pl, 3)) {
		case 0: printf_thd("Send probably OK\r\n"); break;
		case -1: printf_thd("Max RT\r\n"); break;
		case -2: printf_thd("Timeout\r\n"); break;
		default: break;
		}
		printf_thd("\r\n");
		chThdSleepMilliseconds(500);
	}

	return 0;
}

static msg_t rx_thread(void *arg) {
	(void)arg;

	chRegSetThreadName("RX");

	for(;;) {
		char buf[32];
		int len;
		int pipe;

		for(;;) {
			int res = rfhelp_read_rx_data(buf, &len, &pipe);

			// If something was read
			if (res >= 0) {
				printf_thd("Pipe: %i\r\n", pipe);
				printf_thd("PL Length: %i\r\n", len);

				for(int i = 0;i < len;i++) {
					printf_thd("PL B%i: %i\r\n", i, buf[i]);
				}

				printf_thd("\r\n");
			}

			// Stop when there is no more data to read.
			if (res <= 0) {
				break;
			}
		}

		chThdSleepMilliseconds(1);
	}

	return 0;
}

int main(void) {
	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	// initialize serial port driver
	sdStart(&SD1, &sc1_config);

	// Setup pins
	palSetPadMode(GPIOA, 9, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	palSetPadMode(GPIOA, 10, PAL_MODE_INPUT);

	// Initialize mutexes
	chMtxInit(&print_mutex);

	// NRF
	rf_init();
	rfhelp_init();

	print_rf_status();

	// Start threads
	chThdCreateStatic(rx_thread_wa, sizeof(rx_thread_wa), NORMALPRIO, rx_thread, NULL);
	chThdCreateStatic(tx_thread_wa, sizeof(tx_thread_wa), NORMALPRIO, tx_thread, NULL);

	for(;;) {
		chThdSleepMilliseconds(1000);
//		printf_thd("Hello World!\r\n");
	}

	return 0;
}
