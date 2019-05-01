// ADC.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize ADC0
// Last Modified: 11/6/2018
// Student names: Jin Lee (jl67888), Andrew Wu (amw5468)
// Last modification date: 4/19/19

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// ADC initialization function 
// Input: none
// Output: none
// measures from PD2, analog channel 5
void ADC_Init(void){ 
	volatile int delay; 
	SYSCTL_RCGCGPIO_R |= 0x08;		// 1) activate clock for Port D
	while((SYSCTL_RCGCGPIO_R & 0x08) == 0){};
	GPIO_PORTD_DIR_R &= ~0x04;		// 2) make PD2 an input
	GPIO_PORTD_AFSEL_R |= 0x04;		// 3) enable alt func on PD2
	GPIO_PORTD_DEN_R &= ~0x04;		// 4) disable digital I/O on PD2
	GPIO_PORTD_AMSEL_R |= 0x04;		// 5) enable analog func on PD2
	SYSCTL_RCGCADC_R |= 0x01;			// 6) activate ADC0
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;			// extra time to stabilize
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	ADC0_PC_R = 0x01;							// 7) configure for 125k samples/s
	ADC0_SAC_R |= 0x07;						// average of 64 samples in attempt to elimate fluctuation in data readings (in the thousandths place)
	ADC0_SSPRI_R = 0x0123;				// 8) seq3 is highest priority
	ADC0_ACTSS_R &= ~0x0008;			// 9) disable sample sequencer 3
	ADC0_EMUX_R &= ~0xF000;				// 10) seq3 is software trigger
	ADC0_SSMUX3_R &= ~ 0x000F;		// 11) clear SS3 field
	ADC0_SSMUX3_R += 5;						// 		 set channel (Ain5)
	ADC0_SSCTL3_R = 0x0006;				// 12) no TS0 D0, yes IE0 END0
	ADC0_IM_R &= ~0x0008;					// 13) disable SS3 interrupts
	ADC0_ACTSS_R |= 0x0008;				// 14) enable sample sequencer 3
}

//------------ADC_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// measures from PD2, analog channel 5
uint32_t ADC_In(void){  
	unsigned long result;
	ADC0_PSSI_R = 0x0008;							// 1) initiate SS3
	while((ADC0_RIS_R & 0x08) == 0){};// 2) wait for conversion done
	result = ADC0_SSFIFO3_R & 0x0FFF;	// 3) read result
	ADC0_ISC_R = 0x0008;							// 4) acknowledge completion
	return result;
}

