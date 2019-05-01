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

#define PF123			(*((volatile uint32_t *)0x40025038))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
	
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

void testEnemyInit(void);
void moveEnemyRight(void);
void moveEnemyLeft(void);
void moveEnemyDown(void);

void SysTick_Init(void);
void playerInit(void);

uint8_t flag;
uint16_t ADCMAIL;
uint16_t Data;        // 12-bit ADC
int16_t Prev_adc;
int16_t adc_diff;


//****starting code****
int main1(void){
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);

  Output_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  ST7735_DrawBitmap(52, 159, ns, 18,8); // player ship middle bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);

  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);
  ST7735_DrawBitmap(100, 9, SmallEnemy30pointB, 16,10);

  Delay100ms(50);              // delay 5 sec at 80 MHz

  ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
  LCD_OutDec(1234);
  while(1){
  }

}

//****enemy movement sandbox****
struct state{
	int16_t x;
	int16_t y;
	uint8_t width;
	uint8_t height;
	const uint16_t *image;
	// status
};
typedef struct state state_t;
state_t enemy[4];
// test main for sprite movement
int main2(void){uint8_t i;
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
//	ADC_Init();
//	SysTick_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  ST7735_DrawBitmap(52, 159, ns, 18,8); // player ship middle bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);

	testEnemyInit();
  Delay100ms(2);
	EnableInterrupts();
  while(1){
		for(i=0; i<26; i++){
			moveEnemyRight();
			Delay100ms(1);
		}
		for(i=0; i<10; i++){
			moveEnemyDown();
		}
		for(i=0; i<26; i++){
			moveEnemyLeft();
			Delay100ms(1);
		}
		for(i=0; i<10; i++){
			moveEnemyDown();
		}
  }
}



//****player movement sandbox****
// how to interface player movement to slide port?
state_t player;
// test main for sprite movement
int main(void){
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
	ADC_Init();
	SysTick_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  ST7735_DrawBitmap(53, 141, Bunker0, 18,5);

  Delay100ms(2);
	EnableInterrupts();
	Data = ADCMAIL;
	Prev_adc = ADCMAIL;
	playerInit();
	ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height); // player position init based on ADC
  while(1){
		// move player sprite by drawing in updated position (fail)
		if(flag != 0){
			flag = 0;
			Data = ADCMAIL;
			adc_diff = (Data - Prev_adc);
			if((adc_diff <= -32) && (player.x - 1) != 0)																				//move player one pixel to the left if new adc data is less than 32 of the previous
			{
				adc_diff /= 32;
				for(int i = 0; i > adc_diff; i--){
					if((player.x - 1) == 0)break;
					player.x = player.x - 1;
					ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
				}		
			}
			else if((adc_diff >= 32) && ((player.x + 1) != 127 - player.width)){																//move player one pixel to the right if new adc data is equal or greater than 32 of previous value
				adc_diff /= 32;
				for(int i = 0; i < adc_diff; i++){
					if((player.x + 1) == 127 - player.width)break;
					player.x = player.x + 1;
					ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
				}
			}
			Prev_adc = Data;																							//save previous ADC value
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
  NVIC_ST_RELOAD_R = 1333333;  					// 2) reload value = 80MHz/60HZ 
  NVIC_ST_CURRENT_R = 0;                // 3) any write to current clears it
  NVIC_ST_CTRL_R = 0x0007;              // 4) enable SysTick with core clock and interrupts
}

void SysTick_Handler(void){ // every 16.67 ms
  ADCMAIL = ADC_In();	// save the ADC data to a mailbox (global variable)
	flag = 1;						// set the semaphore flag 
}

void playerInit(void){
	player.x = Data/32;
	player.y = 159;
	player.width = 24;
	player.height = 14;
	player.image = onepixelborder;
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

