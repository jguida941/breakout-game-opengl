#include "GameFlowInternal.h"

#include <cstdio>
#include <cstring>
#include <iostream>

using std::cout;
using std::endl;
using std::size_t;
using std::vector;

namespace {
// UI draw/text constants live here so layout tuning is quick.
constexpr float kOutlineLineWidth = 2.0f;
constexpr int kGlyphRows = 5;
constexpr int kGlyphColumns = 3;
constexpr int kGlyphAdvanceColumns = 4;
constexpr float kGlyphPixelFillScale = 0.90f;
constexpr size_t kHudLineBufferSize = 128;
constexpr size_t kInfoLineBufferSize = 64;
constexpr size_t kTitleBufferSize = 512;
const char *const kWindowTitleBase = "CS330 Breakout";

struct Rgb {
  float r;
  float g;
  float b;
};

namespace UiText {
// On-screen text only. Console text is handled in Show*Message functions.
const char *const kMenuStart = "ENTER START GAME";
const char *const kMenuSettings = "S OPEN SETTINGS";
const char *const kMenuControls = "C OPEN CONTROLS";
const char *const kEscQuit = "ESC QUIT";
const char *const kEscMenu = "ESC MENU";

const char *const kControlsMove = "A D OR LEFT RIGHT MOVE";
const char *const kControlsBall = "SPACE USES ONE RESERVE BALL";
const char *const kControlsMainMenu = "M MAIN MENU";
const char *const kControlsReturn = "ENTER OR C RETURN";

const char *const kGameplayFooter =
    "M MENU C CONTROLS SPACE USE RESERVE ESC MENU";
} // namespace UiText

// Primitive shape helpers for menu and HUD-like visuals.
void DrawFilledRect(float centerX, float centerY, float width, float height,
                    float red, float green, float blue) {
  float halfWidth = width * 0.5f;
  float halfHeight = height * 0.5f;

  glColor3f(red, green, blue);
  glBegin(GL_POLYGON);
  glVertex2f(centerX + halfWidth, centerY + halfHeight);
  glVertex2f(centerX + halfWidth, centerY - halfHeight);
  glVertex2f(centerX - halfWidth, centerY - halfHeight);
  glVertex2f(centerX - halfWidth, centerY + halfHeight);
  glEnd();
}

void DrawRectOutline(float centerX, float centerY, float width, float height,
                     float red, float green, float blue) {
  float halfWidth = width * 0.5f;
  float halfHeight = height * 0.5f;

  glColor3f(red, green, blue);
  glLineWidth(kOutlineLineWidth);
  glBegin(GL_LINE_LOOP);
  glVertex2f(centerX + halfWidth, centerY + halfHeight);
  glVertex2f(centerX + halfWidth, centerY - halfHeight);
  glVertex2f(centerX - halfWidth, centerY - halfHeight);
  glVertex2f(centerX - halfWidth, centerY + halfHeight);
  glEnd();
}

void DrawBox(float centerX, float centerY, float width, float height,
             Rgb fillColor, Rgb lineColor) {
  DrawFilledRect(centerX, centerY, width, height, fillColor.r, fillColor.g,
                 fillColor.b);
  DrawRectOutline(centerX, centerY, width, height, lineColor.r, lineColor.g,
                  lineColor.b);
}

struct Glyph3x5 {
  char symbol;
  unsigned char rows[kGlyphRows];
};

// Tiny built-in bitmap font so this project has no external font dependency.
const Glyph3x5 kGlyphTable[] = {
    {' ', {0, 0, 0, 0, 0}}, {'A', {2, 5, 7, 5, 5}}, {'B', {6, 5, 6, 5, 6}},
    {'C', {3, 4, 4, 4, 3}}, {'D', {6, 5, 5, 5, 6}}, {'E', {7, 4, 6, 4, 7}},
    {'F', {7, 4, 6, 4, 4}}, {'G', {3, 4, 5, 5, 3}}, {'H', {5, 5, 7, 5, 5}},
    {'I', {7, 2, 2, 2, 7}}, {'J', {1, 1, 1, 5, 2}}, {'K', {5, 5, 6, 5, 5}},
    {'L', {4, 4, 4, 4, 7}}, {'M', {5, 7, 5, 5, 5}}, {'N', {5, 7, 7, 7, 5}},
    {'O', {2, 5, 5, 5, 2}}, {'P', {6, 5, 6, 4, 4}}, {'Q', {2, 5, 5, 7, 3}},
    {'R', {6, 5, 6, 5, 5}}, {'S', {3, 4, 2, 1, 6}}, {'T', {7, 2, 2, 2, 2}},
    {'U', {5, 5, 5, 5, 7}}, {'V', {5, 5, 5, 5, 2}}, {'W', {5, 5, 7, 7, 5}},
    {'X', {5, 5, 2, 5, 5}}, {'Y', {5, 5, 2, 2, 2}}, {'Z', {7, 1, 2, 4, 7}},
    {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}}, {'2', {7, 1, 7, 4, 7}},
    {'3', {7, 1, 7, 1, 7}}, {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 7, 1, 7}},
    {'6', {7, 4, 7, 5, 7}}, {'7', {7, 1, 1, 1, 1}}, {'8', {7, 5, 7, 5, 7}},
    {'9', {7, 5, 7, 1, 7}}, {':', {0, 2, 0, 2, 0}}, {'/', {1, 1, 2, 4, 4}},
    {'-', {0, 0, 7, 0, 0}}, {'.', {0, 0, 0, 0, 2}}, {'+', {0, 2, 7, 2, 0}}};

