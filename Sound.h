// Sound.h
// Runs on TM4C123 or LM4F120
// Prototypes for basic functions to play sounds from the
// original Space Invaders.
// Jonathan Valvano
// November 17, 2014
#ifndef Sound_h
#define Sound_h
#include <stdint.h>
extern const unsigned char silence[1];
extern const unsigned char laser5[3600];
extern const unsigned char bigMissileFiringSound[7250]; // for LED missile audio (about .658 sec)
extern const unsigned char laserFiringSound[10225]; // for laser secondary attack firing audio (about .927 secs)
extern const unsigned char waveclearExplosionSound[21615]; // for waveclear secondary attack explosion audio (about 1.8 secs)
//extern const unsigned char explosion1[6963]; //the old explosion audio (bad volume)
extern const unsigned char explosion2[6960]; // 0.631 seconds == 631 ms
extern const unsigned char enemyoof[2617]; //.237 secs
extern const unsigned char playerdeathaudio[23748]; // about 2.15 secs
extern const unsigned char powerupequipped[6515];
extern const unsigned char nocharges[7057];
extern const unsigned char gameover[31750]; //about 2.88 secs
extern const unsigned char winmusic[47482];
extern const unsigned char dramaticexplosion[33134]; // about 3 seconds

//void Sound_Init(void);
//void Sound_Play(const uint8_t *pt, uint32_t count);
//void Sound_Shoot(void);
//void Sound_Killed(void);
//void Sound_Explosion(void);

//void Sound_Fastinvader1(void);
//void Sound_Fastinvader2(void);
//void Sound_Fastinvader3(void);
//void Sound_Fastinvader4(void);
void Sound_Highpitch(void);

#endif
