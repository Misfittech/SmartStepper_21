/*
 * UART.h
 *
 *  Created on: Jul 6, 2018
 *      Author: tstern
 */

#ifndef DRIVERS_UART_UART_H_
#define DRIVERS_UART_UART_H_


#include "drivers/pin/pin.h"
#include "libraries/fifo/fifo.h"
#include "drivers/NanoDMA/NanoDMA.h"
#include "drivers/systick/systick.h"

#define UART_FIFO_SIZE_TX (64)
#define UART_FIFO_SIZE_RX (64)
class UART {
public:
	UART();
	bool setup(const pin_t tx, const pin_t rx, uint32_t baud);
	bool setBaud(uint32_t baud);
	uint32_t getBaud(void) { return _baud;}
	size_t write(const uint8_t *ptrData, size_t numberBytes);
	size_t read(uint8_t *ptrData, size_t numberBytes);
	size_t available(void);
	void flush_tx(void); 
private:
	static void isrHandler(void *ptr);
	Sercom *_ptrSercom;
	uint32_t _baud;
	
	bool _reset(void); 
	volatile bool _in_progress;

	
	volatile uint32_t _tx_buf_num;
	volatile uint32_t _tx_buf_index;
	__attribute__ ((aligned (8))) uint8_t _dma_buff[UART_FIFO_SIZE_TX];
	uint8_t _tx_buff[2][UART_FIFO_SIZE_TX];
	NanoDMA _tx_dma;

	
	//FIFO<uint8_t, UART_FIFO_SIZE_TX> txFifo;
	FIFO<uint8_t, UART_FIFO_SIZE_RX> rxFifo;
friend void txDMADone(void *ptrClass, NanoDMAInterruptFlag_t flag);
};


#endif /* DRIVERS_UART_UART_H_ */
