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
#include "DAC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "Timer2.h"
#include "Switches.h"

#define PF123			(*((volatile uint32_t *)0x40025038))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
	
#define PE01      (*((volatile uint32_t *)0x4002400C))
#define PE0       (*((volatile uint32_t *)0x40024004))
#define PE1				(*((volatile uint32_t *)0x40024008))
	

//FUNCTION PROTOTYPES BELOW
//----------------------------------------------------------------------------------------------------
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

void moveEnemyRight(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num);
void moveEnemyLeft(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num);
void moveEnemyDown(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num);

void SysTick_Init(void);
void playerInit(void);
void missilesInit(void);
void collisionFlag_Init(void);
void explosionsInit(void);

void wave0Init(void); void wave1Init(void); void wave2Init(void); void wave3Init(void);
void allwaves_statusInit(void);

void playsound(void);	// function that outputs samples of sound arrays depending on global varables (based on current global flags of sound effect and length of sample array)
void animation_spawn_delay(void); //periodic function that is called every 125 ms through the Timer0 ISR
void move_projectiles(void);

void reload_movement_cts(uint8_t i, uint8_t j);

void init_txt_pts(void);
//----------------------------------------------------------------------------------------------------



//ENUM AND STRUCT DEFINITIONS
//---------------------------------------------------------------------
enum state{alive, moving, dead, unspawned};
typedef enum state status_t;		// status_t for states

struct sprite{
	int16_t x;
	int16_t y;
	uint8_t width;
	uint8_t height;
	const uint16_t *image;
	status_t status; 							//alive, moving, dead, or dying
};
typedef struct sprite sprite_t;	// sprite_t for sprites

struct enemy_sprite{
	int16_t x;
	int16_t y;
	uint8_t width;
	uint8_t height;
	const uint16_t *image;
	status_t status; 							//alive, moving, dead, or dying
	uint8_t speed;								//initialize speed to 1 and increment for later waves to make sprites faster
	uint8_t collision_flag;
	uint8_t health;
	uint8_t score;
};
typedef struct enemy_sprite enemy_t;

struct wave{
	enemy_t enemy[4][4];					//4 rows of 4 enemy sprites each
	uint8_t clear;								//2 for last sprite animation done, 1 for whole wave clear detected in ISR, otherwise 0
	uint8_t row_clear[4]; 				//1 for whole row clear deteced in ISR, 0 otherwise
	uint8_t row_spawn[4];
	uint8_t row_speed[4];
};
typedef struct wave wave_t;
//-------------------------------------------------------------------



//GLOBAL VARIABLES DECLARATIONS BELOW
//-----------------------------------------------------------------------
uint8_t flag;
uint16_t ADCMAIL;
uint16_t Data;        // 12-bit ADC
int16_t Prev_adc;
int16_t adc_diff;
uint8_t counter_125ms = 0; //used in animation periodic task (next row will not spawn until 3 second has passed since last row spawned)
														//when counter_125ms = 24 then 3 seconds has passed
uint32_t total_score = 0;

//wave/row management global variables
uint8_t wave_number = 0;   //current wave number starting with 0
uint8_t current_row_number = 0; //current row number that needs to be spawned
uint8_t wave_spawn_done = 0;
uint8_t deadcounter = 0; 	//how many sprites dead per row
uint8_t totaldeadcounter = 0;  //how many sprites dead in entire wave

//missile global variables
uint8_t SW0;				
uint8_t SW1;
uint8_t m1spawn = 0;
uint8_t m2spawn = 0;
uint8_t movemissileflag;
uint8_t missileCollisionFlag[2] = {0,0};
uint8_t firerate_limit_counter = 2;		//allow player to fire at the beginning of the game with no restrictions
		//uint8_t enemyCollisionFlag[6] = {0,0,0,0,0,0};

//sprites
sprite_t player;
sprite_t missiles[2];
//enemy_t enemy[4];
sprite_t explosion_anim[5];
wave_t wave[4];						 //4 wave structs; wave[0] refers to first wave, wave[1] is second wave... (EACH WAVE STRUCT CONTAINS A 2D ARRAY OF 4 ROWS OF 4 ENEMIES)

//sound global variable flags
const unsigned char *sound_pointer = silence; 
uint32_t sound_length;						//size of current sample array
uint32_t sound_index = 0;

