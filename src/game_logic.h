#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>

#define RANK_COUNT 15

typedef enum {
    SUIT_HEARTS,
    SUIT_DIAMONDS,
    SUIT_CLUBS,
    SUIT_SPADES
} Suit;

typedef enum {
    RANK_6 = 6,
    RANK_7,
    RANK_8,
    RANK_9,
    RANK_10,
    RANK_J,
    RANK_Q,
    RANK_K,
    RANK_A
} Rank;

typedef struct {
    Suit suit;
    Rank rank;
} Card;

typedef struct {
    Card cards[36];
    int count;
} Deck;

typedef struct {
    Card cards[36];
    int count;
} Hand;

typedef Hand CardCollection;

typedef struct {
    Card attacking_card;
    Card defending_card;
    bool has_defender;
} TablePair;

typedef struct {
    Deck deck;
    Hand player_hand;
    Hand ai_hand;
    TablePair table[6];
    int table_count;
    Card trump_card;
    bool is_player_turn;
    bool player_took_cards;
    bool ai_took_cards;
    bool round_over;
} GameState;

void InitGame(GameState* game);
void SetupTutorialGame(GameState* game);
bool PlayerAttack(GameState* game, int card_index);
bool PlayerDefend(GameState* game, int hand_index, int table_index);
void PlayerPass(GameState* game); // Player passes attack or takes defense
bool PlayerCheat(GameState* game); // Sleight of hand (random card swap)
bool PlayerCheatSpecific(GameState* game, int hand_index, Card* out_old_card, Card* out_new_card); // Sleight of hand (specific card swap)
void EndRound(GameState* game);

// Helper functions
bool CanAttack(const GameState* game, Card card);
bool CanDefend(const GameState* game, Card attacking, Card defending);
int CheckWinCondition(const GameState* game); // 0: continue, 1: player win, 2: ai win, 3: draw

#endif // GAME_LOGIC_H
