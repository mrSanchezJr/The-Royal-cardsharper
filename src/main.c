#include "raylib.h"
#include "game_logic.h"
#include "render.h"
#include "ai.h"
#include "story.h"
#include "save_system.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

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

// True if the King has at least one card in hand able to beat `c` right now
static bool KingCanBeatCard(const GameState* game, Card c) {
    for (int i = 0; i < game->ai_hand.count; i++) {
        if (CanDefend(game, c, game->ai_hand.cards[i])) return true;
    }
    return false;
}

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    float sway;
} Heart;
typedef struct { bool active; float t; Vector2 from, to; Card card; int tableIdx; bool isDef; } CardFlight;

int main(void) {
    // Virtual (logical) canvas size — all game coordinates are in this space (HD+ for quality)
    const int screenWidth  = 1600;
    const int screenHeight = 900;

    // Resolution presets (index matches save_data.window_resolution)
    const int RES_W[6] = { 640,  960, 1280, 1366, 1600, 1920 };
    const int RES_H[6] = { 360,  540,  720,  768,  900, 1080 };
    const int RES_COUNT = 6;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "The Royal Cardshaper (beta)");
    SetWindowMinSize(640, 360);
    SetWindowMaxSize(1920, 1080);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Disable default ESC exit
    srand(time(NULL));

    // Virtual canvas: game renders at 1600x900 (HD+) into this texture, then scales to window
    RenderTexture2D canvas = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    InitRenderFont();
    InitKingSprites();
    InitWandererSprites();
    InitPrincessSprite();
    InitFinalSprite();
    InitTableSprite();

    KingState g_king_state = KING_STATE_IDLE;

    SaveData saveData;
    LoadSaveData(&saveData);

    InitGameAudio();
    ApplyAudioVolumes(saveData.music_volume / 10.0f, saveData.sfx_volume / 10.0f);

    // Apply saved window resolution
    SetWindowSize(RES_W[saveData.window_resolution], RES_H[saveData.window_resolution]);

    AppState appState = saveData.first_launch_done ? STATE_MENU : STATE_LANG_SELECT;
    bool showQuitModal = false;

    int introStep = -4;
    int tutStep = 0;
    int epiStep = 0;
    float epiRevealTimer = 0;
    float epiFade = 0;
    float creditsOffset = 0;
    float creditsSpeed = 42.0f; // adjustable: pixels per second for credits scroll
    float epilogueEndFade = 0; // fade to black after credits leave screen
    float introFade = 0; // announcement -> dialogue transition
    int introFadeDir = 0; // 1 = to black, -1 = from black
        Vector2 heartEmitA = { screenWidth * 0.26f - 200, screenHeight * 0.88f - 500}; // <-- editable left emitter (image)