//explosion animation global variables
uint8_t explosion_counter = 0;
uint8_t explosion_frame = 0;
uint8_t explosion_done = 1;
uint8_t clear_explosion_early = 0;
uint16_t x_save;
uint16_t y_save;

//enemy movement flags/counters
uint8_t move_enemy = 0;
uint8_t moveRightInit_ct[4][4] = {{26,26,26,26},{26,26,26,26},{26,26,26,26}, {26,26,26,26}}; //variable initialized for each row
uint8_t moveRight_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveLeft_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveDownR_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveDownL_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};

//text arrays and pointers
char msg1pt[15] = {'W', 'A', 'V', 'E', ' ', '1', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg2pt[15] = {'W', 'A', 'V', 'E', ' ', '2', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg3pt[15] = {'W', 'A', 'V', 'E', ' ', '3', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg4pt[15] = {'W', 'A', 'V', 'E', ' ', '4', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg5pt[15] = {'F', 'I', 'N', 'A', 'L', ' ', 'B', 'O', 'S' , 'S', 0};
//------------------------------------------------------------------------------------------------



//MAIN STARTS HERE
//-------------------------------------------------------------------------------------------------------------------------------------------
int main(void){
  PLL_Init(Bus80MHz); // Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
	ADC_Init();
	DAC_Init();
	PortE_Init();
	SysTick_Init();
	Timer1_Init(&playsound, 7256);						// Timer1 interrupts at 11.025 kHZ (reload value needed is 7256.24)
	Timer2_Init(&move_projectiles, 1000000); 	//Timer2 interrupts at 80 Hz (all projectile movement)
  ST7735_FillScreen(0x0000);								// set screen to black
  
	  //ST7735_DrawBitmap(53, 141, Bunker0, 18,5); //no bunker in our game
	missilesInit();
	collisionFlag_Init();
	explosionsInit();
	allwaves_statusInit();
	wave0Init();
	wave1Init();
	wave2Init();
	wave3Init();
	Timer0_Init(&animation_spawn_delay, 10000000); 				//Timer0 interrupts at 8 Hz (every 125 ms) ALSO CONTROLS ROW SPAWN EVERY 3 s)
	playerInit();
//  Delay100ms(2);
	EnableInterrupts();
	Data = ADCMAIL;			// get init player position (ADC)
	Prev_adc = Data;		// init previous adc value
	
	ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height); 		// player position init based on ADC
// START OF GAME ENGINE
  while(1){
		// **** MISSILE SPAWN: based off of m1spawn and m2spawn global flags (set in SysTick ISR) ****************************
			if(m1spawn == 1)
			{
					m1spawn = 0;					//clear flag
					ST7735_DrawBitmap(missiles[0].x, missiles[0].y, missiles[0].image, missiles[0].width, missiles[0].height);
					firerate_limit_counter = 0;		//reset counter once missile has spawned			 
					sound_pointer = laser5;
				  sound_length = 3600;
			  	sound_index = 0;
					missiles[0].status = moving;
			}
			if(m2spawn == 1){
					m2spawn = 0;					//clear flag
				  ST7735_DrawBitmap(missiles[1].x, missiles[1].y, missiles[1].image, missiles[1].width, missiles[1].height);
				  firerate_limit_counter = 0;		//reset counter once missile has spawned
				  sound_pointer = laser5;
					sound_length = 3600;
				  sound_index = 0;
					missiles[1].status = moving;
			}//******************************************************************************************************************
			
			
   // **** PLAYER SPRITE HORIZONTAL MOVEMENT by comparing the current ADC sample to the previous ADC value *****
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
		}//******************************************************************************************************
		
		
		
   // ***** MISSILE MOVEMENT (up by 1 pixel based on movemissile global flag) *******************************************
		if(movemissileflag == 1){
			movemissileflag = 0;													//clear flag
			for(int i = 0; i < 2; i++)
			{
				if(missiles[i].status == moving){
					missiles[i].y--;
					ST7735_DrawBitmap(missiles[i].x, missiles[i].y, missiles[i].image, missiles[i].width, missiles[i].height);
				}
			}
		}//***************************************************************************************************************
		
		
		
	
		//**** MISSILE COLLISION/DEATH (Draw over bmps with black rectangle) **************************************************
		for(int i = 0; i < 2; i++){
			if(missileCollisionFlag[i] == 1){
				missileCollisionFlag[i] = 0;	// acknowledge
				ST7735_FillRect(missiles[i].x, missiles[i].y - missiles[i].height, missiles[i].width, missiles[i].height, 0x0000);
			}
		}//******************************************************************************************************************************
		

		//**** IF COLLISION FLAG ATTRIBUTE SET FOR ENEMY, DRAW OVER ENEMY AND DRAW FIRST FRAME OF EXPLOSION*******************************************************************************************
					for(int i = 0; i < 4; i++){	//for each row
							for(int j = 0; j < 4; j++){//for each enemy
								//if(wave[wave_number].enemy[i][j].status == dead){ //skip checking enemies that are unspawned or alive
									if(wave[wave_number].enemy[i][j].collision_flag == 1){
										wave[wave_number].enemy[i][j].collision_flag = 0;		// acknowledge
										ST7735_FillRect(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y - wave[wave_number].enemy[i][j].height, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height, 0x0000);

										//draw first frame of explosion
										ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);					
										explosion_counter = 0; //clear counter
										explosion_done = 0;		//timer0 starts incrementing explosion_counter in the animation/spawn_delay function
										explosion_frame++;
									}
							//}checking if status is dead does same thing as checking if collision_flag is 1
						}
					}
		//*************************************************************************************************************************************************************************************************
			
					


	//**** EXPLOSION	ANIMATIONS (CHECKING FRAMES AND CLEARING EXPLOSIONS ONCE ANIMATION IS DONE) **************************************************************************************************************************************************
		if(explosion_done == 0 && explosion_counter >= 1 && explosion_frame < 5){	//only draw next frame if explosion counter is set (125 ms has passed since last frame); if no collosions occur, explosion counter = 0 and explosion frame = 6
				ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);
		    explosion_counter = 0;
				explosion_frame++;				//increment frame after drawing previous frame
		}
		if(explosion_frame == 5){ //last explosion frame has just been drawn --> clear explosion and set explosion_done flag for timer to stop incrementing counter
				ST7735_FillRect(explosion_anim[4].x, explosion_anim[4].y - explosion_anim[4].height, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
				explosion_done = 1;
				explosion_counter = 0;
			  explosion_frame = 6;  //set to 6 until it is reset to 0 on the next collision
				
				if(wave[wave_number].clear == 1){
						wave[wave_number].clear = 2; //set to 2 once last enemy explosion animation is done
				}
		}
		if(clear_explosion_early == 1){						//clears a previous explosions BMP if another explosion happens to occur before the first is done animating (prevents previous image from staying forever)
			clear_explosion_early = 0; //clear flag
			ST7735_FillRect(x_save, y_save - explosion_anim[4].height, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
			
			if(wave[wave_number].clear == 1){
						wave[wave_number].clear = 2; //set to 2 once last enemy explosion animation is done
				}
		}
		
		
		//**** DRAW NEW ENEMY POSITION *************************************************************************
		for(int i = 0; i < 4; i++){
					if(wave[wave_number].row_clear[i] == 0){	//only move ith row if the row has not been cleared
						for(int j = 0; j < 4; j++){
							if(wave[wave_number].enemy[i][j].status != dead && wave[wave_number].enemy[i][j].status != unspawned){// do nothing if enemy is dead
								ST7735_DrawBitmap(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y, wave[wave_number].enemy[i][j].image, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height);
							}
						}
					}
		};// **************************************************************************************************


		
		
			
//ADD MESSAGES ABOUT WAVE CLEAR HERE OR SOME DELAY UNTIL NEXT WAVE SPAWNS**************************************************************************
		if(wave[wave_number].clear == 2){ //set to 1 when deadcounter is 16, set to 2 when last enemy explosion ends
			//ST7735_SetTextColor(0xFFFF); //set text color to white
			//ST7735_SetCursor(26, 40);
				switch(wave_number){
					case 0:
						ST7735_DrawString(4, 3, msg1pt, 0xFFFF);
						break;
					case 1:
						ST7735_DrawString(4, 3, msg2pt, 0xFFFF);
						break;
					case 2:
						ST7735_DrawString(4, 3, msg3pt, 0xFFFF);
				
						break;
					case 3:
						ST7735_DrawString(4, 3, msg4pt, 0xFFFF);
						break;
					case 4:
						ST7735_DrawString(4,3, msg5pt, 0xFFFF);	//final boss msg
					default:
						
				//output you win message and display score here
						break;
			}
			
		//ADD THIS IN LATER-- delay a few seconds, overwrite text on screen, then clear the wave_spawn_done flag and reset current_row_number to 0
			Delay100ms(20); //POST ON PIAZZA IF WE CAN USE DELAYS IF WE ALREADY HAVE 4 INTERRUPTS (break between waves of enemies)
			ST7735_FillRect(20, 21, 107, 21, 0x0000);  //clear text
			current_row_number = 0;
			wave_spawn_done = 0;
			wave_number++;	//increment wave_number only if whole wave is dead
			for(int i=0; i<4; i++){
				for(int j=0; j<4; j++){
					reload_movement_cts(i,j);			//re-initialize the movement counters for the next wave
				}
			}
		}//********************************************************************************************************************************************************
		
	
			
	//**** NEXT ROW SPAWN (every .5 s) ********************************
		if(current_row_number == 4){
			wave_spawn_done = 1; //set global flag
		}
		else if(counter_125ms == 24){// ONLY DRAW NEW ROW AFTER 3 SECONDS HAS PASSED
			counter_125ms = 0;	//reset counter
			for(int j = 0; j < 4; j++){
				ST7735_DrawBitmap(wave[wave_number].enemy[current_row_number][j].x, wave[wave_number].enemy[current_row_number][j].y, wave[wave_number].enemy[current_row_number][j].image, wave[wave_number].enemy[current_row_number][j].width, wave[wave_number].enemy[current_row_number][j].height);
				wave[wave_number].enemy[current_row_number][j].status = alive; //each sprite in this row is now alive once spawned
			}
			wave[wave_number].row_spawn[current_row_number] = 1;
			current_row_number++;	//increment spawn row # for next second
		}//**************************************************************
		
		
}//END OF GAME ENGINE
	
}//END OF MAIN----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------




//SYSTICK ISR START------------------------------------------------------------------------------------------------------------------------------------------------------
void SysTick_Handler(void){ // every 16.67 ms
	
//**** PLAYER HORIZONTAL MOVEMENT: get ADC reading from slideport and save to mailbox *********
  ADCMAIL = ADC_In();	// save the ADC data to a mailbox (global variable)
	flag = 1;						// set the semaphore flag 
//*********************************************************************************************
	
	
//**** BUTTON CHECKING: CHECK PORT E PIN 0 (PRIMARY FIRE BUTTON) TO DETERMINE MISSILE SPAWN (only 2 missiles allowed onscreen at a time)****
		if(PE0 == 1 && firerate_limit_counter >=2){ //only sets missile spawn flags if at least 250 ms has passed since the last missile was fired
				SW0 = PE0;				// read data to clear Port E data
				if(missiles[0].status == dead){// case for if 1st missile gone/offscreen
					missiles[0].x = (player.x + player.width/2) - missiles[0].width/2;			// assign starting x pos of missile
					missiles[0].y = (player.y - player.height);															// assign starting y pos of missile	
					m1spawn = 1;
				}															// case for if 2nd missile gone/offscreen and 1st missile has traveled at least 11 pixels
				else if((missiles[0].y <= 134) && (missiles[0].status == moving) && (missiles[1].status == dead)){							
					missiles[1].x = (player.x + player.width/2) - missiles[1].width/2;			// assign starting x pos of missile
					missiles[1].y = (player.y - player.height);															// assign starting y pos of missile	
					m2spawn = 1;
				}
		}
		else{
			SW0 = PE0; //clear Port E data
		}
			
	SW1 = PE1 >> 1;
//*******************************************************************************************************************************************

		
// ENEMY COLLISION CHECK AND MOVEMENT DECISION
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	for(int i = 0; i < 2; i++){
					if(missiles[i].status == moving){
						for(int k = 0; k < 4; k++){ //for each of 4 rows
						 if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
							for(int j = 0; j < 4; j++){
								if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
											// check vertical overlap (top)
									if((((missiles[i].y < wave[wave_number].enemy[k][j].y) && (missiles[i].y > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height))) ||
											// check vertical overlap (bottom)
										(((missiles[i].y - missiles[i].height) < wave[wave_number].enemy[k][j].y) && ((missiles[i].y - missiles[i].height) > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height)))) &&
											// check horizontal overlap (right)
											(((missiles[i].x > wave[wave_number].enemy[k][j].x) && (missiles[i].x < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))) ||	
											// check horizontal overlap (left)
										((missiles[i].x + missiles[i].width > wave[wave_number].enemy[k][j].x) && (missiles[i].x + missiles[i].width < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))))){
												wave[wave_number].enemy[k][j].status = dead;  		// update enemy sprite status to DEAD
												wave[wave_number].enemy[k][j].collision_flag = 1;	// set collision flag
												missiles[i].status = dead;	// update missile status to DEAD
												missileCollisionFlag[i] = 1;// set collision flag
			
												sound_pointer = explosion1; //set current_sound to explosion
												sound_length = 6964;				//set sound length
												sound_index = 0;						//reset index
									
												explosion_frame = 0;
												explosion_counter = 1;
												if(explosion_done == 0){		//if previous explosion is not done yet once another collosion occurs, set clear_explosion_early flag and save previous coordinates before overwriting
												clear_explosion_early = 1;
												x_save = explosion_anim[4].x;
												y_save = explosion_anim[4].y;
												}
												for(int i = 0; i < 5; i++){
												explosion_anim[i].status = alive;
												explosion_anim[i].x = (wave[wave_number].enemy[k][j].x - 2); //center explosion at middle of enemy sprite
												explosion_anim[i].y = (wave[wave_number].enemy[k][j].y + 10);
												}						
									}//end of if-statement for collision check on all 4 sides
								}
							}//iterate for each of 4 enemies
						 }//only check uncleared rows
						}//iterate for each of 4 rows
					}
				}//end of collision check**************************************************************************************************************************************************************************

				
		// ****ENEMY MOVEMENT DECISIONS *************************************************************************************
					for(int i = 0; i < 4; i++){// check each row
						if(wave[wave_number].row_clear[i] == 0 && wave[wave_number].row_spawn[i] == 1){//skip movement decision if ith row is cleared or not spawned yet
							for(int j = 0; j<4; j++){
								if(wave[wave_number].enemy[i][j].status == alive){
										if(moveRightInit_ct[i][j] > 0){// row of enemies initially moves 26 pixels to right right from center
												//for(int j=0; j < 4; j++){
													//if(wave[wave_number].enemy[i][j].status == alive)	//only change position if the enemy is alive
													//{
														if(wave[wave_number].enemy[i][j].image == enemy_steppermotor){
															wave[wave_number].enemy[i][j].x += 2;
														}
														else
														{
															wave[wave_number].enemy[i][j].x += 1;
														}
													//}
												//}
												moveRightInit_ct[i][j]--;
												if(moveRightInit_ct[i][j] == 0){// if max right position reached, set count to move down (on the right side)
														moveDownR_ct[i][j] = 10;
												}
										}	
										else if(moveRight_ct[i][j] > 0){	// move row of enemies right
												moveEnemyRight(wave_number, i, j);		//since all enemies in the row should have the same speed, can use index 0 here
												moveRight_ct[i][j]--;
												if(moveRight_ct[i][j] == 0){		// if right border reached, set count to move down (on the right side)
													moveDownR_ct[i][j] = 10;
												}
										}
										else if(moveDownR_ct[i][j] > 0){	// move row of enemies down (on the right side)
												moveEnemyDown(wave_number, i, j);
												moveDownR_ct[i][j]--;
												if(moveDownR_ct[i][j] == 0){		// if row has moved down 10 pixels, set count to move left
													moveLeft_ct[i][j] = 52/(wave[wave_number].enemy[i][j].speed);
												}
										}	
										else if(moveLeft_ct[i][j] > 0){		// move row of enemies left
												moveEnemyLeft(wave_number, i, j);
												moveLeft_ct[i][j]--;
												if(moveLeft_ct[i][j] == 0){			// if left border reached, set count to move down (on the left side)
													moveDownL_ct[i][j] = 10;
												}
										}
										else if(moveDownL_ct[i][j] > 0){	// if row has moved down 10 pixels, set count to move right
												moveEnemyDown(wave_number, i, j);
												moveDownL_ct[i][j]--;
												if(moveDownL_ct[i][j] == 0){		// if row has moved down 10 pixels, set count to move right
													moveRight_ct[i][j] = 52/(wave[wave_number].enemy[i][j].speed);
												}
										}					
									}//inner if_statement (skip to here is enemy is not alive yet)
									}//j-loop... iterate for each enemy
							}//if-statement (skip ith row  if row is cleared or not spawned)
					}//outer i-loop... iterate for each row
		//*******************************************************************************************************************

					
					
