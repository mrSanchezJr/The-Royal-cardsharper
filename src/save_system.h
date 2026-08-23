#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <stdbool.h>

typedef struct {
    int matches_won;
    bool tutorial_completed;
    int language;        // 0: RU, 1: EN
    bool first_launch_done;
    int ai_difficulty;   // 0: Easy, 1: Medium, 2: Hard
    int window_resolution; // 0:640x360 1:960x540 2:1280x720 3:1366x768 4:1600x900 5:1920x1080
    int music_volume;    // 0..10
    int sfx_volume;      // 0..10
} SaveData;

// Load save data from file, populates with defaults if not found
void LoadSaveData(SaveData* data);

// Save current data to file
void SaveSaveData(const SaveData* data);

#endif // SAVE_SYSTEM_H
