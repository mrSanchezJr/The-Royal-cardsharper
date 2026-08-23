#include "story.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* GetSuitLocName(Suit suit, Language lang) {
    if (lang == LANG_EN) {
        switch (suit) {
            case SUIT_HEARTS: return "Hearts (♥)";
            case SUIT_DIAMONDS: return "Diamonds (♦)";
            case SUIT_CLUBS: return "Clubs (♣)";
            case SUIT_SPADES: return "Spades (♠)";
        }
    } else {
        switch (suit) {
            case SUIT_HEARTS: return "Черви (♥)";
            case SUIT_DIAMONDS: return "Бубны (♦)";
            case SUIT_CLUBS: return "Трефы (♣)";
            case SUIT_SPADES: return "Пики (♠)";
        }
    }
    return "";
}

const char* GetRankLocName(Rank rank, Language lang) {
    switch (rank) {
        case RANK_6: return "6";
        case RANK_7: return "7";
        case RANK_8: return "8";
        case RANK_9: return "9";
        case RANK_10: return "10";
        case RANK_J: return (lang == LANG_EN) ? "Jack" : "Валет";
        case RANK_Q: return (lang == LANG_EN) ? "Queen" : "Дама";
        case RANK_K: return (lang == LANG_EN) ? "King" : "Король";
        case RANK_A: return (lang == LANG_EN) ? "Ace" : "Туз";
        default: return "6";
    }
}

