#include "save_system.h"
#include <stdio.h>

#define SAVE_FILE "save.ini"

void LoadSaveData(SaveData* data) {
    // Defaults
    data->matches_won = 0;
    data->tutorial_completed = false;
    data->language = 0;
    data->first_launch_done = false;
    data->ai_difficulty = 0; // Default: Easy

    FILE* file = fopen(SAVE_FILE, "r");
    if (file) {
        int tut = 0;
        int firstDone = 0;
        fscanf(file, "matches_won=%d\n", &data->matches_won);
        fscanf(file, "tutorial_completed=%d\n", &tut);
        fscanf(file, "language=%d\n", &data->language);
        fscanf(file, "first_launch_done=%d\n", &firstDone);
        fscanf(file, "ai_difficulty=%d\n", &data->ai_difficulty);
        
        data->tutorial_completed = (tut != 0);
        data->first_launch_done = (firstDone != 0);
        if (data->language < 0 || data->language > 1) data->language = 0;
        if (data->ai_difficulty < 0 || data->ai_difficulty > 2) data->ai_difficulty = 0;
        fclose(file);
    }
}

void SaveSaveData(const SaveData* data) {
    FILE* file = fopen(SAVE_FILE, "w");
    if (file) {
        fprintf(file, "matches_won=%d\n", data->matches_won);
        fprintf(file, "tutorial_completed=%d\n", data->tutorial_completed ? 1 : 0);
        fprintf(file, "language=%d\n", data->language);
        fprintf(file, "first_launch_done=%d\n", data->first_launch_done ? 1 : 0);
        fprintf(file, "ai_difficulty=%d\n", data->ai_difficulty);
        fclose(file);
    }
}
