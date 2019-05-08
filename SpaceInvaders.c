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
	
#define bigMissile_Idx	0
#define laser_Idx				1
#define waveClear_Idx		2
//FUNCTION PROTOTYPES BELOW
//----------------------------------------------------------------------------------------------------
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

void moveEnemyRight(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num); // helper movement functions that move a specific enemy based on the row and column parameters
void moveEnemyLeft(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num);  // Right and Left movement works for both speeds
void moveEnemyDown(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num); // Down movement is always 1 pixel regardless of speed

void SysTick_Init(void);
void playerInit(void);
void missilesInit(void);
void powerupInit(void);
void collisionFlag_Init(void);
void explosionsInit(void);

void wave0Init(void); void wave1Init(void); void wave2Init(void); void wave3Init(void);
void allwaves_statusInit(void);

void playsound(void);	// function that outputs samples of sound arrays depending on global varables (based on current global flags of sound effect and length of sample array)
void animation_spawn_delay(void); //periodic function that is called every 125 ms through the Timer0 ISR
void missile_move(void);

void reload_movement_cts(void); // updated to reload every value of the movement 2D arrays


//MAIN HELPER FUNCTIONS
void gameOver(void); //disables interrupts except timer1 for sound, displays game over messages and final score, then remain indefinitely in a while loop to freeze final screen
void gameOverPlayerDeathAnimation(void); // freezes player, flash 3 times, play losing audio, big explosion animation/audio
void gameOverEnemyDeathAnimation(void); // similar to previous function: freezes screen, but only flashes enemy, then explosion animation and change gameOver_flag to 2
void setAndDrawNewPlayerPos(void); // Move player sprite BY 1 PIXEL PER 32 ADC VALUE based off of difference in ADC values (IGNORES CHANGES IN ADC VALUE LESS THAN 32)(doesn't run if semaphore flag not set)
void redrawSamePlayerPos(void); //restores the player sprite if any fillrectangles erases the player (FUNCTION CALLED AFTER ANY EXPLOSION OR ENEMY FILL RECT)
void spawnBasicMissile(void); // checks the systick ISR if the m1spawn and m2spawn flags were set to 1, if so, then reset flag, draw missile bmp, reset fireratelimit_counter, and set the missile sprite status to moving
void spawnBigMissile(void);
void spawnLaser(void);
void moveAllMissiles(void); //moves ALL PLAYER MISSILES (including basic, big LED, and laser) //NOT CALLED YET CURRENTLY COMMENTED OUT
void moveBasicMissile(void); //move player basic missile based on movemissile global flag; decrement y position of missile and draws missile at new position
void moveBigMissile(void);
void moveLaser(void);
void fillBasicMissile(void); //FillRect over the basic missile and SET MISSILE SPRITE STATUS TO DEAD if missileCollisionFlag[i] is set
void fillBigMissile(void);
void fillEnemyDrawExplosionFrame1(void);  //IF COLLISION FLAG ATTRIBUTE SET FOR ENEMY, DRAW OVER ENEMY AND DRAW FIRST FRAME OF EXPLOSION
void explosionAnimationUpdate(void); // Draw NEWEXPLOSION_ANIM FRAMES AND CLEARING EXPLOSIONS ONCE ANIMATION IS DONE
void movePowerup(void);
void moveEnemies(void); // iterate through 4 rows, if row uncleared, draw bmp of new position for enemies that are still alive
void printWaveClearedMessages(void); // uses a switch(wave_number) to print messages after each wave is cleared (using ST7735_DrawString)
void printWinScreen(void);
void waveClearedTransition(void); // DISABLES INTERRUPTS DURING FUNCTION, PRINT WAVE CLEARED MESSAGES, DELAYS 2 ms, AND DOES ALL VARIABLE RELOADS NEEDED FOR NEXT WAVE (such as current_row_number, movement_cts, powerup_spawn, wave_spawn flag...)
void printCurrentScore(void); // converts total_score to a character array and draws 4 digit score to top left corner of LCD
void spawnNextRow(void); // spawns new row every 3 seconds based on counter_125ms until current_row_number == 4


// SYSTICK HELPER FUNCTIONS
void DetectEnemyPlayerCollision(void); // check if enemy has collided with player sprite --> IF SO, SETS gaeOver_flag TO 1
void DetectEnemyAtBottom(void); // Check if enemy y position == 159 aka touches bottom of screen --> IF SO, SETS gaeOver_flag TO 1
void getADC(void); // put ADC data into ADCMAIL using ADC_In() and set semaphore flag to 1 (DOES NOT RUN IF gaeOver_flag SET TO 1)
void DetectPE0(void); // check primary button press (red) (DOES NOT RUN IF GAMEOVERFLAG SET TO 1)
void DetectPE1(void); // check secondary button press (yellow) (DOES NOT RUN IF GAMEOVERFLAG SET TO 1)
void DetectMissilePowerupCollision(void);
//void updateUpgrade(void);
void DetectMissileEnemyCollision(void);
void PowerupMovement_PositionUpdate(void);
void EnemyMovement_PositionUpdate(void);
void UpdateWaveStructAttr(void); // checks which enemies are dead and updates wave[wave_number].clear and wave[wave_number].row_clear[4] attributes accordingly
// end of function prototypes--------------------------------------------------------------------------------------------------------------------------------------------------------------------- 



//ENUM AND STRUCT DEFINITIONS
//---------------------------------------------------------------------
enum state{alive, moving, dead, dying, unspawned};
typedef enum state status_t;		// status_t for states

enum secondary{unequipped, led, laser, waveclear, set_led, set_laser, set_waveclear}; // ADDED NEW ENUM TYPES TO TRY TO DEBUG THE LASER + POWERUP COLLISION BUG (ONLY SET THE secondary_t enum upgrade IN MAIN) 
typedef enum secondary secondary_t;

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
	//uint8_t row_speed[4];
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
//uint8_t update_player_position_flag = 0;
uint8_t counter_125ms = 0; //used in animation periodic task (next row will not spawn until 3 second has passed since last row spawned)
														//when counter_125ms = 24 then 3 seconds has passed

uint32_t total_score = 0; //total_score is converted to a null terminated character array at the end of the game engine to display the score (using %10 and /10 then DrawString)
uint32_t total_score_copy = 0; //used to convert score without overwriting game score
uint8_t gameOver_flag = 0;

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
int8_t firerate_limit_counter = 2;		//allow player to fire at the beginning of the game with no restrictions
		//uint8_t enemyCollisionFlag[6] = {0,0,0,0,0,0};

// powerup global variables
uint8_t powerupIdx = 0;		// selects which powerup to spawn
uint8_t powerupSpawn = 0; // flag set once powerup has spawned (1 per wave)
uint8_t powerup_ct[3] = {0, 0, 0};		// counter for secondary attacks

// secondary attack global variables

// big missile (LED)
uint8_t bigMissile_counter = 0;	// intialize to 4 when first equipped counter value of 4 corresponds to minimum of 0.5 s passing between each shot (RESET TO 0 UPON BIG MISSILE SPAWN)
																	//set to 4 upon any secondary missile collision w powerup so 0 ms must pass before led can be spawned and fired
uint8_t bigMissile_spawn = 0;
uint8_t moveBigMissile_flag = 0;
uint8_t bigMissileCollision_flag = 0;

// laser
uint8_t laser_counter = 0; //init to 3 when first equipped (in PE1 function >= check with counter value of 4 corresponds to minimum of 0.5 s passing between each shot)(RESET TO 0 UPON LASER SPAWN) 
														//set to 3 upon any secondary missile collision w powerup so 125 ms must pass before laser can be spawned and fired
uint8_t laser_spawn = 0;
uint8_t moveLaser_flag = 0;
uint8_t laserCollision_flag = 0;
const uint16_t *laserFrame[3];
uint8_t laserFrame_idx = 0;	// laser image frame index
uint8_t laserFrame_dwell = 0;

// waveclear
uint8_t waveClear_counter = 0; //Init to 2 when equipped Set to 0 upon secondary projectile collision w powerup and set to 2 when waveClearDone_flag is set to 1 (ready to use again when the powerup has finished being used)
uint8_t waveClearDone_flag = 0;
uint8_t waveClearEnabled_flag = 0;
//sprites
sprite_t player;
sprite_t missiles[2];
sprite_t missile_LED;
sprite_t missile_laser;
secondary_t upgrade = unequipped; 
sprite_t powerup[4];
//enemy_t enemy[4];
sprite_t explosion_anim[5];
wave_t wave[4];						 //4 wave structs; wave[0] refers to first wave, wave[1] is second wave... (EACH WAVE STRUCT CONTAINS A 2D ARRAY OF 4 ROWS OF 4 ENEMIES)

//sound global variable flags
const unsigned char *sound_pointer = silence; 
uint32_t sound_length;						// size of current sound effect array
uint32_t sound_index = 0;					// index of current sound effect
uint8_t interrupting_laser_sound_flag = 0; // set whenever player missile fire (laser5 sound) interrupts any other sound; cleared once sound index == laser5 sound array length

const unsigned char *sound_pointer_save = silence; //these three are used if a longer sound is interrupted by a shorter sound and first sound needs to be resumed
uint32_t sound_length_save = 0;
uint32_t sound_index_save = 0;

//explosion animation global variables
uint8_t explosion_counter = 0;
uint8_t explosion_frame = 0;
uint8_t explosion_done = 1;
uint8_t clear_explosion_early = 0;
uint16_t x_save;
uint16_t y_save;

//enemy movement flags/counters
uint8_t move_enemy = 0;
uint8_t moveRightInit_ct[4][4] = {{26,26,26,26},{26,26,26,26},{26,26,26,26}, {26,26,26,26}}; // movement variables initialized for every enemy (row, column)
uint8_t moveRight_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveLeft_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveDownR_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
uint8_t moveDownL_ct[4][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};

//score pointer and digit variables
char 
scorept[5] = {'0', '0', '0', '0', 0};	// will hold the digits of the score to display