// -------------------------------------------------------------
// UI & Interface Text Dictionary (RU, EN)
// -------------------------------------------------------------
const char* GetUIText(const char* key, Language lang) {
    if (strcmp(key, "SELECT_LANG_TITLE") == 0) {
        if (lang == LANG_EN) return "Please select your language\nПожалуйста, выберите язык";
        return "Пожалуйста, выберите язык\nPlease select your language";
    }
    if (strcmp(key, "LANG_NAME_RU") == 0) return "Русский";
    if (strcmp(key, "LANG_NAME_EN") == 0) return "English";

    if (strcmp(key, "MENU_TITLE") == 0) {
        if (lang == LANG_EN) return "THE ROYAL CARDSHAPER";
        return "КОРОЛЕВСКИЙ ШУЛЕР";
    }
    if (strcmp(key, "CONTINUE") == 0) {
        if (lang == LANG_EN) return "CONTINUE MATCH";
        return "ПРОДОЛЖИТЬ ИГРУ";
    }
    if (strcmp(key, "NEW_GAME") == 0) {
        if (lang == LANG_EN) return "NEW GAME";
        return "НОВАЯ ИГРА";
    }
    if (strcmp(key, "TUTORIAL") == 0) {
        if (lang == LANG_EN) return "TUTORIAL";
        return "ОБУЧЕНИЕ";
    }
    if (strcmp(key, "SETTINGS") == 0) {
        if (lang == LANG_EN) return "SETTINGS";
        return "НАСТРОЙКИ";
    }
    if (strcmp(key, "AI_DIFFICULTY") == 0) {
        if (lang == LANG_EN) return "AI SKILL";
        return "СЛОЖНОСТЬ ИИ";
    }
    if (strcmp(key, "AI_DIFF_EASY") == 0) {
        if (lang == LANG_EN) return "AI SKILL: EASY";
        return "СЛОЖНОСТЬ ИИ: ЛЁГКИЙ";
    }
    if (strcmp(key, "AI_DIFF_MEDIUM") == 0) {
        if (lang == LANG_EN) return "AI SKILL: MEDIUM";
        return "СЛОЖНОСТЬ ИИ: СРЕДНИЙ";
    }
    if (strcmp(key, "AI_DIFF_HARD") == 0) {
        if (lang == LANG_EN) return "AI SKILL: HARD";
        return "СЛОЖНОСТЬ ИИ: СЛОЖНЫЙ";
    }
    if (strcmp(key, "LANGUAGE_TOGGLE") == 0) {
        if (lang == LANG_EN) return "LANGUAGE: ENGLISH";
        return "ЯЗЫК: РУССКИЙ";
    }
    if (strcmp(key, "TRUMP_HEADER") == 0) {
        return (lang == LANG_EN) ? "TRUMP" : "КОЗЫРЬ";
    }
    if (strcmp(key, "DECK_CHIP_FMT") == 0) {
        return (lang == LANG_EN) ? "In deck: %d" : "В колоде: %d";
    }
    if (strcmp(key, "MUSIC_VOL_FMT") == 0) {
        return (lang == LANG_EN) ? "MUSIC: %d" : "МУЗЫКА: %d";
    }
    if (strcmp(key, "SFX_VOL_FMT") == 0) {
        return (lang == LANG_EN) ? "SFX: %d" : "ЗВУКИ: %d";
    }
    if (strcmp(key, "WINDOW_SIZE_640")  == 0) { return (lang == LANG_EN) ? "WINDOW: 640 x 360"  : "ОКНО: 640 x 360";  }
    if (strcmp(key, "WINDOW_SIZE_960")  == 0) { return (lang == LANG_EN) ? "WINDOW: 960 x 540"  : "ОКНО: 960 x 540";  }
    if (strcmp(key, "WINDOW_SIZE_1280") == 0) { return (lang == LANG_EN) ? "WINDOW: 1280 x 720"  : "ОКНО: 1280 x 720 (по умолч.)"; }
    if (strcmp(key, "WINDOW_SIZE_1366") == 0) { return (lang == LANG_EN) ? "WINDOW: 1366 x 768"  : "ОКНО: 1366 x 768"; }
    if (strcmp(key, "WINDOW_SIZE_1600") == 0) { return (lang == LANG_EN) ? "WINDOW: 1600 x 900"  : "ОКНО: 1600 x 900"; }
    if (strcmp(key, "WINDOW_SIZE_1920") == 0) { return (lang == LANG_EN) ? "WINDOW: 1920 x 1080" : "ОКНО: 1920 x 1080"; }
    if (strcmp(key, "CREDITS") == 0) {
        if (lang == LANG_EN) return "CREDITS";
        return "ОБ АВТОРАХ";
    }
    if (strcmp(key, "EXIT_GAME") == 0) {
        if (lang == LANG_EN) return "EXIT GAME";
        return "ВЫХОД ИЗ ИГРЫ";
    }
    if (strcmp(key, "BACK") == 0) {
        if (lang == LANG_EN) return "BACK";
        return "НАЗАД";
    }
    if (strcmp(key, "QUIT_CONFIRM_TITLE") == 0) {
        if (lang == LANG_EN) return "Return to Main Menu?";
        return "Вернуться в главное меню?";
    }
    if (strcmp(key, "YES") == 0) {
        if (lang == LANG_EN) return "YES";
        return "ДА";
    }
    if (strcmp(key, "NO") == 0) {
        if (lang == LANG_EN) return "NO";
        return "НЕТ";
    }
    if (strcmp(key, "CHEAT_PROMPT") == 0) {
        if (lang == LANG_EN) return "Press E, then CLICK a card in hand to swap!";
        return "Нажми E, затем КЛИКНИ на карту в руке!";
    }
    if (strcmp(key, "SELECT_CARD_TO_SWAP") == 0) {
        if (lang == LANG_EN) return "CLICK a card in your hand to swap with the deck!";
        return "КЛИКНИ на карту в своей руке для подмены!";
    }
    if (strcmp(key, "YOUR_TURN") == 0) {
        if (lang == LANG_EN) return "Your turn (Attack) - ENTER (Pass)";
        return "Твой ход (Атака) - ENTER (Готово)";
    }
    if (strcmp(key, "DEFEND_TURN") == 0) {
        if (lang == LANG_EN) return "Defend! - ENTER (Take)";
        return "Защищайся! - ENTER (Забрать)";
    }
    if (strcmp(key, "MATCH_INFO_FMT") == 0) {
        if (lang == LANG_EN) return "Match: %d/3 (H - Help)";
        return "Партия: %d/3 (H - Обучение)";
    }
    if (strcmp(key, "TRUMP_LABEL_FMT") == 0) {
        if (lang == LANG_EN) return "Trump: %s";
        return "Козырь: %s";
    }
    if (strcmp(key, "DECK_LABEL_FMT") == 0) {
        if (lang == LANG_EN) return "Deck: %d";
        return "Колода: %d";
    }
    if (strcmp(key, "STATUS_BITO") == 0) {
        if (lang == LANG_EN) return "BITO (DISCARD)!";
        return "БИТО!";
    }
    if (strcmp(key, "STATUS_PLAYER_TOOK") == 0) {
        if (lang == LANG_EN) return "Player took cards!";
        return "Игрок забрал карты!";
    }
    if (strcmp(key, "STATUS_AI_TOOK") == 0) {
        if (lang == LANG_EN) return "King took cards!";
        return "ИИ забрал карты!";
    }
    if (strcmp(key, "SWAP_NOTICE_TITLE") == 0) {
        if (lang == LANG_EN) return "SLEIGHT OF HAND!";
        return "ЛОВКОСТЬ РУК!";
    }
    if (strcmp(key, "HINT_BEST_MOVE") == 0) {
        if (lang == LANG_EN) return "Best move";
        return "Лучший ход";
    }
    if (strcmp(key, "HINT_TRUMP") == 0) {
        if (lang == LANG_EN) return "Trump";
        return "Козырь";
    }
    if (strcmp(key, "HINT_TOSS") == 0) {
        if (lang == LANG_EN) return "Toss!";
        return "Подкинуть!";
    }
    if (strcmp(key, "HINT_CAN_DEFEND") == 0) {
        if (lang == LANG_EN) return "Can Defend!";
        return "Отбиться!";
    }
    if (strcmp(key, "HINT_TRUMP_DEFEND") == 0) {
        if (lang == LANG_EN) return "Trump Defend!";
        return "Козырь!";
    }
    if (strcmp(key, "HINT_NO_DEFENSE_ENTER") == 0) {
        if (lang == LANG_EN) return "No cards to defend! Press ENTER to take cards";
        return "Биться нечем! Нажмите ENTER, чтобы забрать карты";
    }
    if (strcmp(key, "HINT_NO_TOSS_ENTER") == 0) {
        if (lang == LANG_EN) return "Nothing more to toss! Press ENTER for BITO";
        return "Подкинуть нечего! Нажмите ENTER для БИТО";
    }
    if (strcmp(key, "KNOW_DURAK_Q") == 0) {
        if (lang == LANG_EN) return "Do you know how to play Durak?";
        return "Знаете ли вы правила карточной игры Дурак?";
    }
    if (strcmp(key, "OPT_YES") == 0) {
        if (lang == LANG_EN) return "Yes, let's play!";
        return "Да, я умею играть!";
    }
    if (strcmp(key, "OPT_NO") == 0) {
        if (lang == LANG_EN) return "No, teach me (Tutorial)";
        return "Нет, научи меня (Обучение)";
    }
    return key;
}

