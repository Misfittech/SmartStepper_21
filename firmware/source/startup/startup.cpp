

#include "sam.h"
#include "board.h"
#include <stdio.h>

uint32_t SystemCoreClock=VARIANT_MCK;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
void __libc_init_array(void);
#ifdef __cplusplus
}
#endif // __cplusplus
void config_gclk_oe(uint32_t id, uint32_t source, uint32_t div)
{
	 GCLK->GENCTRL[id].reg = GCLK_GENCTRL_SRC(source)  |
			 GCLK_GENCTRL_IDC  | //improve duty cycle
			 GCLK_GENCTRL_GENEN |  //enable
			 GCLK_GENCTRL_OE |
			 GCLK_GENCTRL_DIV(div); //setup divider

	  while ( GCLK->SYNCBUSY.reg & (0x01<<(id+GCLK_SYNCBUSY_GENCTRL0_Pos)) ){
	    /* Wait for synchronization */
	  }
}

void SystemInit( void )
{

  NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_RWS(5); //5 wait states for 120Mhz


  #ifndef CRYSTALLESS


  OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ENABLE | OSC32KCTRL_XOSC32K_EN32K | OSC32KCTRL_XOSC32K_EN32K | OSC32KCTRL_XOSC32K_CGM_XT | OSC32KCTRL_XOSC32K_XTALEN;

  while( (OSC32KCTRL->STATUS.reg & OSC32KCTRL_STATUS_XOSC32KRDY) == 0 ){
    /* Wait for oscillator to be ready */
  }

  #endif //CRYSTALLESS

  //software reset

  GCLK->CTRLA.bit.SWRST = 1;
  while ( GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SWRST ){
	  /* wait for reset to complete */
  }

#ifndef CRYSTALLESS

  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_XOSC32K].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC32K) |
    GCLK_GENCTRL_GENEN;
 #else

  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_XOSC32K].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSCULP32K) | GCLK_GENCTRL_GENEN;
 #endif


  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_XOSC32K_SYNC ){
    /* Wait for synchronization */
  }

  GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSCULP32K) | GCLK_GENCTRL_GENEN;
  
  while ( GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL0 ){
    /* Wait for synchronization */
  }

  
 // GCLK->PCHCTRL[OSCCTRL_GCLK_ID_DFLL48].reg = (1 << GCLK_PCHCTRL_CHEN_Pos) | GCLK_PCHCTRL_GEN(GENERIC_CLOCK_GENERATOR_XOSC32K);

  OSCCTRL->DFLLCTRLA.reg = 0;
  //GCLK->PCHCTRL[OSCCTRL_GCLK_ID_DFLL48].reg = (1 << GCLK_PCHCTRL_CHEN_Pos) | GCLK_PCHCTRL_GEN(GCLK_PCHCTRL_GEN_GCLK3_Val);

  OSCCTRL->DFLLMUL.reg = OSCCTRL_DFLLMUL_CSTEP( 0x1 ) |
    OSCCTRL_DFLLMUL_FSTEP( 0x1 ) |
    OSCCTRL_DFLLMUL_MUL( 0 );

  while ( OSCCTRL->DFLLSYNC.reg & OSCCTRL_DFLLSYNC_DFLLMUL )
    {
      /* Wait for synchronization */
    }

  OSCCTRL->DFLLCTRLB.reg = 0;
  while ( OSCCTRL->DFLLSYNC.reg & OSCCTRL_DFLLSYNC_DFLLCTRLB )
      {
        /* Wait for synchronization */
      }

  OSCCTRL->DFLLCTRLA.reg = OSCCTRL_DFLLCTRLA_ENABLE;
  while ( OSCCTRL->DFLLSYNC.reg & OSCCTRL_DFLLSYNC_ENABLE )
      {
        /* Wait for synchronization */
      }

  OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg;
  while( OSCCTRL->DFLLSYNC.bit.DFLLVAL );

  OSCCTRL->DFLLCTRLB.reg = OSCCTRL_DFLLCTRLB_WAITLOCK |
  OSCCTRL_DFLLCTRLB_CCDIS | OSCCTRL_DFLLCTRLB_USBCRM ;
  
  while ( OSCCTRL->DFLLSYNC.reg & OSCCTRL_DFLLSYNC_DFLLCTRLB )
      {
        /* Wait for synchronization */
      }
  while ( !OSCCTRL->STATUS.bit.DFLLRDY )
    {
      /* Wait for synchronization */
    }

  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_1M].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DFLL_Val) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_DIV(24u);
  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_1M_SYNC ){
    /* Wait for synchronization */
  }


  /* ------------------------------------------------------------------------
  * Set up the PLLs
  */

  //PLL0 is 120MHz
  GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0].reg = (1 << GCLK_PCHCTRL_CHEN_Pos) | GCLK_PCHCTRL_GEN(GENERIC_CLOCK_GENERATOR_1M);

  OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(0x00) | OSCCTRL_DPLLRATIO_LDR(59); //120 Mhz

  while(OSCCTRL->Dpll[0].DPLLSYNCBUSY.bit.DPLLRATIO);

  //MUST USE LBYPASS DUE TO BUG IN REV A OF SAMD51
  OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK | OSCCTRL_DPLLCTRLB_LBYPASS;

  OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

  while( OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY == 0 || OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK == 0 );

  //PLL1 is 100MHz

  GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL1].reg = (1 << GCLK_PCHCTRL_CHEN_Pos) | GCLK_PCHCTRL_GEN(GENERIC_CLOCK_GENERATOR_1M);

  OSCCTRL->Dpll[1].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(0x00) | OSCCTRL_DPLLRATIO_LDR(49);

  while(OSCCTRL->Dpll[1].DPLLSYNCBUSY.bit.DPLLRATIO);

  //MUST USE LBYPASS DUE TO BUG IN REV A OF SAMD51
