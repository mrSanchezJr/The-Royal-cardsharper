#include "save_system.h"
#include <stdio.h>

#define SAVE_FILE "save.ini"

void LoadSaveData(SaveData* data) {
    // Defaults
    data->matches_won = 0;
    data->tutorial_completed = false;
    data->language = 0;
    data->first_launch_done = false;
    data->ai_difficulty = 0;
    data->window_resolution = 4; // Default: 1600x900 (HD+)
    data->music_volume = 7;
    data->sfx_volume = 7;

    FILE* file = fopen(SAVE_FILE, "r");
    if (file) {
        int tut = 0;
        int firstDone = 0;
        fscanf(file, "matches_won=%d\n", &data->matches_won);
        fscanf(file, "tutorial_completed=%d\n", &tut);
        fscanf(file, "language=%d\n", &data->language);
        fscanf(file, "first_launch_done=%d\n", &firstDone);
        fscanf(file, "ai_difficulty=%d\n", &data->ai_difficulty);
        fscanf(file, "window_resolution=%d\n", &data->window_resolution);
        fscanf(file, "music_volume=%d\n", &data->music_volume);
        fscanf(file, "sfx_volume=%d\n", &data->sfx_volume);

        data->tutorial_completed = (tut != 0);
        data->first_launch_done = (firstDone != 0);
        if (data->language < 0 || data->language > 1) data->language = 0;
        if (data->ai_difficulty < 0 || data->ai_difficulty > 2) data->ai_difficulty = 0;
        if (data->window_resolution < 0 || data->window_resolution > 5) data->window_resolution = 2;
        if (data->music_volume < 0 || data->music_volume > 10) data->music_volume = 7;
        if (data->sfx_volume < 0 || data->sfx_volume > 10) data->sfx_volume = 7;
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
        fprintf(file, "window_resolution=%d\n", data->window_resolution);
        fprintf(file, "music_volume=%d\n", data->music_volume);
        fprintf(file, "sfx_volume=%d\n", data->sfx_volume);
        fclose(file);
    }
}
