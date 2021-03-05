/*
 * I2C.c
 *
 *  Created on: May 3, 2018
 *      Author: xps15
 */


#include "libraries/syslog/syslog.h"
#include "drivers/sercom/sercom.h"
#include "drivers/delay/delay.h"
#include "drivers/I2C/I2CMaster.h"

bool_t sync(Sercom *ptrSercom)
{
	int32_t t0=1000;
	while(t0>0 && ptrSercom->I2CM.SYNCBUSY.reg)
	{
		// Waiting for synchronization
		t0--;
		delay_us(100);
	}
	if (t0==0)
	{
		ERROR("sync error");
		return false;
	}
	return true;
}


bool_t reset(Sercom *ptrSercom)
{
	//I2CM OR I2CS, no matter SWRST is the same bit.
	//Setting the Software bit to 1
	ptrSercom->I2CM.CTRLA.bit.SWRST = 1;

	if (!sync(ptrSercom))
	{
		ERROR("sync error");
		return false;
	}
	return true;
}

bool_t enable(Sercom *ptrSercom)
{
	// Enable the I�C
	ptrSercom->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE ;

	if(!sync(ptrSercom))
	{
		ERROR("sync");
		return false;
	}

	// Setting bus idle mode
	ptrSercom->I2CM.STATUS.bit.BUSSTATE = 1 ;

	if(!sync(ptrSercom))
	{
		ERROR("sync");
		return false;
	}
	return true;
}

void I2CMasterISRCallback(__attribute__((unused))void *ptrParam);
void I2CMasterISRCallback(__attribute__((unused))void *ptrParam)
{
	//	I2CMasterDevice_t *ptrDevice;
	//
	//	ptrDevice=(I2CMasterDevice_t *)ptrParam;
	//We currently do not use the interrupt for I2C master
}

bool_t I2CMaster::init(const pin_t SDA, const pin_t SCL, uint32_t clockRate)
{
	if (SDA.ptrPerherial == 0 ||
			SCL.ptrPerherial ==0 ||
			SDA.ptrPerherial != SCL.ptrPerherial)
	{
		ERROR("Pins peripherals do not match");
		return false;
	}


	pinSCL=SCL;
	pinSDA=SDA;

	PinSetAsInput(pinSCL);
	PinSetAsInput(pinSDA);
	if (!PinRead(pinSCL))
	{
		ERROR("SCL pin low");
		return false;
	}
	if (!PinRead(pinSDA))
	{
		ERROR("SDA pin low");
		return false;
	}

	ptrSercom=(Sercom *)pinSCL.ptrPerherial;

	ASSERT(enableSercomNVICAndClocks(ptrSercom, I2CMasterISRCallback,NULL));

	ASSERT(reset(ptrSercom));

	ptrSercom->I2CM.CTRLA.reg =  SERCOM_I2CM_CTRLA_MODE(0x5);
	// Enable Smart mode and Quick Command
	//sercom->I2CM.CTRLB.reg =  SERCOM_I2CM_CTRLB_SMEN /*| SERCOM_I2CM_CTRLB_QCEN*/ ;
	// Enable all interrupts
	//  sercom->I2CM.INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB | SERCOM_I2CM_INTENSET_ERROR ;
	ptrSercom->I2CM.BAUD.bit.BAUD = 48000000UL / ( 2 * clockRate) - 1 ;

	PinConfig(SCL);
	PinConfig(SDA);
	ASSERT(enable(ptrSercom));

	return true;
}

bool_t isBusIdle( Sercom *ptrSercom )
{
	return ptrSercom->I2CM.STATUS.bit.BUSSTATE == 1;
}

bool_t I2CSendAddress(Sercom *ptrSercom, uint8_t txAddress, bool_t readFlag)
{

	int32_t t0;
	txAddress=txAddress<<1;
	if (readFlag)
	{
		txAddress |= 0x01; //add the read bit (R/~W)
	}

	ASSERT(ptrSercom);

	if (ptrSercom->I2CM.STATUS.bit.BUSSTATE==0)
	{
		ERROR("Unknown bus state");
		return false;
	}

	t0=1000;
	while (t0>0 && ptrSercom->I2CM.STATUS.bit.BUSSTATE==0x3)
	{
		delay_us(100);
		t0--;
	}
	if (t0==0)
	{
		ERROR("Bus busy");
		return false;
	}

	//set the address
	ptrSercom->I2CM.ADDR.bit.ADDR = txAddress;

	if (!readFlag) //we are doing a write
	{
		//wait for transmission to be completed
		t0=1000;
		while (t0>0 && !ptrSercom->I2CM.INTFLAG.bit.MB)
		{
			delay_us(100);
			t0--;
		}
		if (t0==0)
		{
			ERROR("Bus busy");
			return false;
		}
	}else
	{
		//we are doing a read
		//wait for Slave on bus flag
		t0=1000;
		while (t0>0 && !ptrSercom->I2CM.INTFLAG.bit.SB)
		{
			//if the master bit set it indicates and error from slave
			// NACK or slave not there
			if (ptrSercom->I2CM.INTFLAG.bit.MB)
			{
				ptrSercom->I2CM.CTRLB.bit.CMD=3; //issue stop command
				return false;
			}
			delay_us(100);
			t0--;
		}
		if (t0==0)
		{
			ERROR("Bus error");
			return false;
		}
	}

	//now lets check for ACk/NACK
	if(ptrSercom->I2CM.STATUS.bit.RXNACK)
	{
		//this is normal and not and error
		return false;
	}

	return true;
}


