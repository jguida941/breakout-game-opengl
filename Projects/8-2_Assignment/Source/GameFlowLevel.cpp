#include "GameFlowSystems.h"

#include <cmath>

using std::vector;

namespace {
// Shared value helpers.
constexpr float kColorChannelMin = 0.0f;
constexpr float kColorChannelMax = 1.0f;
constexpr float kNoBallSpawnXOffset = 0.0f;

// Start values for a full run reset.
constexpr int kInitialLevel = 1;
constexpr int kInitialScore = 0;

// Paddle defaults. Keeping these together makes balancing easier.
constexpr float kPaddleStartY = -0.90f;
constexpr float kPaddleHeight = 0.06f;
constexpr int kPaddleRequiredHits = 1;
constexpr float kPaddleColorRed = 0.96f;
constexpr float kPaddleColorGreen = 0.96f;
constexpr float kPaddleColorBlue = 0.24f;

// Brick layout scaling.
constexpr int kMinBrickRows = 3;
constexpr int kMaxBrickRows = 6;
constexpr float kTopBrickRowY = 0.80f;
constexpr float kBrickRowStepY = 0.13f;
constexpr int kEvenRowColumns = 6;
constexpr int kOddRowColumns = 5;
constexpr float kEvenRowBrickWidth = 0.24f;
constexpr float kOddRowBrickWidth = 0.28f;
constexpr float kBrickWidthShrinkPerLevel = 0.01f;
constexpr float kBrickMinWidth = 0.17f;
constexpr float kBrickMaxWidth = 0.30f;
constexpr float kBrickHeight = 0.09f;
constexpr float kBrickGap = 0.05f;
constexpr float kMinBrickRowY = 0.18f;
constexpr int kMinBrickHits = 1;
constexpr int kMaxBrickHits = 5;

// Brick color gradient.
constexpr float kBrickColorRedBase = 0.32f;
constexpr float kBrickColorRedByColumn = 0.54f;
constexpr float kBrickColorRedByRow = 0.05f;
constexpr float kBrickColorGreenBase = 0.80f;
constexpr float kBrickColorGreenByColumn = 0.42f;
constexpr float kBrickColorGreenByRow = 0.04f;
constexpr float kBrickColorBlueBase = 0.35f;
constexpr float kBrickColorBlueByColumn = 0.45f;

// White accent bricks.
constexpr int kWhiteBrickLevelThreshold = 3;
constexpr int kWhiteBrickBaseHits = 2;
constexpr float kWhiteBrickXOffset = 0.20f;
constexpr float kWhiteBrickY = 0.30f;
constexpr float kWhiteBrickWidth = 0.24f;
constexpr float kWhiteBrickHeight = 0.08f;
constexpr float kWhiteBrickColor = 0.92f;
constexpr int kWhiteBrickLevelBonusHits = 1;

// Ball launch tuning.
constexpr float kLaunchAngleMinDegrees = 30.0f;
constexpr float kLaunchAngleMaxDegrees = 150.0f;
constexpr float kLaunchSpeedMinMultiplier = 1.10f;
constexpr float kLaunchSpeedMaxMultiplier = 1.60f;
constexpr float kLaunchSpeedMaxCeilingMultiplier = 0.92f;
constexpr float kLaunchMinVerticalSpeed = 0.006f;
constexpr float kBallColorMin = 0.2f;
constexpr float kBallColorMax = 1.0f;

Circle MakeBallForLaunch(const GameSession &session, float spawnX,
                         float spawnY) {
  // Launch mostly upward so early hits are predictable.
  float launchAngle =
      RandomFloat(kLaunchAngleMinDegrees, kLaunchAngleMaxDegrees) *
      kDegreesToRadians;
  float launchSpeed =
      RandomFloat(session.state.minBallSpeed * kLaunchSpeedMinMultiplier,
                  session.state.minBallSpeed * kLaunchSpeedMaxMultiplier);
  if (launchSpeed >
      (session.state.maxBallSpeed * kLaunchSpeedMaxCeilingMultiplier)) {
    launchSpeed = session.state.maxBallSpeed * kLaunchSpeedMaxCeilingMultiplier;
  }

  float velocityX = cosf(launchAngle) * launchSpeed;
  float velocityY = sinf(launchAngle) * launchSpeed;
  if (fabsf(velocityY) < kLaunchMinVerticalSpeed) {
    velocityY = kLaunchMinVerticalSpeed;
  }

  return Circle(spawnX, spawnY, DifficultyBallSize(session.state.difficulty),
                velocityX, velocityY, RandomFloat(kBallColorMin, kBallColorMax),
                RandomFloat(kBallColorMin, kBallColorMax),
                RandomFloat(kBallColorMin, kBallColorMax));
}

void ConfigurePaddle(const GameSession &session, Brick &paddle, float centerX) {
  paddle = CreateDefaultPaddle(session, centerX);
}

void ResetInputLatches(GameSession &session) {
  // Prevent stale key-edge events when switching screens.
  for (int key = 0; key <= GLFW_KEY_LAST; key++) {
    session.state.wasKeyDownLastFrame[key] = false;
  }
}
} // namespace