Vector2 heartEmitB = { screenWidth * 0.74f - 700, screenHeight * 0.88f - 500}; // <-- editable right emitter (credits)
    #define MAX_HEARTS 44
    Heart hearts[MAX_HEARTS];
    for (int i = 0; i < MAX_HEARTS; i++) hearts[i].life = -1;
    
    TypewriterText tw;
    InitTypewriter(&tw, GetAnnouncementDialogPart(0, (Language)saveData.language), 0.03f);

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
    float post_match_timer = 0;

    Card swap_old_card = {0}, swap_new_card = {0};
    float swap_notice_timer = 0;

    const float cardW = 80, cardH = 118;
    CardFlight cardFlight = {0};
    Vector2 prevMousePos = {0};

    TriggerKingComment(KING_EVENT_START, (Language)saveData.language);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateGameAudio();
        UpdateUiBlock(dt);
        if (cardFlight.active) {
            cardFlight.t += dt * 4.6f;
            if (cardFlight.t >= 1.0f) { cardFlight.t = 1.0f; cardFlight.active = false; }
        }
        UpdateVictoryParticles(dt);
        UpdateCursorParticles(dt);
        {
            MusicTrack want = MUSIC_MENU;
            if (appState == STATE_TUTORIAL) want = MUSIC_GAME_CALM;
            else if (appState == STATE_GAME || appState == STATE_CAUGHT) {
                if (ai.match_level == 1 || saveData.matches_won == 1) want = MUSIC_DUSK_ROAD;
                else if (ai.match_level >= 2 || saveData.matches_won >= 2) want = MUSIC_GAME;
                else want = MUSIC_GAME_CALM;
            } else if (appState == STATE_EPILOGUE) want = MUSIC_EPILOGUE;
            SetMusicTrack(want);
        }

        if (g_king_comment_timer > 0) {
            g_king_comment_timer -= dt;
        }
        if (swap_notice_timer > 0) {
            swap_notice_timer -= dt;
        }
        if (g_invalid_move_timer > 0) {
            g_invalid_move_timer -= dt;
        }

        // F11: toggle fullscreen
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
                SetWindowSize(screenWidth, screenHeight);
            } else {
                int monitor = GetCurrentMonitor();
                SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
                ToggleFullscreen();
            }
        }

        // Convert real mouse position to virtual 1280x720 canvas coordinates (letterbox-aware)
        int realW = GetScreenWidth();
        int realH = GetScreenHeight();
        float scale = (realW / (float)screenWidth < realH / (float)screenHeight)
                      ? realW / (float)screenWidth
                      : realH / (float)screenHeight;
        float offsetX = (realW - screenWidth  * scale) * 0.5f;
        float offsetY = (realH - screenHeight * scale) * 0.5f;
        Vector2 rawMouse = GetMousePosition();
        Vector2 mousePos = {
            (rawMouse.x - offsetX) / scale,
            (rawMouse.y - offsetY) / scale
        };
        {
            float dx = mousePos.x - prevMousePos.x;
            float dy = mousePos.y - prevMousePos.y;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > 2.8f) {
                SpawnCursorDust(mousePos);
                if (dist > 14) SpawnCursorDust((Vector2){(mousePos.x+prevMousePos.x)*0.5f, (mousePos.y+prevMousePos.y)*0.5f});
            }
            prevMousePos = mousePos;
        }

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
                    if (introFadeDir != 0) {
                        introFade += dt / 0.55f * introFadeDir;
                        if (introFadeDir == 1 && introFade >= 1.0f) {
                            introFade = 1.0f;
                            introStep = 0;
                            InitTypewriter(&tw, GetIntroDialogueText(0, (Language)saveData.language), 0.03f);
                            introFadeDir = -1;
                        } else if (introFadeDir == -1 && introFade <= 0.0f) {
                            introFade = 0.0f;
                            introFadeDir = 0;
                        }
                        break;
                    }
                    UpdateTypewriter(&tw, dt);
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (tw.is_finished) {
                            if (introStep < 0) {
                                if (introStep == -1) {
                                    introFadeDir = 1;
                                    introFade = 0.0f;
                                    BlockUiClicks(0.35f);
                                } else {
                                    introStep++;
                                    InitTypewriter(&tw, GetAnnouncementDialogPart(introStep + 4, (Language)saveData.language), 0.03f);
                                    BlockUiClicks(0.30f);
                                }
                            } else if (introStep <= 3) {
                                introStep++;
                                if (introStep <= 2) {
                                    InitTypewriter(&tw, GetIntroDialogueText(introStep, (Language)saveData.language), 0.03f);
                                    BlockUiClicks(0.30f);
                                } else {
                                    appState = STATE_INTRO_QUESTION;
                                    BlockUiClicks(0.35f);
                                }
                            } else {
                                appState = STATE_INTRO_QUESTION;
                                BlockUiClicks(0.35f);
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
                        ai.distraction_index = GetDistractionCount() - 1; // Musician fits the lesson
                    } else {
                        UpdateAILooking(&ai, dt);
                    }

                    switch (tutStep) {
                        case 0: // Lesson 1: Goal & Ranks
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                tutStep = 1;
                                InitTypewriter(&tw, GetTutorialStepText(1, (Language)saveData.language), 0.02f);
                                BlockUiClicks(0.35f);
                            }
                            break;
                        case 1: // Lesson 2: Suits & Trumps
                            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                tutStep = 2;
                                InitTypewriter(&tw, GetTutorialStepText(2, (Language)saveData.language), 0.02f);
                                BlockUiClicks(0.35f);
                            }
                            break;
                        case 2: // Lesson 3: Live Attack Practice
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
                                for (int i = 0; i < game.player_hand.count; i++) {
                                    Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                                    if (CheckCollisionPointRec(mousePos, r)) {
                                        if (PlayerAttack(&game, i)) {
                                            PlayGameSfx(SFX_CARD_PLAY);
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
                                            {
                                                Vector2 from = {r.x + cardW/2, r.y + cardH/2};
                                                float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + (game.table_count-1)*100 + cardW/2;
                                                Vector2 to = {tX, screenHeight/2.0f + 95 + cardH/2};
                                                cardFlight = (CardFlight){true, 0, from, to, game.table[game.table_count-1].attacking_card, game.table_count-1, false};
                                            }
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
                                BlockUiClicks(0.35f);
                                
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
                                            PlayGameSfx(SFX_CARD_PLAY);
                                            tutStep = 5;
                                            InitTypewriter(&tw, GetTutorialStepText(5, (Language)saveData.language), 0.02f);
                                            {
                                                Vector2 from = {r.x + cardW/2, r.y + cardH/2};
                                                float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + 0*100 + 16 + cardW/2;
                                                Vector2 to = {tX, screenHeight/2.0f + 125 + cardH/2};
                                                cardFlight = (CardFlight){true, 0, from, to, game.table[0].defending_card, 0, true};
                                            }
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
                                BlockUiClicks(0.35f);
                                ai.is_looking_away = true;
                                ai.look_away_timer = 999.0f;
                                ai.distraction_index = GetDistractionCount() - 1;
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
                                    PlayerCheatTop(&game, selected_cheat_hand_index, &swap_old_card, &swap_new_card);
                                    PlayGameSfx(SFX_CHEAT_SWAP);
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
                                g_king_state = KING_STATE_IDLE;
                                TriggerKingComment(KING_EVENT_START, (Language)saveData.language);
                                BlockUiClicks(0.35f);
                            }
                            break;
                    }
                    break;

                case STATE_GAME:
                    // Match transition pause: show the overwhelmed King briefly
                    if (post_match_timer > 0) {
                        post_match_timer -= dt;
                        if (post_match_timer <= 0) {
                            InitGame(&game);
                            InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);
                            g_king_state = KING_STATE_IDLE;
                        }
                        break;
                    }

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
                                    if (saveData.matches_won < 3) saveData.matches_won++;
                                    if (saveData.matches_won > 3) saveData.matches_won = 3;
                                    SaveSaveData(&saveData);
                                    TriggerKingComment(KING_EVENT_WIN, (Language)saveData.language);
                                    g_king_state = KING_STATE_OVERWHELMED; // King lost — show overwhelmed sprite
                                    ai.is_looking_away = false; // He turns back, shocked
                                    ai.look_away_timer = 0;
                                    SpawnVictoryParticles((Vector2){screenWidth/2, 300});
                                    if (saveData.matches_won >= 3) {
                                        appState = STATE_EPILOGUE;
                                        epiStep = 0;
                                        epiRevealTimer = 0;
                                        epiFade = 0;
                                        creditsOffset = 0;
                                        epilogueEndFade = 0;
                                        for (int i = 0; i < MAX_HEARTS; i++) hearts[i].life = -1;
                                        InitTypewriter(&tw, GetEpilogueStepText(0, (Language)saveData.language), 0.03f);
                                    } else {
                                        // Brief pause so the player sees the overwhelmed King,
                                        // then the next match starts with the idle sprite
                                        post_match_timer = 1.8f;
                                    }
                                } else { // AI won or draw — restart only the CURRENT match
                                    TriggerKingComment(KING_EVENT_LOSS, (Language)saveData.language);
                                    g_king_state = KING_STATE_IDLE; // King won — back to idle
                                    InitGame(&game);
                                    InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);
                                }
                            }
                        }
                    } else if (!game.round_over) {
                        // Initiating cheat selection
                        if (IsKeyPressed(KEY_E) && !is_cheating) {
                            if (ai.is_looking_away) {
                                is_selecting_cheat_card = true;
                            } else { // Caught immediately!
                                PlayGameSfx(SFX_CAUGHT);
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
                                PlayGameSfx(SFX_CAUGHT);
                                appState = STATE_CAUGHT;
                                InitTypewriter(&tw, GetCaughtText((Language)saveData.language), 0.03f);
                                is_cheating = false;
                            } else if (cheating_progress >= CHEAT_DURATION) {
                                PlayerCheatSpecific(&game, selected_cheat_hand_index, &swap_old_card, &swap_new_card);
                                PlayGameSfx(SFX_CHEAT_SWAP);
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
                                                PlayGameSfx(SFX_CARD_PLAY);
                                                TriggerKingComment(KING_EVENT_PLAYER_ATTACK, (Language)saveData.language);
                                                {
                                                    Vector2 from = {r.x + cardW/2, r.y + cardH/2};
                                                    float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + (game.table_count-1)*100 + cardW/2;
                                                    Vector2 to = {tX, screenHeight/2.0f + 95 + cardH/2};
                                                    cardFlight = (CardFlight){true, 0, from, to, game.table[game.table_count-1].attacking_card, game.table_count-1, false};
                                                }
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
                                                        PlayGameSfx(SFX_CARD_PLAY);
                                                        TriggerKingComment(KING_EVENT_PLAYER_DEFEND, (Language)saveData.language);
                                                        {
                                                            Vector2 from = {r.x + cardW/2, r.y + cardH/2};
                                                            float tableStartX = screenWidth/2.0f - (game.table_count*100)/2.0f;
                                                            Vector2 to = {tableStartX + t*100 + 16 + cardW/2, screenHeight/2.0f + 125 + cardH/2};
                                                            cardFlight = (CardFlight){true, 0, from, to, game.table[t].defending_card, t, true};
                                                        }
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
                                int _prevCnt = game.table_count;
                                bool _prevDef[6] = {0};
                                for (int _i=0; _i<_prevCnt; _i++) _prevDef[_i]=game.table[_i].has_defender;
                                AITakeTurn(&game, saveData.ai_difficulty);
                                if (game.table_count > _prevCnt) {
                                    int idx = game.table_count-1;
                                    Vector2 from = {screenWidth/2, 380};
                                    float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + idx*100 + cardW/2;
                                    Vector2 to = {tX, screenHeight/2.0f + 95 + cardH/2};
                                    cardFlight = (CardFlight){true, 0, from, to, game.table[idx].attacking_card, idx, false};
                                } else {
                                    for (int _t=0; _t<game.table_count; _t++) if (!_prevDef[_t] && game.table[_t].has_defender) {
                                        Vector2 from = {screenWidth/2, 380};
                                        float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + _t*100 + 16 + cardW/2;
                                        Vector2 to = {tX, screenHeight/2.0f + 125 + cardH/2};
                                        cardFlight = (CardFlight){true, 0, from, to, game.table[_t].defending_card, _t, true};
                                        break;
                                    }
                                }
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
                                    int _prevCnt = game.table_count;
                                    bool _prevDef[6] = {0};
                                    for (int _i=0; _i<_prevCnt; _i++) _prevDef[_i]=game.table[_i].has_defender;
                                    AITakeTurn(&game, saveData.ai_difficulty);
                                    for (int _t=0; _t<game.table_count; _t++) if (!_prevDef[_t] && game.table[_t].has_defender) {
                                        Vector2 from = {screenWidth/2, 380};
                                        float tX = screenWidth/2.0f - (game.table_count*100)/2.0f + _t*100 + 16 + cardW/2;
                                        Vector2 to = {tX, screenHeight/2.0f + 125 + cardH/2};
                                        cardFlight = (CardFlight){true, 0, from, to, game.table[_t].defending_card, _t, true};
                                        break;
                                    }
                                }
                                ai_defend_timer = 0;
                            }
                        }
                        
                        if (game.round_over) {
                            PlayGameSfx(game.player_took_cards ? SFX_CARD_TAKE : SFX_ROUND_END);
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
                            // Caught: restart only the CURRENT match, campaign progress is kept
                            InitGame(&game);
                            InitAI(&ai, saveData.matches_won < 2 ? saveData.matches_won : 2, saveData.ai_difficulty);
                            g_king_state = KING_STATE_IDLE;
                            is_cheating = false;
                            is_selecting_cheat_card = false;
                            BlockUiClicks(0.35f);
                        } else {
                            FinishTypewriter(&tw);
                        }
                    }
                    break;

                case STATE_EPILOGUE:
                    if (epiStep == 1) {
                        epiRevealTimer -= dt;
                        if (epiRevealTimer <= 0) {
                            epiStep = 2;
                            epiRevealTimer = 1.5f;
                        }
                        break;
                    }
                    if (epiStep == 2) {
                        epiRevealTimer -= dt;
                        if (epiRevealTimer <= 0) {
                            epiStep = 3;
                            InitTypewriter(&tw, GetEpilogueStepText(3, (Language)saveData.language), 0.03f);
                        }
                        break;
                    }
                    if (epiStep == 5) {
                        epiFade += dt / 1.2f;
                        if (epiFade > 1) epiFade = 1;
                        creditsOffset += dt * creditsSpeed * 0.3f;
                        for (int i = 0; i < MAX_HEARTS; i++) {
                            if (hearts[i].life <= 0 && GetRandomValue(0, 100) < 12) {
                                bool isA = GetRandomValue(0,1)==0;
                                Vector2 emit = isA ? heartEmitA : heartEmitB;
                                hearts[i].pos = emit;
                                hearts[i].pos.x += GetRandomValue(-18, 18);
                                float vx = isA ? GetRandomValue(5, 28)/10.0f : GetRandomValue(-28, -5)/10.0f; // different directions
                                hearts[i].vel = (Vector2){ vx, - (GetRandomValue(55, 95)) };
                                hearts[i].maxLife = GetRandomValue(28, 42)/10.0f;
                                hearts[i].life = hearts[i].maxLife;
                                hearts[i].size = GetRandomValue(10, 18);
                                hearts[i].sway = GetRandomValue(0, 628)/100.0f;
                            }
                            if (hearts[i].life > 0) {
                                hearts[i].life -= dt;
                                hearts[i].pos.x += hearts[i].vel.x * dt + sinf(hearts[i].life*2.2f + hearts[i].sway)*8.0f*dt;
                                hearts[i].pos.y += hearts[i].vel.y * dt;
                            }
                        }
                        if (epiFade >= 1.0f) {
                            epiStep = 6;
                        }
                        break;
                    }
                    if (epiStep == 6) {
                        creditsOffset += dt * creditsSpeed;
                        for (int i = 0; i < MAX_HEARTS; i++) {
                            if (hearts[i].life <= 0 && GetRandomValue(0, 100) < 10) {
                                bool isA = GetRandomValue(0,1)==0;
                                Vector2 emit = isA ? heartEmitA : heartEmitB;
                                hearts[i].pos = emit;
                                hearts[i].pos.x += GetRandomValue(-20, 20);
                                float vx = isA ? GetRandomValue(4, 30)/10.0f : GetRandomValue(-30, -4)/10.0f; // different directions
                                hearts[i].vel = (Vector2){ vx, - (GetRandomValue(60, 105)) };
                                hearts[i].maxLife = GetRandomValue(30, 44)/10.0f;
                                hearts[i].life = hearts[i].maxLife;
                                hearts[i].size = GetRandomValue(11, 19);
                                hearts[i].sway = GetRandomValue(0, 628)/100.0f;
                            }
                            if (hearts[i].life > 0) {
                                hearts[i].life -= dt;
                                hearts[i].pos.x += hearts[i].vel.x * dt + sinf(hearts[i].life*2.0f + hearts[i].sway)*10.0f*dt;
                                hearts[i].pos.y += hearts[i].vel.y * dt;
                            }
                        }
                        // Auto transition when credits fully leave screen
                        {
                            const char* cred = GetFinalCreditsText((Language)saveData.language);
                            Vector2 csz = MeasureAppText(cred, 28);
                            float startY = screenHeight * 0.5f;
                            float y = startY - creditsOffset;
                            if (y + csz.y < -60) {
                                epilogueEndFade += dt / 1.0f;
                                if (epilogueEndFade >= 1.0f) {
                                    appState = STATE_MENU;
                                    epilogueEndFade = 0;
                                    BlockUiClicks(0.35f);
                                }
                            }
                        }
                        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            appState = STATE_MENU;
                            epilogueEndFade = 0;
                            BlockUiClicks(0.35f);
                        }
                        break;
                    }
                    UpdateTypewriter(&tw, dt);
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (!tw.is_finished) {
                            FinishTypewriter(&tw);
                        } else {
                            if (epiStep == 0) {
                                epiStep = 1;
                                epiRevealTimer = 1.0f;
                            } else if (epiStep == 3) {
                                epiStep = 4;
                                InitTypewriter(&tw, GetEpilogueStepText(4, (Language)saveData.language), 0.03f);
                            } else if (epiStep == 4) {
                                epiStep = 5;
                                epiFade = 0;
                                creditsOffset = 0;
                                for (int i = 0; i < MAX_HEARTS; i++) hearts[i].life = -1;
                            }
                        }
                    }
                    break;

                case STATE_CREDITS:
                    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        appState = STATE_MENU;
                        BlockUiClicks(0.35f);
                    }
                    break;
            }
        }

        // Draw Pass — render everything into virtual canvas
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        if (appState == STATE_LANG_SELECT) {
            DrawMenuBackground(screenWidth, screenHeight);

            // Header
            const char* title = GetUIText("SELECT_LANG_TITLE", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 26);
            Rectangle langHeader = {screenWidth/2.0f - 350, 70, 700, 100};
            DrawRoyalPanel(langHeader, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, 85, 24, GOLD);

            float btnW = 340, btnH = 60;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 240;

            // 1. Русский
            Rectangle rRU = {btnX, startY, btnW, btnH};
            bool hRU = CheckCollisionPointRec(mousePos, rRU);
            if (DrawMenuButton(rRU, GetUIText("LANG_NAME_RU", LANG_RU), hRU, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                saveData.language = 0;
                saveData.first_launch_done = true;
                SaveSaveData(&saveData);
                appState = STATE_MENU;
            }
            startY += 90;

            // 2. English
            Rectangle rEN = {btnX, startY, btnW, btnH};
            bool hEN = CheckCollisionPointRec(mousePos, rEN);
            if (DrawMenuButton(rEN, GetUIText("LANG_NAME_EN", LANG_EN), hEN, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                saveData.language = 1;
                saveData.first_launch_done = true;
                SaveSaveData(&saveData);
                appState = STATE_MENU;
            }
        }
        else if (appState == STATE_MENU) {
            DrawMenuBackground(screenWidth, screenHeight);

            // Title Header
            const char* title = GetUIText("MENU_TITLE", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 38);
            Rectangle menuTitle = {screenWidth/2.0f - titleSz.x/2.0f - 24, 35, titleSz.x + 48, 60};
            DrawRoyalPanel(menuTitle, GOLD);
            DrawTitleFlourish((Vector2){screenWidth / 2.0f, menuTitle.y + menuTitle.height + 12}, 420, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f + 1.5f, 48 + 2.0f, 38, GOLD);

            // Menu Buttons
            float btnW = 360, btnH = 54;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 135;

            // 1. Continue
            if (saveData.matches_won > 0) {
                Rectangle rCont = {btnX, startY, btnW, btnH};
                bool hCont = CheckCollisionPointRec(mousePos, rCont);
                if (DrawMenuButton(rCont, GetUIText("CONTINUE", (Language)saveData.language), hCont, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                    if (saveData.matches_won >= 3) {
                        appState = STATE_EPILOGUE;
                        epiStep = 0;
                        epiRevealTimer = 0;
                        epiFade = 0;
                        creditsOffset = 0;
                        epilogueEndFade = 0;
                        for (int i = 0; i < MAX_HEARTS; i++) hearts[i].life = -1;
                        InitTypewriter(&tw, GetEpilogueStepText(0, (Language)saveData.language), 0.03f);
                    } else {
                        appState = STATE_GAME;
                    }
                }
                startY += 66;
            }

            // 2. New Game
            Rectangle rNew = {btnX, startY, btnW, btnH};
            bool hNew = CheckCollisionPointRec(mousePos, rNew);
            if (DrawMenuButton(rNew, GetUIText("NEW_GAME", (Language)saveData.language), hNew, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                saveData.matches_won = 0;
                SaveSaveData(&saveData);
                InitGame(&game);
                InitAI(&ai, 0, saveData.ai_difficulty);
                g_king_state = KING_STATE_IDLE; // Reset king appearance for fresh game
                appState = STATE_INTRO_DIALOGUE;
                introStep = -4;
                InitTypewriter(&tw, GetAnnouncementDialogPart(0, (Language)saveData.language), 0.03f);
            }
            startY += 66;

            // 3. Tutorial
            Rectangle rTut = {btnX, startY, btnW, btnH};
            bool hTut = CheckCollisionPointRec(mousePos, rTut);
            if (DrawMenuButton(rTut, GetUIText("TUTORIAL", (Language)saveData.language), hTut, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                appState = STATE_TUTORIAL;
                tutStep = 0;
                SetupTutorialGame(&game);
                InitTypewriter(&tw, GetTutorialStepText(0, (Language)saveData.language), 0.02f);
            }
            startY += 66;

            // 4. Settings (Доп. настройки)
            Rectangle rSettings = {btnX, startY, btnW, btnH};
            bool hSettings = CheckCollisionPointRec(mousePos, rSettings);
            if (DrawMenuButton(rSettings, GetUIText("SETTINGS", (Language)saveData.language), hSettings, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                appState = STATE_SETTINGS;
            }
            startY += 66;

            // 5. Credits
            Rectangle rCred = {btnX, startY, btnW, btnH};
            bool hCred = CheckCollisionPointRec(mousePos, rCred);
            if (DrawMenuButton(rCred, GetUIText("CREDITS", (Language)saveData.language), hCred, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                appState = STATE_CREDITS;
            }
            startY += 66;

            // 6. Exit Game
            Rectangle rExit = {btnX, startY, btnW, btnH};
            bool hExit = CheckCollisionPointRec(mousePos, rExit);
            if (DrawMenuButton(rExit, GetUIText("EXIT_GAME", (Language)saveData.language), hExit, (Color){80, 30, 30, 230}, (Color){140, 50, 50, 250})) {
                CloseGameAudio();
                UnloadKingSprites();
                UnloadWandererSprites();
                UnloadPrincessSprite();
                UnloadFinalSprite();
                UnloadTableSprite();
                UnloadRenderFont();
                CloseWindow();
                return 0;
            }
        }
        else if (appState == STATE_SETTINGS) {
            DrawMenuBackground(screenWidth, screenHeight);

            // Title Header
            const char* title = GetUIText("SETTINGS", (Language)saveData.language);
            Vector2 titleSz = MeasureAppText(title, 38);
            Rectangle setTitle = {screenWidth/2.0f - titleSz.x/2.0f - 24, 60, titleSz.x + 48, 60};
            DrawRoyalPanel(setTitle, GOLD);
            DrawTitleFlourish((Vector2){screenWidth / 2.0f, setTitle.y + setTitle.height + 12}, 420, GOLD);
            DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, 72, 38, GOLD);

            float btnW = 440, btnH = 58;
            float btnX = screenWidth / 2.0f - btnW / 2.0f;
            float startY = 180;

            // 1. AI Difficulty Toggle (0: Easy, 1: Medium, 2: Hard)
            const char* diffKey = "AI_DIFF_EASY";
            if (saveData.ai_difficulty == 1) diffKey = "AI_DIFF_MEDIUM";
            else if (saveData.ai_difficulty == 2) diffKey = "AI_DIFF_HARD";

            Rectangle rDiff = {btnX, startY, btnW, btnH};
            bool hDiff = CheckCollisionPointRec(mousePos, rDiff);
            if (DrawMenuButton(rDiff, GetUIText(diffKey, (Language)saveData.language), hDiff, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                saveData.ai_difficulty = (saveData.ai_difficulty + 1) % 3;
                SaveSaveData(&saveData);
            }
            startY += 72;

            // 2. Language Toggle
            Rectangle rLang = {btnX, startY, btnW, btnH};
            bool hLang = CheckCollisionPointRec(mousePos, rLang);
            if (DrawMenuButton(rLang, GetUIText("LANGUAGE_TOGGLE", (Language)saveData.language), hLang, (Color){42, 33, 22, 235}, (Color){82, 63, 36, 250})) {
                saveData.language = (saveData.language + 1) % 2;
                SaveSaveData(&saveData);
            }
            startY += 72;

            // 3. Window Resolution — [◄] [метка] [►]
            {
                const char* resKeys[6] = {
                    "WINDOW_SIZE_640", "WINDOW_SIZE_960", "WINDOW_SIZE_1280",
                    "WINDOW_SIZE_1366", "WINDOW_SIZE_1600", "WINDOW_SIZE_1920"
                };

                // Detect if user manually resized with mouse (custom size)
                int curW = IsWindowFullscreen() ? RES_W[saveData.window_resolution] : GetScreenWidth();
                int curH = IsWindowFullscreen() ? RES_H[saveData.window_resolution] : GetScreenHeight();
                bool isCustom = (curW != RES_W[saveData.window_resolution] || curH != RES_H[saveData.window_resolution]);

                // Build label text
                char resLabel[64];
                if (isCustom) {
                    snprintf(resLabel, sizeof(resLabel),
                             (saveData.language == 0) ? "ВРУЧНУЮ: %d x %d" : "CUSTOM: %d x %d",
                             curW, curH);
                } else {
                    snprintf(resLabel, sizeof(resLabel), "%s",
                             GetUIText(resKeys[saveData.window_resolution], (Language)saveData.language));
                }

                float arrowW   = 52.0f;
                float gap      = 6.0f;
                float labelW   = btnW - (arrowW + gap) * 2;

                // ◄ decrease
                Rectangle rLeft = { btnX, startY, arrowW, btnH };
                bool hLeft = (saveData.window_resolution > 0) && CheckCollisionPointRec(mousePos, rLeft);
                Color arrowBase  = { 30, 50, 70, 230 };
                Color arrowHover = { 50, 80, 110, 250 };
                Color arrowDim   = { 20, 30, 45, 180 };
                if (DrawMenuButton(rLeft, "<", hLeft,
                                   saveData.window_resolution > 0 ? arrowBase : arrowDim, arrowHover)) {
                    if (saveData.window_resolution > 0) {
                        saveData.window_resolution--;
                        SaveSaveData(&saveData);
                        if (!IsWindowFullscreen())
                            SetWindowSize(RES_W[saveData.window_resolution], RES_H[saveData.window_resolution]);
                    }
                }

                // Centre label (display-only panel)
                Rectangle rLabel = { btnX + arrowW + gap, startY, labelW, btnH };
                DrawRectangleRounded(rLabel, 0.15f, 10, (Color){ 24, 19, 13, 235 });
                DrawRectangleRoundedLines(rLabel, 0.15f, 10,
                                          isCustom ? SKYBLUE : (Color){ 146, 116, 63, 220 });
                {
                    float fs = 20.0f;
                    Vector2 lsz = MeasureAppText(resLabel, fs);
                    DrawAppText(resLabel,
                                rLabel.x + (rLabel.width  - lsz.x) * 0.5f,
                                rLabel.y + (rLabel.height - lsz.y) * 0.5f,
                                fs, isCustom ? SKYBLUE : WHITE);
                }

                // ► increase
                Rectangle rRight = { btnX + arrowW + gap + labelW + gap, startY, arrowW, btnH };
                bool hRight = (saveData.window_resolution < RES_COUNT - 1) && CheckCollisionPointRec(mousePos, rRight);
                if (DrawMenuButton(rRight, ">", hRight,
                                   saveData.window_resolution < RES_COUNT - 1 ? arrowBase : arrowDim, arrowHover)) {
                    if (saveData.window_resolution < RES_COUNT - 1) {
                        saveData.window_resolution++;
                        SaveSaveData(&saveData);
                        if (!IsWindowFullscreen())
                            SetWindowSize(RES_W[saveData.window_resolution], RES_H[saveData.window_resolution]);
                    }
                }
            }
            startY += 72;

            // 4/5. Music & SFX Volume — [◄] [метка] [►]
            for (int volRow = 0; volRow < 2; volRow++) {
                int* pVol = (volRow == 0) ? &saveData.music_volume : &saveData.sfx_volume;
                const char* fmtKey = (volRow == 0) ? "MUSIC_VOL_FMT" : "SFX_VOL_FMT";

                char volLabel[48];
                snprintf(volLabel, sizeof(volLabel), GetUIText(fmtKey, (Language)saveData.language), *pVol);

                float arrowW   = 52.0f;
                float gap      = 6.0f;
                float labelW   = btnW - (arrowW + gap) * 2;

                Rectangle rLeft = { btnX, startY, arrowW, btnH };
                bool hLeft = (*pVol > 0) && CheckCollisionPointRec(mousePos, rLeft);
                Color arrowBase  = { 30, 50, 70, 230 };
                Color arrowHover = { 50, 80, 110, 250 };
                Color arrowDim   = { 20, 30, 45, 180 };
                if (DrawMenuButton(rLeft, "<", hLeft,
                                   *pVol > 0 ? arrowBase : arrowDim, arrowHover)) {
                    if (*pVol > 0) {
                        (*pVol)--;
                        SaveSaveData(&saveData);
                        ApplyAudioVolumes(saveData.music_volume / 10.0f, saveData.sfx_volume / 10.0f);
                    }
                }

                Rectangle rLabel = { btnX + arrowW + gap, startY, labelW, btnH };
                DrawRectangleRounded(rLabel, 0.15f, 10, (Color){ 24, 19, 13, 235 });
                DrawRectangleRoundedLines(rLabel, 0.15f, 10, (Color){ 146, 116, 63, 220 });
                {
                    float fs = 20.0f;
                    Vector2 lsz = MeasureAppText(volLabel, fs);
                    DrawAppText(volLabel,
                                rLabel.x + (rLabel.width  - lsz.x) * 0.5f,
                                rLabel.y + (rLabel.height - lsz.y) * 0.5f,
                                fs, WHITE);
                }

                Rectangle rRight = { btnX + arrowW + gap + labelW + gap, startY, arrowW, btnH };
                bool hRight = (*pVol < 10) && CheckCollisionPointRec(mousePos, rRight);
                if (DrawMenuButton(rRight, ">", hRight,
                                   *pVol < 10 ? arrowBase : arrowDim, arrowHover)) {
                    if (*pVol < 10) {
                        (*pVol)++;
                        SaveSaveData(&saveData);
                        ApplyAudioVolumes(saveData.music_volume / 10.0f, saveData.sfx_volume / 10.0f);
                        //if (volRow == 1) PlayGameSfx(SFX_CARD_PLAY);
                    }
                }

                startY += 72;
            }

            // 6. Back to Main Menu
            Rectangle rBack = {btnX, startY, btnW, btnH};
            bool hBack = CheckCollisionPointRec(mousePos, rBack);
            if (DrawMenuButton(rBack, GetUIText("BACK", (Language)saveData.language), hBack, (Color){70, 40, 40, 230}, (Color){110, 60, 60, 250})) {
                appState = STATE_MENU;
            }

            // F11 hint
            const char* f11Hint = (saveData.language == 0) ? "F11 - полноэкранный режим" : "F11 - toggle fullscreen";
            DrawAppText(f11Hint, screenWidth/2.0f - MeasureAppText(f11Hint, 18).x/2.0f, startY + 70, 18, (Color){160, 160, 160, 200});
        }
        else if (appState == STATE_CREDITS) {
            DrawMenuBackground(screenWidth, screenHeight);
            
            Rectangle box = {screenWidth/2.0f - 300, 90, 600, 520};
            DrawRoyalPanel(box, GOLD);

            DrawAppText(GetUIText("CREDITS", (Language)saveData.language), screenWidth/2.0f - 60, 120, 32, GOLD);

            const char* credInfo = GetCreditsText((Language)saveData.language);
            DrawAppText(credInfo, screenWidth/2.0f - 240, 190, 22, WHITE);
        }
        else if (appState == STATE_INTRO_DIALOGUE) {
            DrawMenuBackground(screenWidth, screenHeight);
            if (introStep < 0) {
                // Announcement in paper style — less wide, more elegant
                float annW = 1120;
                float annH = 360;
                Rectangle annBox = { (screenWidth - annW)/2, 75, annW, annH };
                DrawPaperPanel(annBox);
                const char* latin = GetAnnouncementLatinText();
                DrawAppText(latin, annBox.x + 24, annBox.y + 18, 19, (Color){58, 42, 28, 255});
                Rectangle princessRect = { annBox.x + annBox.width * 0.67f + 6, annBox.y + 14, annBox.width * 0.33f - 12, annBox.height - 28 };
                DrawPrincess(princessRect, WHITE);
            } else {
                bool isKingSpeaking = (introStep != 1);
                Rectangle kingRect = { screenWidth * 0.25f +100, 249, 100, 100 };
                Rectangle wanRect  = { screenWidth * 0.75f - 190, 249, 100, 100 };
                DrawKingTinted(kingRect, false, KING_STATE_IDLE, isKingSpeaking ? WHITE : (Color){70, 70, 70, 255});
                DrawWanderer(wanRect, isKingSpeaking ? (Color){70, 70, 70, 255} : WHITE);
            }
        }
        else if (appState == STATE_EPILOGUE) {
            if (epiStep >= 5) {
                DrawMenuBackground(screenWidth, screenHeight);
                float fade = epiFade;
                if (epiStep == 6) fade = 1.0f;
                // Hearts behind credits behind image
                for (int i = 0; i < MAX_HEARTS; i++) if (hearts[i].life > 0) {
                    float a = hearts[i].life / hearts[i].maxLife;
                    float alpha = a * (epiStep == 5 ? fade * 0.85f : 0.9f);
                    Color pink = { 255, 110, 180, (unsigned char)(alpha * 255) };
                    Color hi   = { 255, 210, 230, (unsigned char)(alpha * 200) };
                    DrawHeart(hearts[i].pos, hearts[i].size, pink);
                    DrawHeart((Vector2){hearts[i].pos.x, hearts[i].pos.y - 1}, hearts[i].size*0.45f, hi);
                }
                // Credits behind image, in front of hearts — separate final credits
                // Editable: font size and colors, startY = spawn at screen center
                // Imaginary line at center: credits clipped below y = screenHeight*0.5
                {
                    const char* cred = GetFinalCreditsText((Language)saveData.language);
                    float credAlpha = (epiStep == 5) ? fade * 0.85f : 1.0f;
                    float creditsFontSize = 28.0f; // <-- editable: size
                    float startY = screenHeight * 0.5f; // <-- editable: spawn at center line
                    float y = startY - creditsOffset;
                    Vector2 csz = MeasureAppText(cred, creditsFontSize);
                    // Right side to avoid image (image is left 42..778)
                    float rightAreaX = screenWidth * 0.52f; // <-- editable right area start
                    float rightAreaW = screenWidth * 0.46f; // <-- editable right area width
                    float x = rightAreaX + (rightAreaW - csz.x) * 0.5f;
                    // Clip below center line — text emerges from center upward
                    BeginScissorMode(0, 0, screenWidth, screenHeight/2);
                    // Gold, highly visible (no panel per request)
                    DrawAppText(cred, x+1.8f, y+2.5f, creditsFontSize, Fade((Color){0,0,0,255}, credAlpha*0.75f));
                    DrawAppText(cred, x, y, creditsFontSize, Fade(GOLD, credAlpha));
                    // Inner highlight
                    DrawAppText(cred, x+0.6f, y+0.6f, creditsFontSize, Fade(ROYAL_PARCHMENT, credAlpha*0.35f));
                    EndScissorMode();
                    // Optional: faint center line for debug — uncomment if needed
                    // DrawLine(0, screenHeight/2, screenWidth, screenHeight/2, Fade(GOLD, 0.18f));
                }
                // Final image in front
                DrawFinalImageCentered(screenWidth, screenHeight, fade);
                // Fading characters and dialogue box during step 5
                if (epiStep == 5) {
                    float charAlpha = 1.0f - fade;
                    Rectangle kingRect = { screenWidth * 0.25f +100, 249, 100, 100 };
                    Rectangle wanRect  = { screenWidth * 0.75f - 190, 249, 100, 100 };
                    DrawKingTinted(kingRect, false, KING_STATE_OVERWHELMED, Fade((Color){70,70,70,255}, charAlpha));
                    DrawWandererEx(wanRect, false, Fade(WHITE, charAlpha));
                    // Fading dialogue box
                    float boxW = 880, boxH = 150;
                    Rectangle box = { screenWidth/2.0f - boxW/2.0f, 500, boxW, boxH };
                    // Draw panel with faded alpha by overlaying dark
                    DrawRoyalPanel(box, Fade(GOLD, charAlpha));
                    char buffer[2048] = {0};
                    strncpy(buffer, tw.text, tw.current_len);
                    DrawAppText(buffer, box.x + 25 + 1.0f, box.y + 20 + 1.5f, 22, Fade((Color){0,0,0,255}, charAlpha*0.6f));
                    DrawAppText(buffer, box.x + 25, box.y + 20, 22, Fade(WHITE, charAlpha));
                    DrawRectangleRounded(box, 0.1f, 10, Fade((Color){0,0,0,255}, fade * 90));
                }
                if (epilogueEndFade > 0) {
                    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, epilogueEndFade));
                }
            } else {
                DrawMenuBackground(screenWidth, screenHeight);
                KingState epiKingState = (epiStep >= 3) ? KING_STATE_OVERWHELMED : KING_STATE_IDLE;
                bool epiWandererHooded = (epiStep == 0 || epiStep == 1);
                Color kingTint, wanTint;
                if (epiStep == 1 || epiStep == 2) {
                    kingTint = (Color){70, 70, 70, 255};
                    wanTint = WHITE;
                } else {
                    bool epiKingSpeaking = (epiStep == 0 || epiStep == 3);
                    bool epiWanSpeaking = (epiStep == 4);
                    kingTint = epiKingSpeaking ? WHITE : (Color){70, 70, 70, 255};
                    wanTint = epiWanSpeaking ? WHITE : (Color){70, 70, 70, 255};
                }
                Rectangle kingRect = { screenWidth * 0.25f +100, 249, 100, 100 };
                Rectangle wanRect  = { screenWidth * 0.75f - 190, 249, 100, 100 };
                DrawKingTinted(kingRect, false, epiKingState, kingTint);
                DrawWandererEx(wanRect, epiWandererHooded, wanTint);
                if (epiStep == 2) {
                    float elapsed = 1.5f - epiRevealTimer;
                    float p = elapsed / 0.65f;
                    if (p >= 0.0f && p <= 1.0f) DrawWandererRevealEffect(wanRect, p);
                }
            }
        }
        else if (appState == STATE_INTRO_QUESTION) {
            DrawTableTexture(screenWidth, screenHeight);

            Rectangle kingRect = {screenWidth / 2.0f - 50, 219, 100, 100};
            DrawKing(kingRect, false, KING_STATE_IDLE);

            Rectangle box = {100, 420, screenWidth - 220, 380};
            DrawRoyalPanel(box, GOLD);

            const char* qText = GetUIText("KNOW_DURAK_Q", (Language)saveData.language);
            Vector2 qSz = MeasureAppText(qText, 28);
            DrawAppText(qText, screenWidth/2.0f - qSz.x/2.0f, 260 + 200, 28, GOLD);

            // Choice 1: Yes, let's play
            Rectangle rOpt1 = {screenWidth/2.0f - 260, 360 + 200, 520, 60};
            bool h1 = CheckCollisionPointRec(mousePos, rOpt1);
            if (DrawMenuButton(rOpt1, GetUIText("OPT_YES", (Language)saveData.language), h1, (Color){40, 80, 40, 230}, (Color){60, 130, 60, 250})) {
                appState = STATE_GAME;
                TriggerKingComment(KING_EVENT_START, (Language)saveData.language);
            }

            // Choice 2: No, teach me (Tutorial)
            Rectangle rOpt2 = {screenWidth/2.0f - 260, 450 + 200, 520, 60};
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

            // "ESC to Exit" hint in the top-left corner during gameplay
            if (appState == STATE_GAME) {
                const char* escTxt = GetUIText("ESC_EXIT_HINT", (Language)saveData.language);
                DrawAppText(escTxt, 21.0f, 16.0f, 18, (Color){0, 0, 0, 160});
                DrawAppText(escTxt, 20.0f, 15.0f, 18, (Color){215, 210, 195, 175});
            }

            // Draw King
            Rectangle kingRect = {screenWidth / 2.0f - 50, 219, 100, 100};
            DrawKing(kingRect, ai.is_looking_away, appState == STATE_CAUGHT ? KING_STATE_ANGRY : g_king_state);

            // Draw Speech Bubble above King:
            // while he is turned away — the distraction (servant, courtier, etc.)
            // otherwise — his reaction to game events
            if ((appState == STATE_GAME || appState == STATE_TUTORIAL) && ai.is_looking_away) {
                DrawSpeechBubble((Vector2){screenWidth / 2.0f + 60, 65},
                                 GetDistractionText(ai.distraction_index, (Language)saveData.language), 1.0f);
            } else {
                DrawSpeechBubble((Vector2){screenWidth / 2.0f + 60, 65}, g_king_comment, g_king_comment_timer);
            }

            // Draw cheating prompt (left side of the screen)
            if (ai.is_looking_away && !is_cheating && !is_selecting_cheat_card && (appState == STATE_GAME || tutStep == 6)) {
                const char* cheatTxt = GetUIText("CHEAT_PROMPT", (Language)saveData.language);
                float blink = 0.65f + 0.35f * sinf(GetTime() * 5.0f);
                DrawAppText(cheatTxt, 30 + 1.5f, 90 + 2.0f, 22, (Color){0, 0, 0, 180});
                DrawAppText(cheatTxt, 30, 90, 22, Fade(YELLOW, blink));
            }

            // Card selection prompt banner
            if (is_selecting_cheat_card) {
                const char* selectTxt = GetUIText("SELECT_CARD_TO_SWAP", (Language)saveData.language);
                Vector2 sz = MeasureAppText(selectTxt, 24);
                Rectangle selBox = {screenWidth/2.0f - sz.x/2.0f - 16, 135, sz.x + 32, 42};
                DrawRoyalPanel(selBox, GOLD);
                DrawAppText(selectTxt, screenWidth/2.0f - sz.x/2.0f, 143, 24, GOLD);
            }

            // Draw Cheating progress & Arm Reaching
            if (is_cheating) {
                DrawCheatingProgress((Vector2){screenWidth / 2.0f - 70, screenHeight - 185}, cheating_progress / CHEAT_DURATION);
                DrawReachingArm(cheat_start_pos, deck_pos, cheating_progress / CHEAT_DURATION);
            }

            // Draw AI Hand (Card backs)
            // ---- Масштабированные карты Короля (AI) ----
const float aiCardScale = 0.7f;                // 70% от обычного размера
float aiCardW = cardW * aiCardScale;           // 56 px
float aiCardH = cardH * aiCardScale;           // ~83 px
float aiStep = 55.0f * aiCardScale;            // шаг между левыми краями ≈38.5 px

int aiCount = game.ai_hand.count;
// Общая ширина группы для центрирования:
float totalAiWidth = (aiCount - 1) * aiStep + aiCardW;
float aiStartX = (screenWidth - totalAiWidth) / 2.0f;
float aiY = 415.0f;  // опускаем ниже (было 135)

for (int i = 0; i < aiCount; i++) {
    Rectangle r = { aiStartX + i * aiStep, aiY, aiCardW, aiCardH };
    DrawCard((Card){0,0}, r, false);
}
            // Draw Deck & Full Trump Card Sprite on Left Panel
            if (game.deck.count > 0) {
                const char* suitName = GetSuitLocName(game.trump_card.suit, (Language)saveData.language);

                Rectangle infoPanel = { 24, screenHeight / 2.0f - 165, 202, 302 };
                DrawRoyalPanel(infoPanel, GOLD);

                // Panel header: "КОЗЫРЬ"
                {
                    const char* hdr = GetUIText("TRUMP_HEADER", (Language)saveData.language);
                    Vector2 hsz = MeasureAppText(hdr, 20);
                    DrawAppText(hdr, infoPanel.x + (infoPanel.width - hsz.x) / 2.0f + 1.5f, infoPanel.y + 12 + 2.0f, 20, (Color){0, 0, 0, 170});
                    DrawAppText(hdr, infoPanel.x + (infoPanel.width - hsz.x) / 2.0f, infoPanel.y + 12, 20, GOLD);
                    DrawTitleFlourish((Vector2){ infoPanel.x + infoPanel.width / 2.0f, infoPanel.y + 42 }, 120, Fade(GOLD, 0.7f));
                }

                // Suit name, colored by suit
                {
                    bool red = (game.trump_card.suit == SUIT_HEARTS || game.trump_card.suit == SUIT_DIAMONDS);
                    Color nameCol = red ? (Color){255, 120, 110, 255} : ROYAL_PARCHMENT;
                    Vector2 ssz = MeasureAppText(suitName, 19);
                    float sx = infoPanel.x + (infoPanel.width - ssz.x) / 2.0f;
                    DrawAppText(suitName, sx + 1.0f, infoPanel.y + 54 + 1.5f, 19, (Color){0, 0, 0, 160});
                    DrawAppText(suitName, sx, infoPanel.y + 54, 19, nameCol);
                }

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

                // Deck count chip
                {
                    char countTxt[48];
                    snprintf(countTxt, sizeof(countTxt), GetUIText("DECK_CHIP_FMT", (Language)saveData.language), game.deck.count);
                    float fs = 20.0f;
                    Vector2 csz = MeasureAppText(countTxt, fs);
                    float chipW = csz.x + 26.0f, chipH = 34.0f;
                    Rectangle chip = { infoPanel.x + (infoPanel.width - chipW) / 2.0f, screenHeight / 2.0f + 78, chipW, chipH };
                    DrawRectangleRounded(chip, 0.4f, 8, (Color){16, 13, 9, 235});
                    DrawRectangleRoundedLines(chip, 0.4f, 8, Fade(GOLD, 0.8f));
                    DrawAppText(countTxt, chip.x + (chipW - csz.x) / 2.0f + 1.0f, chip.y + (chipH - csz.y) / 2.0f + 1.5f, fs, (Color){0, 0, 0, 180});
                    DrawAppText(countTxt, chip.x + (chipW - csz.x) / 2.0f, chip.y + (chipH - csz.y) / 2.0f, fs, ROYAL_PARCHMENT);
                }
            }

            // Draw Swap Banner Notification with Mini-Cards!
            DrawSwapNotification((Vector2){screenWidth/2.0f, screenHeight/2.0f - 140}, swap_old_card, swap_new_card, swap_notice_timer, (Language)saveData.language);

            // Draw Invalid Move Rejection Detailed Hint Box
            if (g_invalid_move_timer > 0) {
                Vector2 sz = MeasureAppText(g_invalid_move_text, 20);
                float boxW = sz.x + 40 > 680 ? sz.x + 40 : 680;
                float boxH = 100;
                Rectangle box = {screenWidth/2.0f - boxW/2.0f, 135, boxW, boxH};
                DrawRoyalPanel(box, RED);
                DrawAppText(g_invalid_move_text, box.x + 20, box.y + 16, 20, WHITE);
            }

            // Draw Table Cards (Centered at y: 340)
            float tableStartX = screenWidth / 2.0f - (game.table_count * 100) / 2.0f;
            for (int i = 0; i < game.table_count; i++) {
                bool isFlyingAtt = cardFlight.active && cardFlight.tableIdx==i && !cardFlight.isDef;
                bool isFlyingDef = cardFlight.active && cardFlight.tableIdx==i && cardFlight.isDef;
                if (!isFlyingAtt) {
                    Rectangle rAtt = {tableStartX + i * 100, screenHeight / 2.0f + 95, cardW, cardH};
                    DrawCard(game.table[i].attacking_card, rAtt, true);
                }
                if (game.table[i].has_defender && !isFlyingDef) {
                    Rectangle rDef = {tableStartX + i * 100 + 16, screenHeight / 2.0f + 125, cardW, cardH};
                    DrawCard(game.table[i].defending_card, rDef, true);
                }
            }
            if (cardFlight.active) {
                float e = EaseOutCubic(cardFlight.t);
                Vector2 p = { cardFlight.from.x + (cardFlight.to.x - cardFlight.from.x)*e,
                              cardFlight.from.y + (cardFlight.to.y - cardFlight.from.y)*e - sinf(e*PI)*28.0f };
                float scl = 1.0f + 0.09f * sinf(e*PI);
                Rectangle r = { p.x - cardW*scl/2, p.y - cardH*scl/2, cardW*scl, cardH*scl };
                DrawRectangleRounded((Rectangle){r.x+5, r.y+7, r.width, r.height}, 0.08f, 10, Fade(BLACK, 0.22f));
                DrawCard(cardFlight.card, r, true);
            }
            DrawVictoryParticles();

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
                Rectangle statusBox = {screenWidth/2.0f - sz.x/2.0f - 20, screenHeight/2.0f - 125, sz.x + 40, 50};
                DrawRoyalPanel(statusBox, statusCol);
                DrawAppText(statusTxt, screenWidth/2.0f - sz.x/2.0f + 1.0f, screenHeight/2.0f - 117 + 1.5f, 34, (Color){0, 0, 0, 160});
                DrawAppText(statusTxt, screenWidth/2.0f - sz.x/2.0f, screenHeight/2.0f - 117, 34, statusCol);
            }

            // Draw Player Hand Cards & Dynamic Tutorial Highlights & Level 1 Tactical Hints
            float startX = screenWidth / 2.0f - (game.player_hand.count * 90) / 2.0f;
            for (int i = 0; i < game.player_hand.count; i++) {
                Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                bool hovered = CheckCollisionPointRec(mousePos, r) && !is_selecting_cheat_card && !is_cheating && (appState==STATE_GAME || appState==STATE_TUTORIAL) && end_round_timer<=0 && !game.round_over && !cardFlight.active;
                if (hovered) {
                    r.y -= 14;
                    r.x -= 4;
                    r.width += 8;
                    r.height += 8;
                    r.y += sinf(GetTime()*3.8f + i*0.9f)*1.6f;
                    r.x += sinf(GetTime()*2.2f + i*1.1f)*1.0f;
                }
                DrawCard(game.player_hand.cards[i], r, true);
                if (hovered) DrawCardHighlight(r, Fade(GOLD, 0.55f));

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
                    bool endgameRule = (game.deck.count == 0 && game.ai_hand.count < game.player_hand.count);

                    if (game.table_count == 0) {
                        int minNonTrumpRank = 999;
                        int bestIdx = -1;
                        int unbeatableNT = -1, unbeatableNTRank = 999;
                        int unbeatableT  = -1, unbeatableTRank  = 999;

                        for (int k = 0; k < game.player_hand.count; k++) {
                            Card pc = game.player_hand.cards[k];
                            bool isTrump = (pc.suit == game.trump_card.suit);
                            if (!isTrump && pc.rank < minNonTrumpRank) {
                                minNonTrumpRank = pc.rank;
                                bestIdx = k;
                            }
                            if (endgameRule && !KingCanBeatCard(&game, pc)) {
                                if (!isTrump && pc.rank < unbeatableNTRank) {
                                    unbeatableNTRank = pc.rank;
                                    unbeatableNT = k;
                                }
                                if (isTrump && pc.rank < unbeatableTRank) {
                                    unbeatableTRank = pc.rank;
                                    unbeatableT = k;
                                }
                            }
                        }

                        // Endgame rule: deck empty & King holds fewer cards ->
                        // prefer a card he cannot beat (he takes it and runs out)
                        if (endgameRule) {
                            bestIdx = (unbeatableNT != -1) ? unbeatableNT
                                    : (unbeatableT  != -1) ? unbeatableT
                                    : bestIdx;
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
                        int bestToss = -1, bestTossRank = 999;

                        for (int k = 0; k < game.player_hand.count; k++) {
                            Card pc = game.player_hand.cards[k];
                            if (!CanAttack(&game, pc)) continue;
                            tossCount++;
                            if (endgameRule && !KingCanBeatCard(&game, pc) && pc.rank < bestTossRank) {
                                bestTossRank = pc.rank;
                                bestToss = k;
                            }
                        }

                        for (int i = 0; i < game.player_hand.count; i++) {
                            Rectangle r = {startX + i * 90, screenHeight - 165, cardW, cardH};
                            Vector2 topCenter = {r.x + cardW / 2.0f, r.y};
                            if (i == bestToss) {
                                DrawCardHintBadge(topCenter, GetUIText("HINT_BEST_MOVE", (Language)saveData.language), GREEN);
                            } else if (CanAttack(&game, game.player_hand.cards[i])) {
                                DrawCardHintBadge(topCenter, GetUIText("HINT_TOSS", (Language)saveData.language), GREEN);
                            }
                        }

                        if (tossCount == 0) {
                            const char* noTossTxt = GetUIText("HINT_NO_TOSS_ENTER", (Language)saveData.language);
                            DrawHintBanner(screenWidth / 2.0f, screenHeight - 215, noTossTxt, GOLD);
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
                            DrawHintBanner(screenWidth / 2.0f, screenHeight - 215, noDefTxt, (Color){255, 90, 90, 255});
                        }
                    }
                }
            }
            
            // Draw Turn info & Help hint
            if (appState == STATE_GAME && end_round_timer <= 0) {
                bool attacking = game.is_player_turn;
                const char* turnTxt = GetUIText(attacking ? "YOUR_TURN" : "DEFEND_TURN", (Language)saveData.language);
                Color accent = attacking ? (Color){110, 220, 130, 255} : (Color){255, 110, 100, 255};
                float fs = 22.0f;
                Vector2 tsz = MeasureAppText(turnTxt, fs);
                float chipW = tsz.x + 30.0f, chipH = 38.0f;
                Rectangle chip = { 20, screenHeight - 52, chipW, chipH };
                DrawRectangleRounded((Rectangle){chip.x + 2, chip.y + 3, chipW, chipH}, 0.35f, 8, (Color){0, 0, 0, 120});
                DrawRectangleRounded(chip, 0.35f, 8, (Color){16, 13, 9, 238});
                DrawRectangleRoundedLines(chip, 0.35f, 8, Fade(accent, 0.85f));
                DrawDiamond((Vector2){chip.x + 14, chip.y + chipH / 2.0f}, 4.0f, accent);
                DrawAppText(turnTxt, chip.x + 26 + 1.0f, chip.y + (chipH - tsz.y) / 2.0f + 1.5f, fs, (Color){0, 0, 0, 170});
                DrawAppText(turnTxt, chip.x + 26, chip.y + (chipH - tsz.y) / 2.0f, fs, accent);

                char diffTxt[64];
                snprintf(diffTxt, sizeof(diffTxt), GetUIText("MATCH_INFO_FMT", (Language)saveData.language), saveData.matches_won + 1);

                DrawAppText(diffTxt, screenWidth - 300, 15, 22, WHITE);
            }
        }

        // Draw Non-Overlapping Dialogue / Tutorial Box
        if (appState == STATE_INTRO_DIALOGUE || (appState == STATE_EPILOGUE && epiStep < 5) || appState == STATE_CAUGHT || (appState == STATE_TUTORIAL && !is_selecting_cheat_card && g_invalid_move_timer <= 0)) {
            if (appState == STATE_CAUGHT) {
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){255, 0, 0, 110});
            }
            
            float boxW = 880, boxH = 150;
            float boxY = (appState == STATE_TUTORIAL) ? 332 : 500;
            Rectangle box = {screenWidth/2.0f - boxW/2.0f, boxY, boxW, boxH};
            DrawRoyalPanel(box, GOLD);
            
            char buffer[2048] = {0};
            strncpy(buffer, tw.text, tw.current_len);
            DrawAppText(buffer, box.x + 25, box.y + 20, 22, WHITE);
        }

        if (appState == STATE_INTRO_DIALOGUE && introFadeDir != 0) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, introFade));
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

        DrawCursorParticles();

        EndTextureMode();

        // Blit virtual canvas to real window with letterboxing (preserves 16:9)
        {
            int rW = GetScreenWidth();
            int rH = GetScreenHeight();
            float s = (rW / (float)screenWidth < rH / (float)screenHeight)
                      ? rW / (float)screenWidth
                      : rH / (float)screenHeight;
            float ox = (rW - screenWidth  * s) * 0.5f;
            float oy = (rH - screenHeight * s) * 0.5f;

            Rectangle src = { 0, 0, (float)canvas.texture.width, -(float)canvas.texture.height };
            Rectangle dst = { ox, oy, screenWidth * s, screenHeight * s };

            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
            EndDrawing();
        }
    }

    UnloadRenderTexture(canvas);
    CloseGameAudio();
    UnloadKingSprites();
    UnloadWandererSprites();
    UnloadPrincessSprite();
    UnloadFinalSprite();
    UnloadTableSprite();
    UnloadRenderFont();
    CloseWindow();
    return 0;
}
