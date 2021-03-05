/*
 * UART.cpp
 *
 *  Created on: Jul 6, 2018
 *      Author: tstern
 */

#include "UART.h"
#include "drivers/pin/pin.h"
#include "drivers/SERCOM/SERCOM.h"
#include "board.h"

void UART::isrHandler(void *ptr)
{
	UART *ptrUart=(UART *)ptr;


	//    if (ptrUart->_ptrSercom->USART.INTFLAG.bit.DRE && (ptrUart->_ptrSercom->USART.INTENSET.reg & SERCOM_USART_INTENSET_DRE))
	//    {
	//	if (ptrUart->txFifo.empty())
	//	{
	//	    //disable interrupts
	//	    ptrUart->_ptrSercom->USART.INTFLAG.bit.DRE=1; //clear flag
	//	    //disable tx interrupt
	//	    ptrUart->_ptrSercom->USART.INTENCLR.reg=SERCOM_USART_INTENSET_DRE;
	//	    //disableInterrupt(ptrUart->_ptrSercom);
	//	} else
	//	{
	//	    uint8_t data;
	//	    ptrUart->txFifo.pop(&data);
	//	    ptrUart->_ptrSercom->USART.DATA.reg=data;
	//	}
	//    }


	if (ptrUart->_ptrSercom->USART.STATUS.bit.BUFOVF)
	{
		ERROR("uart overflow");
		ptrUart->_ptrSercom->USART.STATUS.bit.BUFOVF=1;
	}

	if (ptrUart->_ptrSercom->USART.INTFLAG.bit.RXC)
	{
		uint8_t x;
		x=ptrUart->_ptrSercom->USART.DATA.reg; 
		ptrUart->rxFifo.push(&x);	
	}
}

UART::UART()
{
	_ptrSercom=NULL;
	_tx_buf_num=0;
	_tx_buf_index=0;
	_in_progress=false;

}

void UART::flush_tx(void)
{
	volatile uint32_t n, buf;

	if (_tx_buf_index==0)
	{

		return;
	}

	uint32_t to=millis(); 
	while(_tx_dma.isBusy())
	{
		if ((millis()-to)>100)
		{
			//we have waited 100ms for UART to transmit data, something is wrong
			_reset();
			ERROR("UART ERROR");
			return; 
		}
	}

	bool isr_state=InterruptDisable();
	_in_progress=true;
	n=_tx_buf_index;
	buf=_tx_buf_num;
	_tx_buf_num++;
	if (_tx_buf_num>=2)
	{
		_tx_buf_num=0;
	}
	_tx_buf_index=0;
	_in_progress=false;
	//    InterruptEnable(isr_state);
	//    
	//   
	//    
	//    isr_state=InterruptDisable();
	memcpy(_dma_buff,&_tx_buff[buf][0],UART_FIFO_SIZE_TX);

	_tx_dma.config(4, getSercomDMA_TX_ID(_ptrSercom), DMAC_CHCTRLA_TRIGACT_BURST_Val, 0, false);

	_tx_dma.ptrUserData=(void *) this;
	// _tx_dma.setCallback(txDMADone);


	_tx_dma.setFirstTransfer( _dma_buff, (void *)&_ptrSercom->USART.DATA.reg, 
			n, DMA_BEAT_SIZE_BYTE,
			true,false, NULL);

	__DSB();
	__ISB();
	_tx_dma.startTransfer(0); //start with highest interrupt priority. 

	InterruptEnable(isr_state);
}


bool UART::setBaud(uint32_t baud)
{
	uint32_t bit;
	_baud=0;
	if (_ptrSercom == NULL) {return false;}

	bool isrState=disableInterrupt(_ptrSercom);

	//save the enabled state. 
	bit =  _ptrSercom->USART.CTRLA.bit.ENABLE;

	//disable the UART
	_ptrSercom->USART.CTRLA.bit.ENABLE=0;

	if (!waitTimeoutWhile([this](void) {return _ptrSercom->USART.SYNCBUSY.bit.ENABLE==1;}, 100000))
	{
		ERROR("Time out waiting for sync busy");
		return false;
	}


	//set baud
	int32_t x=(uint32_t)( ((uint64_t)65536UL * (48000000UL - 16 * baud))
			/ 48000000UL);
	_ptrSercom->USART.BAUD.reg = x;

	//restore the enable bit
	_ptrSercom->USART.CTRLA.bit.ENABLE=bit;

	if (!waitTimeoutWhile([this](void) {return _ptrSercom->USART.SYNCBUSY.bit.ENABLE==1;}, 100000))
	{
		ERROR("Time out waiting for sync busy");
		return false;
	}

	//restore the interrupt state
	restoreInterrupt(_ptrSercom,isrState);
	_baud=baud; 
	return true;
}

