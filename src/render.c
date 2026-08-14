#include "render.h"
#include <stdio.h>
#include <string.h>

static Font g_font;
static bool g_font_loaded = false;

void InitRenderFont(void) {
    int codepoints[3000];
    int count = 0;
    
    // ASCII
    for (int i = 32; i <= 126; i++) codepoints[count++] = i;
    // Cyrillic
    for (int i = 0x0400; i <= 0x04FF; i++) codepoints[count++] = i;
    // Suit symbols
    codepoints[count++] = 0x2660;
    codepoints[count++] = 0x2663;
    codepoints[count++] = 0x2665;
    codepoints[count++] = 0x2666;

    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\georgia.ttf",   // Royal Serif
        "C:\\Windows\\Fonts\\cambria.ttf",   // Royal Serif
        "C:\\Windows\\Fonts\\pala.ttf",      // Palatino Linotype
        "C:\\Windows\\Fonts\\segoeui.ttf",   // Segoe UI
        "C:\\Windows\\Fonts\\arial.ttf"      // Arial
    };

    for (int i = 0; i < 5; i++) {
        if (FileExists(fontPaths[i])) {
            g_font = LoadFontEx(fontPaths[i], 32, codepoints, count);
            if (g_font.texture.id > 0) {
                g_font_loaded = true;
                SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
                break;
            }
        }
    }
}

void UnloadRenderFont(void) {
    if (g_font_loaded) {
        UnloadFont(g_font);
        g_font_loaded = false;
    }
}

Font GetAppFont(void) {
    if (g_font_loaded) return g_font;
    return GetFontDefault();
}

void DrawAppText(const char* text, float posX, float posY, float fontSize, Color color) {
    if (g_font_loaded) {
        DrawTextEx(g_font, text, (Vector2){posX, posY}, fontSize, 1.0f, color);
    } else {
        DrawText(text, (int)posX, (int)posY, (int)fontSize, color);
    }
}

Vector2 MeasureAppText(const char* text, float fontSize) {
    if (g_font_loaded) {
        return MeasureTextEx(g_font, text, fontSize, 1.0f);
    }
    return (Vector2){ (float)MeasureText(text, (int)fontSize), fontSize };
}

bool DrawMenuButton(Rectangle rect, const char* text, bool isHovered, Color baseColor, Color hoverColor) {
    Color bg = isHovered ? hoverColor : baseColor;
    DrawRectangleRounded(rect, 0.15f, 10, bg);
    DrawRectangleRoundedLines(rect, 0.15f, 10, isHovered ? GOLD : LIGHTGRAY);

    float fontSize = 24;
    Vector2 sz = MeasureAppText(text, fontSize);
    float textX = rect.x + (rect.width - sz.x) / 2.0f;
    float textY = rect.y + (rect.height - sz.y) / 2.0f;
    DrawAppText(text, textX, textY, fontSize, isHovered ? GOLD : WHITE);

    return isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int DrawConfirmationModal(const char* title, const char* yesText, const char* noText, int screenWidth, int screenHeight, Vector2 mousePos) {
    // Backdrop dim
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});

    // Modal Box
    float boxW = 540, boxH = 220;
    Rectangle box = {screenWidth/2.0f - boxW/2.0f, screenHeight/2.0f - boxH/2.0f, boxW, boxH};
    DrawRectangleRounded(box, 0.12f, 10, (Color){25, 25, 25, 250});
    DrawRectangleRoundedLines(box, 0.12f, 10, GOLD);

    // Title text
    Vector2 titleSz = MeasureAppText(title, 26);
    DrawAppText(title, screenWidth/2.0f - titleSz.x/2.0f, box.y + 40, 26, GOLD);

    // Buttons
    float btnW = 150, btnH = 50;
    float btnY = box.y + 125;

    // YES Button
    Rectangle rYes = {screenWidth/2.0f - btnW - 20, btnY, btnW, btnH};
    bool hYes = CheckCollisionPointRec(mousePos, rYes);
    if (DrawMenuButton(rYes, yesText, hYes, (Color){60, 30, 30, 240}, (Color){110, 40, 40, 255})) {
        return 1; // YES clicked
    }

    // NO Button
    Rectangle rNo = {screenWidth/2.0f + 20, btnY, btnW, btnH};
    bool hNo = CheckCollisionPointRec(mousePos, rNo);
    if (DrawMenuButton(rNo, noText, hNo, (Color){30, 60, 30, 240}, (Color){40, 110, 40, 255})) {
        return 2; // NO clicked
    }

    return 0; // Nothing clicked
}

