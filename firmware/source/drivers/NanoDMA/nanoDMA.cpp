/*
 * nanoDMA.cpp
 *
 *  Created on: Aug 10, 2017
 *      Author: tstern
 */

#include "nanoDMA.h"
#include "drivers/system/system.h"
#include "libraries/syslog/syslog.h"

#define NANO_DMA_INVALID_CHANNEL (255)
__attribute__ ((aligned (16))) DmacDescriptor NanoDMA::_descriptor[NANO_DMA_NUM_CHANNELS];
__attribute__ ((aligned (16))) DmacDescriptor NanoDMA::_write_back_section[NANO_DMA_NUM_CHANNELS];


volatile bool NanoDMA::_perpherialInitDone= false;
volatile uint16_t NanoDMA::_usedChannels=0;
NanoDMA * NanoDMA::ptrDMAS[NANO_DMA_NUM_CHANNELS]={0};

uint8_t NanoDMA::findNextFreeChannel(void)
{
    uint32_t i,j;

    j=0x01;
    i=0;
    while (_usedChannels & j)
    {
	i++;
	j=j<<1;
	if (i>=NANO_DMA_NUM_CHANNELS)
	{
	    ERROR("All DMA channels used");
	    return NANO_DMA_INVALID_CHANNEL;
	}
    }
    return i;
}

DmacDescriptor  *NanoDMA::getFirstDescriptor(void)
{
    return &_descriptor[_channel];
}



NanoDMA::NanoDMA(void)
{
    _channel = NANO_DMA_INVALID_CHANNEL;
    _ptrLastDescriptorTransfered=NULL;
    _status=NANO_DMA_STATUS_IDLE;
    _callback=NULL;
}

bool NanoDMA::release(void)
{
    if (_channel != NANO_DMA_INVALID_CHANNEL)
    {
	ptrDMAS[_channel]=NULL ;
	//lets do a reset on the channel
	DMAC->Channel[_channel].CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
	DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    }
    _channel = NANO_DMA_INVALID_CHANNEL;
    _ptrLastDescriptorTransfered=NULL;
    _status=NANO_DMA_STATUS_IDLE;
    _callback=NULL;
    return true;
}

bool NanoDMA::setup(void)
{

    _ptrLastDescriptorTransfered=NULL;
    _status=NANO_DMA_STATUS_IDLE;

    if (!_perpherialInitDone)
    {
	//enable the clock for the peripheral
	//first we need to enable a clock source for the DMA as generic clock source 0
	MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC ;

	// Perform a software reset before enable DMA controller
	DMAC->CTRL.reg &= ~DMAC_CTRL_DMAENABLE;
	DMAC->CTRL.reg = DMAC_CTRL_SWRST;
	/* Setup descriptor base address and write back section base
	 * address */
	DMAC->BASEADDR.reg = (uint32_t)_descriptor;
	DMAC->WRBADDR.reg = (uint32_t)_write_back_section;

	/* Enable all priority level at the same time */
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);
	_perpherialInitDone=true;
    }




    //find the next unused DMA channel
    _channel=findNextFreeChannel();

    //register the channel as used
    _usedChannels|=1ul<<_channel;


    if (_channel == NANO_DMA_INVALID_CHANNEL)
    {
	ERROR("could not allocate a DMA channel");
	return false;
    }
    ptrDMAS[_channel]=this;

    //lets do a reset on the channel
    //DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHID_ID(_channel);
    DMAC->Channel[_channel].CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHCTRLA_SWRST;

    return true;
}
bool NanoDMA::isBusy(void) 
{
    if (_channel == NANO_DMA_INVALID_CHANNEL)
    {
	return false; 
    }

    //if interrupts are enabled the status will change
    if (_status != NANO_DMA_STATUS_BUSY) //_status == NANO_DMA_STATUS_DONE || _status == NANO_DMA_STATUS_IDLE)
    {
	return false;
    }
    
    //if channel is disabled then the unit is not busy
    if (DMAC->Channel[_channel].CHCTRLA.bit.ENABLE ==0)
    {
	return false; 
    }

    if (ISRActive() || InterruptsEnabled()==false )
    {
	if (DMAC->Channel[_channel].CHSTATUS.bit.PEND || DMAC->Channel[_channel].CHSTATUS.bit.BUSY )
	{
	    return true;
	}

//	if (DMAC->Channel[_channel].CHSTATUS.bit.FERR || DMAC->Channel[_channel].CHSTATUS.bit.CRCERR )
//	{
//	    return false; // nothing we can do with error so assume done. 
//	}

	//if interrupts are not being called the TCMPL will be called. 
	if (DMAC->Channel[_channel].CHINTFLAG.bit.TCMPL)
	{
	    return false;
	}

    }
    return true;
}