//wave-clear message pointers to character arrays
char msg1pt[15] = {'W', 'A', 'V', 'E', ' ', '1', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg2pt[15] = {'W', 'A', 'V', 'E', ' ', '2', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg3pt[15] = {'W', 'A', 'V', 'E', ' ', '3', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg4pt[15] = {'W', 'A', 'V', 'E', ' ', '4', ' ', 'C', 'L', 'E', 'A', 'R', 'E', 'D', 0};
char msg5pt[15] = {'F', 'I', 'N', 'A', 'L', ' ', 'B', 'O', 'S' , 'S', 0};
char winpmsgpt[] = {'Y', 'O', 'U', ' ', 'P', 'A', 'S', 'S', 'E', 'D', '.', 0}; // 11 characters
char msg_gameOver_line1[] = {'G', 'A', 'M', 'E', ' ', 'O', 'V', 'E', 'R', ' ', 'S', 'T', 'U', 'D', 'E', 'N', 'T', '.', 0}; // 18 characters long (offset x column by 1)
char msg_gameOver_line2[] ={'Y', 'O', 'U', ' ', 'F', 'A', 'I', 'L', 'E', 'D', ' ', 'E', 'E', '3', '1', '9', 'K', '.', 0}; // 18 characters long (offset by 1 as well)
char msg_finalscore[] = {'F', 'I', 'N', 'A', 'L', ' ', 'S', 'C', 'O', 'R', 'E', ':', ' ', 0}; // 13 chars plus 4 chars for score = 17 chars (offset by 1)

//ANDY'S GAME OVER IDEA GLOBAL VARIABLES
uint8_t collided_enemy_i; // save the row position of the colliding enemies that caused game over
uint8_t collided_enemy_j = 0; // save the col position of the colliding enemy that touched the player
int8_t collided_enemies_j[4] = {-1, -1, -1, -1}; // save the col positions of the enemies that reached bottom that caused game over
uint8_t j_index = 0;
//------------------------------------------------------------------------------------------------




//MAIN STARTS HERE======================================================================================================================================================================================================
//======================================================================================================================================================================================================================
//======================================================================================================================================================================================================================
int main(void){
	//All GPIO port/interfacing/interrupts initialization calls
	DisableInterrupts();
  PLL_Init(Bus80MHz); // Bus clock is 80 MHz 
  Random_Init(1);
  Output_Init();
	ADC_Init();
	DAC_Init();
	PortE_Init();
	SysTick_Init();
	Timer1_Init(&playsound, 7256);						// Timer1 interrupts at 11.025 kHZ (period value needed is 7256.24) // for 44.1 kHz --> (period value is 1814.05) butWC script is only 11.025 samples
	Timer2_Init(&missile_move, 1000000); 			//Timer2 interrupts at 80 Hz (player primary fire... missiles travel 1 pixel per 12.5 ms = 80 pixels per second)
  ST7735_FillScreen(0x0000);								// set screen to black
  
	//All sprite init calls (except player sprite)
	missilesInit();
	powerupInit();
	collisionFlag_Init();
	explosionsInit();
	allwaves_statusInit();
	wave0Init();
	wave1Init();
	wave2Init();
	wave3Init();
	//playerInit();
	
	EnableInterrupts();
	Timer0_Init(&animation_spawn_delay, 10000000); 				//Timer0 interrupts at 8 Hz (every 125 ms) ALSO CONTROLS ROW SPAWN EVERY 3 s)
	Data = ADCMAIL;			// get init player position (ADC)
	Prev_adc = Data;		// init previous adc value
	
	playerInit(); // moved playerInit here after ADC reading to try to fix the initial player position bug
	ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height); 		// player position init based on ADC


	
  while(1){ // START OF GAME ENGINE //	
	//== GAME OVER CHECK IF PLAYER COLLISION WITH ENEMY DETECTED ====================================================================================================================
			 if(player.status == dying && gameOver_flag == 1){ // run this if the enemy collided with player
				  gameOverPlayerDeathAnimation(); // only runs this function if enemy collided with player gameOver_flag is set to 1
					gameOver(); // disables all interrupts except timer1 for sound, displays game over messages and final score, then remain indefinitely in a while loop to freeze final screen
			 }
			 else if(gameOver_flag == 1){ // only run this function if enemy has reached bottom of screen (doesn't set player status to dying yet if detected in ISR)
					gameOverEnemyDeathAnimation();
					gameOver();
			 }
	//===============================================================================================================================================================================
	
			 
	//== DRAW NEW PLAYER POSITION THAT IS SET IN SYSTICK ISR ==========
			 setAndDrawNewPlayerPos(); // only runs if the semaphore flag is set
	//=================================================================

			 
	//== ALL TYPES OF MISSILE SPAWNS=================================================================================================================================================================
	//== Based off of global flags set in SysTick ISR to draw missile bmp, clear flag, reset firerate counter, and CHANGE MISSILE STATUS TO MOVING
			 // BASIC MISSILE SPAWN (primary missile)
				  spawnBasicMissile();
					
			 // BIG MISSILE SPAWN: based off of bigMissile_spawn global flag (set in SysTick ISR)
				  spawnBigMissile();
				
			 // LASER SPAWN: based off of laser_spawn global flag (set in SysTick ISR)
					spawnLaser();
	//================================================================================================================================================================================================
	
			 
	//== ALL MISSILE MOVEMENT==========================================================================================================================================================================
			 // THE THREE MISSILE MOVEMENT FUNCTIONS BELOW CAN BE REPLACED BY THE FUNCTION BELOW:
				  moveAllMissiles();
			 
			 //BASIC MISSILE MOVEMENT (by 1 pixel based on movemissile global flag that is set every 1/80th sec by Timer2 ISR (missile_move periodic task)) 
				 //moveBasicMissile();

			 //BIG MISSILE MOVEMENT (up by 1 pixel based on moveBigMissile_flag global flag)
			   //moveBigMissile();
						
			 //LASER MOVEMENT (up by 1 pixel based on moveLaser_flag global flag)
				 //moveLaser();
	//===============================================================================================================================================================================================
	
						
	//== ALL MISSILE FILLING/DEATH (Fill missiles if their collision flag has been set in SysTick ISR) ===================================
			// Basic missile fill (if the collison flag is set)
			   fillBasicMissile();
							
			// Big missile fill (if the collison flag is set)
			   fillBigMissile();
	//=====================================================================================================================================

				
	//== FILL ENEMY UPON COLLISION AND DRAW FIRST EXPLOSION_ANIM FRAME=========
			 fillEnemyDrawExplosionFrame1();
	//=========================================================================================================

				
	//== EXPLOSION ANIMATION FRAME UPDATES (CHECKING FRAMES AND CLEARING EXPLOSIONS ONCE ANIMATION IS DONE) =========================================================================================================
		   explosionAnimationUpdate();
	//===============================================================================================================================================================================================================
		

				
	//== MOVE POWERUP BOX (only if the powerup sprite status is alive and has not exited the left side of LCD screen) =========================================================
			 movePowerup();
	//========================================================================================================================================================================


	//== MOVE ENEMIES BY DRAWING NEW ENEMY POSITION (COORDINATES INCREMENTED IN SYSTICK ISR) ======
	//== only moves enemy sprites whose statuses are == alive
			 moveEnemies();
	//=============================================================================================

		
	//== WAVE CLEARED MESSAGES, DISABLE INTERRUPTS, DELAYS, AND VARIABLE RELOADS (IF CLEAR ATTRIBUTE == 2 FOR CURRENT WAVE STRUCT) =========================
			 if(wave[wave_number].clear == 2){ //set to 1 when deadcounter is 16, set to 2 when last enemy explosion ends
				 while(sound_pointer != silence){} // wait for last sound to finish before proceeding
					waveClearedTransition(); // only do this transition after last enemy in the wave dies and explodes (ALL INTERRUPTS DISABLED IN THIS FUNCTION!!!)
			 }
	//======================================================================================================================================================

		
	//== RUNNING SCORE DISPLAY (AS GAME IS PLAYED) ======
			 printCurrentScore();
	//===================================================
	
		
	//== NEXT ROW SPAWN (every 3 s) ====
		   spawnNextRow();
	//==================================
			 
			 
	}//END OF GAME ENGINE
	
}//END OF MAIN==========================================================================================================================================================================================================
//======================================================================================================================================================================================================================
//======================================================================================================================================================================================================================
















//SYSTICK ISR START=====================================================================================================================================================================================================
//======================================================================================================================================================================================================================
//======================================================================================================================================================================================================================
void SysTick_Handler(void){ // every 16.67 ms
	//== ENEMY + PLAYER COLLISION (GAME OVER) ===============================
			 DetectEnemyPlayerCollision(); //Sets gameOver_flag to 1 if detected
	//=======================================================================
	
	//== ENEMY REACHED BOTTOM OF SCREEN (GAME OVER) ===============================
			 DetectEnemyAtBottom(); //Sets gameOver_flag to 1 if detected
	//=======================================================================
	
	
	//== PLAYER HORIZONTAL MOVEMENT: get ADC reading from slideport and save to ADCMAIL, then set flag = 1 ========================================================
			 if(gameOver_flag == 0){ //player can only move if enemy has NOT collided with them
				  getADC(); // sets semaphore: flag = 1
			 }
	//=============================================================================================================================================================

	
	//== BUTTON CHECKING: CHECK PORT E PIN 0 (PRIMARY FIRE BUTTON) TO DETERMINE MISSILE SPAWN ==================================================================================
	//== PLAYER PRIMARY FIRE LIMITS: only 2 missiles allowed onscreen at a time, 250 ms delay between shots, missile images can never overlap (minimum of 5 pixels apart)
		   if(gameOver_flag == 0){ // only allowed to fire basic misiles if player hasn't lost
					DetectPE0();
			 }
	//==========================================================================================================================================================================
		

	//DEBUG THIS FUNCTION (BUGS WITH SECONDARY MISSILE COLLIDING WITH POWERUPS MOVED THIS FUNCTION DIRECTLY AFTER DETECTPE1() FUNCTION (1:31 PM)
	//DEBUG THIS FUNCTION
	//== ALL MISSILES + POWERUPS COLLISION DETECTION =========================================
			 if(gameOver_flag == 0){ // no need to check for powerup collision if player has lost
					DetectMissilePowerupCollision();
			 }
	//========================================================================================
	//DEBUG THIS FUNCTION
	//DEBUG THIS FUNCTION

			 
	//== BUTTON CHECKING: CHECK PORT E PIN 1 (SECONDARY FIRE BUTTON) TO DETERMINE SECONDARY MISSILE SPAWN ===============================================
	//== PLAYER SECONDARY FIRE LIMITS: only 1 missiles allowed onscreen at a time, 500 ms delay per shot, limited number of shot charges (3 BIG LEDS, 2 LASERS, 1 WAVECLEAR)
		   if(gameOver_flag == 0){ // only allowed to fire secondary misiles if player hasn't lost
					DetectPE1();
			 }
	//=================================================================================================================================================
	 

	//== ALL MISSILES + ENEMIES COLLISION DETECTION ==========================================
			 if(gameOver_flag == 0){ // no need to check for any more collision if player has lost
					DetectMissileEnemyCollision();
			 }	  
	//========================================================================================


	//== POWERUP MOVEMENT ==========================================================
			 if(gameOver_flag == 0){ // no need to move powerup icon if player has lost
					PowerupMovement_PositionUpdate();
			 }	  
	//==============================================================================
	
	
	//== ENEMY MOVEMENT DECISIONS =============================================
			 if(gameOver_flag == 0){ // no need to move enemies if player has lost
					EnemyMovement_PositionUpdate();
			 }	 
	//=========================================================================

			 
	//== Update row clear attributes and wave clear attributes after enemies have been written over(depending on current wave #) =====
			 if(gameOver_flag == 0){ // no need to update attributes if player has lost
					UpdateWaveStructAttr();
			 }
	//================================================================================================================================
			
			 
}//END OF SYSTICK ISR===================================================================================================================================================================================================
//======================================================================================================================================================================================================================
//======================================================================================================================================================================================================================
//======================================================================================================================================================================================================================











//EVERYTHING BELOW THIS LINE IS INITIALIZATION AND HELPER FUNCTIONS
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


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
					sound_index_save++;
			}
			else{	// sound_index == sound_length and sound effect is done playing
					if(sound_pointer == laser5 && interrupting_laser_sound_flag == 1){ //case if laser sound interrupting explosion sound is finished --> RESTORE INTERRUPTED EXPLOSION SOUND STATE
						interrupting_laser_sound_flag = 0;
						sound_pointer = sound_pointer_save;
						sound_length = sound_length_save;
						sound_index = sound_index_save;
						if(sound_index >= sound_length) //if interrupted sound effect happened to finish during the laser5 sound, then just set sound_pointer to silence and index back to 0
						{
									//if(sound_pointer == game_over_screen){
									//	sound_index = 0;
									//}
								//	else{
										sound_pointer = silence;
								//	}
									//sound_index = 0; // sound index is set manually before pointer is reloaded
						}
					}
					else{
						sound_pointer = silence;
						//sound_index = 0;			// if silence is the sound_pointer, then playsound has no work to do
					}					
			}
	}
}

