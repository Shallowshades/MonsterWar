# ImGui 显示单位信息：选择系统 + 命名上下文

## 问题

上一课（006）把 ImGui 接进了引擎，但 `DebugUISystem` 只能画一个演示窗口——里面是"窗口1 + 按钮 + 音量滑条 + Demo 窗口"，和游戏完全无关。当时预留了 `mRegistry` 成员（注释写着"后续课程用其查看实体信息"），但一直没有真正用上。

游戏里**看不到任何单位的具体状态**：敌人多少血、走多远了、我这个远程单位攻击范围多大……想了解依然只能靠 log。调试 GUI 的核心价值——"运行时直接观察游戏内部"——还没兑现。

一句话：**把上一课搭好的 ImGui 管线从"演示玩具"升级为"单位侦察工具"——悬浮看信息、点击选单位、选中看面板、画攻击范围圆。**

## 结论

新增一个 `SelectionSystem` 专门处理"鼠标悬浮"和"鼠标选中"两类交互，`DebugUISystem` 只读状态、负责画两样东西：

```
SelectionSystem（每帧 update + 输入回调）
    ├─ update()：鼠标位置 vs 每个单位 → 距离平方 ≤ HOVER_RADIUS² → 记录"悬浮单位"（玩家优先）
    ├─ onMouseLeftClick()：悬浮单位是玩家 → 选中它 + 加 ShowRangeTag（画范围圆信号）+ return true
    └─ onMouseRightClick()：清除选中 + return false（穿透给取消放置）
            │ 写入 registry.ctx() 命名上下文（emplace_as）
            ▼
DebugUISystem（每帧只读）
    ├─ renderHoveredUnit()：BeginTooltip 悬浮小窗 → 等级/HP/ATK/射程……
    └─ renderSelectedUnit()：左上角「角色状态」窗口 → 同上 + 阻挡数量
RenderRangeSystem（第二段 view）
    └─ ShowRangeTag + Transform + Stats → drawFilledCircle 画已放置单位攻击范围圆
```

支撑它的一块新地基：**`registry.ctx().emplace_as<T>("key"_hs, value)` 命名上下文**——用 FNV-1a 哈希字符串做键，把 GameScene 的两个成员（`mSelectedUnit`/`mHoveredUnit`）的**引用**注册进去，多个系统通过 `ctx().get<T&>("key"_hs)` 读写**同一份**状态。

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| 选择系统 `SelectionSystem` | `src/game/system/selection_system.h/.cpp`（新增） |
| 悬浮检测半径 `HOVER_RADIUS` | `src/game/defs/constants.h` |
| 悬浮 tooltip / 角色状态窗口 | `src/game/system/debug_ui_system.h/.cpp` |
| 已放置单位范围圆 | `src/game/system/render_range_system.cpp` |
| 命名上下文注册（emplace_as） | `src/game/scene/game_scene.cpp:initRegistryContext` |
| 选中/悬浮状态成员 | `src/game/scene/game_scene.h` |
| CMake 源文件 | `CMakeLists.txt`（+`selection_system.cpp`） |

### 2. `emplace_as` 命名上下文：系统间共享可变状态的新通道

这是本课最重要的新知识点。回顾项目里系统间共享数据的三种方式：

| 方式 | 用法 | 场景 |
|------|------|------|
| 构造传引用 | 系统构造时持有 `registry&`、`context&` | 稳定的模块依赖 |
| 值语义 ctx | `ctx().emplace<GameStats>(mGameStats)` | 每帧重建的关卡统计 |
| **命名 ctx（本课）** | `ctx().emplace_as<T&>("key"_hs, ref)` | **多个系统需要读写同一份"当前状态"** |

```cpp
// game_scene.cpp — 注册阶段（一次）
mRegistry.ctx().emplace_as<entt::entity&>("selected_unit"_hs, mSelectedUnit);
mRegistry.ctx().emplace_as<entt::entity&>("hovered_unit"_hs, mHoveredUnit);

// SelectionSystem::update — 写（每帧）
mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs) = entity;

// DebugUISystem::renderHoveredUnit — 读（每帧）
auto& entity = mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs);
```

关键点：`emplace_as<T&>` 存的是**引用**，`get<T&>("key"_hs)` 取回的也是**引用**——所以 SelectionSystem 写的值，DebugUISystem 下一帧就能读到，中间没有任何拷贝，双方都操作 GameScene 里的那一个 `entt::entity`。`"selected_unit"_hs` 是 `entt::hashed_string` 字面量（FNV-1a 哈希成 `entt::id_type`），键是编译期常量，查询是 O(1)。

