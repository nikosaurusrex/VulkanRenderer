#define MINIAUDIO_IMPLEMENTATION
#include "Sound.h"

#include "../Common.h"

static ma_engine engine;

Sound::Sound(const char *path) {
    ma_result result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_NO_SPATIALIZATION, 0, 0, &handle);

    if (result != MA_SUCCESS) {
        LogFatal("Failed to load sound file: %s", path);
    }    
}

Sound::~Sound() {
	// TODO: This seems to cause a crash
    // ma_sound_uninit(&handle); 
}

void Sound::Play() {
    ma_sound_start(&handle);
}

void InitSound() {
    ma_result result = ma_engine_init(0, &engine);

    if (result != MA_SUCCESS) {
        LogFatal("Failed to initialize audio engine");
    }
    
    LogInfo("Audio engine initialized");
}

void DeinitSound() {
    ma_engine_uninit(&engine);
}