const unsigned char *FindGlyphRows(char symbol) {
  if ((symbol >= 'a') && (symbol <= 'z')) {
    symbol = static_cast<char>(symbol - ('a' - 'A'));
  }

  for (const Glyph3x5 &glyph : kGlyphTable) {
    if (glyph.symbol == symbol) {
      return glyph.rows;
    }
  }

  return kGlyphTable[0].rows;
}

float TextLineWidth(const char *text, float pixelSize) {
  if ((text == nullptr) || (text[0] == '\0')) {
    return 0.0f;
  }

  size_t length = std::strlen(text);
  // Width is: 3 columns per glyph + 1 spacer column, minus final spacer.
  return ((kGlyphAdvanceColumns * static_cast<float>(length)) - 1.0f) *
         pixelSize;
}

void DrawTextLineLeft(const char *text, float leftX, float topY,
                      float pixelSize, float red, float green, float blue) {
  float glyphX = leftX;
  size_t textLength = std::strlen(text);
  for (size_t i = 0; i < textLength; i++) {
    const unsigned char *rows = FindGlyphRows(text[i]);
    for (int row = 0; row < kGlyphRows; row++) {
      for (int column = 0; column < kGlyphColumns; column++) {
        unsigned char mask =
            static_cast<unsigned char>(1 << ((kGlyphColumns - 1) - column));
        if ((rows[row] & mask) == 0) {
          continue;
        }

        float centerX =
            glyphX + (static_cast<float>(column) + 0.5f) * pixelSize;
        float centerY = topY - (static_cast<float>(row) + 0.5f) * pixelSize;
        DrawFilledRect(centerX, centerY, pixelSize * kGlyphPixelFillScale,
                       pixelSize * kGlyphPixelFillScale, red, green, blue);
      }
    }

    glyphX += kGlyphAdvanceColumns * pixelSize;
  }
}

void DrawTextLineCentered(const char *text, float centerX, float topY,
                          float pixelSize, float red, float green, float blue) {
  float lineWidth = TextLineWidth(text, pixelSize);
  DrawTextLineLeft(text, centerX - (lineWidth * 0.5f), topY, pixelSize, red,
                   green, blue);
}

void DrawCenterText(const char *text, float topY, float pixelSize, Rgb color) {
  DrawTextLineCentered(text, 0.0f, topY, pixelSize, color.r, color.g, color.b);
}

int DifficultyMarkerCount(const GameSession &session) {
  if (session.state.difficulty == DIFFICULTY_EASY) {
    return 1;
  }
  if (session.state.difficulty == DIFFICULTY_NORMAL) {
    return 2;
  }
  return 3;
}
} // namespace

void ShowMainMenuMessage() {
  cout << endl;
  cout << "=== MAIN MENU ===" << endl;
  cout << "ENTER: Start game" << endl;
  cout << "S: Settings (difficulty)" << endl;
  cout << "C: Controls screen" << endl;
  cout << "ESC: Quit" << endl;
}

void ShowSettingsMessage() {
  cout << endl;
  cout << "=== SETTINGS ===" << endl;
  cout << "1: Easy  2: Normal  3: Hard" << endl;
  cout << "ENTER: Back to menu" << endl;
}