//**** Update row clear attributes and wave clear attributes after enemies have been written over(depending on current wave #) AND INCREMENT WAVE_NUMBER IF CLEAR FLAG IS SET*******
			for(int i=0; i<4; i++){ //iterate for each row
					for(int j=0; j<4; j++){ //iterate for each enemy
							if(wave[wave_number].enemy[i][j].status == dead){
								deadcounter++;
							}
					}
					if(deadcounter == 4)
					{
						wave[wave_number].row_clear[i] = 1;			//set atttribute flag if all enemies in the ith row are dead
					}
					totaldeadcounter += deadcounter;
					deadcounter = 0;	//reset counter
			}
			
			if(totaldeadcounter == 16){	//check if whole wave is dead
					wave[wave_number].clear = 1;							//set attribute flag if whole wave clear
					//wave_number++;	//increment wave_number only if whole wave is dead
			}
			totaldeadcounter = 0;		//reset counter
//**********************************************************************************************************************************************************************************

}//END OF SYSTICK ISR------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//PERIODIC TASKS FOR TIMERS 
//TIMER0 (animation counter for explosions)
//TIMER1(CREATE SOUND EFFECTS USING DAC_OUT) 
//TIMER2 (PROJECTILE MOVEMENT)
//---------------------------------------------------------------------------------------------------------------------