void DrawSuitSymbol(Vector2 center, float radius, Suit suit, Color color) {
    Vector2 c = center;
    switch (suit) {
        case SUIT_HEARTS: {
            float r = radius * 0.55f;
            Vector2 leftCircle = { c.x - r * 0.75f, c.y - r * 0.35f };
            Vector2 rightCircle = { c.x + r * 0.75f, c.y - r * 0.35f };
            DrawCircleV(leftCircle, r, color);
            DrawCircleV(rightCircle, r, color);
            Vector2 p1 = { c.x - r * 1.55f, c.y - r * 0.1f };
            Vector2 p2 = { c.x + r * 1.55f, c.y - r * 0.1f };
            Vector2 p3 = { c.x, c.y + radius * 1.05f };
            DrawTriangle(p1, p3, p2, color);
            break;
        }
        case SUIT_DIAMONDS: {
            Vector2 top = { c.x, c.y - radius * 1.15f };
            Vector2 bottom = { c.x, c.y + radius * 1.15f };
            Vector2 left = { c.x - radius * 0.85f, c.y };
            Vector2 right = { c.x + radius * 0.85f, c.y };
            DrawTriangle(left, right, top, color);
            DrawTriangle(left, bottom, right, color);
            break;
        }
        case SUIT_CLUBS: {
            float r = radius * 0.48f;
            Vector2 topC = { c.x, c.y - r * 0.85f };
            Vector2 leftC = { c.x - r * 0.85f, c.y + r * 0.35f };
            Vector2 rightC = { c.x + r * 0.85f, c.y + r * 0.35f };
            DrawCircleV(topC, r, color);
            DrawCircleV(leftC, r, color);
            DrawCircleV(rightC, r, color);
            Vector2 stemTop = { c.x, c.y };
            Vector2 stemBL = { c.x - r * 0.6f, c.y + radius * 1.1f };
            Vector2 stemBR = { c.x + r * 0.6f, c.y + radius * 1.1f };
            DrawTriangle(stemTop, stemBL, stemBR, color);
            break;
        }
        case SUIT_SPADES: {
            float r = radius * 0.55f;
            Vector2 leftCircle = { c.x - r * 0.75f, c.y + r * 0.35f };
            Vector2 rightCircle = { c.x + r * 0.75f, c.y + r * 0.35f };
            DrawCircleV(leftCircle, r, color);
            DrawCircleV(rightCircle, r, color);
            Vector2 p1 = { c.x - r * 1.5f, c.y + r * 0.2f };
            Vector2 p2 = { c.x + r * 1.5f, c.y + r * 0.2f };
            Vector2 p3 = { c.x, c.y - radius * 1.1f };
            DrawTriangle(p1, p2, p3, color);
            Vector2 stemTop = { c.x, c.y + r * 0.3f };
            Vector2 stemBL = { c.x - r * 0.6f, c.y + radius * 1.2f };
            Vector2 stemBR = { c.x + r * 0.6f, c.y + radius * 1.2f };
            DrawTriangle(stemTop, stemBL, stemBR, color);
            break;
        }
    }
}

