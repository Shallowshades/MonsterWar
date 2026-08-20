# ImGui 集成：调试 GUI 三件套 + 逻辑分辨率冲突

## 问题

游戏开发到第 5 课，内部状态已经相当丰富——实体、属性（HP/ATK/射程）、cost、基地血量、波次……但**调试全靠日志**：

- 想"看看这个敌人到底多少血、走到哪了"？没门，得加 log 打印
- 想快速调个参数（cost 恢复速率、敌人属性）验证手感？改 JSON 重编译，慢
- 想做个临时 UI 原型？项目里没有任何即时 GUI 手段

同时，`external/imgui` 早就 vendored 进项目、CMake 也在编译它（`IMGUI_SOURCES`），但**从未接线**——编译了却没人用，相当于买了一台示波器没拆封。

一句话：**需要一个横切的调试 GUI 能力，让"看状态、调参数、做原型"在运行时直接可视化。**

## 结论

把 ImGui 按官方标准**三件套**接入引擎，并新增一个 `DebugUISystem` 在游戏场景里每帧绘制调试窗口：

```
GameApp::initImGui()         ① 初始化：CreateContext → 配置/缩放/透明度 → 中文字体 → SDL3 后端
InputManager::update()       ② 事件：SDL_PollEvent 循环里 ImGui_ImplSDL3_ProcessEvent + WantCaptureMouse 拦截
DebugUISystem::update()      ③ 渲染：beginFrame → renderDemoUI → endFrame（挂在 GameScene::render 最后）
```

支撑它的两块地基：

1. **`GameState` 逻辑分辨率开关**：`disableLogicalPresentation()` / `enableLogicalPresentation()`——ImGui 对 SDL letterbox（逻辑分辨率）支持不好，画 ImGui 前临时关掉、画完恢复，让鼠标坐标 1:1 到屏幕像素
2. **中文像素字体**：`assets/fonts/VonwaonBitmap-16px.ttf` + `GetGlyphRangesChineseSimplifiedCommon()`，加载失败回退默认字体不崩溃

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| ImGui 初始化（CreateContext/字体/SDL3后端） | `src/engine/core/game_app.cpp:initImGui` |
| ImGui 事件处理 + 鼠标穿透拦截 | `src/engine/input/input_manager.cpp:update/processEvent` |
| 逻辑分辨率开关 | `src/engine/core/game_state.h/.cpp` |
| 调试 UI 系统 `DebugUISystem` | `src/game/system/debug_ui_system.h/.cpp` |
| ImGui 渲染挂载点 | `src/game/scene/game_scene.cpp:render` |
| CMake 源文件 | `CMakeLists.txt`（`IMGUI_SOURCES` 块 + SOURCES 加 debug_ui_system.cpp） |

### 2. ImGui 的三件套：初始化、事件、渲染，一个都不能少

ImGui 不是"创建窗口后自动工作"的库，它需要宿主程序在每个环节显式配合：

**① 初始化（一次性）** `GameApp::initImGui()`，在 `initSceneManager` 之后、创建第一个场景之前调用：

```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();                     // 全局上下文（所有 ImGui 状态）
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // 可选：键盘导航
io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // 可选：手柄导航
ImGui::StyleColorsDark();                   // 主题

// 与系统 DPI 缩放保持一致（高分屏不至于糊/小）
float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
ImGuiStyle& style = ImGui::GetStyle();
style.ScaleAllSizes(main_scale);
style.FontScaleDpi = main_scale;

// 中文支持：加一个含中文码段的字体
ImFont* font = io.Fonts->AddFontFromFileTTF(
    "assets/fonts/VonwaonBitmap-16px.ttf", 16.0f, nullptr,
    io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
if (!font) { io.Fonts->AddFontDefault(); spdlog::warn("无法加载中文字体..."); }

// 绑定 SDL3 后端（窗口 + 渲染器）
ImGui_ImplSDL3_InitForSDLRenderer(mWindow, mSDLRenderer);
ImGui_ImplSDLRenderer3_Init(mSDLRenderer);
```

**② 事件（每帧）** `InputManager::update()` 的 SDL 轮询循环里，把每个事件先喂给 ImGui：

