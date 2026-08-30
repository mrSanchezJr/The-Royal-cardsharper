#include "game_logic.h"
#include <stdlib.h>
#include <time.h>

static void ShuffleDeck(Deck* deck) {
    for (int i = deck->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
}

static void DealCards(GameState* game) {
    // Classic: attacker replenishes first
    if (game->is_player_turn) {
        while (game->player_hand.count < 6 && game->deck.count > 0) {
            game->player_hand.cards[game->player_hand.count++] = game->deck.cards[--game->deck.count];
        }
        while (game->ai_hand.count < 6 && game->deck.count > 0) {
            game->ai_hand.cards[game->ai_hand.count++] = game->deck.cards[--game->deck.count];
        }
    } else {
        while (game->ai_hand.count < 6 && game->deck.count > 0) {
            game->ai_hand.cards[game->ai_hand.count++] = game->deck.cards[--game->deck.count];
        }
        while (game->player_hand.count < 6 && game->deck.count > 0) {
            game->player_hand.cards[game->player_hand.count++] = game->deck.cards[--game->deck.count];
        }
    }
}

void InitGame(GameState* game) {
    game->deck.count = 0;
    for (int s = SUIT_HEARTS; s <= SUIT_SPADES; s++) {
        for (int r = RANK_6; r <= RANK_A; r++) {
            game->deck.cards[game->deck.count].suit = (Suit)s;
            game->deck.cards[game->deck.count].rank = (Rank)r;
            game->deck.count++;
        }
    }
    
    ShuffleDeck(&game->deck);
    
    game->player_hand.count = 0;
    game->ai_hand.count = 0;
    game->table_count = 0;
    
    game->trump_card = game->deck.cards[0];
    
    // Initial deal: player first (as before), then determine first attacker by lowest trump
    game->is_player_turn = true;
    DealCards(game);
    
    // Classic rule: lowest trump attacks first
    int pTrump = 99, aTrump = 99;
    for (int i = 0; i < game->player_hand.count; i++) if (game->player_hand.cards[i].suit == game->trump_card.suit && game->player_hand.cards[i].rank < pTrump) pTrump = game->player_hand.cards[i].rank;
    for (int i = 0; i < game->ai_hand.count; i++) if (game->ai_hand.cards[i].suit == game->trump_card.suit && game->ai_hand.cards[i].rank < aTrump) aTrump = game->ai_hand.cards[i].rank;
    if (pTrump != 99 && aTrump != 99) game->is_player_turn = (pTrump < aTrump);
    else if (pTrump != 99) game->is_player_turn = true;
    else if (aTrump != 99) game->is_player_turn = false;
    else game->is_player_turn = true; // no trumps - fallback

    game->player_took_cards = false;
    game->ai_took_cards = false;
    game->round_over = false;
}

void SetupTutorialGame(GameState* game) {
    // Fixed Trump Card: Hearts
    game->trump_card = (Card){SUIT_HEARTS, RANK_6};

    // Clear table and states
    game->table_count = 0;
    game->is_player_turn = true;
    game->player_took_cards = false;
    game->ai_took_cards = false;
    game->round_over = false;

    // Player Hand:
    // 0: Clubs 6 (attack with)
    // 1: Spades 9 (can defend vs Spades 7)
    // 2: Diamonds 8
    // 3: Hearts 8 (Trump! can defend vs Spades 7)
    // 4: Spades 10 (can defend vs Spades 7)
    // 5: Hearts A (Ace of Trumps! can defend vs Spades 7)
    game->player_hand.count = 6;
    game->player_hand.cards[0] = (Card){SUIT_CLUBS, RANK_6};
    game->player_hand.cards[1] = (Card){SUIT_SPADES, RANK_9};
    game->player_hand.cards[2] = (Card){SUIT_DIAMONDS, RANK_8};
    game->player_hand.cards[3] = (Card){SUIT_HEARTS, RANK_8};
    game->player_hand.cards[4] = (Card){SUIT_SPADES, RANK_10};
    game->player_hand.cards[5] = (Card){SUIT_HEARTS, RANK_A};

    // King Hand:
    game->ai_hand.count = 6;
    game->ai_hand.cards[0] = (Card){SUIT_CLUBS, RANK_8};
    game->ai_hand.cards[1] = (Card){SUIT_CLUBS, RANK_10};
    game->ai_hand.cards[2] = (Card){SUIT_DIAMONDS, RANK_9};
    game->ai_hand.cards[3] = (Card){SUIT_SPADES, RANK_7};
    game->ai_hand.cards[4] = (Card){SUIT_DIAMONDS, RANK_A};
    game->ai_hand.cards[5] = (Card){SUIT_HEARTS, RANK_K};

    // Deck with top card ready for Sleight of Hand
    game->deck.count = 24;
    for (int i = 0; i < 24; i++) {
        game->deck.cards[i] = (Card){SUIT_DIAMONDS, RANK_7};
    }
    game->deck.cards[23] = (Card){SUIT_HEARTS, RANK_Q}; // Swaps into Queen of Trumps!
    game->deck.cards[0] = game->trump_card;
}

bool CanAttack(const GameState* game, Card card) {
    if (game->table_count == 0) return true;
    for (int i = 0; i < game->table_count; i++) {
        if (game->table[i].attacking_card.rank == card.rank) return true;
        if (game->table[i].has_defender && game->table[i].defending_card.rank == card.rank) return true;
    }
    return false;
}

bool CanDefend(const GameState* game, Card attacking, Card defending) {
    Suit trump_suit = game->trump_card.suit;
    if (defending.suit == attacking.suit) {
        return defending.rank > attacking.rank;
    }
    if (defending.suit == trump_suit && attacking.suit != trump_suit) {
        return true;
    }
    return false;
}

bool PlayerAttack(GameState* game, int card_index) {
    if (card_index < 0 || card_index >= game->player_hand.count) return false;
    if (game->table_count >= 6) return false;
    // Classic "Durak" rule: attacker cannot put more cards on table than
    // the defender had at the START of this round. Their original count =
    // current hand + cards already played in defense this round.
    int defended_this_round = 0;
    for (int i = 0; i < game->table_count; i++) {
        if (game->table[i].has_defender) defended_this_round++;
    }
    int defender_original = game->ai_hand.count + defended_this_round;
    if (game->table_count >= defender_original) return false;
    
    Card card = game->player_hand.cards[card_index];
    if (CanAttack(game, card)) {
        game->table[game->table_count].attacking_card = card;
        game->table[game->table_count].has_defender = false;
        game->table_count++;
        
        for (int i = card_index; i < game->player_hand.count - 1; i++) {
            game->player_hand.cards[i] = game->player_hand.cards[i+1];
        }
        game->player_hand.count--;
        return true;
    }
    return false;
}

bool PlayerDefend(GameState* game, int hand_index, int table_index) {
    if (hand_index < 0 || hand_index >= game->player_hand.count) return false;
    if (table_index < 0 || table_index >= game->table_count) return false;
    if (game->table[table_index].has_defender) return false;
    
    Card defending = game->player_hand.cards[hand_index];
    Card attacking = game->table[table_index].attacking_card;
    
    if (CanDefend(game, attacking, defending)) {
        game->table[table_index].defending_card = defending;
        game->table[table_index].has_defender = true;
        
        for (int i = hand_index; i < game->player_hand.count - 1; i++) {
            game->player_hand.cards[i] = game->player_hand.cards[i+1];
        }
        game->player_hand.count--;
        return true;
    }
    return false;
}

void PlayerPass(GameState* game) {
    if (game->is_player_turn) {
        game->round_over = true;
    } else {
        game->player_took_cards = true;
        game->round_over = true;
    }
}

bool PlayerCheat(GameState* game) {
    if (game->player_hand.count == 0 || game->deck.count == 0) return false;
    int idx = rand() % game->player_hand.count;
    return PlayerCheatSpecific(game, idx, NULL, NULL);
}

bool PlayerCheatSpecific(GameState* game, int hand_index, Card* out_old_card, Card* out_new_card) {
    if (hand_index < 0 || hand_index >= game->player_hand.count) return false;

    // Deck's bottom card (index 0) is the trump card — it must never be stolen
    if (game->deck.count <= 1) return false;

    Card oldCard = game->player_hand.cards[hand_index];

    // Take a RANDOM card from the deck (excluding the trump at index 0);
    // our old card is hidden in its place
    int r = 1 + rand() % (game->deck.count - 1);
    Card newCard = game->deck.cards[r];

    game->player_hand.cards[hand_index] = newCard;
    game->deck.cards[r] = oldCard;

    if (out_old_card) *out_old_card = oldCard;
    if (out_new_card) *out_new_card = newCard;

    return true;
}

// Deterministic variant used by the tutorial: always swaps with the TOP deck
// card (the tutorial stacks a specific lesson card there).
bool PlayerCheatTop(GameState* game, int hand_index, Card* out_old_card, Card* out_new_card) {
    if (hand_index < 0 || hand_index >= game->player_hand.count || game->deck.count <= 1) return false;

    Card oldCard = game->player_hand.cards[hand_index];
    Card newCard = game->deck.cards[game->deck.count - 1];

    game->player_hand.cards[hand_index] = newCard;
    game->deck.cards[game->deck.count - 1] = oldCard;

    if (out_old_card) *out_old_card = oldCard;
    if (out_new_card) *out_new_card = newCard;

    return true;
}

void EndRound(GameState* game) {
    if (game->player_took_cards) {
        for (int i = 0; i < game->table_count; i++) {
            game->player_hand.cards[game->player_hand.count++] = game->table[i].attacking_card;
            if (game->table[i].has_defender) {
                game->player_hand.cards[game->player_hand.count++] = game->table[i].defending_card;
            }
        }
        game->is_player_turn = false;
    } else if (game->ai_took_cards) {
        for (int i = 0; i < game->table_count; i++) {
            game->ai_hand.cards[game->ai_hand.count++] = game->table[i].attacking_card;
            if (game->table[i].has_defender) {
                game->ai_hand.cards[game->ai_hand.count++] = game->table[i].defending_card;
            }
        }
        game->is_player_turn = true;
    } else {
        game->is_player_turn = !game->is_player_turn;
    }
    
    game->table_count = 0;
    game->player_took_cards = false;
    game->ai_took_cards = false;
    game->round_over = false;
    
    DealCards(game);
}

int CheckWinCondition(const GameState* game) {
    if (game->deck.count == 0) {
        if (game->player_hand.count == 0 && game->ai_hand.count == 0) {
            // Both empty after BИТО – win for who went out first (the attacker)
            // After EndRound with BИТО, is_player_turn is next attacker, so last attacker is opposite
            return !game->is_player_turn ? 1 : 2;
        }
        if (game->player_hand.count == 0) return 1; // Player win
        if (game->ai_hand.count == 0) return 2; // AI win
    }
    return 0;
}
