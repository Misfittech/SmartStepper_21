/*
 * system.cpp
 *
 *  Created on: Aug 7, 2019
 *      Author: Trampas
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
//we need to decrease the optimization so the the compiler
//does not ignore func and args

#pragma GCC push_options
#pragma GCC optimize ("O0")
void service_call(__attribute__((unused)) void (*func)(void*), __attribute__((unused)) void* args) {
     //by convention func is in r0 and args is in r1
	//asm volatile("MOV R0, %0\n\t" : "=r" (func));
	//asm volatile("MOV R1, %0\n\t" : "=r" (args));
     asm volatile("svc 0");
}
#pragma GCC pop_options

typedef void (*svcall_t)(void*);

void SVC_Handler(void){

  __asm volatile (
  			"	MRS R3, MSP \n"	//Store stack pointer in RO
  			"	ISB \n"
		    "   LDR R2,[R3, #0]\n" //load the function address
		  	"   LDR R0,[R3, #4]\n" //load the argument for function
  			//handle the FPU
  			"   VSTMDB R3!, {S16-S31}\n" //if FPU store the registers
  			//END FPU
  			"   STMDB R3!, {R4-R11,R14}\n"  //store the other registers in case of task switch
		  	"	ISB \n"
		    "   BLX     R2\n" //jump to call
			"	ISB \n"
			"   LDMIA R3!, {R4-R11,R14}\n" //load the registers last saved
			//handle the FPU
			"   vldmia R3!, {S16-S31}\n" //if FPU store the registers
			//END FPU
			"   msr msp,r3\n" //move stack pointer to the MSP
			"   isb\n"
			"   bx LR\n" //return from ISR
		);
}


#ifdef __cplusplus
}
#endif