void ShowControlsMessage() {
  cout << endl;
  cout << "=== CONTROLS ===" << endl;
  cout << "A / LEFT / J / NumPad4: Move paddle left" << endl;
  cout << "D / RIGHT / L / NumPad6: Move paddle right" << endl;
  cout << "SPACE: Spawn one extra ball (uses reserve)" << endl;
  cout << "ENTER or C: Return to previous screen" << endl;
  cout << "M: Open main menu" << endl;
  cout << "ESC: Back to menu" << endl;
}

void ShowGameplayMessage(const GameSession &session) {
  cout << endl;
  cout << "=== GAME STARTED ===" << endl;
  cout << "Difficulty: " << DifficultyName(session.state.difficulty) << endl;
  cout << "Reserve balls: " << session.state.reserveBallsRemaining << endl;
  cout << "Power-ups: cyan=multi-ball, yellow=wide paddle, green=extra life"
       << endl;
  cout << "Ball collisions randomly: merge, recolor, disappear, or split"
       << endl;
  cout << "ESC or M: Back to menu" << endl;
}

void RenderGameplayScene(const GameSession &session,
                         const vector<Brick> &bricks, const Brick &paddle) {
  for (const Brick &brick : bricks) {
    brick.Draw();
  }
  paddle.Draw();

  for (const Circle &ball : session.balls) {
    ball.Draw();
  }

  for (const PowerUp &powerUp : session.powerUps) {
    powerUp.Draw();
  }

  DrawBox(0.0f, 0.94f, 1.92f, 0.10f, Rgb{0.05f, 0.07f, 0.11f},
          Rgb{0.24f, 0.30f, 0.40f});

  char hudLine[kHudLineBufferSize];
  snprintf(hudLine, sizeof(hudLine),
           "LEVEL %d SCORE %d LIVES %d BALLS %d RESERVE %d",
           session.state.level, session.state.score, session.state.livesRemaining,
           static_cast<int>(session.balls.size()),
           session.state.reserveBallsRemaining);
  DrawTextLineCentered(hudLine, 0.0f, 0.965f, 0.0090f, 0.92f, 0.95f, 0.98f);

  DrawBox(0.0f, -0.96f, 1.92f, 0.07f, Rgb{0.05f, 0.07f, 0.11f},
          Rgb{0.24f, 0.30f, 0.40f});
  DrawCenterText(UiText::kGameplayFooter, -0.945f, 0.0075f,
                 Rgb{0.90f, 0.94f, 0.98f});
}

void RenderMainMenuScreen(const GameSession &session) {
  const Rgb buttonOutline = {0.04f, 0.08f, 0.16f};
  DrawBox(0.0f, 0.0f, 1.65f, 1.45f, Rgb{0.14f, 0.18f, 0.26f},
          Rgb{0.80f, 0.88f, 0.98f});
  DrawBox(0.0f, 0.40f, 1.20f, 0.16f, Rgb{0.24f, 0.58f, 0.94f}, buttonOutline);
  DrawBox(0.0f, 0.14f, 1.20f, 0.16f, Rgb{0.42f, 0.70f, 0.40f}, buttonOutline);
  DrawBox(0.0f, -0.12f, 1.20f, 0.16f, Rgb{0.90f, 0.68f, 0.34f}, buttonOutline);

  for (int i = 0; i < 5; i++) {
    float x = -0.55f + (i * 0.28f);
    DrawBox(x, -0.52f, 0.10f, 0.10f, Rgb{0.96f, 0.96f, 0.26f},
            Rgb{0.08f, 0.08f, 0.08f});
  }

  DrawCenterText("BREAKOUT GAME", 0.66f, 0.016f, Rgb{0.95f, 0.97f, 0.99f});

  char difficultyLine[kInfoLineBufferSize];
  snprintf(difficultyLine, sizeof(difficultyLine), "DIFFICULTY %s",
           DifficultyName(session.state.difficulty));
  DrawCenterText(difficultyLine, 0.58f, 0.010f, Rgb{0.84f, 0.90f, 0.98f});

  DrawCenterText(UiText::kMenuStart, 0.44f, 0.012f, Rgb{0.06f, 0.10f, 0.16f});
  DrawCenterText(UiText::kMenuSettings, 0.18f, 0.012f,
                 Rgb{0.06f, 0.10f, 0.16f});
  DrawCenterText(UiText::kMenuControls, -0.08f, 0.012f,
                 Rgb{0.06f, 0.10f, 0.16f});
  DrawCenterText(UiText::kEscQuit, -0.66f, 0.011f, Rgb{0.88f, 0.92f, 0.98f});
}