bool UART::_reset(void)
{
	//reset the UART peripheral
	_ptrSercom->USART.CTRLA.reg=SERCOM_USART_CTRLA_SWRST;

	//wait for the peripheral to sync
	if (!waitTimeoutWhile([this](void) {return _ptrSercom->USART.SYNCBUSY.bit.SWRST==1;}, 100000))
	{
		_ptrSercom=NULL; // clear pointer so we know peripheral not setup
		ERROR("Time out waiting for sync busy");
		return false;
	}


	_ptrSercom->USART.CTRLA.reg=SERCOM_USART_CTRLA_DORD  //LSB transmitted first
			| SERCOM_USART_CTRLA_RXPO(0x01)  //RX pin is pad[1]
			| SERCOM_USART_CTRLA_MODE(0x01); //USART with internal clock

	_ptrSercom->USART.CTRLB.reg=SERCOM_USART_CTRLB_RXEN  //enable RX
			| SERCOM_USART_CTRLB_TXEN				//enable TX
			| SERCOM_USART_CTRLB_CHSIZE(0);   //set as 8 bit mode


	if (!waitTimeoutWhile([this](void) {return _ptrSercom->USART.SYNCBUSY.bit.CTRLB==1;}, 100000))
	{
		_ptrSercom=NULL; // clear pointer so we know peripheral not setup
		ERROR("Time out waiting for sync busy");
		return false;
	}

	if (!setBaud(_baud))
	{
		_ptrSercom=NULL;  // clear pointer so we know peripheral not setup
		ERROR("Could not set baud");
		return false;
	}

	//setup the CTRLA register
	_ptrSercom->USART.CTRLA.reg=SERCOM_USART_CTRLA_DORD  //LSB transmitted first
			| SERCOM_USART_CTRLA_RXPO(0x01)  //RX pin is pad[1]
			| SERCOM_USART_CTRLA_MODE(0x01) //USART with internal clock
			| SERCOM_USART_CTRLA_ENABLE;

	if (!waitTimeoutWhile([this](void) {return _ptrSercom->USART.SYNCBUSY.bit.ENABLE==1;}, 100000))
	{
		_ptrSercom=NULL; // clear pointer so we know peripheral not setup
		ERROR("Time out waiting for sync busy");
		return false;
	}

	_ptrSercom->USART.INTFLAG.reg=SERCOM_I2CM_INTFLAG_MASK;
	_ptrSercom->USART.INTENCLR.reg=SERCOM_USART_INTENSET_MASK;
	// _ptrSercom->USART.INTENSET.reg=SERCOM_USART_INTENSET_DRE;
	_ptrSercom->USART.INTENSET.reg=SERCOM_USART_INTENSET_RXC;
	return true;
}

bool UART::setup(const pin_t tx, const pin_t rx, uint32_t baud)
{

	//set up pins for SERCOM0
	_ptrSercom=(Sercom *)tx.ptrPerherial;
	_baud=baud;

	PinConfig(tx); 
	PinConfig(rx); 

	enableSercomNVICAndClocks(_ptrSercom, isrHandler, this);

	return _reset();

}


size_t  UART::read(uint8_t *ptrData, size_t numberBytes)
{
	uint32_t i,n;
	if (_ptrSercom == NULL) {return 0;}
	_ptrSercom->USART.INTENCLR.reg=SERCOM_USART_INTENSET_RXC;
	n = rxFifo.count();
	_ptrSercom->USART.INTENSET.reg=SERCOM_USART_INTENSET_RXC;
	if (n>numberBytes)
	{
		n=numberBytes;
	}
	for(i=0; i<n; i++)
	{
		uint8_t data=0;

		_ptrSercom->USART.INTENCLR.reg=SERCOM_USART_INTENSET_RXC;
		rxFifo.pop(&data);
		_ptrSercom->USART.INTENSET.reg=SERCOM_USART_INTENSET_RXC;
		ptrData[i]=data;
	}
	return (size_t)n;
}
size_t UART:: available(void)
{
	size_t n;
	if (_ptrSercom == NULL) {return 0;}
	_ptrSercom->USART.INTENCLR.reg=SERCOM_USART_INTENSET_RXC;
	n=(size_t)rxFifo.count();
	_ptrSercom->USART.INTENSET.reg=SERCOM_USART_INTENSET_RXC;
	return n;
}


size_t UART::write(const uint8_t *ptrData, size_t numberBytes)
{
	volatile uint32_t n,x;

	if (_ptrSercom == NULL) {return 0;}

	while (numberBytes>0)
	{
		while(_in_progress) 
		{

		}

		//if we call this function from outside and ISR and then from
		// inside an ISR we could have a buffer overflow. 
		// so we turn off all ISRs during function call. 
		bool isr_state=InterruptDisable();
		n=(UART_FIFO_SIZE_TX-_tx_buf_index);
		if (numberBytes<n)
		{
			n=numberBytes;
		}

#ifdef DEBUG		
		if ((_tx_buf_index+n)>UART_FIFO_SIZE_TX)
		{
			__BKPT(3); //you are overwritting buffer size
		}
#endif

		memcpy(&_tx_buff[_tx_buf_num][_tx_buf_index],ptrData,n);
		_tx_buf_index+=n;

		InterruptEnable(isr_state);
		if (numberBytes==1 || _tx_buf_index==UART_FIFO_SIZE_TX || NULL!=strnstr((char *)ptrData,"\n\r",numberBytes)  )
		{
			flush_tx();
		}
		x=x+n;
		numberBytes -= n;
	}

	return x;

}

