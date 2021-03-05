#include "adc.h"
#include "board.h"
#include "./drivers/delay/delay.h"
#include "./libraries/syslog/syslog.h"
#include "./drivers/delay/delay.h"


static bool_t adcInitDone[ADC_INST_NUM]={0};

float lastVcc=0;
static __inline__ __attribute__((always_inline, unused)) bool_t syncADC(Adc *ptrADC)
{
	uint32_t t0;
	t0=10000;
	while (ptrADC->STATUS.bit.ADCBUSY == 1 && t0>0)
	{
		t0--;
		delay_us(1);
	}
	if (t0==0)
	{
		ERROR("Sync failed");
	}
	return (t0!=0);
}


bool_t adcInit(Adc *ptrADC)
{
	//uint32_t t0;

	if (ptrADC == ADC0)
	{
		MCLK->APBDMASK.reg |= MCLK_APBDMASK_ADC0;
		GCLK->PCHCTRL[ADC0_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_1M | (1 << GCLK_PCHCTRL_CHEN_Pos);




	}else
	{
		MCLK->APBDMASK.reg |= MCLK_APBDMASK_ADC1;
		GCLK->PCHCTRL[ADC1_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_1M | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}



//	// ADC Bias Calibration
//	uint32_t bias = (*((uint32_t *) ADC_FUSES_BIASCAL_ADDR) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
//
//	// ADC Linearity bits 4:0
//	uint32_t linearity = (*((uint32_t *) ADC_FUSES_LINEARITY_0_ADDR) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;
//
//	// ADC Linearity bits 7:5
//	linearity |= ((*((uint32_t *) ADC_FUSES_LINEARITY_1_ADDR) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;
//
//	ADC->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity);

	ptrADC->CTRLB.reg = ADC_CTRLB_RESSEL_12BIT ;         // 12 bits resolution as default

	ptrADC->SAMPCTRL.reg =0x3f;                        // Set  Sampling Time Length
	syncADC(ptrADC);
	ptrADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
	// Averaging (see datasheet table in AVGCTRL register description)
	ptrADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_16 |    // 16 sample only (no oversampling nor averaging)
				ADC_AVGCTRL_ADJRES(4);   // Adjusting result by 16

	ptrADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_AREFA;  //external reference selection.
	syncADC(ptrADC);
	if (ptrADC == ADC0)
	{
		adcInitDone[0]=true;
	} else
	{
		adcInitDone[1]=true;
	}

	return true;
}

uint32_t readPinADC(pin_t pin)
{
	//float adcValue;
	uint32_t adcCounts;
	uint32_t to;
	Adc *ptrADC =(Adc *)pin.ptrPerherial;



	ASSERT(ptrADC);
	ASSERT(pin.id <= 23);

	if (ptrADC == ADC0)
	{
		if (!adcInitDone[0])
		{
			adcInit(ptrADC);
		}
	} else
	{
		if (!adcInitDone[1])
		{
			adcInit(ptrADC);
		}
	}


	PinConfig(pin);

	syncADC(ptrADC);
	ptrADC->INPUTCTRL.bit.MUXPOS =pin.id;

	syncADC(ptrADC);
	ptrADC->CTRLA.reg |= ADC_CTRLA_ENABLE;             // Enable ADC

	syncADC(ptrADC);
	ptrADC->SWTRIG.bit.START = 1;

	// Clear the Data Ready flag
	ptrADC->INTENCLR.reg = ADC_INTENCLR_RESRDY; // Clear the Data Ready flag

	// Start conversion again, since The first conversion after the reference is changed must not be used.
	syncADC(ptrADC);
	ptrADC->SWTRIG.bit.START = 1;

	// Store the value
	to=1000000; //set a time out
	while (ptrADC->INTFLAG.bit.RESRDY == 0 && to>0) // Waiting for conversion to complete
	{
		delay_us(1);
		to--;
	}
	if (to==0)
	{
		ERROR("ADC timed out");
		return 0;
	}
	adcCounts=ptrADC->RESULT.reg;
	//adcValue = (float)adcCounts + 0.5; //adc is a floor operation so add 0.5 to results
	//adcValue=lastVcc*adcValue/4096.0;

	//LOG("ADC for pin %d %0.4fV %d", pin.pinNum,adcValue,adcCounts);
	return adcCounts;
}

#if 0
float readVcc(void)
{
	float value;

	uint32_t i, avg;
	uint32_t adcValue;


	if (!adcInitDone)
	{
		WARNING("You should init the ADC before using ");
		adcInit();
	}
	SYSCTRL->VREF.bit.BGOUTEN=1; //enable the band gap


	ADC->CTRLA.reg &= ~ADC_CTRLA_ENABLE;             // Disable ADC
	ASSERT(syncADC());
	ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_AREFA;  //set to VREFA

	ADC->INPUTCTRL.bit.MUXPOS =ADC_INPUTCTRL_MUXPOS_BANDGAP; //set source to bandgap
	ASSERT(syncADC());

	ADC->CTRLA.reg |= ADC_CTRLA_ENABLE;             // Enable ADC
	ASSERT(syncADC());

	delay_us(10);
	//start first conversion, this will be wrong as we changed the reference
	ADC->INTENCLR.reg = ADC_INTENCLR_RESRDY; // Clear the Data Ready flag
	ADC->SWTRIG.bit.START = 1;
	ASSERT(syncADC());


	avg=0;
	for (i=0; i<1; i++)
	{
		//start a second conversion
		ADC->INTENCLR.reg = ADC_INTENCLR_RESRDY; // Clear the Data Ready flag
		ADC->SWTRIG.bit.START = 1;
		ASSERT(syncADC());

		// Store the value
		uint32_t to=1000000; //set a time out
		while (ADC->INTFLAG.bit.RESRDY == 0 && to>0) // Waiting for conversion to complete
		{
			delay_us(1);
			to--;
		}
		if (to==0)
		{
			ERROR("ADC timed out");
			return 0;
		}


		adcValue=ADC->RESULT.reg;
		//LOG("ADC value %d",adcValue);
		avg=avg+adcValue;
	}
	adcValue=(avg+i/2)/i;

	value=(1.10 / (float)adcValue) * 4096.0; //adc is floor operation so we add 0.5 to results

	//ADC->CTRLA.reg &= ~ADC_CTRLA_ENABLE;             // Disable ADC
	ASSERT(syncADC());
	//LOG("Vcc is %02f",value);
	lastVcc=value;
	//SYSCTRL->VREF.bit.BGOUTEN=0;
	return value;
}
#endif
