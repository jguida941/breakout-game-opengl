#include "GameObjects.h"

#include <cmath>

namespace {
// Shared draw/physics tuning constants for object classes.
constexpr float kDefaultColorChannel = 1.0f;
constexpr float kDefaultBrickSize = 0.1f;
constexpr int kMinimumRequiredHits = 1;

constexpr float kHitTintRedIncrease = 0.55f;
constexpr float kHitTintGreenScale = 0.60f;
constexpr float kHitTintBlueScale = 0.35f;
constexpr float kReflectiveHitRedIncrease = 0.08f;
constexpr float kReflectiveHitGreenIncrease = 0.05f;
constexpr float kReflectiveHitBlueIncrease = 0.02f;
constexpr float kColorChannelMin = 0.0f;
constexpr float kColorChannelMax = 1.0f;

constexpr float kOutlineLineWidth = 1.5f;
constexpr float kOutlineColorRed = 0.08f;
constexpr float kOutlineColorGreen = 0.08f;
constexpr float kOutlineColorBlue = 0.08f;

constexpr float kRandomBallColorMin = 0.2f;
constexpr float kRandomBallColorMax = 1.0f;
constexpr float kSpeedNearlyZeroThreshold = 0.0001f;
constexpr float kFallbackAngleMinDegrees = 20.0f;
constexpr float kFallbackAngleMaxDegrees = 160.0f;
constexpr float kCollisionFrictionMultiplier = 0.98f;
constexpr int kCircleSegments = 48;
constexpr float kFullCircleDegrees = 360.0f;

constexpr float kPowerUpColorMultiBallRed = 0.30f;
constexpr float kPowerUpColorMultiBallGreen = 0.86f;
constexpr float kPowerUpColorMultiBallBlue = 0.94f;
constexpr float kPowerUpColorWidePaddleRed = 0.96f;
constexpr float kPowerUpColorWidePaddleGreen = 0.92f;
constexpr float kPowerUpColorWidePaddleBlue = 0.24f;
constexpr float kPowerUpColorExtraLifeRed = 0.40f;
constexpr float kPowerUpColorExtraLifeGreen = 0.94f;
constexpr float kPowerUpColorExtraLifeBlue = 0.48f;

struct UnitCirclePoint {
  float x;
  float y;
};

void DrawFilledAxisAlignedRect(float leftEdge, float rightEdge,
                               float bottomEdge, float topEdge) {
  // Shared quad renderer for bricks and power-ups.
  glBegin(GL_POLYGON);
  glVertex2f(rightEdge, topEdge);
  glVertex2f(rightEdge, bottomEdge);
  glVertex2f(leftEdge, bottomEdge);
  glVertex2f(leftEdge, topEdge);
  glEnd();
}

void DrawRectOutline(float leftEdge, float rightEdge, float bottomEdge,
                     float topEdge) {
  glColor3f(kOutlineColorRed, kOutlineColorGreen, kOutlineColorBlue);
  glLineWidth(kOutlineLineWidth);
  glBegin(GL_LINE_LOOP);
  glVertex2f(rightEdge, topEdge);
  glVertex2f(rightEdge, bottomEdge);
  glVertex2f(leftEdge, bottomEdge);
  glVertex2f(leftEdge, topEdge);
  glEnd();
}

const UnitCirclePoint *GetUnitCirclePoints() {
  // Precompute once to avoid repeated trig work for each ball draw.
  static UnitCirclePoint unitCirclePoints[kCircleSegments];
  static bool isBuilt = false;
  if (!isBuilt) {
    for (int pointIndex = 0; pointIndex < kCircleSegments; pointIndex++) {
      float angleInDegrees =
          (kFullCircleDegrees * static_cast<float>(pointIndex)) /
          static_cast<float>(kCircleSegments);
      float angleInRadians = angleInDegrees * kDegreesToRadians;
      unitCirclePoints[pointIndex].x = cosf(angleInRadians);
      unitCirclePoints[pointIndex].y = sinf(angleInRadians);
    }
    isBuilt = true;
  }
  return unitCirclePoints;
}
} // namespace

