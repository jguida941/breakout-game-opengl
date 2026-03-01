#pragma once

#include "GameFlowInternal.h"

// Level progression helpers.
bool AreAllBreakableBricksGone(const std::vector<Brick> &bricks);
void BeginNextLevel(GameSession &session, std::vector<Brick> &bricks,
                    Brick &paddle);
void HandleLifeLoss(GameSession &session, Brick &paddle);

// Power-up helpers.
void UpdateTimedPowerEffects(GameSession &session, Brick &paddle,
                             float deltaSeconds);
void MaybeDropPowerUp(GameSession &session, float spawnX, float spawnY);
void UpdatePowerUps(GameSession &session, Brick &paddle, float deltaSeconds);

// Collision helpers.
bool HandleBallBrickCollision(Circle &ball, Brick &brick,
                              const GameState &state,
                              bool *outBrickDestroyed);
bool HandleBallPaddleCollision(Circle &ball, const Brick &paddle,
                               const GameState &state);
void HandleBallBallCollisions(std::vector<Circle> &balls,
                              const GameState &state);
