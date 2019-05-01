// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 11/20/2018 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2018

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2018

 Copyright 2018 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "Switches.h"

#define PF123			(*((volatile uint32_t *)0x40025038))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
	
#define PE01      (*((volatile uint32_t *)0x40024012))
#define PE0       (*((volatile uint32_t *)0x40024004))
#define PE1				(*((volatile uint32_t *)0x40024008))
	
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

void testEnemyInit(void);
void moveEnemyRight(void);
void moveEnemyLeft(void);
void moveEnemyDown(void);

void SysTick_Init(void);
void playerInit(void);
void missilesInit(void);

//enum status declaration and sprite struct declaration
enum state{alive, moving, dead, dying};
typedef enum state status_t;
struct sprite{
	int16_t x;
	int16_t y;
	uint8_t width;
	uint8_t height;
	const uint16_t *image;
	status_t status;					//alive, moving, dead, or dying
};
typedef struct sprite sprite_t;

//global variable declarations below:
uint8_t flag;
uint16_t ADCMAIL;
uint16_t Data;        // 12-bit ADC
int16_t Prev_adc;
int16_t adc_diff;

uint8_t SW0;				
uint8_t SW1;
sprite_t missiles[2];
uint8_t m1spawn = 0;
uint8_t m2spawn = 0;
uint8_t movemissileflag;


sprite_t player;
sprite_t enemy[4];


// test main for sprite movement
int main(void){
  PLL_Init(Bus80MHz);    			// Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
	ADC_Init();
	PortE_Init();
	SysTick_Init();
	Timer1_Init(1333333);				//Timer1 interrupts at 60 Hz
  ST7735_FillScreen(0x0000);	// set screen to black
  
  ST7735_DrawBitmap(53, 141, Bunker0, 18,5);

  Delay100ms(2);
	EnableInterrupts();
	Data = ADCMAIL;			// get init player position (ADC)
	Prev_adc = Data;		// init previous value
	playerInit();
	missilesInit();
	ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height); // player position init based on ADC
  while(1){
			if(m1spawn == 1)
			{
					m1spawn = 0;					//clear flag
					ST7735_DrawBitmap(missiles[0].x, missiles[0].y, missiles[0].image, missiles[0].width, missiles[0].height);
					missiles[0].status = moving;
			}
			if(m2spawn == 1){
					m2spawn = 0;					//clear flag
				  ST7735_DrawBitmap(missiles[1].x, missiles[1].y, missiles[1].image, missiles[1].width, missiles[1].height);
					missiles[1].status = moving;
			}
			
/*		if(SW0 == 1){
		SW0 = 0;		//clear switch 0 flag
			if(missiles[0].status == dead){										//case for if 1st missile gone/offscreen
				missiles[0].x = (player.x + player.width/2) - missiles[0].width/2;			//assign starting x pos of missile
				missiles[0].y = (player.y - player.height);															//assign starting y pos of missile	
				ST7735_DrawBitmap(missiles[0].x, missiles[0].y, missiles[0].image, missiles[0].width, missiles[0].height);
				missiles[0].status = moving;
			}
			else if((missiles[0].status == moving) && (missiles[1].status == dead)){							//case for if 2nd missile gone/offscreen
				missiles[1].x = (player.x + player.width/2) - missiles[1].width/2;			//assign starting x pos of missile
				missiles[1].y = (player.y - player.height);															//assign starting y pos of missile	
				ST7735_DrawBitmap(missiles[1].x, missiles[1].y, missiles[1].image, missiles[1].width, missiles[1].height);
				missiles[1].status = moving;
			}
		}
*/

		
		
		// move player sprite by comparing the current ADC sample to the previous ADC value
		if(flag != 0){
			flag = 0;																			// acknowledge flag
			Data = ADCMAIL;																// update player position
			adc_diff = (Data - Prev_adc);									// calculate difference 
			// if new ADC data is less than 32 of the previous, move player to the left
			if((adc_diff <= -32) && (player.x - 1) != 0){	
				adc_diff /= 32;															// convert to pixels
				for(int i = 0; i > adc_diff; i--){
					if((player.x - 1) == 0)break;							// left border reached
					player.x = player.x - 1;									// move player one pixel to the left
					ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
				}		
			}
			// move player to the right if new adc data is equal or greater than 32 of previous value
			else if((adc_diff >= 32) && ((player.x + 1) != 127 - player.width)){
				adc_diff /= 32;															// convert to pixels
				for(int i = 0; i < adc_diff; i++){					
					if((player.x+1) == 127-player.width)break;// right border reached
					player.x = player.x + 1;									// move player one pixel to the right
					ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
				}
			}
			Prev_adc = Data;															// update previous ADC value
		}
		
		//move the missiles if any of the missiles have status of moving
		if(movemissileflag == 1){
			movemissileflag = 0;													//clear flag
			for(int i = 0; i < 2; i++)
			{
				if(missiles[i].status == moving){
					missiles[i].y--;
					ST7735_DrawBitmap(missiles[i].x, missiles[i].y, missiles[i].image, missiles[i].width, missiles[i].height);
				}
			}
		}
		
		
  }
}



