#include "GameFlowInternal.h"

#include <cstddef>
#include <iostream>

using std::cout;
using std::endl;
using std::size_t;
using std::vector;

namespace {
constexpr float kPaddleMoveStepPerSimulationTick = 0.040f;
constexpr float kNoBallSpawnXOffset = 0.0f;
const int kMenuStartKeys[] = {GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER};
const int kSettingsBackKeys[] = {GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER,
                                 GLFW_KEY_M};
const int kControlsReturnKeys[] = {GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER,
                                   GLFW_KEY_C};

bool IsMoveLeftInputDown(GLFWwindow *window) {
  return (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS);
}

bool IsMoveRightInputDown(GLFWwindow *window) {
  return (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS);
}

bool KeyPressedThisFrame(GLFWwindow *window, GameSession &session, int key) {
  // Return true only on the first frame of a press.
  if ((key < 0) || (key > GLFW_KEY_LAST)) {
    return false;
  }

  bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
  bool wasDown = session.state.wasKeyDownLastFrame[key];
  session.state.wasKeyDownLastFrame[key] = isDown;

  return isDown && !wasDown;
}

template <size_t N>
bool AnyKeyPressedThisFrame(GLFWwindow *window, GameSession &session,
                            const int (&keys)[N]) {
  bool anyPressed = false;
  for (size_t i = 0; i < N; i++) {
    if (KeyPressedThisFrame(window, session, keys[i])) {
      anyPressed = true;
    }
  }
  return anyPressed;
}

void SetScreen(GameSession &session, ScreenMode screen) {
  session.state.screen = screen;
  session.state.spaceHeldLastFrame = false;
}

void ShowScreenMessage(const GameSession &session, ScreenMode screen) {
  switch (screen) {
  case SCREEN_MAIN_MENU:
    ShowMainMenuMessage();
    break;
  case SCREEN_PLAYING:
    ShowGameplayMessage(session);
    break;
  case SCREEN_SETTINGS:
    ShowSettingsMessage();
    break;
  case SCREEN_CONTROLS:
    ShowControlsMessage();
    break;
  default:
    break;
  }
}

void SetScreenAndShow(GameSession &session, ScreenMode screen) {
  SetScreen(session, screen);
  ShowScreenMessage(session, screen);
}

void OpenControlsScreen(GameSession &session, ScreenMode returnTarget) {
  session.state.controlsReturnScreen = returnTarget;
  SetScreenAndShow(session, SCREEN_CONTROLS);
}

void ReturnFromControlsScreen(GameSession &session) {
  SetScreenAndShow(session, session.state.controlsReturnScreen);
}

void SetDifficulty(GameSession &session, DifficultyMode difficulty) {
  session.state.difficulty = difficulty;
  RefreshDifficultyAndLevel(session.state);
  cout << "Difficulty set to " << DifficultyName(session.state.difficulty)
       << "." << endl;
}

void ProcessMenuInput(GLFWwindow *window, GameSession &session,
                      vector<Brick> &bricks,
                      Brick &paddle) {
  if (AnyKeyPressedThisFrame(window, session, kMenuStartKeys)) {
    StartNewRun(session, bricks, paddle);
    return;
  }

  if (KeyPressedThisFrame(window, session, GLFW_KEY_S)) {
    SetScreenAndShow(session, SCREEN_SETTINGS);
    return;
  }

  if (KeyPressedThisFrame(window, session, GLFW_KEY_C)) {
    OpenControlsScreen(session, SCREEN_MAIN_MENU);
    return;
  }
}

void ProcessSettingsInput(GLFWwindow *window, GameSession &session) {
  if (KeyPressedThisFrame(window, session, GLFW_KEY_1)) {
    SetDifficulty(session, DIFFICULTY_EASY);
  } else if (KeyPressedThisFrame(window, session, GLFW_KEY_2)) {
    SetDifficulty(session, DIFFICULTY_NORMAL);
  } else if (KeyPressedThisFrame(window, session, GLFW_KEY_3)) {
    SetDifficulty(session, DIFFICULTY_HARD);
  }

  if (AnyKeyPressedThisFrame(window, session, kSettingsBackKeys)) {
    SetScreenAndShow(session, SCREEN_MAIN_MENU);
  }
}

void ProcessControlsInput(GLFWwindow *window, GameSession &session) {
  if (KeyPressedThisFrame(window, session, GLFW_KEY_M)) {
    SetScreenAndShow(session, SCREEN_MAIN_MENU);
    return;
  }

  if (AnyKeyPressedThisFrame(window, session, kControlsReturnKeys)) {
    ReturnFromControlsScreen(session);
  }
}

void ProcessGameOverInput(GLFWwindow *window, GameSession &session,
                          vector<Brick> &bricks,
                          Brick &paddle) {
  if (KeyPressedThisFrame(window, session, GLFW_KEY_R)) {
    StartNewRun(session, bricks, paddle);
    return;
  }

  if (KeyPressedThisFrame(window, session, GLFW_KEY_M)) {
    SetScreenAndShow(session, SCREEN_MAIN_MENU);
    return;
  }
}
} // namespace

void ProcessInput(GLFWwindow *window, GameSession &session,
                  vector<Brick> &bricks, Brick &paddle, float deltaSeconds) {
  if (KeyPressedThisFrame(window, session, GLFW_KEY_ESCAPE)) {
    if (session.state.screen == SCREEN_MAIN_MENU) {
      glfwSetWindowShouldClose(window, true);
      return;
    }

    SetScreenAndShow(session, SCREEN_MAIN_MENU);
    return;
  }

  switch (session.state.screen) {
  case SCREEN_MAIN_MENU:
    ProcessMenuInput(window, session, bricks, paddle);
    return;
  case SCREEN_SETTINGS:
    ProcessSettingsInput(window, session);
    return;
  case SCREEN_CONTROLS:
    ProcessControlsInput(window, session);
    return;
  case SCREEN_GAME_OVER:
    ProcessGameOverInput(window, session, bricks, paddle);
    return;
  default:
    break;
  }

  if (KeyPressedThisFrame(window, session, GLFW_KEY_M)) {
    SetScreenAndShow(session, SCREEN_MAIN_MENU);
    return;
  }

  if (KeyPressedThisFrame(window, session, GLFW_KEY_C)) {
    OpenControlsScreen(session, SCREEN_PLAYING);
    return;
  }

  bool moveLeft = IsMoveLeftInputDown(window);
  bool moveRight = IsMoveRightInputDown(window);
  float movementStep =
      kPaddleMoveStepPerSimulationTick * StepScaleFromDeltaSeconds(deltaSeconds);

  if (moveLeft) {
    paddle.centerX -= movementStep;
  }
  if (moveRight) {
    paddle.centerX += movementStep;
  }

  KeepPaddleOnScreen(paddle);

  // Treat SPACE as an edge-triggered action so one press = one spawn attempt.
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    session.state.spaceHeldLastFrame = true;
  }
  if (session.state.spaceHeldLastFrame &&
      (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)) {
    session.state.spaceHeldLastFrame = false;
    // Extra balls are limited by reserve count to prevent infinite spamming.
    if (session.state.reserveBallsRemaining > 0) {
      session.state.reserveBallsRemaining--;
      AddBallFromPaddle(session, paddle, kNoBallSpawnXOffset);
    } else {
      cout << "No reserve balls left. Break bricks or clear a level to earn more."
           << endl;
    }
  }
}