Brick::Brick()
    : colorRed(kDefaultColorChannel), colorGreen(kDefaultColorChannel),
      colorBlue(kDefaultColorChannel), baseColorRed(kDefaultColorChannel),
      baseColorGreen(kDefaultColorChannel), baseColorBlue(kDefaultColorChannel),
      centerX(0.0f), centerY(0.0f), width(kDefaultBrickSize),
      height(kDefaultBrickSize), type(REFLECTIVE), isVisible(true),
      maxHits(kMinimumRequiredHits), hitsRemaining(kMinimumRequiredHits) {}

Brick::Brick(BrickType brickType, float initialCenterX, float initialCenterY,
             float brickWidth, float brickHeight, int requiredHits,
             float initialRed, float initialGreen, float initialBlue)
    : colorRed(initialRed), colorGreen(initialGreen), colorBlue(initialBlue),
      baseColorRed(initialRed), baseColorGreen(initialGreen),
      baseColorBlue(initialBlue), centerX(initialCenterX),
      centerY(initialCenterY), width(brickWidth), height(brickHeight),
      type(brickType), isVisible(true) {
  maxHits = requiredHits;
  if (maxHits < kMinimumRequiredHits) {
    maxHits = kMinimumRequiredHits;
  }
  hitsRemaining = maxHits;
}

bool Brick::IsVisible() const { return isVisible; }

float Brick::LeftEdge() const { return centerX - (width * 0.5f); }

float Brick::RightEdge() const { return centerX + (width * 0.5f); }

float Brick::TopEdge() const { return centerY + (height * 0.5f); }

float Brick::BottomEdge() const { return centerY - (height * 0.5f); }

bool Brick::HandleHit() {
  if (!isVisible) {
    return false;
  }

  if (type == DESTRUCTIBLE) {
    hitsRemaining--;
    if (hitsRemaining < 0) {
      hitsRemaining = 0;
    }

    // Darken/shift color as damage feedback before brick disappears.
    float hitRatio = 1.0f - (static_cast<float>(hitsRemaining) /
                             static_cast<float>(maxHits));
    colorRed = KeepFloatInRange(baseColorRed + (kHitTintRedIncrease * hitRatio),
                                kColorChannelMin, kColorChannelMax);
    colorGreen = KeepFloatInRange(baseColorGreen *
                                      (1.0f - (kHitTintGreenScale * hitRatio)),
                                  kColorChannelMin, kColorChannelMax);
    colorBlue = KeepFloatInRange(baseColorBlue *
                                     (1.0f - (kHitTintBlueScale * hitRatio)),
                                 kColorChannelMin, kColorChannelMax);

    if (hitsRemaining <= 0) {
      isVisible = false;
      return true;
    }
  } else {
    colorRed = KeepFloatInRange(colorRed + kReflectiveHitRedIncrease,
                                kColorChannelMin, kColorChannelMax);
    colorGreen = KeepFloatInRange(colorGreen + kReflectiveHitGreenIncrease,
                                  kColorChannelMin, kColorChannelMax);
    colorBlue = KeepFloatInRange(colorBlue + kReflectiveHitBlueIncrease,
                                 kColorChannelMin, kColorChannelMax);
  }

  return false;
}

void Brick::Draw() const {
  if (!isVisible) {
    return;
  }

  glColor3f(colorRed, colorGreen, colorBlue);
  DrawFilledAxisAlignedRect(LeftEdge(), RightEdge(), BottomEdge(), TopEdge());
  DrawRectOutline(LeftEdge(), RightEdge(), BottomEdge(), TopEdge());
}

Circle::Circle(float initialCenterX, float initialCenterY, float ballRadius,
               float initialVelocityX, float initialVelocityY, float initialRed,
               float initialGreen, float initialBlue)
    : colorRed(initialRed), colorGreen(initialGreen), colorBlue(initialBlue),
      radius(ballRadius), centerX(initialCenterX), centerY(initialCenterY),
      velocityX(initialVelocityX), velocityY(initialVelocityY) {}

void Circle::SetRandomColor() {
  colorRed = RandomFloat(kRandomBallColorMin, kRandomBallColorMax);
  colorGreen = RandomFloat(kRandomBallColorMin, kRandomBallColorMax);
  colorBlue = RandomFloat(kRandomBallColorMin, kRandomBallColorMax);
}

