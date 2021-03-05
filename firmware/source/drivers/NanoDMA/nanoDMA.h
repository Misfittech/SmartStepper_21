/*
 * nanoDMA.h
 *
 *  Created on: Aug 10, 2017
 *      Author: tstern
 */

#ifndef NANODMA_H_
#define NANODMA_H_
#include "sam.h"
#include <memory.h>

#define NANO_DMA_NUM_CHANNELS (5)

typedef enum  {
	/** 8-bit access. */
	DMA_BEAT_SIZE_BYTE = 0,
	/** 16-bit access. */
	DMA_BEAT_SIZE_HWORD =1,
	/** 32-bit access. */
	DMA_BEAT_SIZE_WORD = 2,
} NanoDMABeatSize_t;

typedef enum {
	NANO_DMA_TRANSFER_ERROR = 0x01,
	NANO_DMA_TRANSFER_COMPLETE = 0x02,
	NANO_DMA_TRANSFER_SUSPEND =0x04
}NanoDMAInterruptFlag_t;

typedef enum {
	NANO_DMA_STATUS_IDLE = 0,
	NANO_DMA_STATUS_BUSY = 1,
	NANO_DMA_STATUS_ERROR =2,
	NANO_DMA_STATUS_DONE =3,
	NANO_DMA_STATUS_SUSPEND =4,
}NanoDMAStatus_t;


typedef void (*NanoDMAcallback_t)(void *ptrClass, NanoDMAInterruptFlag_t flag);
class NanoDMA {
	private:
		  static __attribute__ ((aligned (8))) DmacDescriptor _descriptor[NANO_DMA_NUM_CHANNELS];
		  static __attribute__ ((aligned (8))) DmacDescriptor _write_back_section[NANO_DMA_NUM_CHANNELS];

		  volatile static bool _perpherialInitDone;
		  static volatile uint16_t _usedChannels; //bit mask for channels that have been used

		  volatile uint8_t _channel;
		  NanoDMAcallback_t _callback;
		  volatile NanoDMAStatus_t _status;

		  DmacDescriptor *_ptrLastDescriptorTransfered;  //this is a pointer to the last descriptor DMA'd

		  uint8_t findNextFreeChannel(void); //find the next channel that is free


	public:
		  void *ptrUserData;
		  friend void DMAC_Handler( void );
		  static NanoDMA * ptrDMAS[NANO_DMA_NUM_CHANNELS];
		  NanoDMA(void);
		  ~NanoDMA(void) {};
		  bool setup(void);
		  bool release(void);
		  NanoDMAcallback_t getCallback(void);
		  DmacDescriptor *getFirstDescriptor(void);
		  DmacDescriptor *findDescriptor(DmacDescriptor *nextDescriptorMatch);
		  bool config(uint8_t priority, uint8_t perpherialTrigger,
				  uint8_t triggerAction, uint8_t eventInputAction,
				   bool eventOutputAction);

		  bool setFirstTransfer(void *source_memory, void *destination_memory,
					  uint32_t xfercount, NanoDMABeatSize_t beatsize,
					  bool srcinc, bool destinc, DmacDescriptor *nextDescriptor=NULL);

		  bool configDescriptor(DmacDescriptor *buffer, void *source_memory, void *destination_memory,
					  uint32_t xfercount, NanoDMABeatSize_t beatsize,
					  bool srcinc, bool destinc, DmacDescriptor *nextDescriptor=NULL);

		  DmacDescriptor *getLastDescriptorTransfered(void) { return _ptrLastDescriptorTransfered;}

		  bool enableDescriptor(DmacDescriptor *ptrDesc);

		  bool setCallback(NanoDMAcallback_t callback);
		  bool startTransfer(uint32_t isr_priority=2);
		  bool stopTransfer(void);
		  void setStatus(NanoDMAStatus_t status);
		  NanoDMAStatus_t getStatus(void) {return _status;}
		  bool isBusy(void);
		  void printPostMortem(void);

		  void *getSourceStartingAddress(DmacDescriptor * ptrDesc);
		  void *getDestStartingAddress(DmacDescriptor * ptrDesc);
		  uint8_t *getActiveSourceBuffer(void);
};





#endif /* NANODMA_H_ */
