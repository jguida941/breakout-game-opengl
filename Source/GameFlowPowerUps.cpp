#include "GameFlowSystems.h"

#include <iostream>

using std::cout;
using std::endl;
using std::size_t;

namespace {
constexpr int kMaxLives = 9;
constexpr int kInitialLevel = 1;

constexpr int kPowerDropChancePercentNormal = 19;
constexpr int kPowerDropChancePercentEasy = 26;
constexpr int kPowerDropChancePercentHard = 14;
constexpr int kPowerDropLevelBonusCap = 8;
constexpr int kPowerDropChancePercentMin = 8;
constexpr int kPowerDropChancePercentMax = 40;
constexpr int kPercentRollRange = 100;

constexpr int kPowerUpTypeCount = 3;
constexpr float kPowerUpFallSpeedPerTickBase = 0.0065f;
constexpr float kPowerUpFallSpeedPerTickPerLevel = 0.0005f;
constexpr float kPowerUpFallSpeedPerTickHardBonus = 0.0015f;
constexpr float kPowerUpSize = 0.055f;

constexpr float kMultiBallSpawnOffset = 0.06f;
constexpr int kMultiBallScoreBonus = 120;
constexpr float kWidePaddleScale = 1.45f;
constexpr float kWidePaddleMinWidth = 0.30f;
constexpr float kWidePaddleMaxWidth = 0.80f;
constexpr int kPowerUpBaseScoreBonus = 100;

int PowerUpDropChancePercent(const GameSession &session) {
  int chance = kPowerDropChancePercentNormal;
  if (session.state.difficulty == DIFFICULTY_EASY) {
    chance = kPowerDropChancePercentEasy;
  } else if (session.state.difficulty == DIFFICULTY_HARD) {
    chance = kPowerDropChancePercentHard;
  }

  chance += KeepIntInRange(session.state.level - kInitialLevel, 0,
                           kPowerDropLevelBonusCap);
  return KeepIntInRange(chance, kPowerDropChancePercentMin,
                        kPowerDropChancePercentMax);
}

void ApplyPowerUp(GameSession &session, const PowerUp &powerUp, Brick &paddle) {
  if (powerUp.type == POWERUP_MULTIBALL) {
    AddBallFromPaddle(session, paddle, -kMultiBallSpawnOffset);
    AddBallFromPaddle(session, paddle, kMultiBallSpawnOffset);
    session.state.score += kMultiBallScoreBonus;
    cout << "Power-up: Multi-ball!" << endl;
    return;
  }

  if (powerUp.type == POWERUP_WIDE_PADDLE) {
    session.state.widePaddleSecondsRemaining = kWidePaddleDurationSeconds;
    paddle.width =
        KeepFloatInRange(session.state.basePaddleWidth * kWidePaddleScale,
                         kWidePaddleMinWidth, kWidePaddleMaxWidth);
    KeepPaddleOnScreen(paddle);
    session.state.score += kPowerUpBaseScoreBonus;
    cout << "Power-up: Wide paddle!" << endl;
    return;
  }

  if (session.state.livesRemaining < kMaxLives) {
    session.state.livesRemaining++;
  }
  session.state.score += kPowerUpBaseScoreBonus;
  cout << "Power-up: Extra life!" << endl;
}

bool PowerUpTouchesPaddle(const PowerUp &powerUp, const Brick &paddle) {
  float halfSize = powerUp.size * 0.5f;
  bool overlapX = (powerUp.centerX + halfSize > paddle.LeftEdge()) &&
                  (powerUp.centerX - halfSize < paddle.RightEdge());
  bool overlapY = (powerUp.centerY + halfSize > paddle.BottomEdge()) &&
                  (powerUp.centerY - halfSize < paddle.TopEdge());
  return overlapX && overlapY;
}
} // namespace

void UpdateTimedPowerEffects(GameSession &session, Brick &paddle,
                             float deltaSeconds) {
  // Tick effect timers in one place so behavior stays predictable.
  if (session.state.widePaddleSecondsRemaining <= 0.0f) {
    return;
  }

  session.state.widePaddleSecondsRemaining -= deltaSeconds;
  if (session.state.widePaddleSecondsRemaining <= 0.0f) {
    paddle.width = session.state.basePaddleWidth;
    KeepPaddleOnScreen(paddle);
    cout << "Wide paddle ended." << endl;
  }
}

void MaybeDropPowerUp(GameSession &session, float spawnX, float spawnY) {
  if (RandomInt(0, kPercentRollRange - 1) >=
      PowerUpDropChancePercent(session)) {
    return;
  }

  PowerUpType powerUpType = static_cast<PowerUpType>(
      RandomInt(0, kPowerUpTypeCount - 1));
  float fallSpeed = kPowerUpFallSpeedPerTickBase +
                    (kPowerUpFallSpeedPerTickPerLevel *
                     static_cast<float>(session.state.level - kInitialLevel));
  if (session.state.difficulty == DIFFICULTY_HARD) {
    // Hard mode reduces reaction time.
    fallSpeed += kPowerUpFallSpeedPerTickHardBonus;
  }

  session.powerUps.push_back(
      PowerUp(powerUpType, spawnX, spawnY, kPowerUpSize, fallSpeed));
}

void UpdatePowerUps(GameSession &session, Brick &paddle, float deltaSeconds) {
  // Index loop lets us erase safely in-place.
  float stepScale = StepScaleFromDeltaSeconds(deltaSeconds);
  for (size_t powerUpIndex = 0; powerUpIndex < session.powerUps.size();) {
    session.powerUps[powerUpIndex].centerY -=
        session.powerUps[powerUpIndex].fallSpeed * stepScale;

    if (PowerUpTouchesPaddle(session.powerUps[powerUpIndex], paddle)) {
      ApplyPowerUp(session, session.powerUps[powerUpIndex], paddle);
      session.powerUps.erase(session.powerUps.begin() + powerUpIndex);
      continue;
    }

    if (session.powerUps[powerUpIndex].TopEdge() < kWorldMin) {
      session.powerUps.erase(session.powerUps.begin() + powerUpIndex);
      continue;
    }

    powerUpIndex++;
  }
}