void playsound(void){
	if(sound_pointer != silence){
			if(sound_index < sound_length){
					DAC_Out(sound_pointer[sound_index]);
					sound_index++;
			}
			else{	// sound_index == sound_length and sound effect is done playing
					sound_pointer = silence;
					sound_index = 0;			// reset index once the sound is done playing
			}
	}
	
}

void animation_spawn_delay(void){
	if(explosion_done == 0){
		explosion_counter++; // increment counter every 125 ms
	}
	
	firerate_limit_counter++; //increment counter every 125 ms
	
	if(wave_spawn_done == 1){//if last row has been spawned, don't increment counter
		counter_125ms = 0; //keep counter at 0 until next wave spawns (MAKE SURE TO CLEAR WAVE SPAWN DONE FLAG UPON THE SPAWN OF THE FIRST ROW OF NEXT WAVE)										
	}
	else{
		counter_125ms++;
	}
}


void move_projectiles(void) {
	// check if player missile has reached top of screen, if not, set movemissileflag
	  for(int i = 0; i < 2; i++){
			if(missiles[i].y == 0){
				missiles[i].status = dead;
			}
			else{
				movemissileflag = 1;
			}
		}
}//-------------------------------------------------------------------------------------------------------------------



// SPRITES INITIALIZATION FUNCTIONS BELOW
//---------------------------------------------------------------------------------------------------------------------
void playerInit(void){
	player.x = Data/32;	
	player.y = 159;
	player.width = 18;
	player.height = 8;
	player.image = tm4ship;
	player.status = alive;
}

