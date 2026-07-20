# 全屏图像自动放大原理分析

## 问题

`TransformComponent` 中存在 `mScale` 成员变量，决定图像的放大比例。当程序全屏时图像会对应放大，但代码中找不到给 `mScale` 赋值的地方。这是为什么？

## 结论

**全屏时的图像缩放与 `TransformComponent::mScale` 完全无关。**

实际起作用的是 SDL3 的 **Logical Presentation（逻辑分辨率）机制**，由 `SDL_SetRenderLogicalPresentation()` 实现。这是一个硬件级的自动缩放，不涉及任何游戏逻辑代码。

---

## 原理分析

### 1. 关键代码位置

**`src/engine/core/game_app.cpp:185-187`**

```cpp
int logical_width = static_cast<int>(static_cast<float>(mConfig->mWindowWidth) * mConfig->mWindowLogicalScale);
int logical_height = static_cast<int>(static_cast<float>(mConfig->mWindowHeight) * mConfig->mWindowLogicalScale);
SDL_SetRenderLogicalPresentation(mSDLRenderer, logical_width, logical_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
```

### 2. SDL3 逻辑分辨率的工作流程

```
游戏逻辑世界（逻辑坐标系 1280×720）
        │
        ▼
各 System 计算实体位置、大小
(RenderSystem::update → render_system.cpp:19)
        │
        ▼
Renderer::drawSprite → SDL_RenderTextureRotated()
(SDL 在逻辑坐标系中绘制)
        │
        ▼
SDL 内部自动等比缩放 + Letterbox
(SDL_LOGICAL_PRESENTATION_LETTERBOX)
        │
        ▼
物理屏幕（窗口模式或全屏模式）
```

### 3. 模式对比

| 模式 | 逻辑分辨率 | 物理窗口 | 实际效果 |
|------|-----------|---------|---------|
| 窗口模式 | 1280×720 | 1280×720 | 1:1 无缩放 |
| 全屏模式 | 1280×720 | 1920×1080 | SDL 自动放大 ~1.5x |

逻辑分辨率固定为 1280×720，当窗口大小变化时，SDL 负责将逻辑画布缩放到物理窗口尺寸。全屏时物理窗口变大，SDL 自动放大输出，游戏代码无需任何改动。

### 4. 为什么 `mScale` 没有被赋值也能工作？

**`mScale` 是局部缩放，全局缩放靠 SDL。**

查看 `render_system.cpp:19` 和 `transform_component.h:25`：

```cpp
// render_system.cpp:19 — 计算最终渲染尺寸
auto size = sprite.mSize * transform.mScale;

// transform_component.h:25 — 默认值
glm::vec2 mScale{ 1.0f };
```

- `mScale` 默认为 `{1.0f, 1.0f}`，不带缩放
- `mScale` 当前未被任何代码赋值，所以所有实体以原始大小渲染
- `mScale` 的用途是**逐实体独立缩放**（如一个 BOSS 设 `mScale = {3,3}` 让它变大）
- 全局的全屏缩放由 SDL 的逻辑分辨率机制处理，与 `mScale` 无关

### 5. 其他相关代码

**Camera 的视口大小来源 — `src/engine/core/game_state.cpp:36-40`**

```cpp
glm::vec2 GameState::getLogicalSize() const {
    SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, NULL);
    return glm::vec2(width, height);
}
```

Camera 从这里获取视口大小，因此裁剪、坐标转换都基于逻辑坐标系，全屏下自动保持正确比例。

**Mouse 坐标转换 — `src/engine/input/input_manager.cpp:132`**

```cpp
SDL_RenderCoordinatesFromWindow(mSDLRenderer, mMousePosition.x, mMousePosition.y,
    &mLogicalMousePosition.x, &mLogicalMousePosition.y);
```

鼠标的物理坐标也要通过 SDL 转换为逻辑坐标，确保点击检测在全屏下依然准确。

---

## 两个"缩放"概念的区分

| 机制 | `TransformComponent::mScale` | SDL 逻辑分辨率 |
|------|---------------------------|---------------|
| **作用范围** | 单个实体 | 整个画面 |
| **实现位置** | `render_system.cpp:19` 计算 size | `game_app.cpp:187` `SDL_SetRenderLogicalPresentation` |
| **默认值** | `{1.0f, 1.0f}`（无缩放） | 1280×720 |
| **触发条件** | 需要主动赋值 | 窗口大小变化时自动生效 |
| **技术层** | 游戏逻辑层（CPU） | SDL 渲染层（GPU/硬件） |
| **用途** | BOSS 变大、角色变形等 | 窗口/全屏切换、分辨率适配 |
