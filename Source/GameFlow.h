#pragma once

#include "GameObjects.h"

#include <vector>

// Runtime gameplay container passed to each gameplay system.
struct GameSession {
  GameState state;
  std::vector<Circle> balls;
  std::vector<PowerUp> powerUps;
};

Brick CreateDefaultPaddle(const GameSession &session, float centerX);
void BuildLevelBricks(std::vector<Brick> &bricks, int level);
void ShowMainMenuMessage();
void ProcessInput(GLFWwindow *window, GameSession &session,
                  std::vector<Brick> &bricks, Brick &paddle,
                  float deltaSeconds);
void UpdatePlayState(GameSession &session, std::vector<Brick> &bricks,
                     Brick &paddle, float deltaSeconds);
void RenderScene(const GameSession &session, const std::vector<Brick> &bricks,
                 const Brick &paddle);
void UpdateWindowTitle(GLFWwindow *window, const GameSession &session);