void missilesInit(void){// at beginning of the game, both missiles have not been fired so status is dead
	for(int i = 0; i < 2; i++){
		missiles[i].x = -1;
		missiles[i].y = -1;
		missiles[i].width = 5;
		missiles[i].height = 10;
		missiles[i].image = missile;
		missiles[i].status = dead;
	}
}

void explosionsInit(void){
		for(int i = 0; i < 5; i++){
			explosion_anim[i].width = 20;
			explosion_anim[i].height = 20;
			explosion_anim[i].image = explosion_animation[i];
			explosion_anim[i].status = dead;
		}
}//--------------------------------------------------------------------------------------------------------------------


// COLLISION FLAG INITIALIZATION (TEST)
//---------------------------------------
void collisionFlag_Init(void){
	for(int i = 0; i < 2; i++){
		missileCollisionFlag[i] = 0;
	}
	
/*
	for(int j = 0; j < 6; j++){
		enemyCollisionFlag[j] = 0;
	}
*/
}

//SYSTICK INITIALIZATION FUNCTION
//---------------------------------------------------
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // 1) disable SysTick during setup
  NVIC_ST_RELOAD_R = 1333333;  					// 2) reload value = 1333333 for clock running at 80MHz --> 60 Hz
  NVIC_ST_CURRENT_R = 0;                // 3) any write to current clears it
  NVIC_ST_CTRL_R = 0x0007;              // 4) enable SysTick with core clock and interrupts
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



