A ChibiOS-project with a driver for the nrf24l01+ RF transceiver.

* Software SPI is used so that it can be mapped to any pins.
* CE can be tied high externally.
* Polling is used instead of reading the INT pin.

The driver is designed to use few pins and have high flexibility. Since it uses polling instead of interrupts, it uses a bit more CPU resources than necessary.
