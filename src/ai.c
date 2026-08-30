#include "ai.h"
#include "story.h"
#include <stdlib.h>

static void RemoveCardFromHand(CardCollection* hand, int index) {
    if (index < 0 || index >= hand->count) return;
    for (int i = index; i < hand->count - 1; i++) {
        hand->cards[i] = hand->cards[i + 1];
    }
    hand->count--;
}

void InitAI(AIState* ai, int match_level, int card_skill) {
    ai->match_level = match_level;
    ai->card_skill = card_skill;
    ai->is_looking_away = false;
    ai->look_away_timer = 0;
    ai->next_look_away_in = 3.5f + (rand() % 25) / 10.0f; 
    ai->look_away_counter = 0;
    ai->distraction_index = 0;
}

void UpdateAILooking(AIState* ai, float dt) {
    if (ai->is_looking_away) {
        ai->look_away_timer -= dt;
        if (ai->look_away_timer <= 0) {
            ai->is_looking_away = false;
            ai->next_look_away_in = 3.5f + (rand() % 20) / 10.0f; 
        }
    } else {
        ai->next_look_away_in -= dt;
        if (ai->next_look_away_in <= 0) {
            ai->is_looking_away = true;
            ai->distraction_index = rand() % GetDistractionCount();
            
            if (ai->match_level == 0) {
                // Level 1: ALWAYS 3.5 seconds
                ai->look_away_timer = 3.5f;
            } else if (ai->match_level == 1) {
                // Level 2: Alternates 1.0s and 3.5s strictly
                if (ai->look_away_counter % 2 == 0) {
                    ai->look_away_timer = 1.0f;
                } else {
                    ai->look_away_timer = 3.5f;
                }
            } else {
                // Level 3: 2 times 1.0s, 1 time 3.5s (every 3rd is 3.5s)
                if (ai->look_away_counter % 3 == 2) {
                    ai->look_away_timer = 3.5f;
                } else {
                    ai->look_away_timer = 1.0f;
                }
            }
            ai->look_away_counter++;
        }
    }
}

void AITakeTurn(GameState* state, int card_skill) {
    if (state->round_over) return;

    if (!state->is_player_turn) {
        // AI is attacking
        // If there are undefended cards on table, AI MUST WAIT for player to defend!
        for (int i = 0; i < state->table_count; i++) {
            if (!state->table[i].has_defender) {
                return;
            }
        }

        // On Easy skill: toss at most 1 extra card with 50% chance, only while deck has cards
        if (card_skill == 0) {
            if (state->table_count >= 1 && state->deck.count == 0) {
                state->round_over = true;
                state->player_took_cards = false;
                state->ai_took_cards = false;
                return;
            }
            if (state->table_count >= 2) {
                state->round_over = true;
                state->player_took_cards = false;
                state->ai_took_cards = false;
                return;
            }
            if (state->table_count == 1) {
                if (rand() % 2 == 0) {
                    // 50% — proceed to toss logic below
                } else {
                    state->round_over = true;
                    state->player_took_cards = false;
                    state->ai_took_cards = false;
                    return;
                }
            }
        }

        // On Medium skill: toss at most 2 cards per turn
        if (card_skill == 1 && state->table_count >= 2) {
            state->round_over = true;
            state->player_took_cards = false;
            state->ai_took_cards = false;
            return;
        }

        // Classic "Durak" rule: cannot put more attacking cards on table
        // than the defender had at the START of this round.
        // Original defender count = current hand + cards already played in defense.
        {
            int defended_this_round = 0;
            for (int i = 0; i < state->table_count; i++) {
                if (state->table[i].has_defender) defended_this_round++;
            }
            int defender_original = state->player_hand.count + defended_this_round;
            if (state->table_count >= defender_original) {
                state->round_over = true;
                state->player_took_cards = false;
                state->ai_took_cards = false;
                return;
            }
        }

        bool attacked = false;
        // Try to find a valid attack card matching table ranks (prefer non-trump)
        for (int r = 0; r < RANK_COUNT && !attacked; r++) {
            for (int i = 0; i < state->ai_hand.count; i++) {
                Card c = state->ai_hand.cards[i];
                if (c.rank == r && CanAttack(state, c) && c.suit != state->trump_card.suit) {
                    state->table[state->table_count].attacking_card = c;
                    state->table[state->table_count].has_defender = false;
                    state->table_count++;
                    
                    RemoveCardFromHand(&state->ai_hand, i);
                    attacked = true;
                    break;
                }
            }
        }
        
        // If leading table (table_count == 0) and couldn't attack without trump, lead with lowest trump
        if (!attacked && state->table_count == 0) {
            for (int r = 0; r < RANK_COUNT && !attacked; r++) {
                for (int i = 0; i < state->ai_hand.count; i++) {
                    Card c = state->ai_hand.cards[i];
                    if (c.rank == r && CanAttack(state, c)) {
                        state->table[state->table_count].attacking_card = c;
                        state->table[state->table_count].has_defender = false;
                        state->table_count++;
                        
                        RemoveCardFromHand(&state->ai_hand, i);
                        attacked = true;
                        break;
                    }
                }
            }
        }

        if (!attacked && state->table_count > 0) {
            // Can't attack anymore and all cards on table are defended -> AI passes (Бито!)
            state->round_over = true;
            state->player_took_cards = false;
            state->ai_took_cards = false;
        }
    } else {
        // AI is defending
        for (int i = 0; i < state->table_count; i++) {
            if (!state->table[i].has_defender) {
                Card attack = state->table[i].attacking_card;
                
                bool defended = false;
                // Try lowest non-trump defense
                for (int r = 0; r < RANK_COUNT && !defended; r++) {
                    for (int j = 0; j < state->ai_hand.count; j++) {
                        Card c = state->ai_hand.cards[j];
                        if (c.rank == r && CanDefend(state, attack, c) && c.suit != state->trump_card.suit) {
                            state->table[i].defending_card = c;
                            state->table[i].has_defender = true;
                            RemoveCardFromHand(&state->ai_hand, j);
                            defended = true;
                            break;
                        }
                    }
                }
                
                // Try trump defense
                if (!defended) {
                    for (int r = 0; r < RANK_COUNT && !defended; r++) {
                        for (int j = 0; j < state->ai_hand.count; j++) {
                            Card c = state->ai_hand.cards[j];
                            if (c.rank == r && CanDefend(state, attack, c)) {
                                state->table[i].defending_card = c;
                                state->table[i].has_defender = true;
                                RemoveCardFromHand(&state->ai_hand, j);
                                defended = true;
                                break;
                            }
                        }
                    }
                }
                
                if (!defended) {
                    // AI cannot defend this card -> AI takes all cards!
                    state->round_over = true;
                    state->ai_took_cards = true;
                    break;
                }
            }
        }
    }
}