为什么这么设计？因为"当前选中的单位"属于**场景级会话状态**——它不属于某个具体系统，而是多个系统（选择、UI、范围渲染）共同关心的横切状态。放 ctx 里，谁需要谁按名取，系统之间零直接依赖。

### 3. SelectionSystem：悬浮（预览）与选中（确定）分离

悬浮和选中是两件**语义完全不同**的事，被刻意拆开：

```cpp
void SelectionSystem::update() {
    auto mouse_pos = mContext.getInputManager().getLogicalMousePosition();
    // 玩家 view 优先
    for (auto entity : view<Transform, PlayerComponent>) {
        if (distanceSquared(transform.mPosition, mouse_pos) <= HOVER_RADIUS²) {
            ctx["hovered_unit"_hs] = entity; return;   // 找到即返回
        }
    }
    // 再查敌人 view
    for (auto entity : view<Transform, EnemyComponent>) { ... }
    ctx["hovered_unit"_hs] = entt::null;               // 都没有 → 清空
}
```

- **hovered（悬浮）**：`update()` **每帧重算**，纯查询不改游戏状态。鼠标不动它就稳定、一动就变，本质是"鼠标当前指的谁"这个瞬时问题的答案。它驱动的是 tooltip——一种"预览"。
- **selected（选中）**：只在 `onMouseLeftClick()` 点击瞬间变更，是**持久选择**。它驱动的是「角色状态」面板和攻击范围圆——需要"定下来"的信息。

悬浮检测用**距离平方**比较（`<= HOVER_RADIUS * HOVER_RADIUS`），避免开平方——`distanceSquared` 是 `math.h` 里现成的工具。先查玩家再查敌人，因为玩家单位通常更值得优先选中（可操作），敌人只可看不可点。

### 4. 输入回调的 collect 语义：return true 拦截派发

`onMouseLeftClick` 的返回值不是摆设：

```cpp
bool SelectionSystem::onMouseLeftClick() {
    auto hovered = ctx["hovered_unit"_hs];
    if (hovered == entt::null || !mRegistry.valid(hovered)) return false;
    if (mRegistry.try_get<PlayerComponent>(hovered)) {
        clearCurrentSelection();
        ctx["selected_unit"_hs] = hovered;
        mRegistry.emplace_or_replace<ShowRangeTag>(hovered);
        return true;   // 选中成功，事件到此为止
    }
    return false;
}
bool SelectionSystem::onMouseRightClick() {
    clearCurrentSelection();
    return false;   // 让右键穿透
}
```

`InputManager::onAction()` 返回的是 `entt::sink`，同一个信号可以连多个订阅者。`mouse_left` 已经有 `PlaceUnitSystem` 注册（放置单位），`SelectionSystem` 又注册一个——事件派发时按注册顺序调用，**某个回调返回 true 就停止继续派发**（collect 语义）。所以：
- 左键点在单位上 → SelectionSystem 先处理，return true → PlaceUnitSystem 不再收到这次点击，不会误放单位
- 左键点在空地上 → SelectionSystem 的悬浮是 null，return false → 事件继续，PlaceUnitSystem 正常放置
- 右键 → 清除选中后 return false → PlaceUnitSystem 的取消放置照常响应

这个"返回布尔决定是否拦截"的机制，正是上一课 `WantCaptureMouse` 在 ImGui 层面的镜像——**横切功能要显式声明自己吞掉了哪些输入**。

### 5. DebugUISystem 只读：观察者不该改状态

`renderHoveredUnit` / `renderSelectedUnit` 全程只 `ctx().get` + `registry_.get/try_get` 读组件，**从不写**。这是调试系统的纪律：观察者只读数据、不修改游戏状态。两个窗口都是即时模式函数：

```cpp
void DebugUISystem::renderHoveredUnit() {
    auto& entity = mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs);
    if (entity == entt::null || !mRegistry.valid(entity)) return;   // 悬浮不存在 → 啥也不画
    if (!ImGui::BeginTooltip()) { ImGui::EndTooltip(); spdlog::error(...); return; }
    // 只有玩家单位有 NameComponent，所以 try_get（可能没有）
    if (auto name = mRegistry.try_get<engine::component::NameComponent>(entity); name) {
        ImGui::Text("%s  ", name->mName.c_str());
        ImGui::SameLine();          // 姓名和职业并排
    }
    ImGui::Text("%s", class_name.mClassName.c_str());
    ImGui::Text("等级: %d", stats.mLevel);
    ImGui::SameLine();
    ImGui::Text("稀有度: %d", stats.mRarity);
    ...
    ImGui::EndTooltip();
}
```