void animation_spawn_delay(void){
	if(explosion_done == 0){
		explosion_counter++; // increment counter every 125 ms
	}
	
	firerate_limit_counter++; //increment counter every 125 ms
	bigMissile_counter++;			// increment counter every 125 ms
	laser_counter++;
	waveClear_counter++;
	
	if(wave_spawn_done == 1){//if last row has been spawned, don't increment counter
		counter_125ms = 0; //keep counter at 0 until next wave spawns (MAKE SURE TO CLEAR WAVE SPAWN DONE FLAG UPON THE SPAWN OF THE FIRST ROW OF NEXT WAVE)										
	}
	else{
		counter_125ms++;
	}
}


void missile_move(void) { // fires ALL player projectiles at 80 HZ (moves 80 pixels per sec)
	// check if player missile has reached top of screen, if not, set movemissileflag
	for(int i = 0; i < 2; i++){
		if(missiles[i].y == 0){
			missiles[i].status = dead;
		}
		else{
			movemissileflag = 1;
		}
	}
	// check if big missile has reached top of screen, if not, set moveBigMissile_flag
	if(missile_LED.status != dead){
		if(missile_LED.y == 0){
			missile_LED.status = dead;
		}
		else{
			moveBigMissile_flag = 1;
		}	
	}
	// check if laser has reached top of screen, if not, set moveLaser_flag
	if(missile_laser.status != dead){
		if(missile_laser.y == 0){
			missile_laser.status = dead;
		}
		else{
			moveLaser_flag = 1;
		}		
	}
}		
//-------------------------------------------------------------------------------------------------------------------



// SPRITES INITIALIZATION FUNCTIONS BELOW
//---------------------------------------------------------------------------------------------------------------------
void playerInit(void){
	player.x = Data/40;	
	player.y = 159;
	player.width = 18;
	player.height = 8;
	player.image = tm4ship;
	player.status = alive;
}

void missilesInit(void){// at beginning of the game, both missiles have not been fired so status is dead
	// basic attack init
	for(int i = 0; i < 2; i++){
		missiles[i].x = 0;
		missiles[i].y = 0;
		missiles[i].width = 5;
		missiles[i].height = 10;
		missiles[i].image = missile;
		missiles[i].status = dead;
	}
	// secondary attack (LED) init
	missile_LED.x = 0;
	missile_LED.y = 0;
	missile_LED.width = 14;
	missile_LED.height = 24;
	missile_LED.image = missile_big;
	missile_LED.status = dead;
	// secondary attack (laser) init
	missile_laser.x = 0;
	missile_laser.y = 0;
	missile_laser.width = 5;
	missile_laser.height = 20;
	missile_laser.image = missile_laser1;
	laserFrame[0] = missile_laser0;
	laserFrame[1] = missile_laser1;
	laserFrame[2] = missile_laser2;
	missile_laser.status = dead;
}


void powerupInit(void){
	for(int i = 0; i < 4; i++){
		powerup[i].x = 111;
		powerup[i].y = 130;
		powerup[i].width = 16;
		powerup[i].height = 10;
		if(i == 0){
			powerup[i].image = power_LED;
		}else if(i == 1){
			powerup[i].image = power_laser;
		}else if(i == 2){
			powerup[i].image = power_waveClear;
		}else if(i == 3){
			powerup[i].image = power_LED;
		}
		powerup[i].status = dead;
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
	
}


//SYSTICK INITIALIZATION FUNCTION
//---------------------------------------------------
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // 1) disable SysTick during setup
  NVIC_ST_RELOAD_R = 1333333;  					// 2) reload value = 1333333 for clock running at 80MHz --> 60 Hz
  NVIC_ST_CURRENT_R = 0;                // 3) any write to current clears it
	NVIC_SYS_PRI3_R = (NVIC_PRI3_R&0x00FFFFFF)|0x20000000; // SET SYSTICK ISR PRIORITY TO 1 (2nd HIGHEST INTERRUPT PRIORITY SINCE TIMER1 IS 0 PRIORITY)
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



//ENEMY INITIALIZATION AND ENEMY MOVEMENT HELPER FUNCTIONS
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
						wave[0].enemy[i][j].health = 3;
						wave[0].enemy[i][j].score = 30;
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
						wave[0].enemy[i][j].health = 3;
						wave[0].enemy[i][j].score = 30;
						wave[0].enemy[i][j].status = unspawned;
					}					
		}
		//wave[0].row_speed[i] = wave[0].enemy[i][0].speed;
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
						wave[1].enemy[i][j].image = enemy_keil;
						wave[1].enemy[i][j].speed = 2;
						wave[1].enemy[i][j].health = 1;
						wave[1].enemy[i][j].score = 20;
					}
					else if(i == 1){ // second row
						wave[1].enemy[i][j].image = enemy_keil;
						wave[1].enemy[i][j].speed = 2;
						wave[1].enemy[i][j].health = 1;
						wave[1].enemy[i][j].score = 20;
						
					}
					else if(i == 2){ // third row
						wave[1].enemy[i][j].image = enemy_mydaq;
						wave[1].enemy[i][j].speed = 1;
						wave[1].enemy[i][j].health = 3;
						wave[1].enemy[i][j].score = 30;
					}
					else if(i == 3){ // fourth row
						wave[1].enemy[i][j].image = enemy_mydaq;
						wave[1].enemy[i][j].speed = 1;
						wave[1].enemy[i][j].health = 3;
						wave[1].enemy[i][j].score = 30;
					}	
		}
		//wave[1].row_speed[i] = wave[1].enemy[i][0].speed;
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
						wave[2].enemy[i][j].image = enemy_keil;
						wave[2].enemy[i][j].speed = 2;
						wave[2].enemy[i][j].health = 1;
						wave[2].enemy[i][j].score = 20;
					}
					else if(i == 1){ // second row
						wave[2].enemy[i][j].image = enemy_mydaq;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 3;
						wave[2].enemy[i][j].score = 30;
						
					}
					else if(i == 2){ // third row
						wave[2].enemy[i][j].image = enemy_mydaq;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 3;
						wave[2].enemy[i][j].score = 30;
					}
					else if(i == 3){ // fourth row
						wave[2].enemy[i][j].image = enemy_debug;
						wave[2].enemy[i][j].speed = 1;
						wave[2].enemy[i][j].health = 6;
						wave[2].enemy[i][j].score = 60;
					}
			
			
		}
		//wave[2].row_speed[i] = wave[2].enemy[i][0].speed;
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
						wave[3].enemy[i][j].health = 6;
						wave[3].enemy[i][j].score = 60;
					}
					else if(i == 1){ // second row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 6;
						wave[3].enemy[i][j].score = 60;
						
					}
					else if(i == 2){ // third row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 6;
						wave[3].enemy[i][j].score = 60;
					}
					else if(i == 3){ // fourth row
						wave[3].enemy[i][j].image = enemy_debug;
						wave[3].enemy[i][j].speed = 1;
						wave[3].enemy[i][j].health = 6;
						wave[3].enemy[i][j].score = 60;
					}
					
		}
		//wave[3].row_speed[i] = wave[3].enemy[i][0].speed;
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
			if(wave[wave_num].enemy[row_num][enemy_num].image == enemy_keil){
					wave[wave_num].enemy[row_num][enemy_num].x += 2;
			}
			else{
					wave[wave_num].enemy[row_num][enemy_num].x += 1;
			}
}
void moveEnemyLeft(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num){
			if(wave[wave_num].enemy[row_num][enemy_num].image == enemy_keil){
					wave[wave_num].enemy[row_num][enemy_num].x -= 2;
			}
			else{
					wave[wave_num].enemy[row_num][enemy_num].x -= 1;
			}
}

void moveEnemyDown(uint8_t wave_num, uint8_t row_num, uint8_t enemy_num){
		wave[wave_num].enemy[row_num][enemy_num].y += 1;
}

//RELOEAD MOVEMENT COUNTER FUNCTION
//RESETS THE COUNTER ARRAYS BEFORE THE NEXT ARRAY FOR PROPER ENEMY MOVEMENT
void reload_movement_cts(void){
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			moveRightInit_ct[i][j] = (26/wave[wave_number].enemy[i][j].speed);		//used to be regular arrays of 4 elements
			moveDownR_ct[i][j] = 0;
			moveDownL_ct[i][j] = 0;
			moveRight_ct[i][j] = 0;
			moveLeft_ct[i][j] = 0;
		}
	}
} // end of reload_movement_cts() function

