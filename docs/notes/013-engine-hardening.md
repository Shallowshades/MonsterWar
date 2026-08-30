# 工程加固：init 失败处理 / std::move / 逻辑分辨率缓存 / 构建优化

## 问题

012 课（通关/结束场景）之后，参考仓库（WispSnow/MonsterWar）又累积了一批**小而实用的工程加固改动**（`5f9f01b`→`cb83201` 共 7 个提交），每个单独看都很小，但合起来是引擎从"能用"走向"可靠"的关键一步：

1. **场景初始化失败被静默吞掉**——`Scene::init()` 是 `void`，GameScene/TitleScene/LevelClearScene/EndScene 里任何一步失败都只是 `return;` 返回，`SceneManager` 照常把它压栈当作初始化成功，程序带着残缺场景跑起来，错误要到很后面才爆。
2. **`Scene::init()` 没被调用**——检查发现 LevelClearScene / EndScene 的 `init()` **压根没调父类 `Scene::init()`**，导致 `mIsInitialized` 永远是 `false`（潜在 bug：下次进这个场景，`SceneManager` 会再次调 `init()` 重新初始化一遍）。
3. **场景构造器拷贝 shared_ptr**——`(Context&, shared_ptr<A>, shared_ptr<B>, …)` 传参时按值拷贝进成员，白白多一次原子引用计数增减。
4. **高 DPI 显示器上 ImGui 窗口偏大/布局不稳定**——`SDL_GetDisplayContentScale` 跟随系统缩放，不同显示器上 UI 尺寸飘。
5. **中文日志乱码**——Windows 控制台默认代码页不是 UTF-8，`spdlog` 的中文日志输出全乱。
6. **ImGui 窗口布局不持久**——每次运行窗口位置/大小重置，调试布局没法记住。
7. **编译慢**——单文件内多个 `.cpp` 串行编。
8. **逻辑分辨率读写竞态**——SDL3.4 改了 `SDL_GetRenderLogicalPresentation` 的行为：逻辑分辨率被禁用时返回 `0x0`。旧代码"先读再设"的模式在 ImGui 临时关掉逻辑分辨率后，读到 `0x0` 再设 `LETTERBOX`，把逻辑分辨率**写成了 0**。
9. **纹理 null 上设 scale 模式**——`IMG_LoadTexture` 失败返回 `nullptr` 时，旧代码**先** `SDL_SetTextureScaleMode(nullptr, …)` **后**判空，对空指针调用会崩。

一句话：**把"场景初始化是否成功"变成显式可检查的返回值并快速失败、修掉 init 链漏调父类、用 std::move 传共享数据、缓存逻辑分辨率状态、加固构建与纹理加载路径**——7 个参考提交合成本课，全是"可靠性 + 代码卫生"类改动，没有新增玩法。

## 结论

```
Scene::init()        [[nodiscard]] virtual bool    子类末尾 return Scene::init()（父类置 mIsInitialized=true）
    │ 任何一步失败 → return false
    ▼
SceneManager::pushScene/replaceScene
    if (!scene->getIsInitialized()) {
        if (!scene->init()) {  error; scene->clean(); trigger<QuitEvent>; return; }   // 快速失败
    }

GameState 逻辑分辨率（SDL3.4 行为变更适配）
    disableLogicalPresentation()   读当前(非0)尺寸 → 缓存 → 设 DISABLED → mLogicalPresentationDisabled=true
    enableLogicalPresentation()    未禁用→直接true；缓存无效→error；恢复缓存的 宽/高/模式
    getLogicalSize()               读到 0x0 且缓存有效 → 回退缓存，不返回 0
    syncLogicalPresentationState() 构造时从渲染器同步一次缓存

场景构造器     shared_ptr 参数 std::move 进成员（<utility>）
ImGui 缩放     固定 1.0f（弃用 SDL_GetDisplayContentScale）
main.cpp       SetConsoleOutputCP(CP_UTF8) + SetConsoleCP(CP_UTF8)（_WIN32）
CMakeLists     /MP 并行编译 + PRE_BUILD 复制 imgui.ini（仅当目标缺失）
texture_manager IMG_LoadTexture 判空 → 再设 SDL_SetTextureScaleMode
```