void RenderSettingsScreen(const GameSession &session) {
  const Rgb darkOutline = {0.04f, 0.08f, 0.16f};
  DrawBox(0.0f, 0.0f, 1.65f, 1.45f, Rgb{0.15f, 0.15f, 0.20f},
          Rgb{0.90f, 0.90f, 0.90f});

  float easyBrightness =
      (session.state.difficulty == DIFFICULTY_EASY) ? 0.96f : 0.45f;
  float normalBrightness =
      (session.state.difficulty == DIFFICULTY_NORMAL) ? 0.96f : 0.45f;
  float hardBrightness =
      (session.state.difficulty == DIFFICULTY_HARD) ? 0.96f : 0.45f;

  DrawBox(0.0f, 0.34f, 1.30f, 0.16f, Rgb{0.26f, easyBrightness, 0.52f},
          darkOutline);
  DrawBox(0.0f, 0.08f, 1.30f, 0.16f,
          Rgb{0.32f, 0.60f * normalBrightness, normalBrightness}, darkOutline);
  DrawBox(0.0f, -0.18f, 1.30f, 0.16f, Rgb{hardBrightness, 0.30f, 0.26f},
          darkOutline);

  for (int i = 0; i < DifficultyMarkerCount(session); i++) {
    float x = -0.18f + (i * 0.18f);
    DrawBox(x, -0.52f, 0.10f, 0.10f, Rgb{0.96f, 0.90f, 0.28f},
            Rgb{0.08f, 0.08f, 0.08f});
  }

  DrawCenterText("SETTINGS", 0.66f, 0.016f, Rgb{0.94f, 0.94f, 0.98f});
  DrawCenterText("1 EASY", 0.38f, 0.012f, Rgb{0.08f, 0.12f, 0.16f});
  DrawCenterText("2 NORMAL", 0.12f, 0.012f, Rgb{0.08f, 0.12f, 0.16f});
  DrawCenterText("3 HARD", -0.14f, 0.012f, Rgb{0.08f, 0.12f, 0.16f});

  char currentLine[kInfoLineBufferSize];
  snprintf(currentLine, sizeof(currentLine), "CURRENT %s",
           DifficultyName(session.state.difficulty));
  DrawCenterText(currentLine, -0.42f, 0.011f, Rgb{0.93f, 0.92f, 0.82f});
  DrawCenterText("ENTER OR M BACK", -0.64f, 0.010f,
                 Rgb{0.86f, 0.90f, 0.96f});
}

void RenderControlsScreen() {
  DrawBox(0.0f, 0.0f, 1.65f, 1.45f, Rgb{0.12f, 0.16f, 0.23f},
          Rgb{0.84f, 0.90f, 0.95f});
  DrawBox(-0.26f, 0.26f, 0.20f, 0.20f, Rgb{0.36f, 0.56f, 0.92f},
          Rgb{0.08f, 0.08f, 0.08f});
  DrawBox(0.00f, 0.26f, 0.20f, 0.20f, Rgb{0.86f, 0.68f, 0.36f},
          Rgb{0.08f, 0.08f, 0.08f});
  DrawBox(0.26f, 0.26f, 0.20f, 0.20f, Rgb{0.36f, 0.56f, 0.92f},
          Rgb{0.08f, 0.08f, 0.08f});
  DrawBox(0.0f, -0.56f, 0.58f, 0.08f, Rgb{0.96f, 0.96f, 0.24f},
          Rgb{0.08f, 0.08f, 0.08f});
  DrawBox(0.0f, -0.30f, 0.10f, 0.10f, Rgb{0.34f, 0.94f, 0.90f},
          Rgb{0.08f, 0.08f, 0.08f});

  DrawCenterText("CONTROLS", 0.66f, 0.016f, Rgb{0.93f, 0.95f, 0.98f});
  DrawCenterText(UiText::kControlsMove, 0.57f, 0.010f,
                 Rgb{0.85f, 0.90f, 0.97f});
  DrawCenterText(UiText::kControlsBall, 0.49f, 0.010f,
                 Rgb{0.85f, 0.90f, 0.97f});
  DrawCenterText(UiText::kControlsMainMenu, -0.40f, 0.011f,
                 Rgb{0.92f, 0.95f, 0.99f});
  DrawCenterText(UiText::kControlsReturn, -0.66f, 0.010f,
                 Rgb{0.92f, 0.95f, 0.99f});
}

