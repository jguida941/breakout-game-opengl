#include "GameFlowSystems.h"

#include <cmath>

using std::size_t;
using std::vector;

namespace {
constexpr float kPaddleHitNormalizationEpsilon = 0.0001f;
constexpr float kPaddleMaxBounceAngleDegrees = 68.0f;
constexpr float kPaddleCollisionEpsilon = 0.001f;

constexpr float kBallPairDistanceEpsilonSquared = 0.00001f;
constexpr float kBallPairDistanceFallbackX = 0.001f;
constexpr float kBallPairDistanceFallbackY = 0.0f;
constexpr float kBallPairOverlapPadding = 0.001f;

constexpr int kBallCollisionOutcomeCount = 4;
constexpr int kBallCollisionMaxActiveBalls = 24;
constexpr float kMergeRadiusScale = 0.75f;
constexpr float kMergeRadiusMin = 0.03f;
constexpr float kMergeRadiusMax = 0.14f;
constexpr int kSplitSpawnCount = 3;
constexpr float kSplitRadiusScale = 0.55f;
constexpr float kSplitRadiusMin = 0.02f;
constexpr float kSplitRadiusMax = 0.05f;
constexpr float kSplitSpeedScale = 1.10f;
constexpr float kSplitLaunchMinAngleDegrees = 20.0f;
constexpr float kSplitLaunchMaxAngleDegrees = 340.0f;

enum BallCollisionOutcome {
  BALL_COLLISION_MERGE = 0,
  BALL_COLLISION_RECOLOR = 1,
  BALL_COLLISION_DISAPPEAR = 2,
  BALL_COLLISION_SPLIT = 3
};

float BallSpeed(const Circle &ball) {
  return sqrtf((ball.velocityX * ball.velocityX) +
               (ball.velocityY * ball.velocityY));
}

Circle BuildMergedBall(const Circle &firstBall, const Circle &secondBall,
                       const GameState &state) {
  float mergedCenterX = (firstBall.centerX + secondBall.centerX) * 0.5f;
  float mergedCenterY = (firstBall.centerY + secondBall.centerY) * 0.5f;
  float mergedRadius = KeepFloatInRange(
      (firstBall.radius + secondBall.radius) * kMergeRadiusScale,
      kMergeRadiusMin, kMergeRadiusMax);
  float mergedVelocityX = (firstBall.velocityX + secondBall.velocityX) * 0.5f;
  float mergedVelocityY = (firstBall.velocityY + secondBall.velocityY) * 0.5f;
  float mergedColorRed = KeepFloatInRange(
      (firstBall.colorRed + secondBall.colorRed) * 0.5f, 0.2f, 1.0f);
  float mergedColorGreen = KeepFloatInRange(
      (firstBall.colorGreen + secondBall.colorGreen) * 0.5f, 0.2f, 1.0f);
  float mergedColorBlue = KeepFloatInRange(
      (firstBall.colorBlue + secondBall.colorBlue) * 0.5f, 0.2f, 1.0f);

  Circle mergedBall(mergedCenterX, mergedCenterY, mergedRadius, mergedVelocityX,
                    mergedVelocityY, mergedColorRed, mergedColorGreen,
                    mergedColorBlue);
  mergedBall.KeepSpeedInRange(state);
  return mergedBall;
}

void ApplyBallBounceAndRecolor(Circle &firstBall, Circle &secondBall,
                               const GameState &state, float normalX,
                               float normalY, float overlap) {
  firstBall.centerX -= normalX * overlap;
  firstBall.centerY -= normalY * overlap;
  secondBall.centerX += normalX * overlap;
  secondBall.centerY += normalY * overlap;

  float relativeVelocity =
      (secondBall.velocityX - firstBall.velocityX) * normalX +
      (secondBall.velocityY - firstBall.velocityY) * normalY;
  if (relativeVelocity < 0.0f) {
    float impulse = -relativeVelocity;
    firstBall.velocityX -= impulse * normalX;
    firstBall.velocityY -= impulse * normalY;
    secondBall.velocityX += impulse * normalX;
    secondBall.velocityY += impulse * normalY;
  }

  firstBall.SetRandomColor();
  secondBall.SetRandomColor();
  firstBall.KeepSpeedInRange(state);
  secondBall.KeepSpeedInRange(state);
}

void AppendSplitBalls(vector<Circle> &balls, const Circle &firstBall,
                      const Circle &secondBall, const GameState &state) {
  if (static_cast<int>(balls.size()) >= kBallCollisionMaxActiveBalls) {
    return;
  }

  float spawnCenterX = (firstBall.centerX + secondBall.centerX) * 0.5f;
  float spawnCenterY = (firstBall.centerY + secondBall.centerY) * 0.5f;
  float spawnRadius =
      KeepFloatInRange(((firstBall.radius + secondBall.radius) * 0.5f) *
                           kSplitRadiusScale,
                       kSplitRadiusMin, kSplitRadiusMax);
  float spawnSpeed =
      KeepFloatInRange(((BallSpeed(firstBall) + BallSpeed(secondBall)) * 0.5f) *
                           kSplitSpeedScale,
                       state.minBallSpeed, state.maxBallSpeed);

  for (int spawnIndex = 0; spawnIndex < kSplitSpawnCount; spawnIndex++) {
    if (static_cast<int>(balls.size()) >= kBallCollisionMaxActiveBalls) {
      break;
    }

    float launchAngle = RandomFloat(kSplitLaunchMinAngleDegrees,
                                    kSplitLaunchMaxAngleDegrees) *
                        kDegreesToRadians;
    Circle newBall(
        spawnCenterX, spawnCenterY, spawnRadius, cosf(launchAngle) * spawnSpeed,
        sinf(launchAngle) * spawnSpeed, RandomFloat(0.2f, 1.0f),
        RandomFloat(0.2f, 1.0f), RandomFloat(0.2f, 1.0f));
    newBall.KeepSpeedInRange(state);
    balls.push_back(newBall);
  }
}
} // namespace

