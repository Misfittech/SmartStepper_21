/*
 * SPI.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: tstern
 */

#include "spi.h"
#include "libraries/syslog/syslog.h"
#include "drivers/delay/delay.h"
#include "board.h"
#include "drivers/SERCOM/SERCOM.h"

static inline bool sercomSync(Sercom *pSercom)
{
	uint32_t t0=1000;

	while(pSercom->SPI.SYNCBUSY.bit.ENABLE && t0>0)
	{
		delay_us(1);
		t0--;
	}
	if (t0==0)
	{
		return false;
	}
	return true;
}


bool SPI::setup(Sercom *pSercom, pin_t mosi, pin_t miso, pin_t sck,pin_t cs)
{
	ASSERT(pSercom);
	_pinMosi=mosi;
	_pinMiso=miso;
	_pinSck=sck;
	_pinCs=cs;
	_pSercom=pSercom;

	//setup the pins
	PinConfig(_pinMosi);
	PinConfig(_pinMiso);
	PinConfig(_pinSck);
	PinConfig(_pinCs);


	//eanble pull up on MISO
	//PORT->Group[PIN_GET_PORT(_pinMiso.pinNum)].PINCFG[PIN_GET_PIN(_pinMiso.pinNum)].reg|=PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	//PORT->Group[PIN_GET_PORT(_pinMiso.pinNum)].OUTSET.reg=(1<<PIN_GET_PIN(_pinMiso.pinNum));

	PinEnableStrongDrive(_pinSck);
	PinEnableStrongDrive(_pinMosi);
	return true;
}
bool SPI::setClockRate(uint32_t frequency)
{
	bool enabled=(_pSercom->SPI.CTRLA.reg & SERCOM_SPI_CTRLA_ENABLE)!=0;
	if (enabled)
	{
		_pSercom->SPI.CTRLA.reg &= ~SERCOM_SPI_CTRLA_ENABLE;
		if (!sercomSync(_pSercom))
		 {
			 ERROR("sync failed");
			 return false;
		 }
	}

	uint32_t baud=0; //default to freq/2
	if (frequency< (TO_MHZ(48)/2))
	{
		baud=TO_MHZ(48)/(2*frequency)-1;
	}

	_pSercom->SPI.BAUD.reg =baud;

	if (enabled)
	{
		_pSercom->SPI.CTRLA.reg |=SERCOM_SPI_CTRLA_ENABLE;
		if (!sercomSync(_pSercom))
		 {
			 ERROR("sync failed");
			 return false;
		 }
	}
	return true;

}
bool SPI::init(uint32_t frequency,SPIMode_t mode,SPIDataOrder_t ord, uint32_t dataSize)
{
	uint32_t t0=1000;


	enableSercomNVICAndClocks(_pSercom, nullptr,nullptr);  //not using interrupts

	_pSercom->SPI.CTRLA.bit.ENABLE = 0; //disable the SERCOM

	 ASSERT(sercomSync(_pSercom));

	 //reset peripheral
	 _pSercom->SPI.CTRLA.bit.SWRST = 1;

	//Wait both bits Software Reset from CTRLA and SYNCBUSY are equal to 0
	t0=1000;
	while((_pSercom->SPI.CTRLA.bit.SWRST || _pSercom->SPI.SYNCBUSY.bit.SWRST)
			&& t0>0)
	{
		delay_us(1);
		t0--;
	}
	if (t0==0)
	{
		ERROR("sync failed");
		return false;
	}


	//Setting the CTRLA register
	 int32_t dopo=-1;
#if defined(__SAMD51__) || defined(__SAME51__)
	  if (_pinMosi.id == 0 && _pinSck.id ==1)
	 {
		 dopo=0;
	 }
	 if (_pinMosi.id == 3 && _pinSck.id ==1)
	 {
		 dopo=2;
	 }

#else
	 if (_pinMosi.id == 0 && _pinSck.id ==1)
	 {
		 dopo=0;
	 }
	 if (_pinMosi.id == 2 && _pinSck.id ==3)
	 {
		 dopo=1;
	 }
	 if (_pinMosi.id == 3 && _pinSck.id ==1)
	 {
		 dopo=2;
	 }
	 if (_pinMosi.id == 0 && _pinSck.id ==3)
	 {
		 dopo=3;
	 }
#endif
	 if (dopo==-1)
	 {
		 ERROR("MOSI and SCK pads are not correct");
		 return false;
	 }
#if defined(__SAMD51__) || defined(__SAME51__)
	 _pSercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE(0x3)  |
			SERCOM_SPI_CTRLA_DOPO(dopo) |
			SERCOM_SPI_CTRLA_DIPO(_pinMiso.id) |
			ord << SERCOM_SPI_CTRLA_DORD_Pos;
#else
	_pSercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_MASTER |
			SERCOM_SPI_CTRLA_DOPO(dopo) |
			SERCOM_SPI_CTRLA_DIPO(_pinMiso.id) |
			ord << SERCOM_SPI_CTRLA_DORD_Pos;
#endif
	int32_t x=-1;
	if (dataSize==8)
	{
		x=0;
	}
	if (dataSize==9)
	{
		x=1;
	}
	if (x==-1)
	{
		ERROR("Invalid data size, 8 and 9 bit are valid");
		return false;
	}
	//Setting the CTRLB register
	_pSercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(x) |
			SERCOM_SPI_CTRLB_RXEN;	//Active the SPI receiver.

	//now set up the clocks
	uint32_t clkPhase= (uint32_t)mode & 0x01;
	uint32_t clkPolarity= ((uint32_t)mode>>1) & 0x01;
	_pSercom->SPI.CTRLA.reg  |=	( clkPhase << SERCOM_SPI_CTRLA_CPHA_Pos ) |
			( clkPolarity << SERCOM_SPI_CTRLA_CPOL_Pos  ) ;

	uint32_t baud=0; //default to freq/2
	if (frequency< (TO_MHZ(48)/2))
	{
		baud=TO_MHZ(48)/(2*frequency)-1;
	}

	_pSercom->SPI.BAUD.reg =baud;

	//_pSercom->SPI.CTRLA.reg=ctrlA;
	_pSercom->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
	if (!sercomSync(_pSercom))
	 {
		 ERROR("sync failed");
		 return false;
	 }
	return true;
}

