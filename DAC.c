// dac.c
// This software configures DAC output
// Lab 6 requires a minimum of 4 bits for the DAC, but you could have 5 or 6 bits
// Runs on LM4F120 or TM4C123
// Program written by: Jin Lee (jl67888) and Andrew Wu (amw5468)
// Date Created: 3/6/17 
// Last Modified: 3/14/19
// Lab number: 6
// Hardware connections:
// 4-bit binary weighted DAC (PB0-3)
// 3.5mm audio jack (1 SJ1-3543N)

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data

#define DAC (*((volatile uint32_t *) 0x4000503C))	// modify only Port B bits 0-3
// **************DAC_Init*********************
// Initialize 4-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x00000002;	// activate Port B
	delay = SYSCTL_RCGCGPIO_R;				// allow time to finish activating
	GPIO_PORTB_AMSEL_R &= ~0x0F;			// no analog
	GPIO_PORTB_PCTL_R &= ~0x0000FFFF;	// regular function
	GPIO_PORTB_DIR_R |= 0x0F;					// make PB0-3 outputs
	GPIO_PORTB_AFSEL_R &= ~0x0F;			// disable alternate function
	GPIO_PORTB_DEN_R |= 0x0F;					// digital enable PB0-3
}

// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// Input=n is converted to n*3.3V/15
// Output: none
void DAC_Out(uint32_t data){
	DAC = data;	// output digital signal to 4-bit DAC
}