bool NanoDMA::config(uint8_t priority, uint8_t perpherialTrigger,
	uint8_t triggerAction, uint8_t eventInputAction,
	bool eventOutputAction)
{
    if (_channel == NANO_DMA_INVALID_CHANNEL)
    {
	setup();
    }
    bool isrState =InterruptDisable(); //disable interrupts if not already done
    uint32_t x=0;

    //lets do a reset on the channel
    //DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHID_ID(_channel);
    DMAC->Channel[_channel].CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHCTRLA_SWRST;

    //LOG("setting up DMA on channel %d",_channel);
    /** Select the DMA channel and clear software trigger */
    DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << _channel));

    DMAC->Channel[_channel].CHPRILVL.reg=priority & 0x03;

    DMAC->Channel[_channel].CHINTFLAG.reg=0x07; //clear any flags 
    DMAC->Channel[_channel].CHINTENSET.reg=0x07;
    DMAC->Channel[_channel].CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(perpherialTrigger) |
	    DMAC_CHCTRLA_TRIGACT(triggerAction);


    if(eventInputAction)
    {
	x = DMAC_CHEVCTRL_EVIE | DMAC_CHEVCTRL_EVACT(eventInputAction);
    }

    /** Enable event output, the event output selection is configured in
     * each transfer descriptor  */
    if (eventOutputAction)
    {
	x |= DMAC_CHEVCTRL_EVOE;
    }

    /* Write config to Event control register */
    DMAC->Channel[_channel].CHEVCTRL.reg=x;

    InterruptEnable(isrState); //enable previous interrupt state
    return true;
}

bool NanoDMA::setFirstTransfer( void *source_memory, void *destination_memory,
	uint32_t xfercount, NanoDMABeatSize_t beatsize,
	bool srcinc, bool destinc,DmacDescriptor *nextDescriptor)
{
    return configDescriptor(&_descriptor[_channel], source_memory, destination_memory,
	    xfercount, beatsize, srcinc,destinc,nextDescriptor);
}

bool NanoDMA::enableDescriptor(DmacDescriptor *ptrDesc)
{
    ASSERT(ptrDesc);
    ptrDesc->BTCTRL.reg |=DMAC_BTCTRL_VALID;
    return true;
}

DmacDescriptor * NanoDMA::findDescriptor(DmacDescriptor *nextDescriptorMatch)
{
    DmacDescriptor *ptrStart=&_descriptor[_channel];
    DmacDescriptor *ptr=&_descriptor[_channel];

    while (ptr != NULL)
    {
	if (ptr->DESCADDR.reg == (uint32_t)nextDescriptorMatch)
	{
	    return ptr;
	}
	ptr=(DmacDescriptor *)ptr->DESCADDR.reg;

	//check for circular buffer
	if (ptr == ptrStart)
	{
	    return NULL;  //we are back where started with no match
	}
    }
    return NULL;
}

