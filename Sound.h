// Sound.h
// Runs on TM4C123 or LM4F120
// Prototypes for basic functions to play sounds from the
// original Space Invaders.
// Jonathan Valvano
// November 17, 2014

#include <stdint.h>
extern const unsigned char silence[1];
extern const unsigned char laser5[3600];
extern const unsigned char explosion1[6964];
extern const unsigned char enemyoof[2770];
extern const unsigned char playerdeathaudio[28154]; // about 2.5 secs
extern const unsigned char dramaticexplosion[59108]; // about 5.35 seconds 

void Sound_Init(void);
void Sound_Play(const uint8_t *pt, uint32_t count);
void Sound_Shoot(void);
void Sound_Killed(void);
void Sound_Explosion(void);

void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
void Sound_Highpitch(void);