- **init 失败 = 快速失败**：任何场景初始化失败立刻 `QuitEvent` 退出，不留残缺状态继续跑。
- **std::move 传共享数据**：shared_ptr 参数被"消费"进成员时 move，避免引用计数拷贝开销。
- **逻辑分辨率缓存/恢复**：`GameState` 持有最近一次有效的 宽/高/模式 + 是否被 ImGui 关闭的标志，恢复用缓存而非"读当前再设"。
- **imgui.ini 只复制不覆盖**：CMake `PRE_BUILD` 仅当目标不存在时复制，不破坏用户自定义布局。

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| `Scene::init` 返回 bool | `src/engine/scene/scene.h` / `scene.cpp` |
| `SceneManager` 检查 init 结果 | `src/engine/scene/scene_manager.cpp`（pushScene/replaceScene） |
| 四个场景 init 改 bool | `src/game/scene/game_scene.h/.cpp`、`title_scene.h/.cpp`、`level_clear_scene.h/.cpp`、`end_scene.h/.cpp` |
| 场景构造器 std::move | 同上 4 个 `.cpp`（ctor 初始化列表） |
| ImGui 缩放固定 1.0 | `src/engine/core/game_app.cpp:initImGui` |
| 控制台 UTF-8 | `src/main.cpp`（initializeEnvironment） |
| imgui.ini 复制 | `CMakeLists.txt`（PRE_BUILD 自定义命令） |
| MSVC /MP | `CMakeLists.txt`（target_compile_options） |
| 逻辑分辨率缓存 | `src/engine/core/game_state.h/.cpp`、`game_app.cpp:initSDL` |
| 纹理 scale 保护 | `src/engine/resource/texture_manager.cpp` |

### 2. `[[nodiscard]] bool init()`：把初始化失败变成"看得见"的错误

原 `void Scene::init()` 的致命伤：**失败无信号**。GameScene 里 `loadLevel` 失败 `return;`，`SceneManager` 以为初始化成功，把场景压栈，接着每帧 update/render 一个"没地图、没实体"的空场景——错误被推迟到深不可测的运行时才浮出水面。

改成 `[[nodiscard]] virtual bool init()`（`[[nodiscard]]` 强制调用方检查返回值，编译期就拦住"忽略失败"）：

```cpp
bool Scene::init() {
    mIsInitialized = true;    // 子类应该最后调用父类的 init 方法
    spdlog::trace("场景 '{}' 初始化完成。", mSceneName);
    return true;
}
```

`SceneManager` 在压栈前检查：

```cpp
if (!scene->getIsInitialized()) {
    if (!scene->init()) {
        spdlog::error("{} 场景 '{}' 初始化失败。", mLogTag.data(), scene->getName());
        scene->clean();
        mContext.getDispatcher().trigger<engine::utils::QuitEvent>();   // 快速失败：直接退出
        return;
    }
}
```

四个游戏场景各自把所有 `return;` 改成 `return false;`，末尾 `return Scene::init();`。

**顺手修掉一个潜伏 bug**：LevelClearScene 和 EndScene 之前**没调 `Scene::init()`**，`mIsInitialized` 永远是 `false`。这带来一个隐蔽后果：`SceneManager` 用 `if (!getIsInitialized())` 判断要不要重新 init——如果这两类场景在旧代码里再次被压栈，会**重复初始化**。改成 `return Scene::init();` 后这个隐患一并消除。这正说明"强制返回 bool + 末尾串父类"这条约定能逼出结构性问题。

另外 `popScene` 弹空栈时的退出从 `enqueue<QuitEvent>()` 改成 `trigger<QuitEvent>()`——**立即触发而非延迟一帧**。`enqueue` 要等 `dispatcher->update()` 才派发，`trigger` 同步派发；弹空栈意味着流程已经走到终点，没有理由再等一帧。

### 3. 场景构造器 `std::move`：shared_ptr 参数"消费"进成员

```cpp
GameScene::GameScene(Context& context,
    std::shared_ptr<BlueprintManager> blueprint_manager,
    std::shared_ptr<SessionData> session_data,
    std::shared_ptr<UIConfig> ui_config,
    std::shared_ptr<LevelConfig> level_config)
    : engine::scene::Scene("GameScene", context),
      mBlueprintManager(std::move(blueprint_manager)),
      mSessionData(std::move(session_data)),
      mUIConfig(std::move(ui_config)),
      mLevelConfig(std::move(level_config)) {
```

shared_ptr 按值传参本身是"拷贝"（引用计数 +1），如果直接把它赋值给成员又是一次拷贝（再 +1），函数结束时参数析构（-1）——一次构造 3 次原子操作，纯粹浪费。`std::move` 后是**转移所有权**：参数是右值引用，成员直接接管控制块，零原子开销。**凡是"参数最终要存进成员、调用方不再需要"的，就 move**；`const&` 留给"只读借用"。

### 4. 逻辑分辨率缓存/恢复：适配 SDL3.4 的 `0x0` 行为

这是本课**技术含量最高**的一处。背景：`GameState` 提供 `disableLogicalPresentation()` / `enableLogicalPresentation()`，给 ImGui 在绘制前临时关掉逻辑分辨率（letterbox 下鼠标坐标跟物理像素不对齐），画完恢复。旧实现是"**读当前 → 重设**"：