void gameOver(void){ // ONLY RUNS THIS FUNCTION IF game_Over_flag == 2 (which is set at end of gameover_playerdeathanimation function)
	if(gameOver_flag == 2){
			//DisableInterrupts();
			//disable_player_controls = 1; // set to 1 again to be safe (use polling with gameOverflag == 1 instead does same thing)
			
			//ONLY NEED TIMER1 INTERRUPTS NOW TO PLAY MUSIC DURING THE GAME OVER SCREEN (NOT WORKING RIGHT NOW)
				NVIC_ST_CTRL_R = 0x0000; // disable all SysTick Interrupts
				TIMER0_CTL_R = 0x00000000;    // disable timer0
				TIMER0_IMR_R = 0x00000000;    // disable timeout interrupt for timer0 that controls animation counters and spawn counters
				TIMER2_CTL_R = 0x00000000;    // disable timer2
				TIMER2_IMR_R = 0x00000000;    // disable timeout interrupt for timer2 that controls player primary fire missile speed
			
			// ending screen music not working as of now
				//sound_pointer = game_over_screen;
				//sound_length = 115456; // check if there is enough storage for 11 sec song clip
				//sound_index= 0;
			
			ST7735_FillScreen(0x0000);
			ST7735_DrawString(2, 6, msg_gameOver_line1, 0xFFFF);
			ST7735_DrawString(2, 7, msg_gameOver_line2, 0xFFFF);
			ST7735_DrawString(2, 8, msg_finalscore, 0xFFFF);
			for(int i = 3; i >= 0; i--){ //convert 4 digit integer score to char array scorept
					scorept[i] = ((total_score % 10) + 0x30);	// integer score value is converted to a character array of 4 digits and a null sentinel
					total_score /= 10;
			}
			scorept[4] = 0; // null terminate the last element in the scorept char array
			ST7735_DrawString(15, 8, scorept, 0xFFFF);
			while(1){} //freeze the game on the game over screen permanantly
			//while(sound_pointer == game_over_screen){} //stay in this loop forever to keep playing music and freeze text
		}
}

void gameOverEnemyDeathAnimation(void){
						NVIC_ST_CTRL_R = 0x0005; // disable systick interrupts to freeze collision frame and prevent all enemy movement, missile fire, adc movement
						//disable_player_controls = 1; // SET DISABLE FLAG TO 1 SINCE NOW PLAYER SHOULDN'T BE ABLE TO DO ANYTHING (use polling with gameOverflag == 1 instead does same thing)
						//DisableInterrupts(); //prevent all interrupts DON'T USE THIS SINCE WE STILL NEED TIMER0 INTERRUPTS FOR ANIMATIONS AND TIMER1 FOR SOUND
						sound_pointer = playerdeathaudio; // set sound effect to player game over jingle
						sound_length = 23748;
						sound_index = 0;
						
				 // Only fill over enemies that are not the colliding enemies
						for(int i = 0; i < 4; i++){
							 for(int j = 0; j < 4; j++){
									if(wave[wave_number].enemy[i][j].status != dead){ // no need to fill enemies that are already dead
										 if(i != collided_enemy_i){
												for(int k = 0; k < j_index; k++){
														if(j != collided_enemies_j[k]){
															 ST7735_FillRect(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y - wave[wave_number].enemy[i][j].height + 1, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height, 0x0000);
														}
												}
										 }
									 }
							 }
						}
	
						// Flash enemy that breached bottom edge 4 times while death audio jingle above is playing
							 for(int i = 0; i < 4; i++){ //flash enemy that touched bottom of screen 4 times
										 Delay100ms(5); // delay 500 ms
										 for(int j = 0; j < j_index; j++){  // fill in every enemy that collided w bottom (could be multiple at a time up to 4)
												 ST7735_FillRect(wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].x, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].y - wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].height + 1, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].width, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].height, 0x0000);
										 }
										 Delay100ms(2); // delay 200 ms
										 for(int j = 0; j < j_index; j++){  // redraw every enemy that collided w bottom (could be multiple at a time up to 4)
												 ST7735_DrawBitmap(wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].x, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].y, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].image, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].width, wave[wave_number].enemy[collided_enemy_i][collided_enemies_j[j]].height);
										 }
							 } // repeat 4 times
						
						// Clear player sprite and all enemies
							 for(int i = 0; i < 4; i++){
									for(int j = 0; j < 4; j++){
										 // if(wave[wave_number].enemy[i][j].status != dead){
											   ST7735_FillRect(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y - wave[wave_number].enemy[i][j].height + 1, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height, 0x0000);
											   wave[wave_number].enemy[i][j].status = dead;
										 // }
									}
							 }						 
							 ST7735_FillRect(player.x, player.y - player.height + 1, player.width, player.height, 0x0000); // clear image of collided enemy and player
							 player.status = dead; // player status is dead since they lost the game
							 while(sound_pointer != silence){} //busy-wait until sound effect is done playing before continuing

							 if(player.status == dead){ // do explosion animation and set sound pt for bigger explosion, also increment gameover flag right before exiting this if-statement
									ST7735_FillScreen(0x0000); // clear screen to all black since explosion animation is done
									gameOver_flag = 2; // set game over flag to 2 once dramatic explosion is done playing
							 }
} // end of gameOverEnemyDeathAnimation() function

void gameOverPlayerDeathAnimation(void){
					if(player.status == dying && gameOver_flag == 1){ // ISR detected enemy+player collision
					NVIC_ST_CTRL_R = 0x0005; // disable systick interrupts to freeze collision frame and prevent all enemy movement, missile fire, adc movement
					//disable_player_controls = 1; // SET DIABLE FLAG TO 1 SINCE NOW PLAYER SHOULDN'T BE ABLE TO DO ANYTHING (use polling with gameOverflag == 1 instead does same thing)
					//DisableInterrupts(); //prevent all interrupts DON'T USE THIS SINCE WE STILL NEED TIMER0 INTERRUPTS FOR ANIMATIONS AND TIMER1 FOR SOUND
					sound_pointer = playerdeathaudio; // set sound effect to player game over jingle
					sound_length = 23748;
					sound_index = 0;
					for(int i = 0; i < 4; i++){ //flash player and colliding enemy 4 times
						Delay100ms(5); // delay 500 ms
						ST7735_FillRect(wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].x, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].y - wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].height + 1, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].width, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].height, 0x0000);
						ST7735_FillRect(player.x, player.y - player.height + 1, player.width, player.height, 0x0000); // clear image of collided enemy and player (player has no top and bottom border so needs + 1)
						Delay100ms(2); // delay 200 ms
						ST7735_DrawBitmap(wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].x, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].y, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].image, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].width, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].height);
						ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
					}
					
					// clear all remaining enemies that are alive
					for(int i = 0; i < 4; i++){
							for(int j = 0; j < 4; j++){
									if(wave[wave_number].enemy[i][j].status != dead){
										 ST7735_FillRect(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y - wave[wave_number].enemy[i][j].height + 1, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height, 0x0000);
										 wave[wave_number].enemy[i][j].status = dead;
									}
							}
					}
						
					// clear player for good
						//redudant code//	ST7735_FillRect(wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].x, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].y - wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].height + 1, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].width, wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].height, 0x0000);
					ST7735_FillRect(player.x, player.y - player.height + 1, player.width, player.height, 0x0000); // clear image of collided enemy and player
					player.status = dead;
						//redundant code// wave[wave_number].enemy[collided_enemy_i][collided_enemy_j].status = dead;
					while(sound_pointer != silence){} //wait until sound effect is done playing before continuing
				}
				if(player.status == dead){ // do explosion animation and set sound pt for bigger explosion, also increment gameover flag right before exiting this if-statement
								//NVIC_ST_CTRL_R = 0x0000;
								//NVIC_ST_RELOAD_R = 5333333; // interrupt at 15 hz (slow-motion)
								//NVIC_ST_CTRL_R = 0x0007; // arm interrupts again but now with player controls disabled and movement speed of all enemies quartered (15 Hz)
					
							//draw first frame of explosion
								ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);					
								sound_pointer = dramaticexplosion; // replace animation with bigger, more dramatic explosion with more frames, and more detail (10 frames?)
								sound_length = 33248; // edit length to match new sound array size
								sound_index = 0;
					
								explosion_counter = 0; //clear counter
								explosion_done = 0;		//timer0 starts incrementing explosion_counter in the animation/spawn_delay function
								explosion_frame++;
					
								while(explosion_done == 0){ // keep running until last frame of explosion is cleared
										if(explosion_done == 0 && explosion_counter >= 4 && explosion_frame < 5){	//only draw next frame if explosion counter is set (375 ms has passed since last frame); if no collosions occur, explosion counter = 0 and explosion frame = 6
												ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);
												explosion_counter = 0;
												explosion_frame++;				//increment frame after drawing previous frame
										}
										if(explosion_frame == 5){ //last explosion frame has just been drawn --> clear explosion and set explosion_done flag for timer to stop incrementing counter
												ST7735_FillRect(explosion_anim[4].x, explosion_anim[4].y - explosion_anim[4].height + 1, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
												explosion_done = 1;
												explosion_counter = 0;
												explosion_frame = 6;  //set to 6 until it is reset to 0 on the next collision
										}
										if(clear_explosion_early == 1){						//clears a previous explosions BMP if another explosion happens to occur before the first is done animating (prevents previous image from staying forever)
												clear_explosion_early = 0; //clear flag
												ST7735_FillRect(x_save, y_save - explosion_anim[4].height + 1, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
										}
								}
								ST7735_FillScreen(0x0000); // clear screen to all black since explosion animation is done
								while(sound_pointer != silence){ //busy-wait until big explosion sound is done playing and animation is over to move on to final game over screen
								}
								gameOver_flag = 2; // set game over flag to 2 once dramatic explosion is done playing
								//gameOver();
									//return 0;				
			}	
} //end of gameOverPlayerDeathAnimation function


//UPDATES THE ENUM VALUES OF UPGRADE TO PREVENT SYSTICK ISR POWERUP+SECONDARY MISSILE COLLISION BUGS
/* updateUpgrade did not fix bug so taking out of code
void updateUpgrade(void){
	if(upgrade == set_led){
		upgrade = led;
	}
	else if(upgrade == set_laser){
		upgrade = laser;
	}
	else if(upgrade == set_waveclear){
		 upgrade = waveclear;
	}
}
*/

// Checks the systick ISR if the m1spawn and m2spawn flags were set to 1, if so, then reset flag, draw missile bmp, reset fireratelimit_counter, and set the missile sprite status to moving
//m1spawn and m2spawn:  flags control basic primary missile
//firerate_limit_counter: global counter that is set to 0 in this function
//NOTE: RESPECTIVE MISSILES[0[ OR MISSILES[1] SPRITE STATUS IS SET TO MOVING IN THIS FUNCTION (if the corresponding spawn flag was set)
//*********************************************************************************************************************************************************************************************
void spawnBasicMissile(void){
	if(m1spawn == 1){
			m1spawn = 0;					//clear flag
			ST7735_DrawBitmap(missiles[0].x, missiles[0].y, missiles[0].image, missiles[0].width, missiles[0].height);
			firerate_limit_counter = 0;		//reset counter once missile has spawned		
			bigMissile_counter = 2; // Must have minimum of 250 ms delay after a basic missile is fired until one can use secondary fire
			laser_counter = 2; // same logic as bigMissile_counter decrementing by 2^^^
			missiles[0].status = moving;
	}
	if(m2spawn == 1){
			m2spawn = 0;					//clear flag
			ST7735_DrawBitmap(missiles[1].x, missiles[1].y, missiles[1].image, missiles[1].width, missiles[1].height);
			firerate_limit_counter = 0;		//reset counter once missile has spawned
			bigMissile_counter = 2; // Must have minimum of 250 ms delay after a basic missile is fired until one can use secondary fire
			laser_counter = 2; // same logic as bigMissile_counter decrementing by 2^^^
			missiles[1].status = moving;		
	}
} // end of spawnBasicMissile() function