//ENEMY INITIALIZATION AND MOVEMENT HELPER FUNCTIONS
//---------------------------------------------------------------------------------------------------------------------
void wave0Init(void){	//intializing each wave separately in case we want different dimensions, images, and speeds for other waves!!!
	for(int i=0; i<4; i++){ //for each row of enemies
		for(int j=0; j<4; j++){ //for each enemy sprite
			wave[0].enemy[i][j].x = 20*j + 26;
			wave[0].enemy[i][j].y = 10;
			wave[0].enemy[i][j].width = 16;
			wave[0].enemy[i][j].height = 10;
			wave[0].enemy[i][j].collision_flag = 0;
			
			
					if(i == 0){				// first row
						wave[0].enemy[i][j].image = enemy_oscilloscope;
						wave[0].enemy[i][j].speed = 1;
						wave[0].enemy[i][j].health = 1;
						wave[0].enemy[i][j].score = 10;
						ST7735_DrawBitmap(wave[0].enemy[i][j].x, wave[0].enemy[i][j].y, wave[0].enemy[i][j].image, wave[0].enemy[i][j].width, wave[0].enemy[i][j].height);
						wave[0].enemy[i][j].status = alive;
						wave[0].row_spawn[0] = 1; //first row spawned
						current_row_number = 1;
					}				
					else if(i == 1){ // second row
						wave[0].enemy[i][j].image = enemy_mydaq;
						wave[0].enemy[i][j].speed = 1;
						wave[0].enemy[i][j].health = 2;
						wave[0].enemy[i][j].score = 20;
						wave[0].enemy[i][j].status = unspawned;
						
					}
					else if(i == 2){ // third row
						wave[0].enemy[i][j].image = enemy_oscilloscope;
						wave[0].enemy[i][j].speed = 1;
						wave[0].enemy[i][j].health = 1;
						wave[0].enemy[i][j].score = 10;
						wave[0].enemy[i][j].status = unspawned;
					}
					else if(i == 3){ // fourth row
						wave[0].enemy[i][j].image = enemy_mydaq;
						wave[0].enemy[i][j].speed = 1;
						wave[0].enemy[i][j].health = 2;
						wave[0].enemy[i][j].score = 20;
						wave[0].enemy[i][j].status = unspawned;
					}					
		}
		wave[0].row_speed[i] = wave[0].enemy[i][0].speed;
		if(i != 0) {
			wave[0].row_spawn[i] = 0;	//all other rows have not been spawned
		}
	}
}

