#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "game_logic.h"
#include "story.h"

// --- Royal UI palette (shared) ---
#define ROYAL_GOLD      GOLD
#define ROYAL_GOLD_DIM  (Color){ 146, 116, 63, 220 }
#define ROYAL_PARCHMENT (Color){ 238, 230, 214, 255 }
#define ROYAL_PANEL_BG  (Color){ 24, 19, 13, 245 }

// --- King sprite states ---
typedef enum {
    KING_STATE_IDLE,        // Normal gameplay
    KING_STATE_ANGRY,       // Caught cheating
    KING_STATE_OVERWHELMED, // King lost the match
    KING_STATE_TURNED       // King turned his head away (cheating window open)
} KingState;

void InitKingSprites(void);
void UnloadKingSprites(void);

void InitWandererSprites(void);
void UnloadWandererSprites(void);
void DrawWanderer(Rectangle rect, Color tint);
void DrawWandererEx(Rectangle rect, bool hooded, Color tint);
void InitFinalSprite(void);
void UnloadFinalSprite(void);
void DrawFinalImage(Rectangle dst, float alpha);
void DrawFinalImageCentered(int screenWidth, int screenHeight, float alpha);
void DrawHeart(Vector2 center, float size, Color col);
void InitPrincessSprite(void);
void UnloadPrincessSprite(void);
void DrawPrincess(Rectangle rect, Color tint);

void InitTableSprite(void);
void UnloadTableSprite(void);

void InitRenderFont(void);
void UnloadRenderFont(void);
Font GetAppFont(void);

void DrawAppText(const char* text, float posX, float posY, float fontSize, Color color);
Vector2 MeasureAppText(const char* text, float fontSize);

bool DrawMenuButton(Rectangle rect, const char* text, bool isHovered, Color baseColor, Color hoverColor);
void DrawRoyalPanel(Rectangle rect, Color accent);
void BlockUiClicks(float seconds);
void UpdateUiBlock(float dt);
void DrawPaperPanel(Rectangle rect);
void DrawTitleFlourish(Vector2 center, float width, Color accent);
void DrawDiamond(Vector2 c, float r, Color col);
void DrawHintBanner(float centerX, float y, const char* text, Color accent);
int DrawConfirmationModal(const char* title, const char* yesText, const char* noText, int screenWidth, int screenHeight, Vector2 mousePos);

void DrawSuitSymbol(Vector2 center, float radius, Suit suit, Color color);
void DrawCard(Card card, Rectangle rect, bool is_face_up);
void DrawCardHighlight(Rectangle rect, Color color);
void DrawCardHintBadge(Vector2 topCenterPos, const char* text, Color color);
void DrawKing(Rectangle rect, bool is_looking_away, KingState state);
void DrawKingTinted(Rectangle rect, bool is_looking_away, KingState state, Color tint);
void DrawSpeechBubble(Vector2 pos, const char* text, float timer);
void DrawTableTexture(int screenWidth, int screenHeight);
void DrawMenuBackground(int screenWidth, int screenHeight);
void DrawCheatingProgress(Vector2 pos, float progress); // progress 0.0 to 1.0

void DrawReachingArm(Vector2 startPos, Vector2 deckPos, float progress);
void DrawSwapNotification(Vector2 pos, Card oldCard, Card newCard, float timer, Language lang);
void DrawWandererRevealEffect(Rectangle rect, float progress); // 0..1 flash + sparkles

// --- Juicy FX ---
float EaseOutCubic(float t);
float EaseOutBack(float t);
void SpawnVictoryParticles(Vector2 origin);
void UpdateVictoryParticles(float dt);
void DrawVictoryParticles(void);
void SpawnCursorDust(Vector2 pos);
void UpdateCursorParticles(float dt);
void DrawCursorParticles(void);

#endif // RENDER_H