void spawnBigMissile(void){
			if(bigMissile_spawn == 1){
					bigMissile_spawn = 0;					//clear flag
					ST7735_DrawBitmap(missile_LED.x, missile_LED.y, missile_LED.image, missile_LED.width, missile_LED.height);
					powerup_ct[bigMissile_Idx]--;	// decrement charge count once the big LED missile is drawn
					firerate_limit_counter = -2;		//reset counter once missile has spawned (at least 500 ms must pass after a secondary missile is fired to fire a primary missile) 	
					bigMissile_counter = 0; // reset to 0 once big LED missile has spawned (now 500 ms must pass before the player can fire the next big LED charge)
					missile_LED.status = moving;
			}	
}// end of spawnBigMissile() function


void spawnLaser(void){					
		 if(laser_spawn == 1){
					laser_spawn = 0;					//clear flag
					ST7735_DrawBitmap(missile_laser.x, missile_laser.y, laserFrame[laserFrame_idx], missile_laser.width, missile_laser.height);
					powerup_ct[laser_Idx]--;	// decrement charge count
					firerate_limit_counter = -2;		//reset counter once missile has spawned (at least 500 ms must pass between a secondary missile fired and the next primary missile) 		 				  
					laser_counter = 0; 		// reset to 0 once laser proj has spawned (now 500 ms must pass before the player can fire the next laser charge)
					missile_laser.status = moving;
					if(laserFrame_dwell == 10){
							laserFrame_idx = (laserFrame_idx + 1) % 3;	// increment frame index
							laserFrame_dwell = 0;	// reset frame dwell counter
					}
		 }			
}// end of spawnLaser() function

//Move player sprite BY 1 PIXEL PER 32 ADC VALUE based off of difference in ADC values (IGNORES CHANGES IN ADC VALUE LESS THAN 32)
//DOES NOT RUN IF SYSTICK HAS NOT RUN AND SET SEMAPHORE FLAG YET (flag  variable set in SYSTICK ISR right after getting new ADC data)
void setAndDrawNewPlayerPos(void){
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
}// end of setAndDrawNewPlayerPos() function


//After each fillrect of explosion frames or enemy fills, make sure to call this to restore player sprite image
void redrawSamePlayerPos(void){
	ST7735_DrawBitmap(player.x, player.y, player.image, player.width, player.height);
} // end of redrawSamePlayerPos func()


void moveAllMissiles(void){
		 moveBasicMissile();
		 moveBigMissile();
	   moveLaser();
}// end of moveAllMissiles() function

// ***** MISSILE MOVEMENT (by 1 pixel based on movemissile global flag that is set every 1/80th sec by Timer2 ISR (missile_move periodic task)) *******************************************
void moveBasicMissile(void){
		 if(movemissileflag == 1){
			  movemissileflag = 0; // clear flag
				for(int i = 0; i < 2; i++)
				{
					if(missiles[i].status == moving){
						missiles[i].y--;
						ST7735_DrawBitmap(missiles[i].x, missiles[i].y, missiles[i].image, missiles[i].width, missiles[i].height);
					}
				}
		 }
}// end of moveBasicMissile() function

void moveBigMissile(void){
			if((missile_LED.status != dead) && (missile_LED.status != dying)){
						if(moveBigMissile_flag == 1){
								moveBigMissile_flag = 0;	//clear flag
								if(missile_LED.status == moving){
									missile_LED.y--;
									ST7735_DrawBitmap(missile_LED.x, missile_LED.y, missile_LED.image, missile_LED.width, missile_LED.height);
								}
						}		
		 }
}// end of moveBigMissile() function


void moveLaser(void){						
			if(missile_laser.status != dead){
					if(moveLaser_flag == 1){
						moveLaser_flag = 0;													//clear flag
						if(missile_laser.status == moving){
								missile_laser.y--;
								ST7735_DrawBitmap(missile_laser.x, missile_laser.y, laserFrame[laserFrame_idx], missile_laser.width, missile_laser.height);
								if(laserFrame_dwell == 10){
									laserFrame_idx = (laserFrame_idx + 1) % 3;	// increment frame index
									laserFrame_dwell = 0;	// reset frame dwell counter
								}else
								{
									laserFrame_dwell++;
								}
						}
					}			
			}
}// end of moveLaser() function

//FillRect over the basic missile and SET MISSILE SPRITE STATUS TO DEAD if corresponding missileCollisionFlag[i] is set in SysTick ISR
//ONLY FILLS MISSILE IF THE MISSILECOLLISIONFLAG[i] IS SET
void fillBasicMissile(void){
		for(int i = 0; i < 2; i++){
				if(missileCollisionFlag[i] == 1){
						missileCollisionFlag[i] = 0;	// acknowledge
						ST7735_FillRect(missiles[i].x, missiles[i].y - missiles[i].height + 1, missiles[i].width, missiles[i].height, 0x0000);
						redrawSamePlayerPos();
						missiles[i].status = dead;	// only set status to dead after they have been drawn over
				}
		}
}// end of fillBasicMissile() function


void fillBigMissile(void){
		 if(bigMissileCollision_flag == 1){
					bigMissileCollision_flag = 0;	// acknowledge
					ST7735_FillRect(missile_LED.x, missile_LED.y - missile_LED.height + 1, missile_LED.width, missile_LED.height, 0x0000);
					redrawSamePlayerPos();
					missile_LED.status = dead;	// only set status to dead after they have been drawn over
		 }
} // end of fillBigMissile() function


//IF COLLISION FLAG ATTRIBUTE SET FOR ENEMY, DRAW OVER ENEMY AND DRAW FIRST FRAME OF EXPLOSION
void fillEnemyDrawExplosionFrame1(void){
			for(int i = 0; i < 4; i++){	//for each row
				for(int j = 0; j < 4; j++){//for each enemy
					//if(wave[wave_number].enemy[i][j].status == dead){ //skip checking enemies that are unspawned or alive
							if(wave[wave_number].enemy[i][j].collision_flag == 1 && gameOver_flag == 0){// don't draw over enemy if enemy has collided with player
									wave[wave_number].enemy[i][j].collision_flag = 0;		// acknowledge
									ST7735_FillRect(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y - wave[wave_number].enemy[i][j].height + 1, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height, 0x0000);
									redrawSamePlayerPos();
									wave[wave_number].enemy[i][j].status = dead; //status set to dead once the enemy is drawn over
										
									//draw first frame of explosion
									ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);					
									redrawSamePlayerPos();
									explosion_counter = 0; //clear counter
									explosion_done = 0;		//timer0 starts incrementing explosion_counter in the animation/spawn_delay function
									explosion_frame++;
							}
					//}checking if status is dead does same thing as checking if collision_flag is 1
				}
			}
}// end of function //


void explosionAnimationUpdate(void){
				if(explosion_done == 0 && explosion_counter >= 1 && explosion_frame < 5){	//only draw next frame if explosion counter is set (125 ms has passed since last frame); if no collosions occur, explosion counter = 0 and explosion frame = 6
						ST7735_DrawBitmap(explosion_anim[explosion_frame].x, explosion_anim[explosion_frame].y, explosion_anim[explosion_frame].image, explosion_anim[explosion_frame].width, explosion_anim[explosion_frame].height);
						redrawSamePlayerPos();
						explosion_counter = 0;
						explosion_frame++;				//increment frame after drawing previous frame
				}
				if(explosion_frame == 5){ //last explosion frame has just been drawn --> clear explosion and set explosion_done flag for timer to stop incrementing counter
						ST7735_FillRect(explosion_anim[4].x, explosion_anim[4].y - explosion_anim[4].height + 1, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
					  redrawSamePlayerPos();
						explosion_done = 1;
						explosion_counter = 0;
						explosion_frame = 6;  //set to 6 until it is reset to 0 on the next collision
						
						if(wave[wave_number].clear == 1){
								wave[wave_number].clear = 2; //set to 2 once last enemy explosion animation is done
						}
				}
				if(clear_explosion_early == 1){						//clears a previous explosions BMP if another explosion happens to occur before the first is done animating (prevents previous image from staying forever)
					 clear_explosion_early = 0; //clear flag
					 ST7735_FillRect(x_save, y_save - explosion_anim[4].height + 1, explosion_anim[4].width, explosion_anim[4].height, 0x0000);
					 redrawSamePlayerPos();
					
					if(wave[wave_number].clear == 1){
								wave[wave_number].clear = 2; //set to 2 once last enemy explosion animation is done
						}
				}
}//end of explosionAnimationUpdate() function


void movePowerup(void){
		 if((powerup[powerupIdx].status == alive) && (powerup[powerupIdx].x + powerup[powerupIdx].width != 0)){// if power-up has reached left side, kill it
				ST7735_DrawBitmap(powerup[powerupIdx].x, powerup[powerupIdx].y, powerup[powerupIdx].image, powerup[powerupIdx].width, powerup[powerupIdx].height);
		 }
		 else if(powerup[powerupIdx].status == dying){
				ST7735_FillRect(powerup[powerupIdx].x, powerup[powerupIdx].y - powerup[powerupIdx].height + 1, powerup[powerupIdx].width, powerup[powerupIdx].height, 0x0000);
				powerup[powerupIdx].status = dead;
		 }
}// end of movePowerup() function



// iterate through 4 rows, if row uncleared, draw bmp of new position for enemies that are still alive
void moveEnemies(void){
			for(int i = 0; i < 4; i++){
					if(wave[wave_number].row_clear[i] == 0){	//only move ith row if the row has not been cleared
						for(int j = 0; j < 4; j++){
							if(wave[wave_number].enemy[i][j].status == alive){// do nothing if enemy is dead/unspawned/dying
								ST7735_DrawBitmap(wave[wave_number].enemy[i][j].x, wave[wave_number].enemy[i][j].y, wave[wave_number].enemy[i][j].image, wave[wave_number].enemy[i][j].width, wave[wave_number].enemy[i][j].height);
							}
						}
					}
			}
}// end of moveEnemies() function



// uses a switch(wave_number) to print messages after each wave is cleared (using ST7735_DrawString)
void printWaveClearedMessages(void){ 
		//CHANGE THIS IF_STATEMENT ONCE WE ADD IN FINAL BOSS
		if(wave_number == 3){
			wave_number++; //goes to case 4 aka printWinScreen
		}
	
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
					printWinScreen();
					//ST7735_DrawString(4,3, msg5pt, 0xFFFF);	//final boss msg
				default:	
					//output you win message and display score here
					break;
		}
}//end of function //


