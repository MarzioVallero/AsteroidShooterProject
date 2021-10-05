#pragma once

#pragma comment(lib, "fmodex_vc.lib" ) // fmod library

#include "fmod.hpp" //fmod c++ header
#include <stdio.h>
#include <stdlib.h>

//Sound array size
#define NUM_SOUNDS 14

//Sound identifiers
enum {
	ALIENBEEP,
	ALIENWRITE,
	MOTHERCONSOLE,
	LASER1,
	LASER2,
	LASER3,
	EXP1,
	EXP2,
	EXP3,
	BRIDGEKHAZADDUM,
	IMPERIALMARCH,
	INDYTHEME,
	BIPOLARNIGHTMARE,
	SPECIALORDER939
};

class Sound
{
public:
	Sound(void);
	virtual ~Sound(void);

	bool Load();
	void Play(int sound_id);
	void StopAll();
	void Update();

	FMOD::System*     system; //handle to FMOD engine
	FMOD::Sound*      sounds[NUM_SOUNDS]; //sound that will be loaded and played
	FMOD::Channel*    ambientChannel;
	FMOD::Channel*    effectsChannel1;
	FMOD::Channel*	  effectsChannel2;
	FMOD::Channel*    effectsChannel3;
	FMOD::Channel*	  explosionChannel1;
	FMOD::Channel*    explosionChannel2;
	FMOD::Channel*    explosionChannel3;
	FMOD::DSP*        dspSmoothStop;
};