void NanoDMA::printPostMortem(void)
{
    DmacDescriptor __attribute__((unused)) *ptr;
    LOG("Channel %d", _channel);
    ptr=&_descriptor[_channel];
    LOG("Descriptor");
    //	LOG("\tSRCADDR 0x%" PRIX32, ptr->SRCADDR.reg);
    //	LOG("\tDSTADDR 0x%" PRIX32, ptr->DSTADDR.reg);
    //	LOG("\tDESCADDR 0x%" PRIX32, ptr->DESCADDR.reg);
    //	LOG("\tBTCNT 0x%" PRIX16, ptr->BTCNT.reg);
    //	LOG("\tBTCTRL 0x%" PRIX16, ptr->BTCTRL.reg);

    ptr=&_write_back_section[_channel];
    LOG("Write Back Descriptor");
    //	LOG("\tSRCADDR 0x%" PRIX32, ptr->SRCADDR.reg);
    //	LOG("\tDSTADDR 0x%" PRIX32, ptr->DSTADDR.reg);
    //	LOG("\tDESCADDR 0x%" PRIX32, ptr->DESCADDR.reg);
    //	LOG("\tBTCNT 0x%" PRIX16, ptr->BTCNT.reg);
    //	LOG("\tBTCTRL 0x%" PRIX16, ptr->BTCTRL.reg);

}
bool NanoDMA::configDescriptor(DmacDescriptor *buffer, void *source_memory, void *destination_memory,
	uint32_t xfercount, NanoDMABeatSize_t beatsize,
	bool srcinc, bool destinc,DmacDescriptor *nextDescriptor)
{
    uint8_t byteSize;
    uint16_t temp;
    ASSERT(buffer);
    ASSERT(source_memory);
    ASSERT(destination_memory);

    //WARNING("Config descriptor on channel %d", _channel);
    if (((uint32_t)buffer%16) != 0)
    {
	ERROR("Descriptor must be aligned to 16 bytes");
	return false;
    }

    //the source address and destination address
    // are last addresses when incremented
    switch(beatsize)
    {
	default:
	{
	    byteSize=1;
	    break;
	}
	case DMA_BEAT_SIZE_HWORD:
	{
	    byteSize=2;
	    break;
	}
	case DMA_BEAT_SIZE_WORD:
	{
	    byteSize=4;
	    break;
	}

    }


    buffer->SRCADDR.reg=(uint32_t)source_memory;
    buffer->DSTADDR.reg=(uint32_t)destination_memory;
    buffer->DESCADDR.reg=(uint32_t)nextDescriptor;
    buffer->BTCNT.reg=xfercount;

    //	if (beatsize != DMA_BEAT_SIZE_BYTE &&
    //			srcinc && destinc)
    //	{
    //		ERROR("can not increment both src and dst by more than 1 byte");
    //		return false;
    //	}
    temp=DMAC_BTCTRL_BEATSIZE(beatsize);

    if (srcinc)
    {
	temp|= DMAC_BTCTRL_STEPSEL_SRC | DMAC_BTCTRL_SRCINC;
	//the source address and destination address
	// are last addresses when incremented
	buffer->SRCADDR.reg=(uint32_t)source_memory + byteSize*xfercount;
    }
    if (destinc)
    {
	//the source address and destination address
	// are last addresses when incremented
	buffer->DSTADDR.reg=(uint32_t)destination_memory + byteSize* xfercount;
	temp|= DMAC_BTCTRL_DSTINC;
    }

    temp |= DMAC_BTCTRL_BLOCKACT_INT ;//interrupt when done
    temp |= DMAC_BTCTRL_VALID; //set the valid bit
    //LOG("BTCTRL 0x%04x",temp);
    buffer->BTCTRL.reg=temp;
    //LOG("Config descriptor done");
    return true;
}


NanoDMAcallback_t NanoDMA::getCallback(void)
{
    return _callback;
}

void *NanoDMA::getDestStartingAddress(DmacDescriptor * ptrDesc)
{
    uint32_t addr;
    uint32_t cnt;
    uint32_t byteSize;

    byteSize=ptrDesc->BTCTRL.bit.BEATSIZE*2;
    cnt=ptrDesc->BTCNT.reg;
    addr= ptrDesc->DSTADDR.reg - (cnt*byteSize);

    return (void *)addr;

}
void *NanoDMA::getSourceStartingAddress(DmacDescriptor * ptrDesc)
{
    uint32_t addr;
    uint32_t cnt;
    uint32_t byteSize;

    byteSize=ptrDesc->BTCTRL.bit.BEATSIZE*2;
    cnt=ptrDesc->BTCNT.reg;
    addr= ptrDesc->SRCADDR.reg - (cnt*byteSize);

    return (void *)addr;
}


uint8_t *NanoDMA::getActiveSourceBuffer(void)
{
    uint32_t addr;
    uint32_t cnt;
    uint32_t byteSize;
    DmacDescriptor * ptrDesc=&_descriptor[_channel];


    cnt=ptrDesc->BTCNT.reg;
    byteSize=ptrDesc->BTCTRL.bit.BEATSIZE*2;

    ptrDesc=&_write_back_section[_channel];
    addr= ptrDesc->SRCADDR.reg - (cnt*byteSize);

    return (uint8_t *)addr;
}




/**
 * \brief DMA interrupt service routine.
 *
 */
