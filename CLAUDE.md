# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```powershell
# Configure (from project root) — Ninja 生成器，并行编译速度快
cmake -S . -B build -G Ninja

# Build
cmake --build build --config Debug

# Run
./build/Debug/MonsterWar-Windows.exe

# Clean rebuild
cmake --build build --config Debug --clean-first
```

The CMake build system (`CMakeLists.txt`) supports static/shared library linking, automatic dependency management (prebuilt > system > local `external/` > FetchContent), and auto-copies assets + DLLs to the output directory. C++17 standard, MSVC `/utf-8` flag on Windows for Chinese text support.

## Project Architecture

SDL3 + EnTT ECS 2D game engine with **engine/game separation**:

### Engine Layer (`src/engine/`)

Provides reusable game infrastructure, never references game-layer code:

| Module        | Path                | Purpose                                                                                                                               |
| ------------- | ------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| **core**      | `engine/core/`      | `GameApp` (lifecycle, init→loop→cleanup), `Config` (JSON-based), `Context` (DI container for all engine modules), `Time`, `GameState` |
| **render**    | `engine/render/`    | `Renderer` (SDL3 wrapper, sprite/UI/fill drawing with camera viewport culling), `Camera`, `TextRenderer`, `Image`                     |
| **component** | `engine/component/` | ECS components: Transform, Sprite, Render, Animation, Velocity, Parallax, Name, Audio, TileLayer                                      |
| **system**    | `engine/system/`    | ECS systems: `RenderSystem`, `AnimationSystem`, `MovementSystem`, `YSortSystem` (Y-sort for depth ordering)                           |
| **loader**    | `engine/loader/`    | `LevelLoader` (Tiled .tmj/.tsj parsing), `BasicEntityBuilder` (Builder pattern for ECS entity construction)                           |
| **resource**  | `engine/resource/`  | `ResourceManager`, `TextureManager`, `FontManager`, `AudioManager`                                                                    |
| **scene**     | `engine/scene/`     | `Scene` base class, `SceneManager` (stack-based scene switching)                                                                      |
| **ui**        | `engine/ui/`        | UI elements (Button, Label, Panel, Image) with State pattern (Normal/Hover/Pressed)                                                   |
| **input**     | `engine/input/`     | `InputManager` for SDL event handling                                                                                                 |
| **audio**     | `engine/audio/`     | `AudioPlayer` via SDL3_mixer                                                                                                          |
| **utils**     | `engine/utils/`     | Math helpers, Events (Quit, PushScene, PopScene, ReplaceScene), Alignment                                                             |

### Game Layer (`src/game/`)

Concrete game logic depending on engine layer:

- `game/scene/GameScene` — main game scene, extends `engine::scene::Scene`

### Application Flow

```
main.cpp
  → GameApp::run()
    → init(): initDispatcher → Config → SDL → GameState → Time → ResourceManager
               → AudioPlayer → Renderer → TextRenderer → Camera → InputManager
               → Context → SceneManager (calls mSceneSetupFunc → PushSceneEvent)
    → loop: handleEvents → update(delta) → render
    → close(): cleanup in reverse order
```

### ECS Architecture

Each `Scene` owns an `entt::registry`. Systems operate on entities filtered by component combinations:

- **MovementSystem**: entities with VelocityComponent + TransformComponent
- **AnimationSystem**: entities with AnimationComponent + SpriteComponent
- **YSortSystem**: entities with RenderComponent → sets depth from Y position
- **RenderSystem**: iterates entities sorted by (layer, depth) via RenderComponent

### Event System

`entt::dispatcher` used for scene transitions and quit:

- `PushSceneEvent` / `PopSceneEvent` / `ReplaceSceneEvent` — managed by `SceneManager`
- `QuitEvent` — exits game loop

### Dependency Injection

`engine::core::Context` holds references to all major engine modules and is passed to Scenes. Scenes access everything through `mContext`.

### Key Design Patterns

- **Builder**: `BasicEntityBuilder` constructs ECS entities step by step
- **State**: UI elements use Normal/Hover/Pressed state classes
- **Scene Stack**: `SceneManager` manages a stack of scenes (push/pop/replace)
- **Y-Sort**: `YSortSystem` updates `RenderComponent::mDepth` from transform Y each frame

## Naming Conventions

- Files: `snake_case` (e.g., `game_app.cpp`, `render_system.cpp`)
- Classes: PascalCase (e.g., `GameApp`, `RenderSystem`)
- Member variables: `m` prefix + PascalCase (e.g., `mWindow`, `mIsRunning`)
- Namespaces: `engine::core`, `engine::system`, `game::scene`
- Accessors: `getXxx()` / `setXxx()` style
- Forward declarations file: `fwd.h` convention

## Code Style

- C++17, `#pragma once` header guards
- Pointers for non-owning references (SDL handles), `unique_ptr` for ownership
- `auto` preferred where type is clear
- `<entt/signal/fwd.hpp>` used over full includes for dispatcher
- Scenes constructed in `main.cpp` via `registerSceneSetup()`, not directly
- `Context` passed by reference, never by pointer — contains non-owning references

## Dependencies

All vendored under `external/` or auto-fetched by CMake:
SDL3, SDL3_image, SDL3_mixer, SDL3_ttf, GLM, nlohmann-json, spdlog, EnTT, ImGui

Assets are copied automatically from `assets/` to the build output directory.
