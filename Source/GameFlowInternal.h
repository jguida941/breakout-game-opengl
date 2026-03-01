#pragma once

#include "GameFlow.h"

// UI/console messaging shared across modules.
void ShowSettingsMessage();
void ShowControlsMessage();
void ShowGameplayMessage(const GameSession &session);

// Gameplay helpers shared across modules.
void KeepPaddleOnScreen(Brick &paddle);
void AddBallFromPaddle(GameSession &session, const Brick &paddle,
                       float xOffset);
void StartNewRun(GameSession &session, std::vector<Brick> &bricks,
                 Brick &paddle);
