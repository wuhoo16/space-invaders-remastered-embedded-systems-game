// Switches.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize Port E pins 0,1
// Student names: Jin Lee (jl67888), Andrew Wu (amw5468)
// Last modification date: 4/30/19

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// PE10 initialization function 
// Input: none
// Output: none
//PE0 will be external positive logic switch(for regular firing)
//PE1 will be external positive logic switch(for power-up attacks)
void PortE_Init(void){ 
	SYSCTL_RCGCGPIO_R |= 0x10;			//turn on clock for Port E
	volatile int delay; 
	delay = SYSCTL_RCGCGPIO_R;			//delay 2 cycles for clock to stablize
	GPIO_PORTE_AMSEL_R &= ~0x03;		// disable analog function
	GPIO_PORTE_AFSEL_R &= ~0x03;		// disable alternate function
	GPIO_PORTE_PCTL_R &= ~0x03;			// regular function
	GPIO_PORTE_DIR_R &= ~0x03;			//make PE10 both input pins
	GPIO_PORTE_DEN_R |= 0x03;       //digitally enable PE10
}