```cpp
// 旧：disable 读当前(可能是0x0)再设 DISABLED —— 侥幸没坏是因为当时 SDL3 返回非0尺寸
bool disableLogicalPresentation() {
    SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, NULL);   // 读到当前尺寸
    return SDL_SetRenderLogicalPresentation(mRenderer, width, height, SDL_LOGICAL_PRESENTATION_DISABLED);
}
// 旧：enable 读当前(已被 disable 成 0x0)再设 LETTERBOX —— 会把 0x0 写回去！
bool enableLogicalPresentation() {
    SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, NULL);   // 此时逻辑分辨率已被禁用 → SDL3.4 返回 0x0
    return SDL_SetRenderLogicalPresentation(mRenderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);  // 宽高 = 0 → 灾难
}
```

SDL 升级到 3.4 后，`SDL_GetRenderLogicalPresentation` 在"逻辑分辨率被禁用"时**返回 `0x0`**（SDL 语义：没设逻辑分辨率就没有尺寸）。于是 `enableLogicalPresentation` 读到 `0,0` 再设 `LETTERBOX`，把逻辑分辨率**归零**，游戏画面比例全乱。

修复思路：**"读当前再设"这种来回往返，依赖 SDL 内部状态在两次调用间不变；跨一个 disable 后这个前提就塌了。** 解法是**在 GameState 里缓存"最近一次有效的逻辑分辨率配置"**，恢复时用缓存而不是现场读：

```cpp
// 缓存（私有成员）
int mLogicalWidth = 0;
int mLogicalHeight = 0;
SDL_RendererLogicalPresentation mLogicalMode = SDL_LOGICAL_PRESENTATION_DISABLED;
bool mLogicalPresentationDisabled = false;   // 是否为 ImGui 暂时关闭

bool GameState::disableLogicalPresentation() {
    // 读当前（非0）→ 先缓存 → 再真正关闭
    if (mode == SDL_LOGICAL_PRESENTATION_DISABLED) { mLogicalPresentationDisabled = true; return true; }  // 已关闭，直接标记
    mLogicalWidth = width; mLogicalHeight = height; mLogicalMode = mode;   // ★ 关键：先缓存
    if (!SDL_SetRenderLogicalPresentation(...DISABLED)) return false;
    mLogicalPresentationDisabled = true;
    return true;
}

bool GameState::enableLogicalPresentation() {
    if (!mLogicalPresentationDisabled) return true;                        // 没关过，无需恢复
    if (mLogicalWidth <= 0 || mLogicalHeight <= 0 || mLogicalMode == DISABLED) return false;  // 没有可恢复的配置
    if (!SDL_SetRenderLogicalPresentation(mLogicalWidth, mLogicalHeight, mLogicalMode)) return false;  // ★ 用缓存恢复
    mLogicalPresentationDisabled = false;
    return true;
}
```

配套两处：

- **`getLogicalSize()`**——若读到 `mode == DISABLED` 且缓存有效，**回退缓存值**而不是返回 `0`，避免调用方拿到 `0x0` 尺寸。
- **`syncLogicalPresentationState()`**——构造时调一次，把渲染器当前配置同步进缓存（若渲染器已是非 DISABLED 模式）；此后 disable/enable 都以缓存为唯一真相源。
- **`initSDL` 的 `SDL_SetRenderLogicalPresentation` 加失败检查**——初始设置逻辑分辨率失败立刻 `return false`，不再静默继续。

### 5. 纹理 scale 模式保护：先判空、后操作

```cpp
SDL_Texture* rawTexture = IMG_LoadTexture(mRenderer, filePath.data());
if (!rawTexture) {   // ★ 先判空
    spdlog::error("加载纹理失败: '{}': {}", filePath.data(), SDL_GetError());
    return nullptr;
}
// ★ 空检查之后才碰纹理
if (!SDL_SetTextureScaleMode(rawTexture, SDL_SCALEMODE_NEAREST)) {
    spdlog::warn("无法设置纹理缩放模式为最邻近插值: {}", SDL_GetError());
}
```

文件缺失/路径错 → `IMG_LoadTexture` 返回 `nullptr` → 旧代码此时 `SDL_SetTextureScaleMode(nullptr,…)` 在空指针上调用（未定义行为，可能崩）。**"先判空再操作资源句柄"**是铁律——纹理加载失败应该走正常错误路径（记日志、返回 `nullptr`），而不是在已失败的句柄上继续调用 SDL API。

### 6. 杂项：ImGui 缩放 / 控制台编码 / /MP / imgui.ini 复制