// -------------------------------------------------------------
// King Commentary (RU, EN)
// -------------------------------------------------------------
const char* GetKingComment(KingEvent event, Language lang) {
    if (lang == LANG_EN) {
        switch (event) {
            case KING_EVENT_START: {
                const char* c[] = { "Let's see what you've got!", "Deal! I never lose at cards.", "A royal duel!" };
                return c[rand() % 3];
            }
            case KING_EVENT_PLAYER_ATTACK: {
                const char* c[] = { "A bold move!", "Interesting card...", "Do you think that saves you?", "Attacking already?" };
                return c[rand() % 4];
            }
            case KING_EVENT_PLAYER_DEFEND: {
                const char* c[] = { "You defended it!", "Good defense...", "Lucky for now!", "Let's see if you can hold on." };
                return c[rand() % 4];
            }
            case KING_EVENT_PLAYER_TAKE: {
                const char* c[] = { "Haha! Take them all!", "No match for my cards!", "My attack is unstoppable!" };
                return c[rand() % 3];
            }
            case KING_EVENT_AI_TAKE: {
                const char* c[] = { "Grr... Well played.", "I took them... but not for long!", "A strategic retreat!" };
                return c[rand() % 3];
            }
            case KING_EVENT_LOOK_AWAY: {
                const char* c[] = { "What is that noise outside?", "My neck hurts...", "Drafty room today...", "Something in my eye..." };
                return c[rand() % 4];
            }
            case KING_EVENT_WIN: return "Impossible! How did you win?!";
            case KING_EVENT_LOSS: return "Ha! The King remains victorious!";
            default: return "";
        }
    } else { // LANG_RU
        switch (event) {
            case KING_EVENT_START: {
                const char* c[] = { "Посмотрим, на что ты способен!", "Раздавай! В этой игре я не знаю поражений.", "Сыграем по-королевски!" };
                return c[rand() % 3];
            }
            case KING_EVENT_PLAYER_ATTACK: {
                const char* c[] = { "Хм, дерзкий ход!", "Интересная карта...", "Думаешь, это тебя спасает?", "Атакуешь? Ну-ну!" };
                return c[rand() % 4];
            }
            case KING_EVENT_PLAYER_DEFEND: {
                const char* c[] = { "Отбился-таки!", "Хорошая защита...", "Посмотрим, отобьешься ли дальше.", "Неплохой ответ!" };
                return c[rand() % 4];
            }
            case KING_EVENT_PLAYER_TAKE: {
                const char* c[] = { "Ха-ха! Забирай всё себе!", "Нечем бить? Твой рукав полон!", "Моя атака несокрушима!" };
                return c[rand() % 3];
            }
            case KING_EVENT_AI_TAKE: {
                const char* c[] = { "Грр... Взял... но это временно!", "Недурно сыграно. Признаю.", "Тактический ход!" };
                return c[rand() % 3];
            }
            case KING_EVENT_LOOK_AWAY: {
                const char* c[] = { "Что там за шум за окном?...", "Ох, шея затекла...", "Свеча что-то сильно гаснет...", "Минутку, соринка в глазу..." };
                return c[rand() % 4];
            }
            case KING_EVENT_WIN: return "Невозможно! Какая у тебя стратегия?! Поражение...";
            case KING_EVENT_LOSS: return "Ха! Я же говорил, королевская корона побеждает!";
            default: return "";
        }
    }
}