void RenderGameOverScreen(const GameSession &session) {
  DrawBox(0.0f, 0.0f, 1.65f, 1.45f, Rgb{0.21f, 0.10f, 0.12f},
          Rgb{0.98f, 0.80f, 0.84f});
  DrawBox(0.0f, 0.22f, 1.20f, 0.24f, Rgb{0.94f, 0.34f, 0.40f},
          Rgb{0.12f, 0.05f, 0.06f});
  DrawBox(0.0f, -0.22f, 1.20f, 0.24f, Rgb{0.26f, 0.36f, 0.54f},
          Rgb{0.08f, 0.08f, 0.08f});

  DrawCenterText("GAME OVER", 0.27f, 0.016f, Rgb{0.12f, 0.05f, 0.06f});

  char scoreLine[kInfoLineBufferSize];
  snprintf(scoreLine, sizeof(scoreLine), "SCORE %d LEVEL %d",
           session.state.score, session.state.level);
  DrawCenterText(scoreLine, -0.17f, 0.012f, Rgb{0.90f, 0.93f, 0.99f});
  DrawCenterText("R RESTART", -0.48f, 0.011f, Rgb{0.96f, 0.92f, 0.92f});
  DrawCenterText(UiText::kControlsMainMenu, -0.58f, 0.011f,
                 Rgb{0.96f, 0.92f, 0.92f});
  DrawCenterText(UiText::kEscMenu, -0.68f, 0.011f, Rgb{0.96f, 0.92f, 0.92f});
}

void RenderScene(const GameSession &session, const vector<Brick> &bricks,
                 const Brick &paddle) {
  // Single render entrypoint; pick screen-specific renderer here.
  switch (session.state.screen) {
  case SCREEN_MAIN_MENU:
    RenderMainMenuScreen(session);
    break;
  case SCREEN_SETTINGS:
    RenderSettingsScreen(session);
    break;
  case SCREEN_CONTROLS:
    RenderControlsScreen();
    break;
  case SCREEN_GAME_OVER:
    RenderGameOverScreen(session);
    break;
  default:
    RenderGameplayScene(session, bricks, paddle);
    break;
  }
}

void UpdateWindowTitle(GLFWwindow *window, const GameSession &session) {
  // Title mirrors controls/stats in case terminal output is not visible.
  char titleText[kTitleBufferSize];
  titleText[0] = '\0';

  switch (session.state.screen) {
  case SCREEN_MAIN_MENU:
    snprintf(titleText, sizeof(titleText),
             "%s | Menu | ENTER Start | S Settings | C Controls | ESC Quit | "
             "Difficulty: %s",
             kWindowTitleBase, DifficultyName(session.state.difficulty));
    break;
  case SCREEN_SETTINGS:
    snprintf(
        titleText, sizeof(titleText),
        "%s | Settings | 1 Easy 2 Normal 3 Hard | ENTER Back | Current: %s",
        kWindowTitleBase, DifficultyName(session.state.difficulty));
    break;
  case SCREEN_CONTROLS:
    snprintf(titleText, sizeof(titleText),
             "%s | Controls | A/D Move | Space Uses Reserve | M Menu | ENTER/C "
             "Back | ESC Menu",
             kWindowTitleBase);
    break;
  case SCREEN_GAME_OVER:
    snprintf(
        titleText, sizeof(titleText),
        "%s | GAME OVER | Score %d | Level %d | R Restart | M Menu | ESC Menu",
        kWindowTitleBase, session.state.score, session.state.level);
    break;
  default:
    snprintf(titleText, sizeof(titleText),
             "%s | %s | Level %d | Score %d | Lives %d | Balls %d | Reserve %d "
             "| M Menu | C Controls | ESC Menu",
             kWindowTitleBase, DifficultyName(session.state.difficulty),
             session.state.level, session.state.score,
             session.state.livesRemaining,
             static_cast<int>(session.balls.size()),
             session.state.reserveBallsRemaining);
    break;
  }

  glfwSetWindowTitle(window, titleText);
}