void testEnemyInit(void){uint8_t i;
	for(i=0; i<4; i++){
		enemy[i].x = 20*i;
		enemy[i].y = 10;
		enemy[i].image = SmallEnemy10pointA;
		ST7735_DrawBitmap(enemy[i].x, enemy[i].y, enemy[i].image, 16, 10);
	}
}
void moveEnemyRight(void){uint8_t i;
	for(i=0; i<4; i++){
		enemy[i].x += 2;
		ST7735_DrawBitmap(enemy[i].x, enemy[i].y, enemy[i].image, 16, 10);
	}
	
}
void moveEnemyLeft(void){uint8_t i;
	for(i=0; i<4; i++){
		enemy[i].x -= 2;
		ST7735_DrawBitmap(enemy[i].x, enemy[i].y, enemy[i].image, 16, 10);
	}
	
}
void moveEnemyDown(void){uint8_t i;
	for(i=0; i<4; i++){
		enemy[i].y += 1;
		ST7735_DrawBitmap(enemy[i].x, enemy[i].y, enemy[i].image, 16, 10);
	}
}


void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // 1) disable SysTick during setup
  NVIC_ST_RELOAD_R = 1333333;  					// 2) reload value = 1333333 for clock running at 80MHz --> 60 Hz
  NVIC_ST_CURRENT_R = 0;                // 3) any write to current clears it
  NVIC_ST_CTRL_R = 0x0007;              // 4) enable SysTick with core clock and interrupts
}

void SysTick_Handler(void){ // every 16.67 ms
  ADCMAIL = ADC_In();	// save the ADC data to a mailbox (global variable)
	flag = 1;						// set the semaphore flag 
	
			if(PE0 == 1){
				SW0 = PE0;				//read data to clear Port E data
				if(missiles[0].status == dead){										//case for if 1st missile gone/offscreen
					missiles[0].x = (player.x + player.width/2) - missiles[0].width/2;			//assign starting x pos of missile
					missiles[0].y = (player.y - player.height);															//assign starting y pos of missile	
					m1spawn = 1;
				}
				else if((missiles[0].y < 133) && (missiles[0].status == moving) && (missiles[1].status == dead)){							//case for if 2nd missile gone/offscreen and 1st missile has traveled at least 12 pixels
					missiles[1].x = (player.x + player.width/2) - missiles[1].width/2;			//assign starting x pos of missile
					missiles[1].y = (player.y - player.height);															//assign starting y pos of missile	
					m2spawn = 1;
				}
		}																											
			
	SW1 = PE1 >> 1;
	
	//if collision occured or missile off top of screen, change missile status to dead //CURRENTLY ONLY CHECKS IF MISSILE IS OFFSCREEN DOES NOT DO COLLISION DETECTION
	for(int i = 0; i < 2; i++){
		if(missiles[i].y == 0){
			missiles[i].status = dead;
		}
		else{
			movemissileflag = 1;
		}
	}
}

void playerInit(void){
	player.x = Data/32;	
	player.y = 159;
	player.width = 24;
	player.height = 14;
	player.image = onepixelborder;
	player.status = alive;
}

void missilesInit(void){									//at beginning of the game, both missiles have not been fired so status is dead
	for(int i = 0; i < 2; i++){
		missiles[i].x = -1;
		missiles[i].y = -1;
		missiles[i].width = 8;
		missiles[i].height = 12;
		missiles[i].image = missile;
		missiles[i].status = dead;
	}
}

// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

