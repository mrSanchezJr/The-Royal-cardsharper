#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "game_logic.h"
#include "story.h"

void InitRenderFont(void);
void UnloadRenderFont(void);
Font GetAppFont(void);

void DrawAppText(const char* text, float posX, float posY, float fontSize, Color color);
Vector2 MeasureAppText(const char* text, float fontSize);

bool DrawMenuButton(Rectangle rect, const char* text, bool isHovered, Color baseColor, Color hoverColor);
int DrawConfirmationModal(const char* title, const char* yesText, const char* noText, int screenWidth, int screenHeight, Vector2 mousePos);

void DrawSuitSymbol(Vector2 center, float radius, Suit suit, Color color);
void DrawCard(Card card, Rectangle rect, bool is_face_up);
void DrawCardHighlight(Rectangle rect, Color color);
void DrawCardHintBadge(Vector2 topCenterPos, const char* text, Color color);
void DrawKing(Rectangle rect, bool is_looking_away, bool is_angry);
void DrawSpeechBubble(Vector2 pos, const char* text, float timer);
void DrawTableTexture(int screenWidth, int screenHeight);
void DrawCheatingProgress(Vector2 pos, float progress); // progress 0.0 to 1.0

void DrawReachingArm(Vector2 startPos, Vector2 deckPos, float progress);
void DrawSwapNotification(Vector2 pos, Card oldCard, Card newCard, float timer, Language lang);

#endif // RENDER_H