void DrawCard(Card card, Rectangle rect, bool is_face_up) {
    // Drop Shadow
    Rectangle shadow = {rect.x + 3, rect.y + 3, rect.width, rect.height};
    DrawRectangleRounded(shadow, 0.08f, 10, (Color){0, 0, 0, 75});

    if (is_face_up) {
        // Crisp White Card Body
        DrawRectangleRounded(rect, 0.08f, 10, (Color){252, 252, 250, 255});
        DrawRectangleRoundedLines(rect, 0.08f, 10, (Color){40, 40, 40, 255});
        
        // Inner Decorative Frame
        Rectangle inner = {rect.x + 4, rect.y + 4, rect.width - 8, rect.height - 8};
        DrawRectangleRoundedLines(inner, 0.07f, 8, (Color){220, 215, 200, 255});

        Color suitColor = (card.suit == SUIT_HEARTS || card.suit == SUIT_DIAMONDS) ? (Color){200, 25, 25, 255} : (Color){25, 25, 25, 255};

        const char* rankStr = "6";
        switch (card.rank) {
            case RANK_6: rankStr = "6"; break;
            case RANK_7: rankStr = "7"; break;
            case RANK_8: rankStr = "8"; break;
            case RANK_9: rankStr = "9"; break;
            case RANK_10: rankStr = "10"; break;
            case RANK_J: rankStr = "В"; break;
            case RANK_Q: rankStr = "Д"; break;
            case RANK_K: rankStr = "К"; break;
            case RANK_A: rankStr = "Т"; break;
            default: break;
        }

        // Top-Left Index
        float indexFontSize = rect.width * 0.24f;
        DrawAppText(rankStr, rect.x + 7, rect.y + 6, indexFontSize, suitColor);
        DrawSuitSymbol((Vector2){rect.x + 14, rect.y + 38}, rect.width * 0.095f, card.suit, suitColor);

        // Bottom-Right Index (Mirrored)
        Vector2 brSize = MeasureAppText(rankStr, indexFontSize);
        DrawAppText(rankStr, rect.x + rect.width - brSize.x - 7, rect.y + rect.height - brSize.y - 6, indexFontSize, suitColor);
        DrawSuitSymbol((Vector2){rect.x + rect.width - 14, rect.y + rect.height - 38}, rect.width * 0.095f, card.suit, suitColor);

        // Center Artwork & Badges
        Vector2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};

        if (card.rank == RANK_A) {
            // Ace: Large Center Suit Symbol + Ornate Gold Ring
            DrawCircleLines(center.x, center.y, rect.width * 0.28f, GOLD);
            DrawSuitSymbol(center, rect.width * 0.24f, card.suit, suitColor);
        } else if (card.rank == RANK_K) {
            // King: Crown Emblem + Center Suit Symbol
            Vector2 cp1 = {center.x - 15, center.y - 12};
            Vector2 cp2 = {center.x + 15, center.y - 12};
            Vector2 cp3 = {center.x, center.y - 26};
            DrawTriangle(cp1, cp3, cp2, GOLD);
            DrawSuitSymbol(center, rect.width * 0.18f, card.suit, suitColor);
        } else if (card.rank == RANK_Q) {
            // Queen: Tiara Emblem + Center Suit Symbol
            DrawCircleLines(center.x, center.y - 15, 11, GOLD);
            DrawSuitSymbol(center, rect.width * 0.18f, card.suit, suitColor);
        } else if (card.rank == RANK_J) {
            // Jack: Shield Crest + Center Suit Symbol
            Rectangle shield = {center.x - 11, center.y - 22, 22, 18};
            DrawRectangleRoundedLines(shield, 0.2f, 4, DARKGRAY);
            DrawSuitSymbol(center, rect.width * 0.18f, card.suit, suitColor);
        } else {
            // Number Cards (6..10): Large Center Suit Symbol
            DrawSuitSymbol(center, rect.width * 0.20f, card.suit, suitColor);
        }
    } else {
        // Advanced Card Back
        DrawRectangleRounded(rect, 0.08f, 10, (Color){20, 45, 95, 255});
        DrawRectangleRoundedLines(rect, 0.08f, 10, WHITE);
        
        Rectangle inner = {rect.x + 6, rect.y + 6, rect.width - 12, rect.height - 12};
        DrawRectangleRoundedLines(inner, 0.08f, 10, SKYBLUE);

        Vector2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
        DrawCircleLines(center.x, center.y, rect.width * 0.25f, GOLD);
        DrawCircleLines(center.x, center.y, rect.width * 0.18f, SKYBLUE);
    }
}