void printWinScreen(void){
		ST7735_DrawString(5,6, winpmsgpt, 0xFFFF);	//you passed.
	  ST7735_DrawString(2, 7, msg_finalscore, 0xFFFF); 
			for(int i = 3; i >= 0; i--){ //convert 4 digit integer score to char array scorept
					scorept[i] = ((total_score % 10) + 0x30);	// integer score value is converted to a character array of 4 digits and a null sentinel
					total_score /= 10;
			}
			scorept[4] = 0; // null terminate the last element in the scorept char array
			ST7735_DrawString(15, 7, scorept, 0xFFFF);
			while(1){} //freeze the game on the game over screen permanantly
}// end of func



// DISABLES INTERRUPTS DURING FUNCTION, PRINT WAVE CLEARED MESSAGES, DELAYS 2 ms, AND DOES ALL VARIABLE CLEARS/INCREMENTS TO PREPARE FOR NEXT WAVE
//CLEARED VARIABLES TO 0: current_row_number, wave_spawn_done, powerupSpawn
//VARIABLES INCREMENTED BY 1: wave_number, powerupIdx
void waveClearedTransition(void){
					DisableInterrupts();	// disable interrupts after the wave dies (no movement needed)
				//** PRINT WAVE CLEARED MESSAGE (SWITCH BASED ON WAVE_NUMBER glob variable) ***********
					printWaveClearedMessages();
				
					
				//** Delay 2 seconds, overwrite text on screen, then clear the wave_spawn_done flag and reset current_row_number to 0 *******
				Delay100ms(20); //POST ON PIAZZA IF WE CAN USE DELAYS IF WE ALREADY HAVE 4 INTERRUPTS (break between waves of enemies)
				ST7735_FillRect(19, 20, 107, 21, 0x0000);  //clear text
				current_row_number = 0;
				wave_spawn_done = 0;
				wave_number++;			//increment wave_number only if whole wave is dead
				powerupSpawn = 0;	// clear powerup spawn flag
				powerupIdx += 1; 		// increment powerup index for next wave
				//for(int i=0; i<4; i++){ // RELOAD FUNCTION ALREADY HAS NESTED FOR LOOPS
					//for(int j=0; j<4; j++){
				reload_movement_cts();			//re-initialize the movement counters for the next wave
					//}
				//}
				EnableInterrupts();
}// end of func


// converts total_score to a character array and draws 4 digit score to top left corner of LCD
void printCurrentScore(void){ 
		total_score_copy = total_score; // don't ever change value of the games total score	
		for(int i = 3; i >= 0; i--){		
				scorept[i] = ((total_score_copy % 10) + 0x30);	// integer score value is converted to a character array of 4 digits and a null sentinel
				total_score_copy /= 10;
		}
		scorept[4] = 0; // null terminate the last element in the array
		ST7735_DrawString(0, 0, scorept, 0x07FF);	//draw the score at the top left corner
}// end of func


// spawns new row every 3 seconds based on counter_125ms until current_row_number == 4
void spawnNextRow(void){
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
		}
} // end of func


 // put ADC data into ADCMAIL using ADC_In() and set semaphore flag to 1
void getADC(void){
		ADCMAIL = ADC_In();		// save the ADC data to a mailbox (global variable)
		flag = 1;							// set the semaphore flag 
}// end of func

// check if enemy has collided with player sprite --> IF SO, SETS GAMEOVERFLAG TO 1
void DetectEnemyPlayerCollision(void){
			for(int k = 0; k < 4; k++){ //for each of 4 rows
				if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
					for(int j = 0; j < 4; j++){
						if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
							// check vertical overlap (top)
							if((((player.y < wave[wave_number].enemy[k][j].y) && (player.y > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height))) ||
							// check vertical overlap (bottom)
							(((player.y - player.height) < wave[wave_number].enemy[k][j].y) && ((player.y - player.height) > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height)))) &&
							// check horizontal overlap (right)
							(((player.x > wave[wave_number].enemy[k][j].x) && (player.x < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))) ||	
							// check horizontal overlap (left)
							((player.x + player.width > wave[wave_number].enemy[k][j].x) && (player.x + player.width < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))))){
								player.status = dying;  		// update enemy sprite status to DYING BUT NOT DEAD
								
								collided_enemy_i = k; // save the row, col information of the sprite that ran into the player
								collided_enemy_j = j;
								gameOver_flag = 1;  // NOW 1 MEANS ENEMY COLLISION WITH PLAYER OCCURED--> GOING TO SET THE FLAG TO 2 AFTER I IMPLEMENT MY IDEA (FREEZE DEATH FRAME, FLASH 3 TIMES)
							
							
								//PLAYER ENEMY EXPLOSION IS HANDLED SEPARATELY IN GAMEOVER_PLAYERDEATHANIMATION FUNCTION?
								explosion_frame = 0;
								explosion_counter = 1;
								if(explosion_done == 0){		//if previous explosion is not done yet once another collosion occurs, set clear_explosion_early flag and save previous coordinates before overwriting
									clear_explosion_early = 1;
									x_save = explosion_anim[4].x;
									y_save = explosion_anim[4].y;
								}
								//CHANGE THESE EXPLOSION_ANIMATIONS TO NEW DRAMATIC EXPLOSION ANIMIATION AND MAKE COORDINATES CENTERED AT PLAYER (IF TIME)
								for(int i = 0; i < 5; i++){
									explosion_anim[i].status = alive;
									explosion_anim[i].x = (player.x - 1); //center explosion at middle of player sprite
									explosion_anim[i].y = (player.y); //explosion will always be at the bottom of the screen
								}	
								
							}
						}
					}
				}
			}	
}// end of func

void DetectEnemyAtBottom(void){
			for(int k = 0; k < 4; k++){ //for each of 4 rows
				if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
					for(int j = 0; j < 4; j++){
						if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
							// Check this enemy's y position with the bottom of LCD screen (y == 159)
								 if(wave[wave_number].enemy[k][j].y == 159){
												//player.status = dying;  		// ONLY SET THIS FOR THE DETECTPLAYERENEMYCOLLISION() FUNCTION
										gameOver_flag = 1;  // NOW 1 MEANS ENEMY COLLISION WITH PLAYER OCCURED--> GOING TO SET THE FLAG TO 2 AFTER I IMPLEMENT MY IDEA (FREEZE DEATH FRAME, FLASH 3 TIMES)
									  collided_enemy_i = k; // save the row information of the sprite that ran into the player
										collided_enemies_j[j_index] = j; // save the col position info of the sprites that ran into player					
										j_index++;
									
										//NO EXPLOSION FOR ENEMY REACHING THE BOTTOM
									 /*
										explosion_frame = 0;
										explosion_counter = 1;
										if(explosion_done == 0){		//if previous explosion is not done yet once another collosion occurs, set clear_explosion_early flag and save previous coordinates before overwriting
											 clear_explosion_early = 1;
											 x_save = explosion_anim[4].x;
											 y_save = explosion_anim[4].y;
										}
										//CHANGE THESE EXPLOSION_ANIMATIONS TO NEW DRAMATIC EXPLOSION ANIMIATION AND MAKE COORDINATES CENTERED AT PLAYER (IF TIME)
										for(int i = 0; i < 5; i++){
												explosion_anim[i].status = alive;
												explosion_anim[i].x = (player.x - 1); //center explosion at middle of player sprite
												explosion_anim[i].y = (player.y); //explosion will always be at the bottom of the screen
										}	
										*/
									
								 }
						}
					}
				}
			}		
			//j_index = 0; // reset to 0 before exiting function (WRONG LOGIC, INSTEAD, THE collided_enemies_j is filled with j_index # of elements
}// end of DetectEnemyAtBottom() function



//Check PE0 primary button press (red)
void DetectPE0(void){
		if(gameOver_flag == 0){ // player can only fire basic missiles if they have not died
			 if(PE0 == 1 && firerate_limit_counter >=2){ //only sets missile spawn flags if at least 250 ms has passed since the last missile was fired
						//SW0 = PE0;				// read data to clear Port E data
						if(missiles[0].status == dead){// case for if 1st missile dead/offscreen
							missiles[0].x = (player.x + player.width/2) - missiles[0].width/2;			// assign starting x pos of missile
							missiles[0].y = (player.y - player.height);															// assign starting y pos of missile	
							m1spawn = 1;
							
								if(sound_pointer != silence){ // missile fire sound can interrupt prev sound if conflicting, resume prev sound after laser sound is done
									sound_pointer_save = sound_pointer;
									sound_pointer = laser5;
									interrupting_laser_sound_flag = 1;
									sound_length_save = sound_length;
									sound_length = 3600;
									sound_index_save = sound_index; // save sound index of prev sound before resetting to 0
									sound_index = 0;						
								}
								else{ // if sound pointer set to silence when missile fire button pressed, just set to laser sound
									sound_pointer = laser5;
									sound_length = 3600;
									sound_index = 0;
								}
						}															
						else if((missiles[0].y <= 134) && (missiles[0].status == moving) && (missiles[1].status == dead)){	// case for if 2nd missile dead/offscreen and at least 5 pixels of separation between successesive missiles						
								missiles[1].x = (player.x + player.width/2) - missiles[1].width/2;			// assign starting x pos of missile
								missiles[1].y = (player.y - player.height);															// assign starting y pos of missile	
								m2spawn = 1;
								
								if(sound_pointer != silence){ // missile fire sound can interrupt explosion sound if conflicting, resume explosion sound after laser sound is done
									sound_pointer_save = sound_pointer;
									sound_pointer = laser5;
									interrupting_laser_sound_flag = 1;
									sound_length_save = sound_length;
									sound_length = 3600;
									sound_index_save = sound_index; // save sound index of prev sound before resetting to 0
									sound_index = 0;
								}
								else{ // if sound pointer set to silence when missile fire button pressed, just set to laser sound
										sound_pointer = laser5;
										sound_length = 3600;
										sound_index = 0;
								}
						}
			 }
		}
}// end of DetectPE0() function