bool HandleBallBrickCollision(Circle &ball, Brick &brick,
                              const GameState &state,
                              bool *outBrickDestroyed) {
  if (outBrickDestroyed != nullptr) {
    *outBrickDestroyed = false;
  }

  if (!brick.IsVisible()) {
    return false;
  }

  float closestPointX =
      KeepFloatInRange(ball.centerX, brick.LeftEdge(), brick.RightEdge());
  float closestPointY =
      KeepFloatInRange(ball.centerY, brick.BottomEdge(), brick.TopEdge());

  float deltaX = ball.centerX - closestPointX;
  float deltaY = ball.centerY - closestPointY;
  float distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
  if (distanceSquared > (ball.radius * ball.radius)) {
    return false;
  }

  // Use axis reflection based on dominant penetration direction.
  if (fabsf(deltaX) > fabsf(deltaY)) {
    ball.velocityX = -ball.velocityX;
  } else {
    ball.velocityY = -ball.velocityY;
  }

  ball.ApplyCollisionFriction(state);
  ball.centerX += ball.velocityX;
  ball.centerY += ball.velocityY;

  bool destroyedNow = brick.HandleHit();
  if (outBrickDestroyed != nullptr) {
    *outBrickDestroyed = destroyedNow;
  }

  return true;
}

