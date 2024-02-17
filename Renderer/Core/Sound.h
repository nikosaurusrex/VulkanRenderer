#ifndef SOUND_H
#define SOUND_H

#include <miniaudio.h>

struct Sound {
    ma_sound handle;

    Sound() { }

    Sound(const char *path);
    ~Sound();
    
    void Play();
};

void InitSound();
void DeinitSound();

#endif