void wave1Init(void){	//intializing each wave separately in case we want different dimensions, images, and speeds for other waves!!!
	for(int i=0; i<4; i++){ //for each row of enemies
		for(int j=0; j<4; j++){ //for each enemy sprite
			wave[1].enemy[i][j].x = 20*j + 26;
			wave[1].enemy[i][j].y = 10;
			wave[1].enemy[i][j].width = 16;
			wave[1].enemy[i][j].height = 10;
			wave[1].enemy[i][j].status = unspawned;
			wave[1].enemy[i][j].collision_flag = 0;
			
					if(i == 0){				// first row
						wave[1].enemy[i][j].image = enemy_steppermotor;
						wave[1].enemy[i][j].speed = 2;
						wave[1].enemy[i][j].health = 1;
						wave[1].enemy[i][j].score = 20;
					}
					else if(i == 1){ // second row
						wave[1].enemy[i][j].image = enemy_steppermotor;
						wave[1].enemy[i][j].speed = 2;
						wave[1].enemy[i][j].health = 1;
						wave[1].enemy[i][j].score = 20;
						
					}
					else if(i == 2){ // third row
						wave[1].enemy[i][j].image = enemy_mydaq;
						wave[1].enemy[i][j].speed = 1;
						wave[1].enemy[i][j].health = 2;
						wave[1].enemy[i][j].score = 20;
					}
					else if(i == 3){ // fourth row
						wave[1].enemy[i][j].image = enemy_mydaq;
						wave[1].enemy[i][j].speed = 1;
						wave[1].enemy[i][j].health = 2;
						wave[1].enemy[i][j].score = 20;
					}	
		}
		wave[1].row_speed[i] = wave[1].enemy[i][0].speed;
		wave[1].row_spawn[i] = 0;	//all rows have not been spawned
	}
}

