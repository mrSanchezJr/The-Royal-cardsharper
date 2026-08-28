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

typedef enum {
    MUSIC_MENU = 0,
    MUSIC_GAME_CALM,   // level 1 — grey road, calm
    MUSIC_DUSK_ROAD,   // level 2 — between calm and bagpipes
    MUSIC_GAME,        // level 3 — bagpipes, calmer than before
    MUSIC_EPILOGUE,
    MUSIC_COUNT
} MusicTrack;

void InitGameAudio(void);
void CloseGameAudio(void);
void UpdateGameAudio(void);
void ApplyAudioVolumes(float music_vol, float sfx_vol);
void PlayGameSfx(SoundFx fx);
void PlayButtonSfx(void);
void SetMusicTrack(MusicTrack track);

#endif // AUDIO_H
