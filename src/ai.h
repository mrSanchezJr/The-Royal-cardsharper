#ifndef AI_H
#define AI_H

#include "game_logic.h"

typedef struct {
    int match_level;       // 0: Match 1, 1: Match 2, 2: Match 3
    int card_skill;        // 0: Easy, 1: Medium, 2: Hard
    bool is_looking_away;
    float look_away_timer;
    float next_look_away_in;
    int look_away_counter; // Counter for deterministic look-away sequence
    int distraction_index; // Which courtier/servant is distracting the King
} AIState;

void InitAI(AIState* ai, int match_level, int card_skill);
void UpdateAILooking(AIState* ai, float dt);

// AI turn logic parameterized by card skill
void AITakeTurn(GameState* state, int card_skill);

#endif // AI_H
