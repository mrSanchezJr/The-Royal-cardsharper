#include "raylib.h"
#include "game_logic.h"
#include "render.h"
#include "ai.h"
#include "story.h"
#include "save_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef enum {
    STATE_LANG_SELECT,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_INTRO_DIALOGUE,
    STATE_INTRO_QUESTION,
    STATE_TUTORIAL,
    STATE_GAME,
    STATE_CAUGHT,
    STATE_EPILOGUE,
    STATE_CREDITS
} AppState;

static const char* g_king_comment = "";
static float g_king_comment_timer = 0.0f;

static char g_invalid_move_text[512] = {0};
static float g_invalid_move_timer = 0.0f;

static void TriggerKingComment(KingEvent event, Language lang) {
    g_king_comment = GetKingComment(event, lang);
    g_king_comment_timer = 3.5f;
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "The Royal Cardshaper - HD");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Disable default ESC exit
    srand(time(NULL));

    InitRenderFont();

    SaveData saveData;
    LoadSaveData(&saveData);

    AppState appState = saveData.first_launch_done ? STATE_MENU : STATE_LANG_SELECT;
    bool showQuitModal = false;

    int introStep = 0;
    int tutStep = 0;
    
    TypewriterText tw;
    InitTypewriter(&tw, GetIntroDialogueText(0, (Language)saveData.language), 0.03f);

    GameState game;
    InitGame(&game);

    AIState ai;
    InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);

    bool is_selecting_cheat_card = false;
    int selected_cheat_hand_index = -1;
    Vector2 cheat_start_pos = {0};
    Vector2 deck_pos = {145, screenHeight / 2.0f};

    bool is_cheating = false;
    float cheating_progress = 0;
    const float CHEAT_DURATION = 0.8f;
    float end_round_timer = 0;

    Card swap_old_card = {0}, swap_new_card = {0};
    float swap_notice_timer = 0;

    const float cardW = 80, cardH = 118;

    TriggerKingComment(KING_EVENT_START, (Language)saveData.language);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (g_king_comment_timer > 0) {
            g_king_comment_timer -= dt;
        }
        if (swap_notice_timer > 0) {
            swap_notice_timer -= dt;
        }
        if (g_invalid_move_timer > 0) {
            g_invalid_move_timer -= dt;
        }

        Vector2 mousePos = GetMousePosition();

        // Global ESC intercept during game/tutorial
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (showQuitModal) {
                showQuitModal = false;
            } else if (appState == STATE_GAME || appState == STATE_TUTORIAL || appState == STATE_INTRO_DIALOGUE || appState == STATE_INTRO_QUESTION) {
                showQuitModal = true;
            } else if (appState == STATE_CREDITS || appState == STATE_SETTINGS) {
                appState = STATE_MENU;
            }
        }

        // Auto-cancel card selection highlight if King turns back to look at player!
        if (!ai.is_looking_away && is_selecting_cheat_card && appState != STATE_TUTORIAL) {
            is_selecting_cheat_card = false;
        }

        // Update Logic
        if (!showQuitModal) {
            switch (appState) {
                case STATE_LANG_SELECT:
                    break;

                case STATE_MENU:
                    break;

                case STATE_SETTINGS:
                    break;

                case STATE_INTRO_DIALOGUE:
                    UpdateTypewriter(&tw, dt);
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (tw.is_finished) {
                            introStep++;
                            if (introStep <= 2) {
                                InitTypewriter(&tw, GetIntroDialogueText(introStep, (Language)saveData.language), 0.03f);
                            } else {
                                appState = STATE_INTRO_QUESTION;
                            }
                        } else {
                            FinishTypewriter(&tw);
                        }
                    }
                    break;

                case STATE_INTRO_QUESTION:
                    break;

                case STATE_TUTORIAL:
                    UpdateTypewriter(&tw, dt);

                    // Maintain indefinite King look-away during Tutorial Step 6 (Sleight of Hand)
                    if (tutStep == 6) {
                        ai.is_looking_away = true;
                        ai.look_away_timer = 999.0f;
                    } else {
                        UpdateAILooking(&ai, dt);
                    }

                    switch (tutStep) {
                        case 0: // Lesson 1: Goal & Ranks
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                tutStep = 1;
                                InitTypewriter(&tw, GetTutorialStepText(1, (Language)saveData.language), 0.02f);
                            }
                            break;
                        case 1: // Lesson 2: Suits & Trumps
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                tutStep = 2;
                                InitTypewriter(&tw, GetTutorialStepText(2, (Language)saveData.language), 0.02f);
                            }
                            break;
                        case 2: // Lesson 3: Live Attack Practice
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                for (int i = 0; i < game.player_hand.count; i++) {
                                    Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                    if (CheckCollisionPointRec(mousePos, r)) {
                                        if (PlayerAttack(&game, i)) {
                                            // King AI defends in real-time over player's card!
                                            Card attCard = game.table[0].attacking_card;
                                            int defIdx = -1;
                                            for (int a = 0; a < game.ai_hand.count; a++) {
                                                if (CanDefend(&game, attCard, game.ai_hand.cards[a])) {
                                                    defIdx = a;
                                                    break;
                                                }
                                            }
                                            if (defIdx != -1) {
                                                game.table[0].defending_card = game.ai_hand.cards[defIdx];
                                                game.table[0].has_defender = true;
                                                for (int k = defIdx; k < game.ai_hand.count - 1; k++) {
                                                    game.ai_hand.cards[k] = game.ai_hand.cards[k + 1];
                                                }
                                                game.ai_hand.count--;
                                            } else {
                                                if (attCard.suit != game.trump_card.suit) {
                                                    game.table[0].defending_card = (Card){attCard.suit, attCard.rank < RANK_A ? (Rank)(attCard.rank + 1) : RANK_A};
                                                } else {
                                                    game.table[0].defending_card = (Card){game.trump_card.suit, RANK_A};
                                                }
                                                game.table[0].has_defender = true;
                                            }
                                            tutStep = 3;
                                            InitTypewriter(&tw, GetTutorialStepText(3, (Language)saveData.language), 0.02f);
                                        } else {
                                            GetInvalidAttackReason(game.player_hand.cards[i], &game, (Language)saveData.language, g_invalid_move_text, sizeof(g_invalid_move_text));
                                            g_invalid_move_timer = 4.0f;
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        case 3: // Lesson 3.5: King Defended -> Move to Bito cleanly and transition to Defense
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                EndRound(&game);
                                tutStep = 4;
                                InitTypewriter(&tw, GetTutorialStepText(4, (Language)saveData.language), 0.02f);
                                
                                // Set King counter-attack card for Defense Lesson
                                game.is_player_turn = false;
                                game.table_count = 1;
                                game.table[0].attacking_card = (Card){SUIT_SPADES, RANK_7};
                                game.table[0].has_defender = false;

                                // Guarantee player has at least 2 cards to beat Spades 7
                                bool hasWinningDefense = false;
                                for (int p = 0; p < game.player_hand.count; p++) {
                                    if (CanDefend(&game, game.table[0].attacking_card, game.player_hand.cards[p])) {
                                        hasWinningDefense = true;
                                        break;
                                    }
                                }
                                if (!hasWinningDefense) {
                                    if (game.player_hand.count < 2) game.player_hand.count = 2;
                                    game.player_hand.cards[0] = (Card){SUIT_SPADES, RANK_9};
                                    game.player_hand.cards[1] = (Card){SUIT_HEARTS, RANK_8};
                                }
                            }
                            break;
                        case 4: // Lesson 4: Live Defense Practice
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                for (int i = 0; i < game.player_hand.count; i++) {
                                    Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                    if (CheckCollisionPointRec(mousePos, r)) {
                                        if (PlayerDefend(&game, i, 0)) {
                                            tutStep = 5;
                                            InitTypewriter(&tw, GetTutorialStepText(5, (Language)saveData.language), 0.02f);
                                        } else {
                                            GetInvalidDefenseReason(game.table[0].attacking_card, game.player_hand.cards[i], game.trump_card.suit, (Language)saveData.language, g_invalid_move_text, sizeof(g_invalid_move_text));
                                            g_invalid_move_timer = 4.0f;
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        case 5: // Lesson 5: Live Bito vs Taking Practice
                            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                EndRound(&game);
                                tutStep = 6;
                                InitTypewriter(&tw, GetTutorialStepText(6, (Language)saveData.language), 0.02f);
                                ai.is_looking_away = true;
                                ai.look_away_timer = 999.0f;
                            }
                            break;
                        case 6: // Lesson 6: Sleight of Hand (Select & Swap)
                            if (IsKeyPressed(KEY_E)) {
                                is_selecting_cheat_card = true;
                            }
                            if (is_selecting_cheat_card && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                for (int i = 0; i < game.player_hand.count; i++) {
                                    Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                    if (CheckCollisionPointRec(mousePos, r)) {
                                        selected_cheat_hand_index = i;
                                        cheat_start_pos = (Vector2){r.x + cardW/2.0f, r.y + cardH/2.0f};
                                        is_selecting_cheat_card = false;
                                        is_cheating = true;
                                        cheating_progress = 0;
                                        break;
                                    }
                                }
                            }
                            if (is_cheating) {
                                cheating_progress += dt;
                                if (cheating_progress >= CHEAT_DURATION) {
                                    PlayerCheatSpecific(&game, selected_cheat_hand_index, &swap_old_card, &swap_new_card);
                                    swap_notice_timer = 3.5f;
                                    is_cheating = false;
                                    ai.is_looking_away = false;
                                    ai.look_away_timer = 0;
                                    tutStep = 7;
                                    InitTypewriter(&tw, GetTutorialStepText(7, (Language)saveData.language), 0.02f);
                                }
                            }
                            break;
                        case 7: // Tutorial Completed
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                appState = STATE_GAME;
                                saveData.tutorial_completed = true;
                                SaveSaveData(&saveData);
                                InitGame(&game);
                                InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);
                                TriggerKingComment(KING_EVENT_START, (Language)saveData.language);
                            }
                            break;
                    }
                    break;

                case STATE_GAME:
                    UpdateAILooking(&ai, dt);

                    if (IsKeyPressed(KEY_H)) {
                        appState = STATE_TUTORIAL;
                        tutStep = 0;
                        SetupTutorialGame(&game);
                        InitTypewriter(&tw, GetTutorialStepText(0, (Language)saveData.language), 0.01f);
                        FinishTypewriter(&tw);
                    }

                    if (end_round_timer > 0) {
                        end_round_timer -= dt;
                        if (end_round_timer <= 0) {
                            EndRound(&game);
                            int winState = CheckWinCondition(&game);
                            if (winState != 0) {
                                if (winState == 1) { // Player won
                                    saveData.matches_won++;
                                    SaveSaveData(&saveData);
                                    TriggerKingComment(KING_EVENT_WIN, (Language)saveData.language);
                                    if (saveData.matches_won >= 3) {
                                        appState = STATE_EPILOGUE;
                                        InitTypewriter(&tw, GetEpilogueText((Language)saveData.language), 0.03f);
                                    } else {
                                        InitGame(&game);
                                        InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);
                                    }
                                } else { // AI won or draw
                                    saveData.matches_won = 0;
                                    SaveSaveData(&saveData);
                                    TriggerKingComment(KING_EVENT_LOSS, (Language)saveData.language);
                                    InitGame(&game);
                                    InitAI(&ai, 0, saveData.ai_difficulty);
                                }
                            }
                        }
                    } else if (!game.round_over) {
                        // Initiating cheat selection
                        if (IsKeyPressed(KEY_E) && !is_cheating) {
                            if (ai.is_looking_away) {
                                is_selecting_cheat_card = true;
                            } else { // Caught immediately!
                                appState = STATE_CAUGHT;
                                InitTypewriter(&tw, GetCaughtText((Language)saveData.language), 0.03f);
                            }
                        }

                        // Selecting specific card for swap
                        if (is_selecting_cheat_card) {
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                for (int i = 0; i < game.player_hand.count; i++) {
                                    Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                    if (CheckCollisionPointRec(mousePos, r)) {
                                        selected_cheat_hand_index = i;
                                        cheat_start_pos = (Vector2){r.x + cardW/2.0f, r.y + cardH/2.0f};
                                        is_selecting_cheat_card = false;
                                        is_cheating = true;
                                        cheating_progress = 0;
                                        break;
                                    }
                                }
                            }
                        }

                        if (is_cheating) {
                            cheating_progress += dt;
                            if (!ai.is_looking_away) {
                                appState = STATE_CAUGHT;
                                InitTypewriter(&tw, GetCaughtText((Language)saveData.language), 0.03f);
                                is_cheating = false;
                            } else if (cheating_progress >= CHEAT_DURATION) {
                                PlayerCheatSpecific(&game, selected_cheat_hand_index, &swap_old_card, &swap_new_card);
                                swap_notice_timer = 3.5f;
                                is_cheating = false;
                            }
                        } else if (!is_selecting_cheat_card) {
                            if (game.is_player_turn) {
                                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                    float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                    for (int i = 0; i < game.player_hand.count; i++) {
                                        Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                        if (CheckCollisionPointRec(mousePos, r)) {
                                            if (game.table_count == 0 || CanAttack(&game, game.player_hand.cards[i])) {
                                                if (PlayerAttack(&game, i)) {
                                                    TriggerKingComment(KING_EVENT_PLAYER_ATTACK, (Language)saveData.language);
                                                }
                                            } else {
                                                GetInvalidAttackReason(game.player_hand.cards[i], &game, (Language)saveData.language, g_invalid_move_text, sizeof(g_invalid_move_text));
                                                g_invalid_move_timer = 4.0f;
                                            }
                                            break;
                                        }
                                    }
                                }
                                if (IsKeyPressed(KEY_ENTER)) {
                                    PlayerPass(&game);
                                }
                            } else { // Defending
                                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                    float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                    for (int i = 0; i < game.player_hand.count; i++) {
                                        Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                        if (CheckCollisionPointRec(mousePos, r)) {
                                            bool defendedAny = false;
                                            for (int t = 0; t < game.table_count; t++) {
                                                if (!game.table[t].has_defender) {
                                                    if (PlayerDefend(&game, i, t)) {
                                                        TriggerKingComment(KING_EVENT_PLAYER_DEFEND, (Language)saveData.language);
                                                        defendedAny = true;
                                                        break;
                                                    } else {
                                                        GetInvalidDefenseReason(game.table[t].attacking_card, game.player_hand.cards[i], game.trump_card.suit, (Language)saveData.language, g_invalid_move_text, sizeof(g_invalid_move_text));
                                                        g_invalid_move_timer = 4.0f;
                                                    }
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                                if (IsKeyPressed(KEY_ENTER)) {
                                    PlayerPass(&game);
                                    TriggerKingComment(KING_EVENT_PLAYER_TAKE, (Language)saveData.language);
                                }
                            }
                        }
                        
                        if (!game.is_player_turn && !is_cheating && !is_selecting_cheat_card) {
                            static float ai_timer = 0;
                            ai_timer += dt;
                            if (ai_timer > 0.8f) {
                                AITakeTurn(&game, saveData.ai_difficulty);
                                ai_timer = 0;
                            }
                        } else if (game.is_player_turn && !is_cheating && !is_selecting_cheat_card) {
                            static float ai_defend_timer = 0;
                            ai_defend_timer += dt;
                            if (ai_defend_timer > 0.8f) {
                                bool needs_defense = false;
                                for (int i = 0; i < game.table_count; i++) {
                                    if (!game.table[i].has_defender) {
                                        needs_defense = true;
                                        break;
                                    }
                                }
                                if (needs_defense) {
                                    AITakeTurn(&game, saveData.ai_difficulty);
                                }
                                ai_defend_timer = 0;
                            }
                        }
                        
                        if (game.round_over) {
                            if (game.ai_took_cards) {
                                TriggerKingComment(KING_EVENT_AI_TAKE, (Language)saveData.language);
                            }
                            end_round_timer = 1.5f;
                        }
                    }
                    break;

                case STATE_CAUGHT:
                    UpdateTypewriter(&tw, dt);
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (tw.is_finished) {
                            appState = STATE_MENU;
                            saveData.matches_won = 0;
                            SaveSaveData(&saveData);
                            InitGame(&game);
                            InitAI(&ai, 0, saveData.ai_difficulty);
                            is_cheating = false;
                            is_selecting_cheat_card = false;
                        } else {
                            FinishTypewriter(&tw);
                        }
                    }
                    break;

                case STATE_EPILOGUE:
                    UpdateTypewriter(&tw, dt);
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (tw.is_finished) {
                            appState = STATE_MENU;
                        } else {
                            FinishTypewriter(&tw);
                        }
                    }
                    break;

                case STATE_CREDITS:
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        appState = STATE_MENU;
                    }
                    break;
            }
        }

        // Draw Pass
        BeginDrawing();
        ClearBackground(BLACK);

        if (appState == STATE_LANG_SELECT) {
            DrawTableTexture(screenWidth, screenHeight);

            // Header
            const char* title = GetUIText("SELECT_LANG_TITLE", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 26);
            DrawRectangle(screenWidth/2.0f - 350, 70, 700, 100, (Color){20, 20, 20, 235});
            DrawRectangleLines(screenWidth/2.0f - 350, 70, 700, 100, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, 85, 24, GOLD);

            float btnW = 340, btnH = 60;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 240;

            // 1. Русский
            Rectangle rRU = {btnX, startY, btnW, btnH};
            bool hRU = CheckCollisionPointRec(mousePos, rRU);
            if (DrawMenuButton(rRU, GetUIText("LANG_NAME_RU", LANG_RU), hRU, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                saveData.language = 0;
                saveData.first_launch_done = true;
                SaveSaveData(&saveData);
                appState = STATE_MENU;
            }
            startY += 90;

            // 2. English
            Rectangle rEN = {btnX, startY, btnW, btnH};
            bool hEN = CheckCollisionPointRec(mousePos, rEN);
            if (DrawMenuButton(rEN, GetUIText("LANG_NAME_EN", LANG_EN), hEN, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                saveData.language = 1;
                saveData.first_launch_done = true;
                SaveSaveData(&saveData);
                appState = STATE_MENU;
            }
        }
        else if (appState == STATE_MENU) {
            DrawTableTexture(screenWidth, screenHeight);

            // Title Header
            const char* title = GetUIText("MENU_TITLE", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 38);
            DrawRectangle(screenWidth/2.0f - titleSz.x/2.0f - 24, 35, titleSz.x + 48, 60, (Color){20, 20, 20, 235});
            DrawRectangleLines(screenWidth/2.0f - titleSz.x/2.0f - 24, 35, titleSz.x + 48, 60, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, 48, 38, GOLD);

            // Menu Buttons
            float btnW = 360, btnH = 54;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 135;

            // 1. Continue
            if (saveData.matches_won > 0) {
                Rectangle rCont = {btnX, startY, btnW, btnH};
                bool hCont = CheckCollisionPointRec(mousePos, rCont);
                if (DrawMenuButton(rCont, GetUIText("CONTINUE", (Language)saveData.language), hCont, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                    appState = STATE_GAME;
                }
                startY += 66;
            }

            // 2. New Game
            Rectangle rNew = {btnX, startY, btnW, btnH};
            bool hNew = CheckCollisionPointRec(mousePos, rNew);
            if (DrawMenuButton(rNew, GetUIText("NEW_GAME", (Language)saveData.language), hNew, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                saveData.matches_won = 0;
                SaveSaveData(&saveData);
                InitGame(&game);
                InitAI(&ai, 0, saveData.ai_difficulty);
                appState = STATE_INTRO_DIALOGUE;
                introStep = 0;
                InitTypewriter(&tw, GetIntroDialogueText(0, (Language)saveData.language), 0.03f);
            }
            startY += 66;

            // 3. Tutorial
            Rectangle rTut = {btnX, startY, btnW, btnH};
            bool hTut = CheckCollisionPointRec(mousePos, rTut);
            if (DrawMenuButton(rTut, GetUIText("TUTORIAL", (Language)saveData.language), hTut, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                appState = STATE_TUTORIAL;
                tutStep = 0;
                SetupTutorialGame(&game);
                InitTypewriter(&tw, GetTutorialStepText(0, (Language)saveData.language), 0.02f);
            }
            startY += 66;

            // 4. Settings (Доп. настройки)
            Rectangle rSettings = {btnX, startY, btnW, btnH};
            bool hSettings = CheckCollisionPointRec(mousePos, rSettings);
            if (DrawMenuButton(rSettings, GetUIText("SETTINGS", (Language)saveData.language), hSettings, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                appState = STATE_SETTINGS;
            }
            startY += 66;

            // 5. Credits
            Rectangle rCred = {btnX, startY, btnW, btnH};
            bool hCred = CheckCollisionPointRec(mousePos, rCred);
            if (DrawMenuButton(rCred, GetUIText("CREDITS", (Language)saveData.language), hCred, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                appState = STATE_CREDITS;
            }
            startY += 66;

            // 6. Exit Game
            Rectangle rExit = {btnX, startY, btnW, btnH};
            bool hExit = CheckCollisionPointRec(mousePos, rExit);
            if (DrawMenuButton(rExit, GetUIText("EXIT_GAME", (Language)saveData.language), hExit, (Color){80, 30, 30, 230}, (Color){140, 50, 50, 250})) {
                UnloadRenderFont();
                CloseWindow();
                return 0;
            }
        }
        else if (appState == STATE_SETTINGS) {
            DrawTableTexture(screenWidth, screenHeight);

            // Title Header
            const char* title = GetUIText("SETTINGS", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 38);
            DrawRectangle(screenWidth/2.0f - titleSz.x/2.0f - 24, 60, titleSz.x + 48, 60, (Color){20, 20, 20, 230});
            DrawRectangleLines(screenWidth/2.0f - titleSz.x/2.0f - 24, 60, titleSz.x + 48, 60, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, 72, 38, GOLD);

            float btnW = 400, btnH = 58;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 180;

            // 1. AI Difficulty Toggle (0: Easy, 1: Medium, 2: Hard)
            const char* diffKey = "AI_DIFF_EASY";
            if (saveData.ai_difficulty == 1) diffKey = "AI_DIFF_MEDIUM";
            else if (saveData.ai_difficulty == 2) diffKey = "AI_DIFF_HARD";

            Rectangle rDiff = {btnX, startY, btnW, btnH};
            bool hDiff = CheckCollisionPointRec(mousePos, rDiff);
            if (DrawMenuButton(rDiff, GetUIText(diffKey, (Language)saveData.language), hDiff, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                saveData.ai_difficulty = (saveData.ai_difficulty + 1) % 3;
                SaveSaveData(&saveData);
            }
            startY += 78;

            // 2. Language Toggle
            Rectangle rLang = {btnX, startY, btnW, btnH};
            bool hLang = CheckCollisionPointRec(mousePos, rLang);
            if (DrawMenuButton(rLang, GetUIText("LANGUAGE_TOGGLE", (Language)saveData.language), hLang, (Color){40, 40, 40, 230}, (Color){70, 70, 70, 250})) {
                saveData.language = (saveData.language + 1) % 2;
                SaveSaveData(&saveData);
            }
            startY += 78;

            // 3. Back to Main Menu
            Rectangle rBack = {btnX, startY, btnW, btnH};
            bool hBack = CheckCollisionPointRec(mousePos, rBack);
            if (DrawMenuButton(rBack, GetUIText("BACK", (Language)saveData.language), hBack, (Color){70, 40, 40, 230}, (Color){110, 60, 60, 250})) {
                appState = STATE_MENU;
            }
        }
        else if (appState == STATE_CREDITS) {
            DrawTableTexture(screenWidth, screenHeight);
            
            Rectangle box = {screenWidth/2.0f - 300, 90, 600, 520};
            DrawRectangleRounded(box, 0.1f, 10, (Color){25, 25, 25, 245});
            DrawRectangleRoundedLines(box, 0.1f, 10, GOLD);

            DrawAppText(GetUIText("CREDITS", (Language)saveData.language), screenWidth/2.0f - 60, 120, 32, GOLD);

            const char* credInfo = GetCreditsText((Language)saveData.language);
            DrawAppText(credInfo, screenWidth/2.0f - 240, 190, 22, WHITE);
        }
        else if (appState == STATE_INTRO_QUESTION) {
            DrawTableTexture(screenWidth, screenHeight);

            Rectangle kingRect = {screenWidth / 2.0f - 50, 40, 100, 100};
            DrawKing(kingRect, false, false);

            Rectangle box = {100, 220, screenWidth - 200, 380};
            DrawRectangleRounded(box, 0.1f, 10, (Color){25, 25, 25, 245});
            DrawRectangleRoundedLines(box, 0.1f, 10, GOLD);

            const char* qText = GetUIText("KNOW_DURAK_Q", (Language)saveData.language);
            Vector2 qSz = MeasureAppText(qText, 28);
            DrawAppText(qText, screenWidth/2.0f - qSz.x/2.0f, 260, 28, GOLD);

            // Choice 1: Yes, let's play
            Rectangle rOpt1 = {screenWidth/2.0f - 260, 360, 520, 60};
            bool h1 = CheckCollisionPointRec(mousePos, rOpt1);
            if (DrawMenuButton(rOpt1, GetUIText("OPT_YES", (Language)saveData.language), h1, (Color){40, 80, 40, 230}, (Color){60, 130, 60, 250})) {
                appState = STATE_GAME;
                TriggerKingComment(KING_EVENT_START, (Language)saveData.language);
            }

            // Choice 2: No, teach me (Tutorial)
            Rectangle rOpt2 = {screenWidth/2.0f - 260, 450, 520, 60};
            bool h2 = CheckCollisionPointRec(mousePos, rOpt2);
            if (DrawMenuButton(rOpt2, GetUIText("OPT_NO", (Language)saveData.language), h2, (Color){80, 40, 40, 230}, (Color){130, 60, 60, 250})) {
                appState = STATE_TUTORIAL;
                tutStep = 0;
                SetupTutorialGame(&game);
                InitTypewriter(&tw, GetTutorialStepText(0, (Language)saveData.language), 0.02f);
            }
        }
        else if (appState == STATE_GAME || appState == STATE_CAUGHT || appState == STATE_TUTORIAL) {
            DrawTableTexture(screenWidth, screenHeight);

            // Draw King
            Rectangle kingRect = {screenWidth / 2.0f - 50, 25, 100, 100};
            DrawKing(kingRect, ai.is_looking_away, appState == STATE_CAUGHT);

            // Draw Speech Bubble above King
            DrawSpeechBubble((Vector2){screenWidth / 2.0f + 60, 65}, g_king_comment, g_king_comment_timer);

            // Draw cheating prompt
            if (ai.is_looking_away && !is_cheating && !is_selecting_cheat_card && (appState == STATE_GAME || tutStep == 6)) {
                DrawAppText(GetUIText("CHEAT_PROMPT", (Language)saveData.language), screenWidth / 2.0f + 70, 135, 22, YELLOW);
            }

            // Card selection prompt banner
            if (is_selecting_cheat_card) {
                const char* selectTxt = GetUIText("SELECT_CARD_TO_SWAP", (Language)saveData.language);
                Vector2 sz = MeasureAppText(selectTxt, 24);
                DrawRectangle(screenWidth/2.0f - sz.x/2.0f - 16, 135, sz.x + 32, 42, (Color){0, 0, 0, 230});
                DrawRectangleLines(screenWidth/2.0f - sz.x/2.0f - 16, 135, sz.x + 32, 42, GOLD);
                DrawAppText(selectTxt, screenWidth/2.0f - sz.x/2.0f, 143, 24, GOLD);
            }

            // Draw Cheating progress & Arm Reaching
            if (is_cheating) {
                DrawCheatingProgress((Vector2){screenWidth / 2.0f - 70, screenHeight - 185}, cheating_progress / CHEAT_DURATION);
                DrawReachingArm(cheat_start_pos, deck_pos, cheating_progress / CHEAT_DURATION);
            }

            // Draw AI Hand (Card backs)
            float aiStartX = screenWidth / 2.0f - (game.ai_hand.count * 55) / 2.0f;
            for (int i = 0; i < game.ai_hand.count; i++) {
                Rectangle r = {aiStartX + i * 55, 135, cardW, cardH};
                DrawCard((Card){0,0}, r, false);
            }

            // Draw Deck & Full Trump Card Sprite on Left Panel
            if (game.deck.count > 0) {
                const char* suitName = GetSuitLocName(game.trump_card.suit, (Language)saveData.language);

                char trumpLabel[48];
                snprintf(trumpLabel, sizeof(trumpLabel), GetUIText("TRUMP_LABEL_FMT", (Language)saveData.language), suitName);

                DrawAppText(trumpLabel, 40, screenHeight / 2.0f - 105, 24, GOLD);

                // Full Rendered Trump Card Sprite
                Rectangle trumpR = {40, screenHeight / 2.0f - 55, cardW, cardH};
                DrawCard(game.trump_card, trumpR, true);

                if (appState == STATE_TUTORIAL && tutStep == 1) {
                    DrawCardHighlight(trumpR, GOLD);
                }

                // Deck Card (Stacked over/next to trump card)
                Rectangle deckR = {125, screenHeight / 2.0f - 55, cardW, cardH};
                DrawCard((Card){0,0}, deckR, false);
                deck_pos = (Vector2){deckR.x + cardW/2.0f, deckR.y + cardH/2.0f};

                char countTxt[48];
                snprintf(countTxt, sizeof(countTxt), GetUIText("DECK_LABEL_FMT", (Language)saveData.language), game.deck.count);
                
                DrawAppText(countTxt, 40, screenHeight / 2.0f + 75, 22, WHITE);
            }

            // Draw Swap Banner Notification with Mini-Cards!
            DrawSwapNotification((Vector2){screenWidth/2.0f, screenHeight/2.0f - 140}, swap_old_card, swap_new_card, swap_notice_timer, (Language)saveData.language);

            // Draw Invalid Move Rejection Detailed Hint Box
            if (g_invalid_move_timer > 0) {
                Vector2 sz = MeasureAppText(g_invalid_move_text, 20);
                float boxW = sz.x + 40 > 680 ? sz.x + 40 : 680;
                float boxH = 100;
                Rectangle box = {screenWidth/2.0f - boxW/2.0f, 135, boxW, boxH};
                DrawRectangleRounded(box, 0.12f, 10, (Color){35, 10, 10, 245});
                DrawRectangleRoundedLines(box, 0.12f, 10, RED);
                DrawAppText(g_invalid_move_text, box.x + 20, box.y + 16, 20, WHITE);
            }

            // Draw Table Cards (Centered at y: 340)
            float tableStartX = screenWidth / 2.0f - (game.table_count * 100) / 2.0f;
            for (int i = 0; i < game.table_count; i++) {
                Rectangle rAtt = {tableStartX + i * 100, screenHeight / 2.0f - 30, cardW, cardH};
                DrawCard(game.table[i].attacking_card, rAtt, true);
                if (game.table[i].has_defender) {
                    Rectangle rDef = {tableStartX + i * 100 + 16, screenHeight / 2.0f + 5, cardW, cardH};
                    DrawCard(game.table[i].defending_card, rDef, true);
                }
            }

            // Draw Round End Banner (Бито / Забрал)
            if (end_round_timer > 0) {
                const char* statusTxt = GetUIText("STATUS_BITO", (Language)saveData.language);
                Color statusCol = GOLD;
                if (game.player_took_cards) {
                    statusTxt = GetUIText("STATUS_PLAYER_TOOK", (Language)saveData.language);
                    statusCol = RED;
                } else if (game.ai_took_cards) {
                    statusTxt = GetUIText("STATUS_AI_TOOK", (Language)saveData.language);
                    statusCol = GREEN;
                }
                Vector2 sz = MeasureAppText(statusTxt, 34);
                DrawRectangle(screenWidth/2.0f - sz.x/2.0f - 20, screenHeight/2.0f - 125, sz.x + 40, 50, (Color){0, 0, 0, 220});
                DrawRectangleLines(screenWidth/2.0f - sz.x/2.0f - 20, screenHeight/2.0f - 125, sz.x + 40, 50, statusCol);
                DrawAppText(statusTxt, screenWidth/2.0f - sz.x/2.0f, screenHeight/2.0f - 117, 34, statusCol);
            }

            // Draw Player Hand Cards & Dynamic Tutorial Highlights & Level 1 Tactical Hints
            float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
            for (int i = 0; i < game.player_hand.count; i++) {
                Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                DrawCard(game.player_hand.cards[i], r, true);

                if (is_selecting_cheat_card) {
                    DrawCardHighlight(r, GOLD);
                }

                if (appState == STATE_TUTORIAL) {
                    if (tutStep == 2) { // Attack Lesson: Highlight valid attack cards ONLY
                        if (game.table_count == 0 || CanAttack(&game, game.player_hand.cards[i])) {
                            DrawCardHighlight(r, GREEN);
                        }
                    } else if (tutStep == 4) { // Defense Lesson: Highlight valid defense cards ONLY
                        if (game.table_count > 0 && !game.table[0].has_defender) {
                            if (CanDefend(&game, game.table[0].attacking_card, game.player_hand.cards[i])) {
                                DrawCardHighlight(r, YELLOW);
                            }
                        }
                    }
                }
            }

            // Level 1 Tactical Hints above Player Cards
            if (appState == STATE_GAME && saveData.matches_won == 0 && end_round_timer <= 0 && !is_selecting_cheat_card && !is_cheating) {
                if (game.is_player_turn) {
                    if (game.table_count == 0) {
                        int minNonTrumpRank = 999;
                        int bestIdx = -1;
                        for (int k = 0; k < game.player_hand.count; k++) {
                            if (game.player_hand.cards[k].suit != game.trump_card.suit) {
                                if (game.player_hand.cards[k].rank < minNonTrumpRank) {
                                    minNonTrumpRank = game.player_hand.cards[k].rank;
                                    bestIdx = k;
                                }
                            }
                        }
                        for (int i = 0; i < game.player_hand.count; i++) {
                            Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                            Vector2 topCenter = {r.x + cardW / 2.0f, r.y};
                            if (i == bestIdx) {
                                DrawCardHintBadge(topCenter, GetUIText("HINT_BEST_MOVE", (Language)saveData.language), GREEN);
                            } else if (game.player_hand.cards[i].suit == game.trump_card.suit) {
                                DrawCardHintBadge(topCenter, GetUIText("HINT_TRUMP", (Language)saveData.language), GOLD);
                            }
                        }
                    } else {
                        int tossCount = 0;
                        for (int i = 0; i < game.player_hand.count; i++) {
                            if (CanAttack(&game, game.player_hand.cards[i])) {
                                Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                Vector2 topCenter = {r.x + cardW / 2.0f, r.y};
                                DrawCardHintBadge(topCenter, GetUIText("HINT_TOSS", (Language)saveData.language), GREEN);
                                tossCount++;
                            }
                        }
                        if (tossCount == 0) {
                            const char* noTossTxt = GetUIText("HINT_NO_TOSS_ENTER", (Language)saveData.language);
                            Vector2 sz = MeasureAppText(noTossTxt, 22);
                            float boxW = sz.x + 36, boxH = 42;
                            Rectangle box = {screenWidth / 2.0f - boxW / 2.0f, screenHeight - 215, boxW, boxH};
                            DrawRectangleRounded(box, 0.25f, 6, (Color){20, 20, 20, 240});
                            DrawRectangleRoundedLines(box, 0.25f, 6, GOLD);
                            DrawAppText(noTossTxt, box.x + 18, box.y + 10, 22, GOLD);
                        }
                    }
                } else {
                    int undefTableIdx = -1;
                    for (int t = 0; t < game.table_count; t++) {
                        if (!game.table[t].has_defender) {
                            undefTableIdx = t;
                            break;
                        }
                    }
                    if (undefTableIdx != -1) {
                        Card att = game.table[undefTableIdx].attacking_card;
                        int defCount = 0;
                        for (int i = 0; i < game.player_hand.count; i++) {
                            if (CanDefend(&game, att, game.player_hand.cards[i])) {
                                Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                Vector2 topCenter = {r.x + cardW / 2.0f, r.y};
                                if (game.player_hand.cards[i].suit == game.trump_card.suit && att.suit != game.trump_card.suit) {
                                    DrawCardHintBadge(topCenter, GetUIText("HINT_TRUMP_DEFEND", (Language)saveData.language), GOLD);
                                } else {
                                    DrawCardHintBadge(topCenter, GetUIText("HINT_CAN_DEFEND", (Language)saveData.language), GREEN);
                                }
                                defCount++;
                            }
                        }
                        if (defCount == 0) {
                            const char* noDefTxt = GetUIText("HINT_NO_DEFENSE_ENTER", (Language)saveData.language);
                            Vector2 sz = MeasureAppText(noDefTxt, 22);
                            float boxW = sz.x + 36, boxH = 42;
                            Rectangle box = {screenWidth / 2.0f - boxW / 2.0f, screenHeight - 215, boxW, boxH};
                            DrawRectangleRounded(box, 0.25f, 6, (Color){35, 10, 10, 240});
                            DrawRectangleRoundedLines(box, 0.25f, 6, RED);
                            DrawAppText(noDefTxt, box.x + 18, box.y + 10, 22, (Color){255, 200, 200, 255});
                        }
                    }
                }
            }
            
            // Draw Turn info & Help hint
            if (appState == STATE_GAME && end_round_timer <= 0) {
                if (game.is_player_turn) {
                    DrawAppText(GetUIText("YOUR_TURN", (Language)saveData.language), 20, screenHeight - 35, 24, GREEN);
                } else {
                    DrawAppText(GetUIText("DEFEND_TURN", (Language)saveData.language), 20, screenHeight - 35, 24, RED);
                }
                char diffTxt[64];
                snprintf(diffTxt, sizeof(diffTxt), GetUIText("MATCH_INFO_FMT", (Language)saveData.language), saveData.matches_won + 1);
                
                DrawAppText(diffTxt, screenWidth - 300, 15, 22, WHITE);
            }
        }

        // Draw Non-Overlapping Dialogue / Tutorial Box
        if (appState == STATE_INTRO_DIALOGUE || appState == STATE_EPILOGUE || appState == STATE_CAUGHT || (appState == STATE_TUTORIAL && !is_selecting_cheat_card && g_invalid_move_timer <= 0)) {
            if (appState == STATE_CAUGHT) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){255, 0, 0, 110});
            }
            
            float boxW = 880, boxH = 150;
            Rectangle box = {screenWidth/2.0f - boxW/2.0f, 135, boxW, boxH};
            DrawRectangleRounded(box, 0.1f, 10, (Color){25, 25, 25, 245});
            DrawRectangleRoundedLines(box, 0.1f, 10, GOLD);
            
            char buffer[2048] = {0};
            strncpy(buffer, tw.text, tw.current_len);
            DrawAppText(buffer, box.x + 25, box.y + 20, 22, WHITE);
        }

        // ESC Quit Confirmation Modal Popup
        if (showQuitModal) {
            int choice = DrawConfirmationModal(
                GetUIText("QUIT_CONFIRM_TITLE", (Language)saveData.language),
                GetUIText("YES", (Language)saveData.language),
                GetUIText("NO", (Language)saveData.language),
                screenWidth, screenHeight, mousePos
            );
            if (choice == 1) { // YES -> Return to Main Menu
                appState = STATE_MENU;
                showQuitModal = false;
            } else if (choice == 2) { // NO -> Resume
                showQuitModal = false;
            }
        }

        EndDrawing();
    }

    UnloadRenderFont();
    CloseWindow();
    return 0;
}
