#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <stdbool.h>

typedef struct {
    int matches_won;
    bool tutorial_completed;
    int language; // 0: RU, 1: EN, 2: KO
    bool first_launch_done;
    int ai_difficulty; // 0: Easy, 1: Medium, 2: Hard
} SaveData;

// Load save data from file, populates with defaults if not found
void LoadSaveData(SaveData* data);

// Save current data to file
void SaveSaveData(const SaveData* data);

#endif // SAVE_SYSTEM_H