//Check PE1 secondary button press (yellow)
void DetectPE1(void){
		 if(gameOver_flag == 0){ // player can only fire secondary missiles if they have not died
			 SW1 = (PE1 >> 1); //SAVE THE PE1 DATA INTO SW1 GLOB VAR
				if((SW1 == 1) && (upgrade != unequipped)){
					//SW1 = (PE1 >> 1); // read from port e pin 1 to clear the data
					// Fire big missile
						 if((upgrade == led) && (bigMissile_counter >= 4) && (powerup_ct[bigMissile_Idx] > 0)){ // 0.5 ms must pass between each big LED missile fire
								if(missile_LED.status == dead){// case for if LED dead/offscreen
									missile_LED.x = (player.x + player.width/2) - missile_LED.width/2;			// assign starting x pos of missile
									missile_LED.y = (player.y - player.height);															// assign starting y pos of missile	
									bigMissile_spawn = 1;
//BUG FOUND//	powerup_ct[bigMissile_Idx]--;	// decrement charge count
									if(powerup_ct[bigMissile_Idx] == 0){
										upgrade = unequipped;
										//upgrade_bigMissile_flag = 0; // unequip big missile
									}

									//Set to missile launcher bigMissileFire sound (sound takes .658 seconds so no need to save interrupted audio))
									sound_pointer = bigMissileFiringSound;
									sound_length = 7250;
									sound_index = 0;
								}															
						 }
					// Fire laser
						 else if((SW1 == 1) && (upgrade == laser) && (laser_counter >= 4) && (powerup_ct[laser_Idx] > 0)){ // 0.5 ms must pass between each laser fire
								if(missile_laser.status == dead){// case for if LED dead/offscreen
									missile_laser.x = (player.x + player.width/2) - missile_laser.width/2;			// assign starting x pos of missile
									missile_laser.y = (player.y - player.height);															// assign starting y pos of missile	
									laser_spawn = 1;
//BUG FOUND// powerup_ct[laser_Idx]--;	// decrement charge count
									if(powerup_ct[laser_Idx] == 0){
										upgrade = unequipped;
									}
									
									//set current sound effect to laserFiringSound[10225] and no need to continue interrupted sound since this sound effect is rly long (about .927 secs)
									sound_pointer = laserFiringSound;
									sound_length = 10225;
									sound_index = 0;
								}
						 }
					// Fire Waveclear powerup (multiple explosions of nearest row of enemies)
						 else if((SW1 == 1) && (upgrade == waveclear) && (waveClear_counter >= 2) && (powerup_ct[waveClear_Idx] > 0)){// waveclear only has 1 charge (waveclear is only delayed after secondary+ powerup collision by 250 ms, otherwise, counter should not be reset to 0)
//BUG FOUND// powerup_ct[waveClear_Idx]--;	// decrement charge count
											if(powerup_ct[waveClear_Idx] == 0){
												 upgrade = unequipped;
										  }
											
									 // Set current sound effect to waveclearExplosionSound and no need to continue interrupted sound since this sound effect is rly long (about .927 secs)
										  sound_pointer = waveclearExplosionSound; // set this to waveclearExplosionSound
											sound_length = 21615; // change this to the length of ^ array
										  sound_index = 0;
						
									// DEBUG THIS CODE CONTROLLING WAVECLEAR USE --> KILLS CLOSEST ENEMY ROW BY SETTING THEIR STATUS TO DYING AND SETTING THEIR COLLISION FLAG (also sets a waveClearDone_flag = 1)
										 for(int k = 0; k < 4; k++){ //for each of 4 rows
												if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
													for(int j = 0; j < 4; j++){
														if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
																	wave[wave_number].enemy[k][j].status = dying;  		// update enemy sprite status to DYING INSTEAD OF DEAD
																	wave[wave_number].enemy[k][j].collision_flag = 1;	// set collision flag
																	total_score += wave[wave_number].enemy[k][j].score; // add the dying enemy's score value to the total_score global variable
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
																	//ALREADY SET WAVECLEAR TO A CHAIN REACTION EXPLOSION SOUND INSTEAD AT LINE 1623 - 1625:
																		//sound_pointer = explosion2; //set current_sound to explosion only if enemy health becomes 0 (replace w explosion1 if new sound doesn't fit animiation)
																		//sound_length = 6960;				//set sound length (<-- change to 6919 if using explosion with louder volume
																		//sound_index = 0;						//reset index
														}// only check enemies that are alive
													}//iterate for each of 4 enemies
													powerup_ct[waveClear_Idx]--;
													waveClearDone_flag = 1;	// flag for possible use
													waveClear_counter = 2; // counter = 2 enables one of the conditions saying that previous waveclear is done
													break;
												}//only check uncleared rows
										}//iterate for each of 4 rows
						}// else-if statement for WAVECLEAR CASE (RUNS IF UPGRADE == WAVECLEAR AND THE USAGE COUNTER IS GREATER THAN 0)
				}// end of if-statement checking if PE1 button is high and upgrade has a powerup equipped ( !unequipped )
				SW1 = 0; //clear variable before exiting function
			} // end of outermost if-statement if gameOver_flag == 0
}// end of DetectPE1() function





void DetectMissilePowerupCollision(void){
		 // Basic missile check
			  for(int i = 0; i < 2; i++){
					if(missiles[i].status != dead){
						if(powerup[powerupIdx].status == alive){	// skip collision detection if powerup is dead
							// check vertical overlap (top)
							if((((missiles[i].y < powerup[powerupIdx].y) && (missiles[i].y > (powerup[powerupIdx].y - powerup[powerupIdx].height))) ||
							// check vertical overlap (bottom)
							(((missiles[i].y - missiles[i].height) < powerup[powerupIdx].y) && ((missiles[i].y - missiles[i].height) > (powerup[powerupIdx].y - powerup[powerupIdx].height)))) &&
							// check horizontal overlap (right)
							(((missiles[i].x > powerup[powerupIdx].x) && (missiles[i].x < (powerup[powerupIdx].x + powerup[powerupIdx].width))) ||	
							// check horizontal overlap (left)
							((missiles[i].x + missiles[i].width > powerup[powerupIdx].x) && (missiles[i].x + missiles[i].width < (powerup[powerupIdx].x + powerup[powerupIdx].width))))){
								powerup[powerupIdx].status = dying;
								if(powerup[powerupIdx].image == power_LED){
									upgrade = led;	// enable secondary attack (big missile)
									powerup_ct[bigMissile_Idx] = 3;	// set 3 charges
								}else if(powerup[powerupIdx].image == power_laser){
									upgrade = laser;// enable secondary attack (laser)
									powerup_ct[laser_Idx] = 2;	// set 2 charges
								}else if(powerup[powerupIdx].image == power_waveClear){
									upgrade = waveclear;	// enable secondary attack (waveClear)
									powerup_ct[waveClear_Idx] = 1;				// set 1 charge
								}
								missiles[i].status = dying;	// update missile status to DYING NOT DEAD
								missileCollisionFlag[i] = 1;// set collision flag
							}

						}//end of if-statement for collision check on all 4 sides
					}// skip is basic missile is dead
			  }// check both basic missiles
		
		// Big missile check
			  if((missile_LED.status == alive) || (missile_LED.status == moving)){ // run only if big LED missile is not dying or dead
						if(powerup[powerupIdx].status == alive){	// skip collision detection if powerup is already dead
							// check vertical overlap (top)
							if((((missile_LED.y < powerup[powerupIdx].y) && (missile_LED.y > (powerup[powerupIdx].y - powerup[powerupIdx].height))) ||
							// check vertical overlap (bottom)
							(((missile_LED.y - missile_LED.height) < powerup[powerupIdx].y) && ((missile_LED.y - missile_LED.height) > (powerup[powerupIdx].y - powerup[powerupIdx].height)))) &&
							// check horizontal overlap (right)
							(((missile_LED.x > powerup[powerupIdx].x) && (missile_LED.x < (powerup[powerupIdx].x + powerup[powerupIdx].width))) ||	
							// check horizontal overlap (left)
							((missile_LED.x + missile_LED.width > powerup[powerupIdx].x) && (missile_LED.x + missile_LED.width < (powerup[powerupIdx].x + powerup[powerupIdx].width))))){
								powerup[powerupIdx].status = dying;
							  
								if(powerup[powerupIdx].image == power_LED){
									 // enable secondary attack (big missile)
					upgrade = led;
					powerup_ct[bigMissile_Idx] = 3;	// set 3 charges									
					bigMissile_counter = 4; //at least 0 ms must pass after collecting a powerup with a big LED missile until the next secondary weapon can be fired
								}else if(powerup[powerupIdx].image == power_laser){
												 // enable secondary attack (laser)
					upgrade = laser;
					missile_laser.status = unspawned;
					powerup_ct[laser_Idx] = 2; // set 2 charges // HARD CODE TO 3 CHARGES IN CASE OF THE BUG
					laser_counter = 1; //at least 375 ms must pass after collecting a powerup with a big LED missile until the laser secondary weapon can be fired
								}else if(powerup[powerupIdx].image == power_waveClear){
								 // enable secondary attack (waveClear)
					upgrade = waveclear;	
					powerup_ct[waveClear_Idx] = 1; // set 1 charge								
					waveClear_counter = 2; //at least 0 ms must pass after collecting a waveclear powerup until the waveclear can be used
								}
								missile_LED.status = dying;	// update missile status to DYING NOT DEAD
								bigMissileCollision_flag = 1;// set collision flag
							}

						}//end of if-statement for collision check on all 4 sides
			  }// skip is big missile is dead
			
		 // Laser check
			  if((missile_laser.status == alive) || (missile_laser.status == moving)){ // run only if laser missile is not dying or dead
						if(powerup[powerupIdx].status == alive){	// skip collision detection if powerup is dead
							// check vertical overlap (top)
							if((((missile_laser.y < powerup[powerupIdx].y) && (missile_laser.y > (powerup[powerupIdx].y - powerup[powerupIdx].height))) ||
							// check vertical overlap (bottom)
							(((missile_laser.y - missile_laser.height) < powerup[powerupIdx].y) && ((missile_laser.y - missile_laser.height) > (powerup[powerupIdx].y - powerup[powerupIdx].height)))) &&
							// check horizontal overlap (right)
							(((missile_laser.x > powerup[powerupIdx].x) && (missile_laser.x < (powerup[powerupIdx].x + powerup[powerupIdx].width))) ||	
							// check horizontal overlap (left)
							((missile_laser.x + missile_laser.width > powerup[powerupIdx].x) && (missile_laser.x + missile_laser.width < (powerup[powerupIdx].x + powerup[powerupIdx].width))))){
								powerup[powerupIdx].status = dying;
								if(powerup[powerupIdx].image == power_LED){
									 // enable secondary attack (big missile)			
					upgrade = led;
					powerup_ct[bigMissile_Idx] = 3;	// set 3 charges				
					bigMissile_counter = 4; //at least 0 ms must pass after collecting a powerup with a LASER missile until the next charge of big LED weapon can be fired

								}else if(powerup[powerupIdx].image == power_laser){
								         // enable secondary attack (laser)
					upgrade = laser;
					missile_laser.status = unspawned;
					powerup_ct[laser_Idx] = 2; // set 2 charges
					laser_counter = 1; //at least 375 ms must pass after collecting a powerup with a big LED missile until the next charge of laser secondary weapon can be fired
								}else if(powerup[powerupIdx].image == power_waveClear){
											   // enable secondary attack (waveClear)
					upgrade = waveclear;
					powerup_ct[waveClear_Idx] = 1; // set 1 charge
					waveClear_counter = 2; //at least 0 ms must pass after collecting a waveclear powerup until the waveclear can be used
								}
							}
						}//end of if-statement for collision check on all 4 sides
			  }// skip is laser is dead
		
}// end of DetectMissilePowerupCollision() function