```cpp
while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);    // ImGui 先处理（它需要鼠标/键盘/窗口事件）
    processEvent(event);                    // 游戏再处理
}
```

**③ 渲染（每帧）** 三个 NewFrame 开启新帧 → 构建窗口 → Render + RenderDrawData 画出来。封装在 `DebugUISystem` 里，挂在 `GameScene::render()` 最后：

```cpp
void DebugUISystem::update() { beginFrame(); renderDemoUI(); endFrame(); }
// beginFrame: ImGui_ImplSDLRenderer3_NewFrame → ImGui_ImplSDL3_NewFrame → ImGui::NewFrame
// renderDemoUI: Begin("窗口1") + Text + Button + SliderFloat + ShowDemoWindow
// endFrame: ImGui::Render → ImGui_ImplSDLRenderer3_RenderDrawData(drawdata, sdl_renderer)
```

**为什么是"三件套"？** 因为 ImGui 是把"UI 逻辑"外包给了宿主：它不管窗口/渲染/事件循环，只在这三个时机拿到宿主喂给它的东西。漏掉任何一件，UI 要么不显示、要么不动、要么不响应输入。这也是"给引擎集成第三方库"的通用模板——**先摸清这个库要求宿主在哪些时机做哪些事**。

### 3. 逻辑分辨率冲突：ImGui 与 SDL letterbox 的坐标战争

本项目的渲染走 **SDL 逻辑分辨率**（`SDL_LOGICAL_PRESENTATION_LETTERBOX`，`initSDL` 里设置）：游戏逻辑坐标（如 1152×648）被 SDL 自动映射/拉伸到窗口物理像素（可能带黑边）。这对游戏是好事——窗口怎么缩，游戏画面不歪。

但 ImGui 的鼠标坐标、窗口布局用的是**物理像素**。当逻辑分辨率开着时，SDL 会把"鼠标在窗口物理像素的位置"换算成逻辑坐标给 ImGui，导致：
- ImGui 窗口位置/大小和鼠标严重错位
- 高分屏或非整数缩放时偏移尤其明显

**解法**（参考实现的做法）：画 ImGui 的帧里，先**关掉**逻辑分辨率（`DISABLED`，此时渲染器用物理像素 1:1），画完**恢复**（`LETTERBOX`）：

```cpp
// game_state.cpp
bool GameState::disableLogicalPresentation() {
    int width, height;
    SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, NULL);  // 读当前逻辑尺寸
    return SDL_SetRenderLogicalPresentation(mRenderer, width, height,
        SDL_LOGICAL_PRESENTATION_DISABLED);
}
bool GameState::enableLogicalPresentation() {
    // ... 同样的读尺寸，但用 SDL_LOGICAL_PRESENTATION_LETTERBOX 恢复
}
```

**关键细节**：切换时用 `SDL_GetRenderLogicalPresentation` 读出**当前尺寸**再原样设回，而不是写死某个分辨率——这样无论逻辑分辨率被谁改过，都能无损恢复。这个开关就是"为调试工具临时切换渲染模式"的通用手法。

### 4. `WantCaptureMouse`：调试 UI 别穿透到游戏

有了 ImGui 窗口，就出现一个新问题：**鼠标悬停在 ImGui 上时，左键点击应该拖 ImGui 的滑块，而不是往游戏里放单位。**

`processEvent` 开头加一道闸：

```cpp
void InputManager::processEvent(const SDL_Event& event) {
    if (ImGui::GetIO().WantCaptureMouse) return;   // ImGui 要这个鼠标，游戏不处理
    switch (event.type) { ... }
}
```

`ImGui::GetIO().WantCaptureMouse` 是 ImGui 在上一帧构建 UI 时算出的"我这次要独占鼠标"标志（比如鼠标在窗口/控件上）。用它拦截后，游戏的 `mouse_left/right` 动作（PlaceUnitSystem）就不会误触发。

> 注意它是**上一帧**的标志，天然有一帧延迟——标准集成都这样，实际无感。

### 5. 渲染顺序：调试 UI 永远最后画

`GameScene::render()` 现在的顺序：