void DrawCardHighlight(Rectangle rect, Color color) {
    Rectangle highlight = {rect.x - 4, rect.y - 4, rect.width + 8, rect.height + 8};
    DrawRectangleRoundedLines(highlight, 0.1f, 10, color);
}

void DrawCardHintBadge(Vector2 topCenterPos, const char* text, Color color) {
    float fontSize = 16.0f;
    Vector2 sz = MeasureAppText(text, fontSize);
    float padX = 8.0f, padY = 4.0f;
    float boxW = sz.x + padX * 2;
    float boxH = sz.y + padY * 2;
    Rectangle r = {topCenterPos.x - boxW / 2.0f, topCenterPos.y - boxH - 6.0f, boxW, boxH};
    
    // Drop shadow
    DrawRectangleRounded((Rectangle){r.x + 2, r.y + 2, r.width, r.height}, 0.35f, 6, (Color){0, 0, 0, 140});
    // Badge Background & Outline
    DrawRectangleRounded(r, 0.35f, 6, (Color){20, 20, 20, 240});
    DrawRectangleRoundedLines(r, 0.35f, 6, color);
    
    // Pointer triangle
    Vector2 p1 = {topCenterPos.x - 4, r.y + boxH};
    Vector2 p2 = {topCenterPos.x + 4, r.y + boxH};
    Vector2 p3 = {topCenterPos.x, r.y + boxH + 4};
    DrawTriangle(p1, p3, p2, color);

    DrawAppText(text, r.x + padX, r.y + padY, fontSize, color);
}

void DrawKing(Rectangle rect, bool is_looking_away, bool is_angry) {
    DrawRectangleRounded(rect, 0.2f, 10, (Color){255, 220, 180, 255});
    DrawRectangleRoundedLines(rect, 0.2f, 10, DARKGRAY);

    // Crown
    Vector2 p1 = {rect.x + 10, rect.y + 10};
    Vector2 p2 = {rect.x + rect.width - 10, rect.y + 10};
    Vector2 p3 = {rect.x + rect.width / 2.0f, rect.y - 15};
    DrawTriangle(p1, p3, p2, GOLD);

    // Eyes
    if (is_looking_away) {
        DrawCircle(rect.x + rect.width - 25, rect.y + 35, 4, BLACK);
    } else {
        DrawCircle(rect.x + 30, rect.y + 35, 4, BLACK);
        DrawCircle(rect.x + rect.width - 30, rect.y + 35, 4, BLACK);
    }

    // Beard
    Rectangle beard = {rect.x + 20, rect.y + 60, rect.width - 40, 25};
    DrawRectangleRounded(beard, 0.5f, 10, LIGHTGRAY);

    if (is_angry) {
        DrawLine(rect.x + 20, rect.y + 25, rect.x + 35, rect.y + 32, RED);
        DrawLine(rect.x + rect.width - 20, rect.y + 25, rect.x + rect.width - 35, rect.y + 32, RED);
    }
}

void DrawSpeechBubble(Vector2 pos, const char* text, float timer) {
    if (timer <= 0 || strlen(text) == 0) return;

    float fontSize = 22;
    Vector2 textSize = MeasureAppText(text, fontSize);
    float padX = 18.0f, padY = 12.0f;
    float boxW = textSize.x + padX * 2;
    float boxH = textSize.y + padY * 2;

    Rectangle bubble = {pos.x, pos.y - boxH / 2.0f, boxW, boxH};
    DrawRectangleRounded(bubble, 0.2f, 10, (Color){255, 255, 240, 245});
    DrawRectangleRoundedLines(bubble, 0.2f, 10, GOLD);

    Vector2 p1 = {pos.x, pos.y - 5};
    Vector2 p2 = {pos.x, pos.y + 5};
    Vector2 p3 = {pos.x - 12, pos.y};
    DrawTriangle(p1, p3, p2, (Color){255, 255, 240, 245});

    DrawAppText(text, bubble.x + padX, bubble.y + padY, fontSize, BLACK);
}

