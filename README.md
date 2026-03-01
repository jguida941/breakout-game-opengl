# CS330Content-5

Course repository for CS-330 OpenGL assignments.
This repo contains shared libraries/utilities and multiple assignment projects.

## Repository Layout
- `Projects/`: assignment projects and solutions
- `Libraries/`: third-party dependencies (GLFW, GLEW, GLM, glad)
- `Utilities/`: shared helper code
- `3DShapes/`: reusable mesh/shape helpers

## Recommended Entry Point
If you only need the final assignment project, start here:
- [`Projects/8-2_Assignment/README.md`](Projects/8-2_Assignment/README.md)
- [`Projects/8-2_Assignment/DEV.md`](Projects/8-2_Assignment/DEV.md) (developer architecture/design notes)

## Build and Run (Windows)
### Requirements
- Windows
- Visual Studio 2022 with C++ workload
- v143 toolset

### Steps
1. Open the solution for your target project under `Projects/`.
2. For the 8-2 project, open:
   - `Projects/8-2_Assignment/8-2_Assignment.sln`
3. Select `Debug|Win32` or `Release|Win32`.
4. Build and run (`Ctrl+F5`).

## Build Check (macOS/Linux)
These projects are configured primarily for Visual Studio.
You can still run syntax checks with `clang++` if headers/libraries are available.

Example (8-2 syntax-only check):
```bash
clang++ -std=c++17 -fsyntax-only \
  -IProjects/8-2_Assignment/Source \
  -ILibraries/GLFW/include \
  -ILibraries/GLEW/include \
  -IUtilities -I3DShapes \
  Projects/8-2_Assignment/Source/GameSettings.cpp \
  Projects/8-2_Assignment/Source/GameObjects.cpp \
  Projects/8-2_Assignment/Source/GameFlow.cpp \
  Projects/8-2_Assignment/Source/GameFlowLevel.cpp \
  Projects/8-2_Assignment/Source/GameFlowProgression.cpp \
  Projects/8-2_Assignment/Source/GameFlowPowerUps.cpp \
  Projects/8-2_Assignment/Source/GameFlowCollision.cpp \
  Projects/8-2_Assignment/Source/GameFlowInput.cpp \
  Projects/8-2_Assignment/Source/GameFlowUI.cpp \
  Projects/8-2_Assignment/Source/MainCode.cpp
```

## Notes
- OpenGL immediate mode is used in several assignments for clarity/teaching.
- Some platforms may show OpenGL deprecation warnings; these are expected for this code style.
