# 8-2 Assignment Developer Notes

## Purpose
This file is for developers working on the codebase.
It documents architecture, system boundaries, and practical rules for making changes safely.

## Project Structure (`Source/`)
- `MainCode.cpp`: app setup and main loop
- `GameSettings.*`: core constants, `GameState`, difficulty/scaling helpers, random helpers
- `GameObjects.*`: `Brick`, `Circle`, `PowerUp` data + rendering + object-specific behavior
- `GameFlow.h`: public gameplay API and `GameSession`
- `GameFlowInternal.h`: shared helpers used across gameplay/input/UI modules
- `GameFlowSystems.h`: internal gameplay system declarations
- `GameFlow.cpp`: simulation coordinator (`UpdatePlayState`)
- `GameFlowLevel.cpp`: level build logic, paddle setup, run reset, ball spawn
- `GameFlowProgression.cpp`: level transitions and life-loss behavior
- `GameFlowPowerUps.cpp`: drop chance, apply effects, update active power-ups
- `GameFlowCollision.cpp`: collision resolution
- `GameFlowInput.cpp`: input routing by screen state
- `GameFlowUI.cpp`: rendering for screens/HUD and window title updates

## Documentation Assets
- `img/`: README screenshots and other doc images
- Current screenshots:
  - `img/main-menu-screen.png`
  - `img/gameplay-screen.png`

## Runtime Data Model
- `GameSession` is the gameplay container passed through systems.
- `GameSession.state` (`GameState`) stores screen mode, difficulty, score/lives, tuned speeds, and input latches.
- `GameSession.balls` stores active balls.
- `GameSession.powerUps` stores active falling power-ups.

## Architectural Design Decisions
### 1. Split by gameplay responsibility
Instead of a single large file, gameplay is split into focused modules (`Input`, `Collision`, `Level`, `PowerUps`, `Progression`, `UI`).
This reduces merge conflicts and keeps behavior changes localized.

### 2. Single runtime state container
`GameSession` is passed into systems so state flow is explicit.
This avoids hidden gameplay globals and makes side effects easier to track.

### 3. Fixed-step simulation loop
Gameplay updates run at a fixed 60 Hz step.
This keeps gameplay behavior stable across different frame rates and machines.

### 4. Input edge-detection by state latches
One-shot actions (menu transitions, toggles) use key-latch tracking in `GameState`.
This prevents repeated trigger spam while keys are held.

### 5. Rule systems separated from drawing
Simulation systems update state first, then UI/rendering reads that state.
This keeps gameplay logic independent from rendering code paths.

### 6. Time and unit consistency
Motion values are treated as per simulation tick.
Effect duration (`widePaddleSecondsRemaining`) is stored as seconds.
`StepScaleFromDeltaSeconds(...)` bridges these where needed.

### 7. Centralized random helpers
Random behavior uses shared helpers (`RandomFloat`, `RandomInt`) from `GameSettings.cpp`.
This gives one place to tune randomness strategy for the project.

### 8. Anti-spam gameplay economy
`SPACE` no longer spawns unlimited balls.
Extra balls now consume `reserveBallsRemaining`, and the player earns reserve balls by progress.
This removes the infinite-ball exploit while keeping a skill/reward loop.

### 9. Life penalty per lost ball
When any ball falls below the world, a life is deducted immediately.
This applies even when multiple balls are active, so extra balls increase both opportunity and risk.

### 10. Randomized ball-ball collision outcomes
Ball-ball impacts now pick one random outcome:
- merge into one larger ball
- recolor and bounce
- both disappear
- split into multiple smaller balls
This creates variety in gameplay while keeping logic inside the collision system module.

## Implemented Refactor Summary
- Replaced frame-counted wide-paddle duration with seconds.
- Threaded `deltaSeconds` through input/update/motion paths.
- Added consistent step-scaling helper for tick-based motion values.
- Replaced `rand()/srand()` usage with `std::mt19937`-backed helpers.
- Normalized comments and naming for readability and maintenance.
- Added reserve-ball economy and life-loss anti-spam rules.
- Added randomized ball-ball collision outcomes (merge/recolor/disappear/split).

## Frame Flow
Each display frame:
1. Poll GLFW events.
2. Run fixed-step updates at 60 Hz.
3. Render current scene.
4. Update window title text.

Fixed-step updates call:
1. `ProcessInput(...)`
2. `UpdatePlayState(...)`

## Timing And Units
- Simulation step: `kSimulationStepSeconds` (`1/60`).
- Motion values (ball velocity, power-up fall speed, paddle movement constants) are treated as per simulation tick.
- `StepScaleFromDeltaSeconds(...)` scales tick-based values when needed.
- Wide-paddle duration is stored as seconds (`widePaddleSecondsRemaining`), not frame count.
- Reserve-ball count is integer gameplay state (`reserveBallsRemaining`).
- Brick progress toward reserve rewards is tracked by `bricksBrokenTowardReserveReward`.

## Randomness
- Use `RandomFloat(...)` and `RandomInt(...)` from `GameSettings.cpp`.
- Do not use `rand()` / `srand()` directly in gameplay systems.

## Coding Rules For This Project
- Keep systems single-purpose by file (input, collision, power-ups, progression, UI).
- Use named constants instead of inline magic numbers.
- Keep comments short and focused on intent for non-obvious logic.
- Prefer clear loop/index names (`ballIndex`, `brickIndex`, `powerUpIndex`).
- Keep public function signatures declared in the matching header.

## Recommended Change Workflow
1. Pick the target system file first (for example, `GameFlowCollision.cpp` for collision behavior).
2. Add/update constants near the top of that file.
3. Keep data ownership in `GameSession`; avoid hidden gameplay globals.
4. Run a syntax build check after edits.

Example syntax check:
```bash
c++ -std=c++17 -fsyntax-only Projects/8-2_Assignment/Source/*.cpp \
  -IProjects/8-2_Assignment/Source -ILibraries/GLFW/include
```

## Current Technical Constraints
- Rendering uses OpenGL immediate mode (`glBegin/glEnd`) by assignment scope.
- On macOS, OpenGL deprecation warnings are expected.