void Circle::KeepSpeedInRange(const GameState &state) {
  float speed = sqrtf((velocityX * velocityX) + (velocityY * velocityY));
  if (speed <= kSpeedNearlyZeroThreshold) {
    // If velocity is near zero, relaunch at a safe random angle.
    float randomAngle =
        RandomFloat(kFallbackAngleMinDegrees, kFallbackAngleMaxDegrees) *
        kDegreesToRadians;
    velocityX = cosf(randomAngle) * state.minBallSpeed;
    velocityY = sinf(randomAngle) * state.minBallSpeed;
    return;
  }

  if (speed < state.minBallSpeed) {
    float scaleUp = state.minBallSpeed / speed;
    velocityX *= scaleUp;
    velocityY *= scaleUp;
  } else if (speed > state.maxBallSpeed) {
    float scaleDown = state.maxBallSpeed / speed;
    velocityX *= scaleDown;
    velocityY *= scaleDown;
  }
}

void Circle::ApplyCollisionFriction(const GameState &state) {
  velocityX *= kCollisionFrictionMultiplier;
  velocityY *= kCollisionFrictionMultiplier;
  KeepSpeedInRange(state);
}

void Circle::UpdateMotion(const GameState &state, float deltaSeconds) {
  float stepScale = StepScaleFromDeltaSeconds(deltaSeconds);
  centerX += velocityX * stepScale;
  centerY += velocityY * stepScale;

  bool bounced = false;

  if (centerX + radius > kWorldMax) {
    centerX = kWorldMax - radius;
    velocityX = -velocityX;
    bounced = true;
  } else if (centerX - radius < kWorldMin) {
    centerX = kWorldMin + radius;
    velocityX = -velocityX;
    bounced = true;
  }

  if (centerY + radius > kWorldMax) {
    centerY = kWorldMax - radius;
    velocityY = -velocityY;
    bounced = true;
  }

  // Bottom edge is handled by game flow (life loss), not by bouncing here.
  if (bounced) {
    ApplyCollisionFriction(state);
  }
}

void Circle::Draw() const {
  glColor3f(colorRed, colorGreen, colorBlue);
  glBegin(GL_POLYGON);
  const UnitCirclePoint *unitCirclePoints = GetUnitCirclePoints();
  for (int pointIndex = 0; pointIndex < kCircleSegments; pointIndex++) {
    glVertex2f((unitCirclePoints[pointIndex].x * radius) + centerX,
               (unitCirclePoints[pointIndex].y * radius) + centerY);
  }
  glEnd();
}

PowerUp::PowerUp(PowerUpType initialType, float initialX, float initialY,
                 float initialSize, float initialFallSpeed)
    : type(initialType), centerX(initialX), centerY(initialY),
      size(initialSize), fallSpeed(initialFallSpeed), isVisible(true) {}

float PowerUp::LeftEdge() const { return centerX - (size * 0.5f); }

float PowerUp::RightEdge() const { return centerX + (size * 0.5f); }

float PowerUp::TopEdge() const { return centerY + (size * 0.5f); }

float PowerUp::BottomEdge() const { return centerY - (size * 0.5f); }

void PowerUp::Draw() const {
  if (!isVisible) {
    return;
  }

  if (type == POWERUP_MULTIBALL) {
    // Color coding matches gameplay message text.
    glColor3f(kPowerUpColorMultiBallRed, kPowerUpColorMultiBallGreen,
              kPowerUpColorMultiBallBlue);
  } else if (type == POWERUP_WIDE_PADDLE) {
    glColor3f(kPowerUpColorWidePaddleRed, kPowerUpColorWidePaddleGreen,
              kPowerUpColorWidePaddleBlue);
  } else {
    glColor3f(kPowerUpColorExtraLifeRed, kPowerUpColorExtraLifeGreen,
              kPowerUpColorExtraLifeBlue);
  }

  DrawFilledAxisAlignedRect(LeftEdge(), RightEdge(), BottomEdge(), TopEdge());
  DrawRectOutline(LeftEdge(), RightEdge(), BottomEdge(), TopEdge());
}