static bool_t setCommandBitsWire(Sercom *ptrSercom,uint8_t cmd)
{
	ptrSercom->I2CM.CTRLB.bit.CMD = cmd;
	if (!sync(ptrSercom))
	{
		ERROR("sync");
		return false;
	}

	return true;
}

bool_t sendDataMasterI2C(Sercom *ptrSercom,uint8_t data)
{
	int32_t t0;
	//Send data
	ptrSercom->I2CM.DATA.bit.DATA = data;
	//Wait transmission successful
	t0=1000;
	while(t0>0 && !ptrSercom->I2CM.INTFLAG.bit.MB) {
		t0--;
		delay_us(100);
		// If a bus error occurs, the MB bit may never be set.
		// Check the bus error bit and bail if it's set.
		if (ptrSercom->I2CM.STATUS.bit.BUSERR) {
			return false;
		}
	}

	//Problems on line? nack received?
	if(ptrSercom->I2CM.STATUS.bit.RXNACK)
		return false;
	else
		return true;
}

bool_t enableAck(Sercom *ptrSercom)
{
	ptrSercom->I2CM.CTRLB.bit.ACKACT = 0;
	if (!sync(ptrSercom))
	{
		ERROR("sync");
		return false;
	}
	return true;
}

bool_t enableNack(Sercom *ptrSercom)
{
	ptrSercom->I2CM.CTRLB.bit.ACKACT = 1;
	if (!sync(ptrSercom))
	{
		ERROR("sync");
		return false;
	}
	return true;
}

bool_t I2CMasterReadByte(Sercom *ptrSercom, uint8_t *ptrData)
{
	int32_t t0=1000;
	while(t0>0 && !ptrSercom->I2CM.INTFLAG.bit.SB)
	{
		t0--;
		delay_ms(1);
	}
	if (t0==0)
	{
		ERROR("time out waiting to read");
		return false;
	}
	*ptrData=ptrSercom->I2CM.DATA.bit.DATA;
	return true;
}

size_t I2CMaster::write( uint8_t txAddress, bool_t stopBit, uint8_t *ptrData, size_t count)
{
	size_t n=0;

	if (!I2CSendAddress(ptrSercom,txAddress,false)) //send address in write mode
	{
		ASSERT(setCommandBitsWire(ptrSercom,3));
		ERROR("Slave did not respond adr=0x%2X",txAddress);
		return n;
	}

	while (n<count)
	{
		if (!sendDataMasterI2C(ptrSercom,ptrData[n]))
		{
			//send stop
			ASSERT(setCommandBitsWire(ptrSercom,3));
			return n;
		}
		n++;
	}
	if (stopBit)
	{
		ASSERT(setCommandBitsWire(ptrSercom,3)); //send stop bit
	}
	return n;
}

size_t I2CMaster::read( uint8_t txAddress, bool_t stopBit, uint8_t *ptrData, size_t count)
{
	size_t n=0;
	uint8_t data;


	if (I2CSendAddress(ptrSercom,txAddress,true)) //send address in read mode
	{
		enableAck(ptrSercom);
		while (n<count)
		{
			if (!I2CMasterReadByte(ptrSercom,&data))
			{
				ERROR("read failed");
				return n;
			}
			ptrData[n]=data;
			n++;
			if (n==count)
			{
				enableNack(ptrSercom);
			}else
			{
				ASSERT(setCommandBitsWire(ptrSercom,2)); //send ack
			}
		}
	}
	if (stopBit)
	{
		ASSERT(setCommandBitsWire(ptrSercom,3)); //send stop bit
	}else
	{
		ASSERT(setCommandBitsWire(ptrSercom,2)); //send ack
	}
	return n;
}