- `BeginTooltip` 是悬浮在鼠标旁的自动定位小窗；`renderSelectedUnit` 用 `SetNextWindowPos(ImVec2(10,10))` + `Begin("角色状态", NoTitleBar)` 钉在左上角
- `try_get<T>` 返回指针，用于"该组件可有可无"的情况——玩家单位有 `NameComponent` 而敌人没有；`BlockerComponent` 同理
- `registry_.valid(entity)` 双重保险：ctx 里的 entity 可能已被清理（死亡），防悬空

### 6. RenderRangeSystem 第二段 view：已放置单位也有范围圆

上一课只画"预备幽灵"的范围圆。本课加了第二段：

```cpp
// 预备幽灵（原有）
auto view_prep = registry.view<ShowRangeTag, Transform, UnitPrepComponent>();
// 已放置单位（新增）—— ShowRangeTag 由 SelectionSystem 选中时打上
auto view_remote = registry.view<ShowRangeTag, Transform, StatsComponent>();
for (auto entity : view_remote) {
    renderer.drawFilledCircle(camera, transform.mPosition, stats.mRange, RANGE_COLOR);
}
```

两个 view 共享 `ShowRangeTag` 但组件签名不同：预备幽灵用 `UnitPrepComponent.mRange`（准备时的范围），已放置单位用 `StatsComponent.mRange`（正式属性）。`ShowRangeTag` 在这里成了"选中"的副作用信号——选中时 `emplace_or_replace<ShowRangeTag>` 打上，清除时 `registry.remove<ShowRangeTag>` 摘掉，`RenderRangeSystem` 只需按 tag 遍历，完全不需要知道"选中"这回事。**标签即信号的 ECS 惯用法**。

### 7. 与参考实现（WispSnow/MonsterWar，commit e6c8c9c）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段 `level_/hp_/atk_...`、`name_.c_str()` | m-prefix：`mLevel/mHp/mAtk...`、`mName.c_str()` | 遵循本地 m-prefix 规范 |
| 2 | 命名 `selected_unit_`/`hovered_unit_`/`registry_`/`context_` | `mSelectedUnit`/`mHoveredUnit`/`mRegistry`/`mContext` | 同上 |
| 3 | `defs::HOVER_RADIUS` | `game::defs::HOVER_RADIUS` | 本地常量全限定命名空间引用 |
| 4 | 逻辑、结构 | 完全照搬 | 本课无本地设计差异 |

---

## 学习要点

### 1. 三种系统间共享状态的方式，按需选择

构造传引用（稳定依赖）、值语义 ctx（每帧数据）、命名 ctx `emplace_as<T&>`（多个系统读写同一份会话状态）。命名 ctx 的哈希字符串键让调用方之间零直接依赖，代价是类型和键在编译期没有互相校验——拼错键会在运行时才暴露。

### 2. "悬浮"和"选中"是两种不同的状态模式

悬浮是**瞬时查询**（每帧重算，回答"鼠标指着谁"），选中是**持久决策**（只在点击时变化）。UI 里预览和确定分离，改动一个不会牵连另一个。思考交互系统时，先把"每帧都在变的东西"和"点一下才变的东西"分开建模。

### 3. collect 语义：回调返回 bool 控制事件是否继续派发

同一信号多个订阅者时，`return true` 表示"我处理了，别再往下传"。这是横切功能声明"我吞了这次输入"的方式——选中成功就拦住放置，右键清除选中但放行取消放置。**返回值不是成功/失败的标志，而是派发闸门**。

### 4. `try_get<T>` vs `get<T>`：组件可有可无时的安全读取

`get<T>` 要求实体必有该组件（没有则 UB/抛错），`try_get<T>` 返回 `T*`，没有就返回 `nullptr`。跨实体的统一查询（玩家有姓名、敌人没有）必须用 `try_get`。判断空指针后优雅降级（不显示姓名行）。

### 5. 标签即信号：副作用通过 tag 驱动其他系统

选中 → 打 `ShowRangeTag`；清除 → 摘 `ShowRangeTag`。`RenderRangeSystem` 只认 tag 不认"选中"概念，两个系统靠一个标签解耦。ECS 里 tag 是"扁平的事件"，比直接调另一个系统的方法更符合组件组合的哲学。

### 6. 调试系统保持只读

`DebugUISystem` 是观察者：所有 `ctx().get` + `try_get` 都是读操作，写状态完全交给 `SelectionSystem`。调试 UI 哪怕画错了也绝不允许污染游戏状态——这是它和"正式逻辑"的边界。