void wave2Init(void){	//intializing each wave separately in case we want different dimensions, images, and speeds for other waves!!!
	for(int i=0; i<4; i++){ //for each row of enemies
		for(int j=0; j<4; j++){ //for each enemy sprite
			wave[2].enemy[i][j].x = 20*j + 26;
			wave[2].enemy[i][j].y = 10;
			wave[2].enemy[i][j].width = 16;
			wave[2].enemy[i][j].height = 10;
			wave[2].enemy[i][j].status = unspawned;
			wave[2].enemy[i][j].collision_flag = 0;
			
					if(i == 0){				// first row
						wave[2].enemy[i][j].image = enemy_steppermotor;
						wave[2].enemy[i][j].speed = 2;
						wave[2].enemy[i][j].health = 1;
						wave[2].enemy[i][j].score = 20;
					}
					else if(i == 1){ // second row
						wave[2].enemy[i][j].image = enemy_mydaq;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 2;
						wave[2].enemy[i][j].score = 20;
						
					}
					else if(i == 2){ // third row
						wave[2].enemy[i][j].image = enemy_mydaq;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 2;
						wave[2].enemy[i][j].score = 20;
					}
					else if(i == 3){ // fourth row
						wave[2].enemy[i][j].image = enemy_debug;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 4;
						wave[2].enemy[i][j].score = 40;
					}
			
			
		}
		wave[2].row_speed[i] = wave[2].enemy[i][0].speed;
		wave[2].row_spawn[i] = 0;	//all rows have not been spawned
	}
}

void wave3Init(void){	//intializing each wave separately in case we want different dimensions, images, and speeds for other waves!!!
	for(int i=0; i<4; i++){ //for each row of enemies
		for(int j=0; j<4; j++){ //for each enemy sprite
			wave[3].enemy[i][j].x = 20*j + 26;
			wave[3].enemy[i][j].y = 10;
			wave[3].enemy[i][j].width = 16;
			wave[3].enemy[i][j].height = 10;
			wave[3].enemy[i][j].status = unspawned;
			wave[3].enemy[i][j].collision_flag = 0;
			
					if(i == 0){				// first row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 4;
						wave[3].enemy[i][j].score = 40;
					}
					else if(i == 1){ // second row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 4;
						wave[3].enemy[i][j].score = 40;
						
					}
					else if(i == 2){ // third row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 4;
						wave[3].enemy[i][j].score = 40;
					}
					else if(i == 3){ // fourth row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 4;
						wave[3].enemy[i][j].score = 40;
					}
					
		}
		wave[3].row_speed[i] = wave[3].enemy[i][0].speed;
		wave[3].row_spawn[i] = 0;	//all rows have not been spawned
	}
}


void allwaves_statusInit(void){
	for(int i = 0; i < 4; i++){
			wave[i].clear = 0;			//no waves are cleared at beginning of game
			for(int j = 0; j < 4; j++){
				wave[i].row_clear[j] = 0;	//no rows are clear at beginning of game
				wave[i].row_spawn[j] = 0;
			}
	}
}

void moveEnemyRight(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num){
			if(wave[wave_num].enemy[row_num][enemy_num].image == enemy_steppermotor){
					wave[wave_num].enemy[row_num][enemy_num].x += 2;
			}
			else{
					wave[wave_num].enemy[row_num][enemy_num].x += 1;
			}
}
void moveEnemyLeft(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num){
				if(wave[wave_num].enemy[row_num][enemy_num].image == enemy_steppermotor){
						wave[wave_num].enemy[row_num][enemy_num].x -= 2;
				}
				else{
						wave[wave_num].enemy[row_num][enemy_num].x -= 1;
				}
}

void moveEnemyDown(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num){
		wave[wave_num].enemy[row_num][enemy_num].y += 1;
}


void reload_movement_cts(uint8_t i, uint8_t j){
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			moveRightInit_ct[i][j] = (26/wave[wave_number].enemy[i][j].speed);		//used to be regular arrays of 4 elements
			moveDownR_ct[i][j] = 0;
			moveDownL_ct[i][j] = 0;
			moveRight_ct[i][j] = 0;
			moveLeft_ct[i][j] = 0;
		}
	}
}