//  OSCCTRL->Dpll[1].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK | OSCCTRL_DPLLCTRLB_LBYPASS | OSCCTRL_DPLLCTRLB_WUF |
//		  OSCCTRL_DPLLCTRLB_DCOFILTER(7) |
//		  OSCCTRL_DPLLCTRLB_FILTER(0xA);
  OSCCTRL->Dpll[1].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK | OSCCTRL_DPLLCTRLB_LBYPASS;


  OSCCTRL->Dpll[1].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

  while( OSCCTRL->Dpll[1].DPLLSTATUS.bit.CLKRDY == 0 || OSCCTRL->Dpll[1].DPLLSTATUS.bit.LOCK == 0 );


  /* ------------------------------------------------------------------------
  * Set up the peripheral clocks
  */

  //48MHZ CLOCK FOR USB AND STUFF
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_48M].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DFLL_Val) |
    GCLK_GENCTRL_IDC |
    //GCLK_GENCTRL_OE |
    GCLK_GENCTRL_GENEN;

  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_48M_SYNC)
    {
      /* Wait for synchronization */
    }

  //100MHZ CLOCK FOR OTHER PERIPHERALS
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_100M].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DPLL1_Val) |
    GCLK_GENCTRL_IDC |
    //GCLK_GENCTRL_OE |
    GCLK_GENCTRL_GENEN;

  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_100M_SYNC)
    {
      /* Wait for synchronization */
    }

  //12MHZ CLOCK FOR DAC
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_12M].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DFLL_Val) |
    GCLK_GENCTRL_IDC |
    GCLK_GENCTRL_DIV(4) |
    //GCLK_GENCTRL_DIVSEL |
    GCLK_GENCTRL_OE |
    GCLK_GENCTRL_GENEN;

  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_12M_SYNC)
    {
      /* Wait for synchronization */
    }

  /*---------------------------------------------------------------------
   * Set up main clock
   */

  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_MAIN].reg = GCLK_GENCTRL_SRC(MAIN_CLOCK_SOURCE) |
    GCLK_GENCTRL_IDC |
    //GCLK_GENCTRL_OE |
    GCLK_GENCTRL_GENEN;


  while ( GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL0 )
    {
      /* Wait for synchronization */
    }

  MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV_DIV1;

//
//  //Configure the external clock  PA14
//  OSCCTRL->XOSCCTRL[0].reg=OSCCTRL_XOSCCTRL_IMULT(4)
//		  | OSCCTRL_XOSCCTRL_IPTAT(3)
//		 // | OSCCTRL_XOSCCTRL_XTALEN //use with external clock on Xin
//		  | OSCCTRL_XOSCCTRL_ENABLE;
//
//  while( (OSCCTRL->STATUS.reg & OSCCTRL_STATUS_XOSCRDY0) == 0 ){
//    /* Wait for oscillator to be ready */
//  }
//
//
//  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_9M].reg = GCLK_GENCTRL_SRC_XOSC0  |
// 			 GCLK_GENCTRL_IDC  | //improve duty cycle
// 			 GCLK_GENCTRL_GENEN |  //enable
// 			 GCLK_GENCTRL_OE |
// 			 GCLK_GENCTRL_DIV(1); //setup divider
//
// 	  while ( GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_9M_SYNC ){
// 	    /* Wait for synchronization */
// 	  }





  /* Use the buck regulator by default */
 // SUPC->VREG.bit.SEL = 1;


  /* If desired, enable cache! */
#if  defined(ENABLE_CACHE)
  __disable_irq();
  CMCC->CTRL.reg = 1;
  __enable_irq();
  __asm__ __volatile__(
 			  " dsb \n"
		  	  " isb \n"
 	);
#endif

  __libc_init_array();



}
