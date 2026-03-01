#include "GameFlow.h"

#include <cstdlib>
#include <iostream>
#include <vector>

// Software version marker for grading; keep this line in place.
const char *const SW_VERSION = "20260301f"; // do not delete this line

namespace {
// Main setup constants.
constexpr int kInitialWindowSize = 480;
const char *const kWindowTitle = "CS330 Breakout";
constexpr float kInitialPaddleCenterX = 0.0f;
constexpr float kBackgroundRed = 0.08f;
constexpr float kBackgroundGreen = 0.09f;
constexpr float kBackgroundBlue = 0.12f;

// Fixed-step simulation values.
const double kFixedUpdateSeconds = static_cast<double>(kSimulationStepSeconds);
constexpr double kMaxFrameSeconds = 0.25;
constexpr int kMaxUpdatesPerFrame = 8;
} // namespace

void ConfigureViewportAndProjection(int frameBufferWidth,
                                    int frameBufferHeight) {
  // Keep a square gameplay surface to prevent stretching when resized.
  int viewportSize = frameBufferWidth;
  if (frameBufferHeight < viewportSize) {
    viewportSize = frameBufferHeight;
  }
  if (viewportSize < 1) {
    viewportSize = 1;
  }

  int viewportX = (frameBufferWidth - viewportSize) / 2;
  int viewportY = (frameBufferHeight - viewportSize) / 2;

  glViewport(viewportX, viewportY, viewportSize, viewportSize);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(kWorldMin, kWorldMax, kWorldMin, kWorldMax, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

int main(void) {
  // Main loop:
  // input -> update -> draw -> present.

  if (!glfwInit()) {
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow *window = glfwCreateWindow(kInitialWindowSize, kInitialWindowSize,
                                        kWindowTitle, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  std::cout << std::endl
            << "Version: " << SW_VERSION
            << std::endl; // do not delete this line

  GameSession session;
  RefreshDifficultyAndLevel(session.state);
  std::vector<Brick> bricks;
  Brick paddle = CreateDefaultPaddle(session, kInitialPaddleCenterX);
  BuildLevelBricks(bricks, session.state.level);

  ShowMainMenuMessage();
  double previousFrameTime = glfwGetTime();
  double accumulatedTime = 0.0;

  while (!glfwWindowShouldClose(window)) {
    // Poll first so key state is current before fixed updates run.
    glfwPollEvents();

    double currentFrameTime = glfwGetTime();
    double frameSeconds = currentFrameTime - previousFrameTime;
    previousFrameTime = currentFrameTime;
    if (frameSeconds < 0.0) {
      frameSeconds = 0.0;
    }
    if (frameSeconds > kMaxFrameSeconds) {
      // Clamp long pauses so we do not run a large catch-up burst.
      frameSeconds = kMaxFrameSeconds;
    }
    accumulatedTime += frameSeconds;

    int updatesThisFrame = 0;
    while ((accumulatedTime >= kFixedUpdateSeconds) &&
           (updatesThisFrame < kMaxUpdatesPerFrame)) {
      ProcessInput(window, session, bricks, paddle,
                   static_cast<float>(kFixedUpdateSeconds));
      UpdatePlayState(session, bricks, paddle,
                      static_cast<float>(kFixedUpdateSeconds));
      accumulatedTime -= kFixedUpdateSeconds;
      updatesThisFrame++;
    }
    if (updatesThisFrame == kMaxUpdatesPerFrame) {
      // Drop excess lag so gameplay remains responsive.
      accumulatedTime = 0.0;
    }

    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    if ((frameBufferWidth <= 0) || (frameBufferHeight <= 0)) {
      continue;
    }

    ConfigureViewportAndProjection(frameBufferWidth, frameBufferHeight);

    glClearColor(kBackgroundRed, kBackgroundGreen, kBackgroundBlue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    RenderScene(session, bricks, paddle);
    UpdateWindowTitle(window, session);

    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
