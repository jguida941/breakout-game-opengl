#include "GameFlowSystems.h"

#include <iostream>

using std::cout;
using std::endl;
using std::vector;

namespace {
constexpr float kNoBallSpawnXOffset = 0.0f;
constexpr int kLevelAdvanceScoreBase = 400;
constexpr int kLevelAdvanceScorePerLevel = 60;
constexpr int kLevelAdvanceReserveBallReward = 1;
} // namespace

bool AreAllBreakableBricksGone(const vector<Brick> &bricks) {
  for (const Brick &brick : bricks) {
    if (brick.IsVisible() && (brick.type == DESTRUCTIBLE)) {
      return false;
    }
  }
  return true;
}

void BeginNextLevel(GameSession &session, vector<Brick> &bricks, Brick &paddle) {
  // Carry progress forward, but clear temporary entities/effects.
  session.state.level++;
  session.state.widePaddleSecondsRemaining = 0.0f;

  RefreshDifficultyAndLevel(session.state);
  BuildLevelBricks(bricks, session.state.level);
  session.powerUps.clear();
  session.balls.clear();

  paddle.width = session.state.basePaddleWidth;
  KeepPaddleOnScreen(paddle);
  AddBallFromPaddle(session, paddle, kNoBallSpawnXOffset);

  session.state.score +=
      kLevelAdvanceScoreBase + (session.state.level * kLevelAdvanceScorePerLevel);
  session.state.reserveBallsRemaining =
      KeepIntInRange(session.state.reserveBallsRemaining +
                         kLevelAdvanceReserveBallReward,
                     0, kReserveBallsMax);
  cout << "Level " << session.state.level << " started." << endl;
  cout << "Reserve balls +" << kLevelAdvanceReserveBallReward << " ("
       << session.state.reserveBallsRemaining << " available)." << endl;
}

void HandleLifeLoss(GameSession &session, Brick &paddle) {
  // Keep life handling centralized so menu/game-over transitions stay consistent.
  session.state.livesRemaining--;
  if (session.state.livesRemaining <= 0) {
    session.state.livesRemaining = 0;
    session.state.screen = SCREEN_GAME_OVER;
    cout << "Game Over! Press R to restart, M for menu, or ESC for menu."
         << endl;
    return;
  }

  session.powerUps.clear();
  session.state.widePaddleSecondsRemaining = 0.0f;
  paddle.width = session.state.basePaddleWidth;
  KeepPaddleOnScreen(paddle);
  AddBallFromPaddle(session, paddle, kNoBallSpawnXOffset);
  cout << "Life lost. " << session.state.livesRemaining << " lives left."
       << endl;
}
