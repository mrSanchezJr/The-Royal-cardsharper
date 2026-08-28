#ifndef STORY_H
#define STORY_H

#include "game_logic.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LANG_RU = 0,
    LANG_EN = 1,
    LANG_COUNT = 2
} Language;

typedef enum {
    KING_EVENT_START,
    KING_EVENT_PLAYER_ATTACK,
    KING_EVENT_PLAYER_DEFEND,
    KING_EVENT_PLAYER_TAKE,
    KING_EVENT_AI_TAKE,
    KING_EVENT_LOOK_AWAY,
    KING_EVENT_WIN,
    KING_EVENT_LOSS
} KingEvent;

typedef struct {
    const char* text;
    int current_len;
    float timer;
    float char_delay;
    bool is_finished;
} TypewriterText;

// --- Centralized Localization API ---
const char* GetUIText(const char* key, Language lang);
const char* GetSuitLocName(Suit suit, Language lang);
const char* GetRankLocName(Rank rank, Language lang);
const char* GetKingComment(KingEvent event, Language lang);
const char* GetIntroDialogueText(int step, Language lang);
const char* GetTutorialStepText(int step, Language lang);
const char* GetEpilogueText(Language lang);
const char* GetEpilogueStepText(int step, Language lang);
const char* GetCaughtText(Language lang);
const char* GetCreditsText(Language lang);
const char* GetFinalCreditsText(Language lang);
const char* GetAnnouncementLatinText(void);
const char* GetAnnouncementDialogText(Language lang);
const char* GetAnnouncementDialogPart(int part, Language lang);

int GetDistractionCount(void);
const char* GetDistractionText(int index, Language lang);

void GetInvalidDefenseReason(Card attack, Card defend, Suit trump, Language lang, char* outBuffer, size_t bufferSize);
void GetInvalidAttackReason(Card attack, const GameState* game, Language lang, char* outBuffer, size_t bufferSize);

void InitTypewriter(TypewriterText* tw, const char* text, float char_delay);
void UpdateTypewriter(TypewriterText* tw, float dt);
void FinishTypewriter(TypewriterText* tw);

#endif // STORY_H
