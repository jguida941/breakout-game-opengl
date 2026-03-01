#include "GameFlowSystems.h"

#include <cstddef>
#include <iostream>

using std::cout;
using std::endl;
using std::size_t;
using std::vector;

namespace {
// Score tuning kept here because it is used only during frame updates.
constexpr int kBrickDestroyScoreBase = 100;
constexpr int kBrickDestroyScorePerLevel = 20;
constexpr int kDestroyedBricksPerReserveBallReward = 8;
constexpr int kReserveBallsPerReward = 1;
constexpr float kNoBallSpawnXOffset = 0.0f;

void RespawnAfterBallDrain(GameSession &session, Brick &paddle) {
  // All active balls are gone, but life was already charged in this frame.
  session.powerUps.clear();
  session.state.widePaddleSecondsRemaining = 0.0f;
  paddle.width = session.state.basePaddleWidth;
  KeepPaddleOnScreen(paddle);
  AddBallFromPaddle(session, paddle, kNoBallSpawnXOffset);
}

// Run one gameplay frame: move balls, resolve collisions, update power-ups.
int UpdateSimulation(GameSession &session, vector<Brick> &bricks, Brick &paddle,
                     float deltaSeconds) {
  UpdateTimedPowerEffects(session, paddle, deltaSeconds);
  int ballsLostThisFrame = 0;

  for (size_t ballIndex = 0; ballIndex < session.balls.size();) {
    session.balls[ballIndex].UpdateMotion(session.state, deltaSeconds);

    for (size_t brickIndex = 0; brickIndex < bricks.size(); brickIndex++) {
      bool brickDestroyed = false;
      if (HandleBallBrickCollision(session.balls[ballIndex], bricks[brickIndex],
                                   session.state, &brickDestroyed)) {
        if (brickDestroyed) {
          session.state.score +=
              kBrickDestroyScoreBase +
              (session.state.level * kBrickDestroyScorePerLevel);
          session.state.bricksBrokenTowardReserveReward++;
          if (session.state.bricksBrokenTowardReserveReward >=
              kDestroyedBricksPerReserveBallReward) {
            int rewardsEarned =
                session.state.bricksBrokenTowardReserveReward /
                kDestroyedBricksPerReserveBallReward;
            session.state.bricksBrokenTowardReserveReward %=
                kDestroyedBricksPerReserveBallReward;
            session.state.reserveBallsRemaining =
                KeepIntInRange(session.state.reserveBallsRemaining +
                                   (rewardsEarned * kReserveBallsPerReward),
                               0, kReserveBallsMax);
            cout << "Reserve ball earned! (" << session.state.reserveBallsRemaining
                 << " available)" << endl;
          }
          MaybeDropPowerUp(session, bricks[brickIndex].centerX,
                           bricks[brickIndex].centerY);
        }
        break;
      }
    }

    HandleBallPaddleCollision(session.balls[ballIndex], paddle, session.state);

    if (session.balls[ballIndex].centerY + session.balls[ballIndex].radius <
        kWorldMin) {
      session.balls.erase(session.balls.begin() + ballIndex);
      ballsLostThisFrame++;
      continue;
    }

    ballIndex++;
  }

  if (!session.balls.empty()) {
    HandleBallBallCollisions(session.balls, session.state);
  }

  if (ballsLostThisFrame > 0) {
    session.state.livesRemaining -= ballsLostThisFrame;
    if (session.state.livesRemaining <= 0) {
      session.state.livesRemaining = 0;
      session.state.screen = SCREEN_GAME_OVER;
      session.balls.clear();
      session.powerUps.clear();
      cout << "Game Over! Press R to restart, M for menu, or ESC for menu."
           << endl;
      return ballsLostThisFrame;
    }
    cout << ballsLostThisFrame << " ball(s) lost. "
         << session.state.livesRemaining << " lives left." << endl;
  }

  UpdatePowerUps(session, paddle, deltaSeconds);
  return ballsLostThisFrame;
}
} // namespace

void UpdatePlayState(GameSession &session, vector<Brick> &bricks,
                     Brick &paddle, float deltaSeconds) {
  if (session.state.screen != SCREEN_PLAYING) {
    return;
  }

  int ballsLostThisFrame = UpdateSimulation(session, bricks, paddle, deltaSeconds);
  if (session.state.screen != SCREEN_PLAYING) {
    return;
  }

  if (session.balls.empty()) {
    if (ballsLostThisFrame > 0) {
      RespawnAfterBallDrain(session, paddle);
    } else {
      HandleLifeLoss(session, paddle);
    }
  }

  if ((session.state.screen == SCREEN_PLAYING) &&
      AreAllBreakableBricksGone(bricks)) {
    BeginNextLevel(session, bricks, paddle);
  }
}