void DrawTableTexture(int screenWidth, int screenHeight) {
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){28, 62, 42, 255}, (Color){14, 34, 22, 255});
    DrawRectangleLines(10, 10, screenWidth - 20, screenHeight - 20, (Color){85, 60, 30, 120});
    DrawRectangleLines(14, 14, screenWidth - 28, screenHeight - 28, (Color){180, 140, 70, 70});
}

void DrawCheatingProgress(Vector2 pos, float progress) {
    if (progress > 1.0f) progress = 1.0f;
    float width = 140;
    float height = 14;
    DrawRectangle(pos.x, pos.y, width, height, (Color){20, 20, 20, 200});
    DrawRectangle(pos.x + 2, pos.y + 2, (width - 4) * progress, height - 4, GOLD);
    DrawRectangleLines(pos.x, pos.y, width, height, WHITE);
}

void DrawReachingArm(Vector2 startPos, Vector2 deckPos, float progress) {
    if (progress <= 0) return;
    if (progress > 1.0f) progress = 1.0f;

    Vector2 handPos;
    if (progress < 0.5f) {
        float t = progress / 0.5f;
        handPos = (Vector2){
            startPos.x + (deckPos.x - startPos.x) * t,
            startPos.y + (deckPos.y - startPos.y) * t - 30.0f * (1.0f - (2.0f * t - 1.0f) * (2.0f * t - 1.0f))
        };
    } else {
        float t = (progress - 0.5f) / 0.5f;
        handPos = (Vector2){
            deckPos.x + (startPos.x - deckPos.x) * t,
            deckPos.y + (startPos.y - deckPos.y) * t - 30.0f * (1.0f - (2.0f * t - 1.0f) * (2.0f * t - 1.0f))
        };
    }

    Vector2 elbow = {
        (startPos.x + handPos.x) / 2.0f - 20.0f,
        (startPos.y + handPos.y) / 2.0f + 25.0f
    };

    // Sleeve
    DrawLineEx(startPos, elbow, 16.0f, (Color){35, 25, 45, 255});
    DrawLineEx(elbow, handPos, 14.0f, (Color){45, 35, 55, 255});
    DrawCircleV(elbow, 9.0f, (Color){35, 25, 45, 255});

    // Gold Cuff
    DrawCircleV(handPos, 8.0f, GOLD);

    // Grasping Hand
    DrawCircleV(handPos, 7.0f, (Color){240, 200, 160, 255});
    DrawCircleV((Vector2){handPos.x - 3, handPos.y - 4}, 3.0f, (Color){230, 190, 150, 255});
    DrawCircleV((Vector2){handPos.x + 3, handPos.y - 4}, 3.0f, (Color){230, 190, 150, 255});
}

void DrawSwapNotification(Vector2 pos, Card oldCard, Card newCard, float timer, Language lang) {
    if (timer <= 0) return;

    const char* title = GetUIText("SWAP_NOTICE_TITLE", lang);

    float boxW = 280, boxH = 135;
    Rectangle box = {pos.x - boxW/2.0f, pos.y - boxH/2.0f, boxW, boxH};

    DrawRectangleRounded(box, 0.15f, 10, (Color){20, 20, 20, 245});
    DrawRectangleRoundedLines(box, 0.15f, 10, GOLD);

    Vector2 titleSz = MeasureAppText(title, 20);
    DrawAppText(title, pos.x - titleSz.x/2.0f, box.y + 8, 20, GOLD);

    // Mini cards dimensions: 54x80
    float miniW = 54, miniH = 80;
    Rectangle rOld = {box.x + 22, box.y + 38, miniW, miniH};
    Rectangle rNew = {box.x + boxW - miniW - 22, box.y + 38, miniW, miniH};

    DrawCard(oldCard, rOld, true);

    const char* arrow = "➔";
    Vector2 arrSz = MeasureAppText(arrow, 26);
    DrawAppText(arrow, pos.x - arrSz.x/2.0f, box.y + 60, 26, GOLD);

    DrawCard(newCard, rNew, true);
}