void DMAC_Handler( void )
{
    uint8_t active_channel;
    uint8_t isr;
    NanoDMA *ptrDMA;
    NanoDMAcallback_t callback;
    DmacDescriptor *ptrNext;
    DmacDescriptor *ptrDone;

    //ATOMIC_SECTION_ENTER

    /* Get Pending channel */
    active_channel =  DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk;


    /* Get active DMA resource based on channel */
    ptrDMA = NanoDMA::ptrDMAS[active_channel];
    callback=ptrDMA->getCallback();

    /* Select the active channel */
    isr = DMAC->Channel[active_channel].CHINTFLAG.reg;

    //find the pointer to next descriptor
    ptrNext=(DmacDescriptor *)NanoDMA::_write_back_section[active_channel].DESCADDR.reg;

    //LOG("\n\rnext %d", (uint32_t)ptrNext);
    ptrDone=ptrDMA->findDescriptor(ptrNext);
    ptrDMA->_ptrLastDescriptorTransfered=ptrDone;
    //LOG("done %d", (uint32_t)ptrDone);


    //	/* Calculate block transfer size of the DMA transfer */
    //	total_size = descriptor_section[resource->channel_id].BTCNT.reg;
    //	write_size = _write_back_section[resource->channel_id].BTCNT.reg;
    //	resource->transfered_size = total_size - write_size;

    /* DMA channel interrupt handler */
    if (isr & DMAC_CHINTENCLR_TERR) {
	/* Clear transfer error flag */
	DMAC->Channel[active_channel].CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;

	//WARNING("DMA IRQ ERROR");
	ptrDMA->setStatus(NANO_DMA_STATUS_ERROR);
	/* Execute the callback function */
	if (callback)
	{
	    callback(ptrDMA,NANO_DMA_TRANSFER_ERROR);
	}
    } else if (isr & DMAC_CHINTENCLR_TCMPL) {
	/* Clear the transfer complete flag */
	DMAC->Channel[active_channel].CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL;

	//WARNING("DMA IRQ DONE");
	ptrDMA->setStatus(NANO_DMA_STATUS_DONE);
	/* Execute the callback function */
	if (callback)
	{
	    callback(ptrDMA,NANO_DMA_TRANSFER_COMPLETE);
	}
    } else if (isr & DMAC_CHINTENCLR_SUSP) {
	/* Clear channel suspend flag */
	DMAC->Channel[active_channel].CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;

	//WARNING("DMA IRQ SUSPEND");
	ptrDMA->setStatus(NANO_DMA_STATUS_SUSPEND);
	/* Execute the callback function */
	if (callback)
	{
	    callback(ptrDMA,NANO_DMA_TRANSFER_SUSPEND);
	}
    }

    //ATOMIC_SECTION_LEAVE
}



bool NanoDMA::setCallback(NanoDMAcallback_t callback)
{
    _callback=callback;
    return true;
}
void  NanoDMA::setStatus(NanoDMAStatus_t status)
{
    _status=status;
}

bool NanoDMA::stopTransfer(void)
{
    bool isrState =InterruptDisable();
    DMAC->Channel[_channel].CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;

    _status=NANO_DMA_STATUS_IDLE;
    InterruptEnable(isrState); //enable previous interrupt state
    return true;
}

bool NanoDMA::startTransfer(uint32_t isr_priority)
{
    bool isrState =InterruptDisable(); //disable interrupts if not already done
    

    DMAC->Channel[_channel].CHINTFLAG.reg=0x07; //clear any flags

    //WARNING("starting transfer on %d",_channel);
    DMAC->Channel[_channel].CHINTENSET.reg = (DMAC_CHINTENSET_MASK & 0x07); //enable all interrupts



    /* Enable the transfer channel */
    DMAC->Channel[_channel].CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    if (_channel == 0)
    {
	NVIC_SetPriority(DMAC_0_IRQn, isr_priority);
	// Enable InterruptVector
	NVIC_EnableIRQ(DMAC_0_IRQn);
    }

    if (_channel == 1)
    {
	NVIC_SetPriority(DMAC_1_IRQn, isr_priority);
	// Enable InterruptVector
	NVIC_EnableIRQ(DMAC_1_IRQn);
    }

    if (_channel == 2)
    {
	NVIC_SetPriority(DMAC_2_IRQn, isr_priority);
	// Enable InterruptVector
	NVIC_EnableIRQ(DMAC_2_IRQn);
    }

    if (_channel == 3)
    {
	NVIC_SetPriority(DMAC_3_IRQn, isr_priority);
	// Enable InterruptVector
	NVIC_EnableIRQ(DMAC_3_IRQn);
    }

    if (_channel >= 4)
    {
	NVIC_SetPriority(DMAC_4_IRQn, 2);
	// Enable InterruptVector
	NVIC_EnableIRQ(DMAC_4_IRQn);
    }

    _status=NANO_DMA_STATUS_BUSY;
    InterruptEnable(isrState); //enable previous interrupt state
    return true;
}

void DMAC_0_Handler(void)
{
    DMAC_Handler();
}

void DMAC_1_Handler(void)
{
    DMAC_Handler();
}

void DMAC_2_Handler(void)
{
    DMAC_Handler();
}
void DMAC_3_Handler(void)
{
    DMAC_Handler();
}
void DMAC_4_Handler(void)
{
    DMAC_Handler();
}