// -------------------------------------------------------------
// Credits Text (RU, EN)
// -------------------------------------------------------------
const char* GetCreditsText(Language lang) {
    if (lang == LANG_EN) {
        return "The Royal Cardshaper\n\n"
               "Engine: Raylib 5.0 (C99)\n"
               "Platform: Windows x64\n\n"
               "Code & AI: Sanchez Jr, Gemeni 3.7\n"
               "Game Design & Narrative: Rui\n\n"
               "[ Click or Press SPACE / ESC to Return ]";
    } else {
        return "The Royal Cardshaper\n\n"
               "Движок: Raylib 5.0 (C99)\n"
               "Платформа: Windows x64\n\n"
               "Код и ИИ: Sanchez Jr, Gemeni 3.7\n"
               "Геймдизайн и Сюжет: Rui\n\n"
               "[ Кликните или нажмите ПРОБЕЛ / ESC для возврата ]";
    }
}

// -------------------------------------------------------------
// King Distractions (why he turns away) — RU, EN
// -------------------------------------------------------------
int GetDistractionCount(void) {
    return 6;
}

const char* GetDistractionText(int index, Language lang) {
    static const char* ru[] = {
        "Слуга: Ваше Величество, донесение из провинций!",
        "Придворный: Государь, нужна ваша печать на указе!",
        "Гонец: Срочное письмо от королевы!",
        "Казначей: Ваше Величество, казна снова пуста!",
        "Повар: Что прикажете подать на вечерний пир?",
        "Музыкант: Какую мелодию сыграть, сир?"
    };
    static const char* en[] = {
        "Servant: Your Majesty, dispatches from the provinces!",
        "Courtier: Sire, your seal is needed on a decree!",
        "Messenger: An urgent letter from the Queen!",
        "Treasurer: Your Majesty, the treasury is empty again!",
        "Cook: What shall be served at tonight's feast?",
        "Musician: Which melody shall I play, sire?"
    };
    if (index < 0 || index >= 6) index = 0;
    return (lang == LANG_EN) ? en[index] : ru[index];
}

