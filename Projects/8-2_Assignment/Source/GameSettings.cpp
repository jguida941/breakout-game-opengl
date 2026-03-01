#include "GameSettings.h"

#include <cmath>
#include <random>
#include <utility>

const float kDegreesToRadians = 3.14159f / 180.0f;
const float kWorldMin = -1.0f;
const float kWorldMax = 1.0f;
const float kBallSpawnHeightAbovePaddle = 0.13f;
const float kSimulationStepSeconds = 1.0f / 60.0f;
const float kWidePaddleDurationSeconds = 15.0f;
const int kReserveBallsMax = 30;

namespace {
// Baseline values used by difficulty + level scaling.
constexpr int kInitialLevel = 1;
constexpr int kInitialScore = 0;
constexpr int kInitialLivesNormal = 3;
constexpr float kInitialPowerUpSeconds = 0.0f;
constexpr bool kSpaceKeyInitiallyHeld = false;

constexpr float kBaseMinBallSpeed = 0.010f;
constexpr float kBaseMaxBallSpeed = 0.045f;
constexpr float kLevelSpeedIncrease = 0.05f;
constexpr float kRequiredSpeedGap = 0.015f;

constexpr float kSpeedMultiplierEasy = 0.85f;
constexpr float kSpeedMultiplierNormal = 1.0f;
constexpr float kSpeedMultiplierHard = 1.25f;

constexpr float kPaddleWidthEasy = 0.56f;
constexpr float kPaddleWidthNormal = 0.45f;
constexpr float kPaddleWidthHard = 0.34f;

constexpr int kLivesEasy = 5;
constexpr int kLivesNormal = 3;
constexpr int kLivesHard = 2;
constexpr int kStartingReserveBallsEasy = 8;
constexpr int kStartingReserveBallsNormal = 6;
constexpr int kStartingReserveBallsHard = 4;

constexpr float kBallSizeEasy = 0.050f;
constexpr float kBallSizeNormal = 0.045f;
constexpr float kBallSizeHard = 0.040f;

std::mt19937 &GlobalRng() {
  static std::mt19937 engine(std::random_device{}());
  return engine;
}

template <typename T>
T ValueForDifficulty(DifficultyMode difficulty, T easyValue, T normalValue,
                     T hardValue) {
  switch (difficulty) {
  case DIFFICULTY_EASY:
    return easyValue;
  case DIFFICULTY_HARD:
    return hardValue;
  default:
    return normalValue;
  }
}
} // namespace

GameState::GameState()
    : difficulty(DIFFICULTY_NORMAL), screen(SCREEN_MAIN_MENU),
      controlsReturnScreen(SCREEN_MAIN_MENU), level(kInitialLevel),
      score(kInitialScore), livesRemaining(kInitialLivesNormal),
      reserveBallsRemaining(kStartingReserveBallsNormal),
      bricksBrokenTowardReserveReward(0),
      minBallSpeed(kBaseMinBallSpeed), maxBallSpeed(kBaseMaxBallSpeed),
      basePaddleWidth(kPaddleWidthNormal),
      widePaddleSecondsRemaining(kInitialPowerUpSeconds),
      spaceHeldLastFrame(kSpaceKeyInitiallyHeld) {
  for (int keyIndex = 0; keyIndex <= GLFW_KEY_LAST; keyIndex++) {
    wasKeyDownLastFrame[keyIndex] = false;
  }
}

float RandomFloat(float minValue, float maxValue) {
  if (maxValue < minValue) {
    std::swap(minValue, maxValue);
  }
  std::uniform_real_distribution<float> distribution(minValue, maxValue);
  return distribution(GlobalRng());
}

int RandomInt(int minValue, int maxValue) {
  if (maxValue < minValue) {
    std::swap(minValue, maxValue);
  }
  std::uniform_int_distribution<int> distribution(minValue, maxValue);
  return distribution(GlobalRng());
}

float KeepFloatInRange(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

int KeepIntInRange(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float StepScaleFromDeltaSeconds(float deltaSeconds) {
  if (deltaSeconds <= 0.0f) {
    return 0.0f;
  }
  return deltaSeconds / kSimulationStepSeconds;
}

const char *DifficultyName(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, "Easy", "Normal", "Hard");
}

// Difficulty tuning rules:
// Easy   = more help (slower speed, wider paddle, bigger ball, more lives)
// Normal = middle values
// Hard   = less help (faster speed, narrower paddle, smaller ball, fewer lives)
float DifficultySpeedMultiplier(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, kSpeedMultiplierEasy,
                            kSpeedMultiplierNormal, kSpeedMultiplierHard);
}

float DifficultyPaddleWidth(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, kPaddleWidthEasy, kPaddleWidthNormal,
                            kPaddleWidthHard);
}

int DifficultyLives(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, kLivesEasy, kLivesNormal, kLivesHard);
}

float DifficultyBallSize(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, kBallSizeEasy, kBallSizeNormal,
                            kBallSizeHard);
}

int DifficultyStartingReserveBalls(DifficultyMode difficulty) {
  return ValueForDifficulty(difficulty, kStartingReserveBallsEasy,
                            kStartingReserveBallsNormal,
                            kStartingReserveBallsHard);
}

void RefreshDifficultyAndLevel(GameState &state) {
  // Recompute runtime speed/paddle values any time difficulty or level changes.
  state.basePaddleWidth = DifficultyPaddleWidth(state.difficulty);

  float levelScale = 1.0f + (kLevelSpeedIncrease *
                             static_cast<float>(state.level - kInitialLevel));
  float difficultyScale = DifficultySpeedMultiplier(state.difficulty);

  state.minBallSpeed = kBaseMinBallSpeed * difficultyScale * levelScale;
  state.maxBallSpeed = kBaseMaxBallSpeed * difficultyScale * levelScale;

  // Keep a minimum gap so min and max speeds do not collapse together.
  if (state.maxBallSpeed < (state.minBallSpeed + kRequiredSpeedGap)) {
    state.maxBallSpeed = state.minBallSpeed + kRequiredSpeedGap;
  }
}