Brick CreateDefaultPaddle(const GameSession &session, float centerX) {
  return Brick(REFLECTIVE, centerX, kPaddleStartY,
               session.state.basePaddleWidth, kPaddleHeight,
               kPaddleRequiredHits, kPaddleColorRed, kPaddleColorGreen,
               kPaddleColorBlue);
}

void BuildLevelBricks(vector<Brick> &bricks, int level) {
  bricks.clear();

  int rowCount = kMinBrickRows + (level - kInitialLevel);
  rowCount = KeepIntInRange(rowCount, kMinBrickRows, kMaxBrickRows);

  for (int row = 0; row < rowCount; row++) {
    int columns = ((row % 2) == 0) ? kEvenRowColumns : kOddRowColumns;
    float baseWidth =
        (columns == kEvenRowColumns) ? kEvenRowBrickWidth : kOddRowBrickWidth;
    float brickWidth = baseWidth -
                       (kBrickWidthShrinkPerLevel *
                        static_cast<float>(level - kInitialLevel));
    brickWidth = KeepFloatInRange(brickWidth, kBrickMinWidth, kBrickMaxWidth);

    float y = kTopBrickRowY - (static_cast<float>(row) * kBrickRowStepY);
    if (y < kMinBrickRowY) {
      break;
    }

    float totalWidth = (columns * brickWidth) + ((columns - 1) * kBrickGap);
    float startX = (-0.5f * totalWidth) + (brickWidth * 0.5f);

    for (int column = 0; column < columns; column++) {
      float x = startX + (column * (brickWidth + kBrickGap));
      int hitsToBreak =
          kMinBrickHits + (row / 2) + ((level - kInitialLevel) / 2);
      hitsToBreak = KeepIntInRange(hitsToBreak, kMinBrickHits, kMaxBrickHits);

      float colorStep =
          (columns <= 1)
              ? 0.0f
              : (static_cast<float>(column) / static_cast<float>(columns - 1));
      float red = KeepFloatInRange(kBrickColorRedBase +
                                       (kBrickColorRedByColumn * colorStep) +
                                       (kBrickColorRedByRow * row),
                                   kColorChannelMin, kColorChannelMax);
      float green = KeepFloatInRange(
          kBrickColorGreenBase - (kBrickColorGreenByColumn * colorStep) -
              (kBrickColorGreenByRow * row),
          kColorChannelMin, kColorChannelMax);
      float blue = KeepFloatInRange(
          kBrickColorBlueBase + (kBrickColorBlueByColumn * (1.0f - colorStep)),
          kColorChannelMin, kColorChannelMax);

      bricks.push_back(Brick(DESTRUCTIBLE, x, y, brickWidth, kBrickHeight,
                             hitsToBreak, red, green, blue));
    }
  }

  int whiteBrickHits =
      kWhiteBrickBaseHits +
      ((level >= kWhiteBrickLevelThreshold) ? kWhiteBrickLevelBonusHits : 0);
  bricks.push_back(Brick(DESTRUCTIBLE, -kWhiteBrickXOffset, kWhiteBrickY,
                         kWhiteBrickWidth, kWhiteBrickHeight, whiteBrickHits,
                         kWhiteBrickColor, kWhiteBrickColor, kWhiteBrickColor));
  bricks.push_back(Brick(DESTRUCTIBLE, kWhiteBrickXOffset, kWhiteBrickY,
                         kWhiteBrickWidth, kWhiteBrickHeight, whiteBrickHits,
                         kWhiteBrickColor, kWhiteBrickColor, kWhiteBrickColor));
}

void KeepPaddleOnScreen(Brick &paddle) {
  float halfWidth = paddle.width * 0.5f;
  paddle.centerX =
      KeepFloatInRange(paddle.centerX, kWorldMin + halfWidth, kWorldMax - halfWidth);
}

void AddBallFromPaddle(GameSession &session, const Brick &paddle,
                       float xOffset) {
  session.balls.push_back(MakeBallForLaunch(
      session, paddle.centerX + xOffset,
      paddle.centerY + kBallSpawnHeightAbovePaddle));
}

void StartNewRun(GameSession &session, vector<Brick> &bricks, Brick &paddle) {
  // A full restart should be deterministic except for ball randomness.
  session.state.level = kInitialLevel;
  session.state.score = kInitialScore;
  session.state.livesRemaining = DifficultyLives(session.state.difficulty);
  session.state.reserveBallsRemaining =
      DifficultyStartingReserveBalls(session.state.difficulty);
  session.state.bricksBrokenTowardReserveReward = 0;
  session.state.widePaddleSecondsRemaining = 0.0f;

  session.balls.clear();
  session.powerUps.clear();

  RefreshDifficultyAndLevel(session.state);
  ConfigurePaddle(session, paddle, kNoBallSpawnXOffset);
  BuildLevelBricks(bricks, session.state.level);

  AddBallFromPaddle(session, paddle, kNoBallSpawnXOffset);
  session.state.screen = SCREEN_PLAYING;
  session.state.spaceHeldLastFrame = false;
  ResetInputLatches(session);

  ShowGameplayMessage(session);
}