size_t SPI::write(const uint8_t *ptrData, size_t bytes)
{
	uint32_t n=0;
	bool ret;
	uint8_t x;
	while (bytes>0)
	{
		ret=transfer(*ptrData,&x);
		if (!ret)
		{
			return n;
		}
		ptrData++;
		bytes--;
		n++;
	}
	return n;
}

size_t SPI::read(uint8_t *ptrData, size_t cnt)
{
	uint32_t n=0;
	bool ret;
	while (cnt>0)
	{
		ret=transfer(n,ptrData);
		if (!ret)
		{
			return n;
		}
		ptrData++;
		cnt--;
		n++;
	}
	return n;
}

bool SPI::transfer(uint8_t txData, uint8_t *ptrRxData)
{
	uint32_t to=0;


	to=10000;
	while( _pSercom->SPI.INTFLAG.bit.DRE == 0 && to>0)
	{
		// Waiting Data Registry Empty
		//_delay_us(1);
		to--;
	}
	if (to==0)
	{
		ERROR("time out on write");
		return false;
	}
	while(_pSercom->SPI.INTFLAG.bit.RXC)
	{
		_pSercom->SPI.DATA.reg;
	}

	_pSercom->SPI.DATA.bit.DATA = txData; // Writing data into Data register
	to=10000;
	while(_pSercom->SPI.INTFLAG.bit.RXC == 0
			&& to>0)
	{
		// Waiting Complete Reception
		//_delay_us(1);
		to--;
	}
	if (to==0)
	{
		ERROR("time out on write");
		return false;
	}

	*ptrRxData=_pSercom->SPI.DATA.bit.DATA;
	return true;
}
size_t SPI::transfer(const uint8_t *ptrTXData, uint8_t *ptrRXData, size_t cnt)
{
	uint32_t n=0;
	bool ret;

	while (cnt>0)
	{
		ret=transfer(*ptrTXData,ptrRXData);
		if (!ret)
		{
			return n;
		}
		ptrRXData++;
		ptrTXData++;
		cnt--;
		n++;
	}
	return n;
}
