#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SFX_CARD_PLAY = 0,
    SFX_CARD_TAKE,
    SFX_CHEAT_SWAP,
    SFX_CAUGHT,
    SFX_ROUND_END,
    SFX_COUNT
} SoundFx;

void InitGameAudio(void);
void CloseGameAudio(void);
void UpdateGameAudio(void);
void ApplyAudioVolumes(float music_vol, float sfx_vol);
void PlayGameSfx(SoundFx fx);

#endif // AUDIO_H