bool HandleBallPaddleCollision(Circle &ball, const Brick &paddle,
                               const GameState &state) {
  float closestPointX =
      KeepFloatInRange(ball.centerX, paddle.LeftEdge(), paddle.RightEdge());
  float closestPointY =
      KeepFloatInRange(ball.centerY, paddle.BottomEdge(), paddle.TopEdge());

  float deltaX = ball.centerX - closestPointX;
  float deltaY = ball.centerY - closestPointY;
  float distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
  if (distanceSquared > (ball.radius * ball.radius)) {
    return false;
  }

  float speed = sqrtf((ball.velocityX * ball.velocityX) +
                      (ball.velocityY * ball.velocityY));
  speed = KeepFloatInRange(speed, state.minBallSpeed, state.maxBallSpeed);

  float halfPaddleWidth = paddle.width * 0.5f;
  float hitOffset = 0.0f;
  if (halfPaddleWidth > kPaddleHitNormalizationEpsilon) {
    hitOffset = (ball.centerX - paddle.centerX) / halfPaddleWidth;
  }
  hitOffset = KeepFloatInRange(hitOffset, -1.0f, 1.0f);

  float maxBounceAngle = kPaddleMaxBounceAngleDegrees * kDegreesToRadians;
  float bounceAngle = hitOffset * maxBounceAngle;
  ball.velocityX = speed * sinf(bounceAngle);
  ball.velocityY = fabsf(speed * cosf(bounceAngle));

  // Nudge above paddle to avoid repeat overlap in the next frame.
  ball.centerY = paddle.TopEdge() + ball.radius + kPaddleCollisionEpsilon;
  ball.ApplyCollisionFriction(state);
  return true;
}

void HandleBallBallCollisions(vector<Circle> &balls, const GameState &state) {
  for (size_t firstIndex = 0; firstIndex < balls.size();) {
    bool removedFirstBall = false;
    for (size_t secondIndex = firstIndex + 1; secondIndex < balls.size();) {
      float separationX = balls[secondIndex].centerX - balls[firstIndex].centerX;
      float separationY = balls[secondIndex].centerY - balls[firstIndex].centerY;
      float minDistance = balls[firstIndex].radius + balls[secondIndex].radius;
      float distanceSquared =
          (separationX * separationX) + (separationY * separationY);

      if (distanceSquared > (minDistance * minDistance)) {
        secondIndex++;
        continue;
      }

      if (distanceSquared < kBallPairDistanceEpsilonSquared) {
        separationX = kBallPairDistanceFallbackX;
        separationY = kBallPairDistanceFallbackY;
        distanceSquared =
            (separationX * separationX) + (separationY * separationY);
      }

      float distance = sqrtf(distanceSquared);
      float normalX = separationX / distance;
      float normalY = separationY / distance;

      float overlap = 0.5f * (minDistance - distance + kBallPairOverlapPadding);
      BallCollisionOutcome outcome = static_cast<BallCollisionOutcome>(
          RandomInt(0, kBallCollisionOutcomeCount - 1));

      // Outcome 1: merge both into one larger ball.
      if (outcome == BALL_COLLISION_MERGE) {
        Circle mergedBall = BuildMergedBall(balls[firstIndex], balls[secondIndex],
                                            state);
        balls[firstIndex] = mergedBall;
        balls.erase(balls.begin() + secondIndex);
        continue;
      }

      // Outcome 2: remove both balls.
      if (outcome == BALL_COLLISION_DISAPPEAR) {
        balls.erase(balls.begin() + secondIndex);
        balls.erase(balls.begin() + firstIndex);
        removedFirstBall = true;
        break;
      }

      // Outcome 3: remove both, then spawn several smaller balls.
      if (outcome == BALL_COLLISION_SPLIT) {
        Circle firstBallSnapshot = balls[firstIndex];
        Circle secondBallSnapshot = balls[secondIndex];
        balls.erase(balls.begin() + secondIndex);
        balls.erase(balls.begin() + firstIndex);
        AppendSplitBalls(balls, firstBallSnapshot, secondBallSnapshot, state);
        removedFirstBall = true;
        break;
      }

      // Outcome 4: default bounce behavior with recolor feedback.
      ApplyBallBounceAndRecolor(balls[firstIndex], balls[secondIndex], state,
                                normalX, normalY, overlap);
      secondIndex++;
    }

    if (!removedFirstBall) {
      firstIndex++;
    }
  }
}
