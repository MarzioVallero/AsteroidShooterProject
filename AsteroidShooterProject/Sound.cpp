#include "Sound.h"

Sound::Sound(void)
{
	FMOD::System_Create(&system);// create an instance of the game engine
	system->init(32, FMOD_INIT_NORMAL, 0);// initialize the game engine with 32 channels
}

Sound::~Sound(void)
{
	for (int i = 0; i<NUM_SOUNDS; i++) sounds[i]->release();
		system->release();
}

bool Sound::Load()
{
	system->createSound("Sounds/AlienBeep.wav", FMOD_SOFTWARE, 0, &sounds[ALIENBEEP]);
	system->createSound("Sounds/AlienWrite.wav", FMOD_SOFTWARE, 0, &sounds[ALIENWRITE]);
	system->createSound("Sounds/MOTHERConsole.mp3", FMOD_SOFTWARE | FMOD_LOOP_NORMAL, 0, &sounds[MOTHERCONSOLE]);
	system->createSound("Sounds/Laser1.mp3", FMOD_SOFTWARE, 0, &sounds[LASER1]);
	system->createSound("Sounds/Laser2.mp3", FMOD_HARDWARE, 0, &sounds[LASER2]);
	system->createSound("Sounds/Laser3.mp3", FMOD_HARDWARE, 0, &sounds[LASER3]);
	system->createSound("Sounds/Explosion1.mp3", FMOD_SOFTWARE, 0, &sounds[EXP1]);
	system->createSound("Sounds/Explosion2.mp3", FMOD_HARDWARE, 0, &sounds[EXP2]);
	system->createSound("Sounds/Explosion3.mp3", FMOD_HARDWARE, 0, &sounds[EXP3]);
	system->createSound("Sounds/ImperialMarch.mp3", FMOD_HARDWARE | FMOD_LOOP_NORMAL, 0, &sounds[IMPERIALMARCH]);
	system->createSound("Sounds/IndyTheme.mp3", FMOD_HARDWARE | FMOD_LOOP_NORMAL, 0, &sounds[INDYTHEME]);
	system->createSound("Sounds/BridgeOfKhazadDum.mp3", FMOD_HARDWARE | FMOD_LOOP_NORMAL, 0, &sounds[BRIDGEKHAZADDUM]);
	system->createSound("Sounds/BipolarNightmare.mp3", FMOD_HARDWARE | FMOD_LOOP_NORMAL, 0, &sounds[BIPOLARNIGHTMARE]);
	system->createSound("Sounds/SpecialOrder939.mp3", FMOD_HARDWARE | FMOD_LOOP_NORMAL, 0, &sounds[SPECIALORDER939]);
	return true;
}

void Sound::Play(int sound_id)
{
	switch (sound_id) {
		case MOTHERCONSOLE:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.7f);
			break;
		case ALIENBEEP:
			effectsChannel1->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &effectsChannel1);
			effectsChannel1->setVolume(0.25f);
			break;
		case ALIENWRITE:
			effectsChannel2->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &effectsChannel2);
			effectsChannel1->setVolume(0.25f);
			break;
		case LASER1:
			effectsChannel1->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &effectsChannel1);
			effectsChannel1->setVolume(0.25f);
			break;
		case LASER2:
			effectsChannel2->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &effectsChannel2);
			effectsChannel2->setVolume(0.25f);
			break;
		case LASER3:
			effectsChannel3->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &effectsChannel3);
			effectsChannel3->setVolume(0.25f);
			break;
		case EXP1:
			explosionChannel1->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &explosionChannel1);
			explosionChannel1->setVolume(0.15f);
			break;
		case EXP2:
			explosionChannel2->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &explosionChannel2);
			explosionChannel2->setVolume(0.15f);
			break;
		case EXP3:
			explosionChannel3->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &explosionChannel3);
			explosionChannel3->setVolume(0.15f);
			break;
		case SPECIALORDER939:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.25f);
			break;
		case BRIDGEKHAZADDUM:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.25f);
			break;
		case INDYTHEME:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.25f);
			break;
		case IMPERIALMARCH:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.25f);
			break;
		case BIPOLARNIGHTMARE:
			ambientChannel->stop();
			system->playSound(FMOD_CHANNEL_FREE, sounds[sound_id], false, &ambientChannel);
			ambientChannel->setVolume(0.25f);
			break;
		default:
			break;
	}
}

void Sound::StopAll()
{
	ambientChannel->stop();
	effectsChannel1->stop();
	effectsChannel2->stop();
	effectsChannel3->stop();
	explosionChannel1->stop();
	explosionChannel2->stop();
	explosionChannel3->stop();
}

void Sound::Update()
{
	system->update();
}