```cpp
mRenderSystem->update(...);      // 游戏世界
mHealthBarSystem->update(...);   // 血量条
mRenderRangeSystem->update(...); // 攻击范围圆
Scene::render();                 // 场景基类（UI 等）
mDebugUISystem->update();        // ImGui 调试 UI —— 最后，盖在最上面
```

调试 UI 是**覆盖层**，必须最后画才能盖住游戏内容。它在 `GameApp::render()` 的 `mRenderer->present()` 之前执行完毕，ImGui 的绘制数据直接写进 SDL renderer，随本帧一起呈现。

### 6. 销毁顺序：后端清理必须在 SDL 渲染器之前

`GameApp::close()` 里，ImGui 的 Shutdown 必须放在 `SDL_DestroyRenderer` **之前**：

```cpp
ImGui_ImplSDLRenderer3_Shutdown();   // 先释放后端（它持有 SDL_Renderer 引用）
ImGui_ImplSDL3_Shutdown();
ImGui::DestroyContext();
// ... 然后才是 SDL_DestroyRenderer(mSDLRenderer)
```

这是资源生命周期的铁律：**谁依赖谁，谁先销毁被依赖方**。ImGui 的 SDL 后端注册在 SDL_Renderer 上，渲染器没了后端就是悬空引用。

### 7. 与参考实现（WispSnow/MonsterWar，commit 04a2fab）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 命名 trailing underscore（`window_`/`registry_`/`context_`） | m-prefix（`mWindow`/`mRegistry`/`mContext`） | 遵循本地编码规范 |
| 2 | 无（逻辑、结构完全照搬） | — | — |

本课逻辑上与参考完全一致——它是纯"接线"课程，不涉及本地此前的设计决策，所以几乎没有差异点。

---

## 学习要点

### 1. 第三方库接进引擎 = 搞清楚"三件套时机"

ImGui 这类库不掌控自己的生命周期，它要求宿主在固定时机做固定动作：**初始化（一次性）、事件（每帧喂入）、渲染（每帧输出）**。接任何 GUI/输入/网络库前，先问三个问题：它要我在启动时建什么？每个事件循环里要喂它什么？每个帧要调用它的什么来输出？

### 2. 即时模式 GUI（Immediate Mode）的心智模型

ImGui 没有"控件对象"，每帧从头开始 `Begin → 摆控件 → End` 重建整个界面。它把"界面应该长什么样"这个**函数式的描述**每帧执行一遍，内部缓存 diff 来决定绘制与命中。好处：界面状态全在代码里，改布局 = 改代码顺序，迭代极快；代价：每帧重建有开销（但几十个窗口足够快）。与保留模式（Retained，如 SDL 的 UIElement 那套：建对象、挂状态）是两种完全不同的心智。

### 3. 逻辑分辨率是"渲染坐标"不是"窗口坐标"

SDL letterbox 把逻辑坐标和物理像素解耦，好处是游戏内容不随窗口变。但任何**用物理像素定位的东西**（调试 GUI、鼠标命中）都会踩到这套映射的坑。方案要么关掉 letterbox 用物理坐标，要么自己做逻辑坐标→物理坐标换算。**先想清楚你的 UI 是在哪个坐标系里。**

### 4. 输入穿透治理：调试工具不该影响游戏操作

调试 UI 捕获输入后，若不拦截，玩家的操作会被误触发（放错单位、点错按钮）。`WantCaptureMouse` 是 ImGui 提供的"我要鼠标"信号，宿主据此决定放行还是拦截。**横切功能（调试 UI）必须显式声明它占用了哪些输入通道，否则和正常游戏逻辑打架。**

### 5. 生命周期顺序：依赖关系决定销毁顺序

ImGui 后端 → SDL_Renderer → SDL_Window → SDL_Init。销毁严格反向。这个"谁依赖谁"的思考方式适用于一切资源（纹理、音频、窗口、场景栈）。

### 6. 调试 UI 是横切关注点：挂最外层、不碰游戏逻辑

`DebugUISystem` 持 `mRegistry` 但本课没用——这是为下一课（ImGui 显示单位信息）留的接口。调试系统这类"观察者"应该只读数据、不修改游戏状态，挂在渲染最外层，与正式逻辑解耦。这也是为什么参考注释写着"正式发布往往删除，不需要过度设计"。
