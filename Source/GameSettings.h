#pragma once

#if defined(_WIN32)
#include <GLFW\glfw3.h>
#else
#include <GLFW/glfw3.h>
#endif

// Core game constants.
extern const float kDegreesToRadians;
extern const float kWorldMin;
extern const float kWorldMax;
extern const float kBallSpawnHeightAbovePaddle;
extern const float kSimulationStepSeconds;
extern const float kWidePaddleDurationSeconds;
extern const int kReserveBallsMax;

enum BrickType { REFLECTIVE, DESTRUCTIBLE };
enum DifficultyMode {
  DIFFICULTY_EASY,
  DIFFICULTY_NORMAL,
  DIFFICULTY_HARD
};
enum ScreenMode {
  SCREEN_MAIN_MENU,
  SCREEN_SETTINGS,
  SCREEN_CONTROLS,
  SCREEN_PLAYING,
  SCREEN_GAME_OVER
};
enum PowerUpType {
  POWERUP_MULTIBALL,
  POWERUP_WIDE_PADDLE,
  POWERUP_EXTRA_LIFE
};

// Stores live game values in one place.
struct GameState {
  // Global mode and screen navigation state.
  DifficultyMode difficulty;
  ScreenMode screen;
  ScreenMode controlsReturnScreen;

  // Player progression state.
  int level;
  int score;
  int livesRemaining;
  int reserveBallsRemaining;
  int bricksBrokenTowardReserveReward;

  // Runtime tuning values (world units per simulation tick).
  float minBallSpeed;
  float maxBallSpeed;
  float basePaddleWidth;

  // Temporary effect state and input edge-detection latches.
  float widePaddleSecondsRemaining;
  bool spaceHeldLastFrame;
  bool wasKeyDownLastFrame[GLFW_KEY_LAST + 1];

  GameState();
};

// Number helpers.
float RandomFloat(float minValue, float maxValue);
int RandomInt(int minValue, int maxValue);
float KeepFloatInRange(float value, float minValue, float maxValue);
int KeepIntInRange(int value, int minValue, int maxValue);
float StepScaleFromDeltaSeconds(float deltaSeconds);

// Difficulty settings.
const char *DifficultyName(DifficultyMode difficulty);
float DifficultySpeedMultiplier(DifficultyMode difficulty);
float DifficultyPaddleWidth(DifficultyMode difficulty);
int DifficultyLives(DifficultyMode difficulty);
float DifficultyBallSize(DifficultyMode difficulty);
int DifficultyStartingReserveBalls(DifficultyMode difficulty);
void RefreshDifficultyAndLevel(GameState &state);