void DetectMissileEnemyCollision(void){
	// Basic missile collision detection
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
											wave[wave_number].enemy[k][j].health -= 1;
											if(wave[wave_number].enemy[k][j].health == 0){
												wave[wave_number].enemy[k][j].status = dying;  		// update enemy sprite status to DYING INSTEAD OF DEAD
												wave[wave_number].enemy[k][j].collision_flag = 1;	// set collision flag
												total_score += wave[wave_number].enemy[k][j].score; // add the dying enemy's score value to the total_score global variable
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
											sound_pointer = explosion2; //set current_sound to explosion only if enemy health becomes 0 (replace w explosion1 if new sound doesn't fit animiation)
											sound_length = 6960;				//set sound length (<-- change to 6919 if using explosion with louder volume
											sound_index = 0;						//reset index
												
											}//end of if-statement for health == 0
											else{	// different sound effect if enemy health not reduced to 0
												sound_pointer = enemyoof;	
												sound_length = 3183;
												sound_index = 0;
											}
											missiles[i].status = dying;	// update missile status to DYING NOT DEAD
											missileCollisionFlag[i] = 1;// set collision flag
										}//end of if-statement for collision check on all 4 sides
									}
								}//iterate for each of 4 enemies
							}//only check uncleared rows
						}//iterate for each of 4 rows
				 }
			}//end of basic missile collision detection*****************************************
		
			 //Big LED missile collision detection
			 if(missile_LED.status == moving){
					for(int k = 0; k < 4; k++){ //for each of 4 rows
						if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
							for(int j = 0; j < 4; j++){
								if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
									// check vertical overlap (top)
									if((((missile_LED.y < wave[wave_number].enemy[k][j].y) && (missile_LED.y > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height))) ||
									// check vertical overlap (bottom)
									(((missile_LED.y - missile_LED.height) < wave[wave_number].enemy[k][j].y) && ((missile_LED.y - missile_LED.height) > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height)))) &&
									// check horizontal overlap (right)
									(((missile_LED.x > wave[wave_number].enemy[k][j].x) && (missile_LED.x < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))) ||	
									// check horizontal overlap (left)
									((missile_LED.x + missile_LED.width > wave[wave_number].enemy[k][j].x) && (missile_LED.x + missile_LED.width < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))))){
										wave[wave_number].enemy[k][j].health -= 1;
										if(wave[wave_number].enemy[k][j].health == 0){
											wave[wave_number].enemy[k][j].status = dying;  		// update enemy sprite status to DYING INSTEAD OF DEAD
											wave[wave_number].enemy[k][j].collision_flag = 1;	// set collision flag
											total_score += wave[wave_number].enemy[k][j].score; // add the dying enemy's score value to the total_score global variable
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
										sound_pointer = explosion2; //set current_sound to explosion only if enemy health becomes 0 (replace w explosion1 if new sound doesn't fit animiation)
										sound_length = 6960;				//set sound length (<-- change to 6919 if using explosion with louder volume
										sound_index = 0;						//reset index
											
										}//end of if-statement for health == 0
										else{	// different sound effect if enemy health not reduced to 0
											sound_pointer = enemyoof;	
											sound_length = 3183;
											sound_index = 0;
										}
										missile_LED.status = dying;	// update missile status to DYING NOT DEAD
										bigMissileCollision_flag = 1;// set collision flag		
									}//end of if-statement for collision check on all 4 sides
								}
							}//iterate for each of 4 enemies
						}//only check uncleared rows
					}//iterate for each of 4 rows
			 }//end of big missile collision detection*****************************************
		
			 //Laser collision detection
		if(missile_laser.status == moving){
					for(int k = 0; k < 4; k++){ //for each of 4 rows
						if(wave[wave_number].row_clear[k] == 0){ //only check kth row if clear flag not set
							for(int j = 0; j < 4; j++){
								if(wave[wave_number].enemy[k][j].status == alive){	// skip collision detection if enemy is dead
									// check vertical overlap (top)
									if((((missile_laser.y < wave[wave_number].enemy[k][j].y) && (missile_laser.y > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height))) ||
									// check vertical overlap (bottom)
									(((missile_laser.y - missile_laser.height) < wave[wave_number].enemy[k][j].y) && ((missile_laser.y - missile_laser.height) > (wave[wave_number].enemy[k][j].y - wave[wave_number].enemy[k][j].height)))) &&
									// check horizontal overlap (right)
									(((missile_laser.x > wave[wave_number].enemy[k][j].x) && (missile_laser.x < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))) ||	
									// check horizontal overlap (left)
									((missile_laser.x + missile_laser.width > wave[wave_number].enemy[k][j].x) && (missile_laser.x + missile_laser.width < (wave[wave_number].enemy[k][j].x + wave[wave_number].enemy[k][j].width))))){
										wave[wave_number].enemy[k][j].health -= 1;
										if(wave[wave_number].enemy[k][j].health == 0){
											wave[wave_number].enemy[k][j].status = dying;  		// update enemy sprite status to DYING INSTEAD OF DEAD
											wave[wave_number].enemy[k][j].collision_flag = 1;	// set collision flag
											total_score += wave[wave_number].enemy[k][j].score; // add the dying enemy's score value to the total_score global variable
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
										sound_pointer = explosion2; //set current_sound to explosion only if enemy health becomes 0 (replace w explosion1 if new sound doesn't fit animiation)
										sound_length = 6960;				//set sound length (<-- change to 6919 if using explosion with louder volume
										sound_index = 0;						//reset index
											
										}//end of if-statement for health == 0
										else{	// different sound effect if enemy health not reduced to 0
											sound_pointer = enemyoof;	
											sound_length = 3183;
											sound_index = 0;
										}	
									}//end of if-statement for collision check on all 4 sides
								}
							}//iterate for each of 4 enemies
						}//only check uncleared rows
					}//iterate for each of 4 rows
			 }//end of laser collision detection*****************************************
	
}// end of DetectMissileEnemyCollision() function


void PowerupMovement_PositionUpdate(void){
				//Set powerup status to alive if the powerupSpawn flag is still 0 (reset to 0 again before each new wave)
				if(powerupSpawn == 0){// if powerup has not yet spawned
						powerup[powerupIdx].status = alive;
						powerupSpawn = 1;	// set flag
				}
				
				//Update powerup position if the powerup status is alive (MOVES LEFT ACROSS THE SCREEN NEAR THE PLAYER BEFORE EACH WAVE SPAWNS
				if(powerup[powerupIdx].status == alive){
					if((powerup[powerupIdx].x + powerup[powerupIdx].width) <= 0){// if power-up has reached left side, kill it
							powerup[powerupIdx].status = dead;
					}
					else{
							powerup[powerupIdx].x -=1;	// else, move power-up left
					}
				}	
}// end of PowerupMovement_PositionUpdate() function


void EnemyMovement_PositionUpdate(void){
		 //Enemy wave movement
			if(wave[wave_number].clear == 0){ // only move enemies if wave is not cleared already
				 for(int i = 0; i < 4; i++){ // check each row
						if(wave[wave_number].row_clear[i] == 0 && wave[wave_number].row_spawn[i] == 1){//skip movement decision if ith row is cleared or not spawned yet
								for(int j = 0; j<4; j++){ // check each enemy in the ith row
										if(wave[wave_number].enemy[i][j].status == alive){ // only move enemies that are alive
												if(moveRightInit_ct[i][j] > 0){// row of enemies initially moves 26 pixels to right right from center
																if(wave[wave_number].enemy[i][j].image == enemy_keil){
																	wave[wave_number].enemy[i][j].x += 2; // motor enemies move 2 pixels right(faster speed)
																}
																else
																{
																	wave[wave_number].enemy[i][j].x += 1; // any other enemy moves 1 pixel right
																}
														moveRightInit_ct[i][j]--;
														if(moveRightInit_ct[i][j] == 0){// if max right position reached, set count to move down (on the right side)
																moveDownR_ct[i][j] = 10;
														}
												}	
												else if(moveRight_ct[i][j] > 0){	// move row of enemies right
														moveEnemyRight(wave_number, i, j); // increment enemy's x position to the right depending on its speed attribute
														moveRight_ct[i][j]--;
														if(moveRight_ct[i][j] == 0){		// if right border reached, set count to move down (on the right side)
															moveDownR_ct[i][j] = 10;
														}
												}
												else if(moveDownR_ct[i][j] > 0){	// move row of enemies down (on the right side)
														moveEnemyDown(wave_number, i, j); // decrement enemy's y position by 1 pixel regardless of speed (steppermotor enemy slows down at the walls)
														moveDownR_ct[i][j]--;
														if(moveDownR_ct[i][j] == 0){		// if row has moved down 10 pixels, set count to move left
															moveLeft_ct[i][j] = 52/(wave[wave_number].enemy[i][j].speed); // movement reload counter is dependent on the enemy's speed attribute (for motor: moves 2 pixels at a time, 26 times)
														}
												}	
												else if(moveLeft_ct[i][j] > 0){		// move row of enemies left
														moveEnemyLeft(wave_number, i, j); // decrement enemy's x position to the left depending on its speed attribute
														moveLeft_ct[i][j]--;
														if(moveLeft_ct[i][j] == 0){			// if left border reached, set count to move down (on the left side)
															moveDownL_ct[i][j] = 10;
														}
												}
												else if(moveDownL_ct[i][j] > 0){	// if row has moved down 10 pixels, set count to move right
														moveEnemyDown(wave_number, i, j); // decrement enemy's y position by 1 pixel regardless of speed (steppermotor enemy slows down at the walls)
														moveDownL_ct[i][j]--;
														if(moveDownL_ct[i][j] == 0){		// if row has moved down 10 pixels, set count to move right
															moveRight_ct[i][j] = 52/(wave[wave_number].enemy[i][j].speed);  // movement reload counter is dependent on the enemy's speed attribute (for motor: moves 2 pixels at a time, 26 times)
														}
												}
												
										}//inner if_statement (skip to here is enemy is not alive yet aka either dead or unspawned)
								}//j-loop... iterate for each enemy
						}//if-statement (skip ith row  if row is cleared or not spawned)
				}//outer i-loop... iterate for each row
			}//outermost if-statement...don't do any movement if current wave is already clear
			
}// end of EnemyMovement_PositionUpdate() function


// checks which enemies are dead and updates wave[wave_number].clear and wave[wave_number].row_clear[4] attributes accordingly
void UpdateWaveStructAttr(void){
			if(wave[wave_number].clear == 0){ // no need to update clearing attributes if the current wave is already clear
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
		}
}// end of UpdateWaveStructAttr() function