// -------------------------------------------------------------
// Epilogue & Caught Texts (RU, EN)
// -------------------------------------------------------------
const char* GetEpilogueText(Language lang) {
    if (lang == LANG_EN) {
        return "You remove your hood. The King gasps.\nYou are a woman!\nThe King frowns, but a royal promise is absolute:\n'I did not specify the suitor must be a man. A deal is a deal...'\n\nCongratulations! You won the Princess and half the kingdom!\n\nCredits:\nCode: Antigravity\nDesign: You\nEngine: Raylib";
    } else {
        return "Ты снимаешь капюшон. Король в шоке.\nПеред ним — девушка!\nКороль хмурится, но слово надо держать:\n'Я не указывал, что претендент должен быть мужчиной. Уговор есть уговор...'\n\nПоздравляем! Ты завоевала принцессу и полцарства!\n\nТитры:\nКод: Antigravity\nДизайн: You\nДвижок: Raylib";
    }
}

const char* GetCaughtText(Language lang) {
    if (lang == LANG_EN) {
        return "Aha, you cheat! I saw that!\nGuards! Throw this fraud out!\n\n[Game Over - Press SPACE or Click]";
    } else {
        return "Ах ты, мошенник! Я всё видел! Стража!\n\n[Игра окончена - Нажми ПРОБЕЛ или Клик]";
    }
}

// -------------------------------------------------------------
// Intro Dialogue (RU, EN)
// -------------------------------------------------------------
const char* GetIntroDialogueText(int step, Language lang) {
    if (lang == LANG_EN) {
        switch (step) {
            case 0: return "King: Who dares enter my hall? Ah, a cloaked stranger...\n\n[Press SPACE or Click]";
            case 1: return "Stranger: I have come to accept your challenge. To test my luck at cards.\n\n[Press SPACE or Click]";
            case 2: return "King: Bold! At stake is my daughter's hand and half the kingdom.\nBut tell me... do you know the rules of DURAK?\n\n[Press SPACE or Click]";
            case 3: return "King: Hm?\n\n[Press SPACE or Click]";
            default: return "";
        }
    } else {
        switch (step) {
            case 0: return "Король: Кто смеет нарушать покой замка? А, незнакомец в плаще...\n\n[Нажми ПРОБЕЛ или Клик]";
            case 1: return "Незнакомец: Я пришел принять твой вызов. Помериться силами в карты.\n\n[Нажми ПРОБЕЛ или Клик]";
            case 2: return "Король: Смельчак! На кону рука моей дочери и полцарства.\nНо знаешь ли ты правила карточной игры ДУРАК?\n\n[Нажми ПРОБЕЛ или Клик]";
            default: return "";
        }
    }
}