- **ImGui 缩放固定 `1.0f`**——原 `SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay())` 跟随系统 DPI，高 DPI 显示器上窗口按缩放系数整体放大、布局偏移不稳定。固定 1.0 牺牲"跟随系统缩放"换**跨显示器稳定**。参考明确注释这是有意的取舍。
- **控制台 UTF-8**——`SetConsoleOutputCP(CP_UTF8)` + `SetConsoleCP(CP_UTF8)`（`_WIN32` 下），配合 CMake 的 `/utf-8` 编译器标志，双端对齐 UTF-8，中文日志不再乱码。
- **/MP 并行编译**——`/MP` 让**单个 cl 进程内并行编译多个 .cpp**（MSVC 特有），和 Ninja 的"多进程并行编译不同文件"是**两个维度**的并行，叠加上限更高。
- **imgui.ini 复制**——用 `file(GENERATE)` + `add_custom_command(PRE_BUILD)` 把仓库里的 `imgui.ini` 复制到 exe 目录，**仅当目标不存在**（`if(NOT EXISTS)`）：首次运行就有预置调试布局；用户之后在窗口上拖动调整过的布局不会被每次构建覆盖。

---

## 与参考实现（WispSnow/MonsterWar，commit 5f9f01b~cb83201）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段后缀 `logical_width_/window_/renderer_` 等 | m-prefix：`mLogicalWidth/mWindow/mRenderer` 等 | 本地 m-prefix 规范 |
| 2 | `setState` 里 `current_state_ = std::move(new_state)`（枚举 move） | 本地保持普通赋值 | 枚举类型 move 与拷贝等价，无实际收益，不追赶 |
| 3 | `setWindowSize/setLogicalSize(const glm::vec2&)` | 本地按值 `glm::vec2` | glm::vec2 仅 8 字节，按值与 const& 开销相当，保持本地风格 |
| 4 | 参考 game_state.h 私有段在前 | 本地公开段在前 | 本地原有头文件布局，不因本课改动 |
| 5 | 缩进 2 空格 | game/scene 用 tab、game_state 用 tab、其余 4 空格 | 本地各目录缩进约定 |
| 6 | — | 本地纹理 scale 保护额外在 warn 里带 `SDL_GetError()` | 更利于定位纹理设置失败原因 |

**本课无玩法/接口变化**：7 个提交全部是引擎层加固与构建卫生，`Scene` 对外接口从 `void init()` 变 `bool init()` 是唯一有语义影响的变更，但四个场景内部调用方（SceneManager）同步适配，外部无感。

---

## 学习要点

### 1. `[[nodiscard]] bool init()`：把失败变成编译期可见的信号

初始化失败被静默吞掉是"错误延迟爆发"的经典案例——残破场景压栈后每帧空转，错误到深不可测的运行时才暴露。`[[nodiscard]]` 让**忽略返回值本身变成编译警告**，加上 `SceneManager` 检查失败即 `QuitEvent`，做到"初始化失败，第一时间、最快路径退出"。**工程上：能编译期拦的，别拖到运行时。**

### 2. 父类 init 串链约定，能逼出结构问题

"子类 init 末尾必须 `return Scene::init()`"这条约定在这次改造里直接揪出 **LevelClearScene/EndScene 没调父类 init 的潜伏 bug**（`mIsInitialized` 恒 false → 可能被重复初始化）。改 `void→bool` 不只是类型变化——它强迫每个子类想清楚"我到底有没有完整走一遍初始化链"。

### 3. shared_ptr 传参：消费就 move，借用就 const&

按值传 shared_ptr 再赋成员 = 3 次原子操作（拷贝+1、赋成员+1、析构-1）。`std::move` 转成所有权转移 = 0 次。**判断标准：参数最终要存进成员、调用方不再需要 → move；只是读 → `const&`。** 这是现代 C++ 性能卫生的基本功。

### 4. 读-改-写往返依赖"中间状态不变"，跨 disable 就塌 → 缓存真相源

`enableLogicalPresentation` 读当前尺寸再设——这个模式隐含假设"读和写之间逻辑分辨率没变"。SDL3.4 一改 `GetRenderLogicalPresentation` 禁用时返回 `0x0`，假设直接崩溃。**对"会被临时关闭/恢复"的配置，永远在关闭前缓存一份"最近有效值"，恢复用缓存**。缓存是唯一真相源，绝不在恢复时去读"当前"（当前可能已是残缺状态）。

### 5. 先判空、后碰资源句柄

`SDL_SetTextureScaleMode(nullptr,…)` 在失败路径上继续调用 SDL API，是把"加载失败"升级成"未定义行为"的典型。**任何资源句柄（纹理/音频/字体）操作前，先判空**；失败走正常错误路径。

### 6. 稳定的调试体验也是工程能力

ImGui 固定 1.0 缩放（跨显示器稳定优先于跟随 DPI）、imgui.ini 预置布局并"仅缺失时复制"（不覆盖用户调整）、/MP 并行编译（叠两个维度的并行）——**这些"看不见"的改动直接决定日常开发体验**。可靠性不只是 crash-free，还包括"调试工具稳定、编译够快、日志不乱码"。
