#pragma once

#include "GameSettings.h"

class Brick {
public:
  float colorRed;
  float colorGreen;
  float colorBlue;

  float baseColorRed;
  float baseColorGreen;
  float baseColorBlue;

  float centerX;
  float centerY;
  float width;
  float height;

  BrickType type;
  bool isVisible;
  int maxHits;
  int hitsRemaining;

  Brick();
  Brick(BrickType brickType, float initialCenterX, float initialCenterY,
        float brickWidth, float brickHeight, int requiredHits,
        float initialRed, float initialGreen, float initialBlue);

  bool IsVisible() const;
  float LeftEdge() const;
  float RightEdge() const;
  float TopEdge() const;
  float BottomEdge() const;

  bool HandleHit();
  void Draw() const;
};

class Circle {
public:
  // Render color channels.
  float colorRed;
  float colorGreen;
  float colorBlue;

  // Position/radius in world coordinates.
  float radius;
  float centerX;
  float centerY;

  // Velocity in world units per simulation tick.
  float velocityX;
  float velocityY;

  Circle(float initialCenterX, float initialCenterY, float ballRadius,
         float initialVelocityX, float initialVelocityY, float initialRed,
         float initialGreen, float initialBlue);

  void SetRandomColor();
  void KeepSpeedInRange(const GameState &state);
  void ApplyCollisionFriction(const GameState &state);
  void UpdateMotion(const GameState &state, float deltaSeconds);
  void Draw() const;
};

class PowerUp {
public:
  PowerUpType type;

  // Position and size in world coordinates.
  float centerX;
  float centerY;
  float size;

  // Vertical speed in world units per simulation tick.
  float fallSpeed;
  bool isVisible;

  PowerUp(PowerUpType initialType, float initialX, float initialY,
          float initialSize, float initialFallSpeed);

  float LeftEdge() const;
  float RightEdge() const;
  float TopEdge() const;
  float BottomEdge() const;
  void Draw() const;
};