// -------------------------------------------------------------
// Tutorial Steps (RU, EN)
// -------------------------------------------------------------
const char* GetTutorialStepText(int step, Language lang) {
    if (lang == LANG_EN) {
        switch (step) {
            case 0: return "TUTORIAL 1: Core Goal & Cards\n36 cards in total. Goal: Empty your hand first!\nRanks: 6 < 7 < 8 < 9 < 10 < J (Jack) < Q (Queen) < K (King) < A (Ace).\nAce (A) is the HIGHEST card in Durak!\n\n[Press SPACE or Click]";
            case 1: return "TUTORIAL 2: The 4 Suits & Trumps\n4 Suits: ♠ Spades (Black), ♣ Clubs (Black), ♥ Hearts (Red), ♦ Diamonds (Red).\nOne suit is designated TRUMP (left panel). TRUMP beats ANY non-trump card!\n(e.g., 6 of Trumps beats Ace of Spades).\n\n[Press SPACE or Click]";
            case 2: return "TUTORIAL 3: Attacking Phase\nWhen it's your turn to attack, play ANY card from your hand.\nPRACTICE: Left-click the highlighted card in your hand to launch an attack!";
            case 3: return "TUTORIAL 3.5: King Defended!\nThe King played a higher card to defend against your attack.\nPress SPACE / Click to send cards to BITO (Discard) and prepare to Defend!";
            case 4: return "TUTORIAL 4: Defending Phase\nThe King launched a counter-attack card!\nTo beat an attacking card, play a HIGHER card of the SAME suit, OR a TRUMP.\nPRACTICE: Left-click the highlighted card to defend!";
            case 5: return "TUTORIAL 5: Bito (Discard) vs Taking Cards\nIf defender CANNOT beat a card — press ENTER to TAKE all table cards into hand.\nIf defender beats all cards — press ENTER to finish round (BITO/Discard).\n\n[Press SPACE or Click]";
            case 6: return "TUTORIAL 6: Sleight of Hand (Select Card to Swap)\nWhen the King turns away — press 'E', then CLICK the card in your hand you want to swap!\nYour hand will reach out to swap it with the top of the deck!\nPRACTICE: Press 'E' and click a card right now!";
            case 7: return "Tutorial Complete!\nYou now master all suits, symbols, and Durak rules.\nPress 'H' at any time during a match for help.\n\n[Press SPACE or Click to Start Match 1]";
            default: return "";
        }
    } else { // LANG_RU
        switch (step) {
            case 0: return "ОБУЧЕНИЕ 1: Цель и Достоинства карт\nВсего 36 карт. Цель: скинуть все карты первее соперника!\nРанги: 6 < 7 < 8 < 9 < 10 < В (Валет) < Д (Дама) < К (Король) < Т (Туз).\nТуз (Т / A) — СТАРШАЯ карта в игре Дурак!\n\n[Нажми ПРОБЕЛ или Клик]";
            case 1: return "ОБУЧЕНИЕ 2: 4 Масти и Власть Козыря\n4 Масти: ♠ Пики (черные), ♣ Трефы (черные), ♥ Черви (красные), ♦ Бубны (красные).\nКОЗЫРЬ (указан слева) бьёт ЛЮБУЮ карту другой масти! (даже 6 козырная > Туз Пик).\n\n[Нажми ПРОБЕЛ или Клик]";
            case 2: return "ОБУЧЕНИЕ 3: Фаза Атаки (Ваш ход)\nКогда твой ход — сходи любой картой из руки.\nПРАКТИКА: Нажми мышкой (ЛКМ) на подсвеченную карту в своей руке!";
            case 3: return "ОБУЧЕНИЕ 3.5: Король отбился!\nКороль выставил карту старше и защитился от твоей атаки.\nНажми ПРОБЕЛ / Клик, чтобы отправить карты в БИТО и перейти к Защите!";
            case 4: return "ОБУЧЕНИЕ 4: Фаза Защиты (Отбой)\nКороль пошел в ответную атаку!\nЧтобы отбить карту, сходи картой ТОЙ ЖЕ масти, но СТАРШЕ, либо КОЗЫРЕМ.\nПРАКТИКА: Кликни мышкой на подсвеченную карту, чтобы отбиться!";
            case 5: return "ОБУЧЕНИЕ 5: Бито vs Забор карт\nЕсли отбиться НЕЧЕМ — жми ENTER, чтобы ЗАБРАТЬ карты со стола.\nЕсли отбился и подкидывать больше нечего — жми ENTER для БИТО (карты уходят из игры).\n\n[Нажми ПРОБЕЛ или Клик]";
            case 6: return "ОБУЧЕНИЕ 6: Шулерство (Выбор карты для подмены!)\nКогда Король отворачивается — нажми 'E', а затем КЛИКНИ на карту в своей руке!\nТвоя рука потянется к колоде и подменит карту!\nПРАКТИКА: Нажми 'E' и кликни на карту прямо сейчас!";
            case 7: return "Обучение успешно завершено!\nТеперь ты знаешь все масти, достоинства и правила Дурака.\nЖми 'H' во время партии для вызова справки.\n\n[Нажми ПРОБЕЛ или Клик, чтобы начать 1-ю партию!]";
            default: return "";
        }
    }
}

