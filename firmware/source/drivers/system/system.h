/*
 * system.h
 *
 *  Created on: Aug 6, 2017
 *      Author: tstern
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

//#include <core_cmFunc.h>
#include <assert.h>
#include "sam.h"
#include "stdbool.h"
#include "drivers/delay/delay.h"
#include "drivers/systick/systick.h"


typedef bool bool_t;



#define HIGH (true)
#define LOW (false)

#define BIT_SET(x,bit) ( (x) |= ((1)<<(bit)) )
#define BIT_CLR(x,bit) ( (x) &= (~((1)<<(bit))) )

#define ABS(x) (((x)<0)?(-(x)):(x))
#define SGN(x) (((x)<0)?(-1):(1)))

#define MAX(x,y) (((x)<(y))?(y):(x))
#define MIN(x,y) (((x)<(y))?(x):(y))

#define TO_MHZ(x)  ((x) * 1000000L) // use uppercase L to avoid confusion
#define TO_KHZ(x)  ((x) * 1000L) // use uppercase L to avoid confusion
#define TO_MSEC(x) ((x) / 1000L )   // with the digit 1

#define PACK            __attribute__((packed))
#define WEAK            __attribute__((weak))
#define INLINE          static inline __attribute__((always_inline))
#define ALIGN(x)		__attribute__ ((aligned (x)))
#define LIMIT(a, b)     (((int)(a) > (int)(b)) ? (int)(b) : (int)(a))

#define POINTER(T)  typeof(T *)
#define ARRAY(T, N) typeof(T [N])

#ifndef PI
#define PI 3.14159265
#endif

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)
#define OFFSETOF(type, member) (size_t)&(((type *)0)->member)

#define ASSERT_FAIL(msg) do { \
   assert(!msg);\
   } while (0)

#define ATOMIC_SECTION_ENTER   { bool __isrState =InterruptDisable();
#define ATOMIC_SECTION_LEAVE   InterruptEnable(__isrState); }

#define ZERO_ARRAY(x)  memset(x,0,sizeof(x));

#define likely(x)  	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)



#define SIGN(x) (((x)>=0) ? (1) : (0))

#define DIVIDE_WITH_ROUND(x, divisor)(          \
{                           \
    typeof(x) __x = x;              \
    typeof(divisor) __d = divisor;          \
    (((typeof(x))-1) > 0 ||             \
     ((typeof(divisor))-1) > 0 || (__x) > 0) ?  \
        (((__x) + ((__d) / 2)) / (__d)) :   \
        (((__x) - ((__d) / 2)) / (__d));    \
}                           \
)


//get the priority of the currently active interrupt
// if no interrupt is enable it will return -1
static uint32_t inline  GetISRPriority(void)
{
	int32_t x;
	x=__get_IPSR() & 0x0FF;
	x=x-16;
	if (x<-15 || x>=PERIPH_COUNT_IRQn)
	{
		return (-1);
	}
	return NVIC_GetPriority((IRQn_Type)x);
}

//returns true if code is being ran in interrupt context. 
static inline bool ISRActive(void)
{
	uint32_t x;
	x=__get_IPSR() & 0x0FF;

	return x!=0;
}

static inline bool InterruptsEnabled(void)
{
	uint32_t prim;
	/* Read PRIMASK register, check interrupt status before you disable them */
    /* Returns 0 if they are enabled, or non-zero if disabled */
    prim = __get_PRIMASK();

    //note the above code looks like a race condition
    // however if interrupts state changed between the two above functions
    // it was from and interrupt and either way we will restore state before we
    // disabled interrupts.
    return (prim == 0);
}
/*
 *  Interrupt Disable routine which returns the state of interrupts
 *  before disabling
 *
 *  This disables all interrupts
 *
 * 	Returns true if interrupts were enabled
 */
static inline bool InterruptDisable(void)
{
	uint32_t prim;
	/* Read PRIMASK register, check interrupt status before you disable them */
    /* Returns 0 if they are enabled, or non-zero if disabled */
    prim = __get_PRIMASK();
      /* Disable interrupts */
    __disable_irq();

    //note the above code looks like a race condition
    // however if interrupts state changed between the two above functions
    // it was from and interrupt and either way we will restore state before we
    // disabled interrupts.
    return (prim == 0);
}


/*
 *  Interrupt enable routine which enables interrupt if passed a true
 *
 *  This enables global interrutps
 */
static inline void InterruptEnable(bool state)
{
	if (state) //if we state is true enable the interrupts
	{
		__enable_irq();
		__ASM volatile ("dsb" : : : "memory");
		__ASM volatile ("isb" : : : "memory");
	}
}

/*
 *  Theses are some simple functions to do a Mutex
 */

typedef volatile int Mutex_t;

static inline void MutexRelease(volatile Mutex_t *ptrMutex)
{
   bool  isrState=InterruptDisable();
   *ptrMutex=0;
   InterruptEnable(isrState);
}

static inline  bool MutexAcquire(volatile Mutex_t *ptrMutex)
{
   bool  isrState=InterruptDisable();
   if (*ptrMutex!=0)
   {
      InterruptEnable(isrState);
      return false; //did not get mute
   }
   *ptrMutex=1;
   InterruptEnable(isrState);
   return true; //got mutex
}

static inline char* lltoa(long long val, int base){

    static char buf[64] = {0};

    int i = 62;
    int sign = (val < 0);
    if(sign) val = -val;

    if(val == 0) return (char *)"0";

    for(; val && i ; --i, val /= base) {
        buf[i] = "0123456789abcdef"[val % base];
    }

    if(sign) {
        buf[i--] = '-';
    }
    return &buf[i+1];

}


#ifdef __cplusplus
#include <functional>
//This function will time out if the function passed in does not return false
// before the microSeconds have passed
static inline bool waitTimeoutWhile(std::function<bool()> ptrFunc, uint32_t microSeconds)
#else
//This function will time out if the function passed in does not return false
// before the microSeconds have passed
static inline bool waitTimeoutWhile(bool (*ptrFunc)(), uint32_t microSeconds)
#endif
{

	//if interrupts are enabled we can use the systick timer
	if (InterruptsEnabled())
	{
		uint32_t to=micros();
		while(ptrFunc())
		{
			if ((micros()-to)>microSeconds)
			{
				return false;
			}
		}
		return true;
	}
	//if interrupts are not enabled we have to use spin loop delay
	// this will not be super accurate but...
	while(ptrFunc())
	{
		delay_us(1);
		if (microSeconds==0)
		{
			return false;
		}
		microSeconds--; //increment after in case someone passed in zero to function.
	}
	return true;
}


#endif /* SYSTEM_H_ */
