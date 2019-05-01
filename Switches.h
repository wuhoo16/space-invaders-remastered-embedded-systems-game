// Switches.h
// Runs on LM4F120/TM4C123
// Provide functions that initialize Port E pins 0, 1

// Student names: Jin Lee (jl67888), Andrew Wu (amw5468)
// Last modification date: 4/30/2019

#ifndef SWITCHES_H
#define SWITCHES_H
#include <stdint.h>

// PE10 initialization function 
// Input: none
// Output: none
//PE0 will be external positive logic switch(for regular firing)
//PE1 will be external positive logic switch(for power-up attacks)
void PortE_Init(void);
#endif