// -------------------------------------------------------------
// Detailed Invalid Move Hints (RU, EN)
// -------------------------------------------------------------
void GetInvalidDefenseReason(Card attack, Card defend, Suit trump, Language lang, char* outBuffer, size_t bufferSize) {
    const char* attSuit = GetSuitLocName(attack.suit, lang);
    const char* defSuit = GetSuitLocName(defend.suit, lang);
    const char* attRank = GetRankLocName(attack.rank, lang);
    const char* defRank = GetRankLocName(defend.rank, lang);
    const char* trumpSuit = GetSuitLocName(trump, lang);

    if (defend.suit == attack.suit) {
        if (defend.rank <= attack.rank) {
            if (lang == LANG_EN) {
                snprintf(outBuffer, bufferSize,
                    "Cannot Defend! [%s %s] is lower or equal to [%s %s]!\n"
                    "To defend this card, you need a HIGHER %s card, or ANY Trump (%s)!",
                    defRank, defSuit, attRank, attSuit, attSuit, trumpSuit);
            } else {
                snprintf(outBuffer, bufferSize,
                    "Нельзя отбиться! [%s %s] ниже или равна [%s %s]!\n"
                    "Чтобы отбить карту, нужна масть %s СТАРШЕ, либо Козырь (%s)!",
                    defRank, defSuit, attRank, attSuit, attSuit, trumpSuit);
            }
            return;
        }
    } else if (defend.suit != trump) {
        if (lang == LANG_EN) {
            snprintf(outBuffer, bufferSize,
                "Cannot Defend! Suit mismatch: [%s] vs [%s]!\n"
                "You must match the attack suit (%s) with a higher card, OR play a Trump (%s)!",
                defSuit, attSuit, attSuit, trumpSuit);
        } else {
            snprintf(outBuffer, bufferSize,
                "Нельзя отбиться! Другая масть: [%s] вместо [%s]!\n"
                "Нужно ходить в масть атаки (%s) картой старше, либо Козырем (%s)!",
                defSuit, attSuit, attSuit, trumpSuit);
        }
        return;
    }
}

void GetInvalidAttackReason(Card attack, const GameState* game, Language lang, char* outBuffer, size_t bufferSize) {
    const char* attSuit = GetSuitLocName(attack.suit, lang);
    const char* attRank = GetRankLocName(attack.rank, lang);

    if (game->table_count > 0) {
        if (lang == LANG_EN) {
            snprintf(outBuffer, bufferSize,
                "Cannot attack with [%s %s]!\n"
                "You can ONLY toss cards whose rank MATCHES cards ALREADY on the table!",
                attRank, attSuit);
        } else {
            snprintf(outBuffer, bufferSize,
                "Нельзя сходить картой [%s %s]!\n"
                "Подкидывать можно ТОЛЬКО карты тех достоинств, которые УЖЕ лежат на столе!",
                attRank, attSuit);
        }
    }
}

// -------------------------------------------------------------
// Typewriter Utility
// -------------------------------------------------------------
static int GetNextCharLen(const char* ptr) {
    unsigned char c = (unsigned char)*ptr;
    if (c == 0) return 0;
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

void InitTypewriter(TypewriterText* tw, const char* text, float char_delay) {
    tw->text = text;
    tw->current_len = 0;
    tw->timer = 0;
    tw->char_delay = char_delay;
    tw->is_finished = false;
}

void UpdateTypewriter(TypewriterText* tw, float dt) {
    if (tw->is_finished) return;
    
    tw->timer += dt;
    if (tw->timer >= tw->char_delay) {
        tw->timer = 0;
        int step = GetNextCharLen(tw->text + tw->current_len);
        if (step == 0) {
            tw->is_finished = true;
        } else {
            tw->current_len += step;
            if ((size_t)tw->current_len >= strlen(tw->text)) {
                tw->current_len = strlen(tw->text);
                tw->is_finished = true;
            }
        }
    }
}

void FinishTypewriter(TypewriterText* tw) {
    tw->current_len = strlen(tw->text);
    tw->is_finished = true;
}
