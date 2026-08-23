# 主场景的完善：暂停系统 / 场景重开 / DebugUI 四大窗口 / 盾御守卫动画

## 问题

第 9 课（009）之后，游戏"能打"了：出击布阵、波次刷怪、自动战斗、技能施放全链路都通了。但主场景 GameScene 仍然是**测试向**的壳子，离"可玩向"差几块拼图：

1. **没有暂停系统**——战斗一旦开打只能看完，没法停下看状态、调参数。
2. **没有重开/返回/保存**——GameScene 的数据全部在 `init()` 里懒创建，场景之间无法共享同一份数据，自然也无法"重开本关复用同一份进度"。
3. **DebugUI 只有悬浮 tooltip + 角色状态两个窗口**——关卡信息（基地血量/COST/波次）、设置工具（倍速/音量/暂停/重开）、调试工具（加钱/通关）都没有。
4. **选中面板没有升级/撤退**——玩家单位只能"摆在那"，升级（U）、撤退（R 返还一半 cost）这两个塔防核心操作缺失。
5. **盾御（shield）的 guard 动画缺失**——技能激活时角色应该摆出"守卫"姿态，但动画系统只会让玩家回到 idle。
6. 冒烟测试中还暴露了**两个既有 bug**：基地被毁时 `GameEndEvent` 无人消费导致 EnTT dispatcher 迭代器失效崩溃（BEX64），以及基地血量会继续变负、重复触发游戏结束。

一句话：**把 GameScene 从"能看能打"完善成"可暂停、可重开、可调参、可操作"的可玩主场景**——引入暂停系统、场景重开事件与 GameScene 构造器依赖注入、DebugUI 四大窗口、升级/撤退按钮、盾御守卫动画，并顺手修掉冒烟测试暴露的崩溃与负数 bug。

## 结论

本课新增一条"主场景操作层"：

```
DebugUISystem（ImGui 四大窗口）
    ├── renderHoveredPortrait（肖像悬浮 tooltip：会话数据 + 蓝图 statModify 重算属性）
    ├── renderSelectedUnit（角色状态 + 升级U + 撤退R + 技能面板）
    ├── renderInfoUI（关卡信息：基地血量/COST/剩余波次/下一波/击杀/关卡号）
    ├── renderSettingUI（设置工具：暂停P/重开/返回标题/保存 + 倍速 + 音量 + 显示调试工具开关）
    └── renderDebugUI（调试工具：COST+10/+100 / 通关）
              │
              ▼ 事件
    RestartEvent / BackToTitleEvent / SaveEvent（场景控制）
    UpgradeUnitEvent / RetreatEvent（单位操作 → GameRuleSystem 处理）
              │
              ▼
GameScene（构造器 DI + 暂停分支 + 场景回调）
    ├── 暂停：update() 开头 isPaused() → 只跑放置/排序/选择/肖像，战斗冻结
    ├── onRestart：用同一份共享数据 requestReplaceScene(新 GameScene)
    └── SkillSystem / AnimationStateSystem / AttackStarterSystem（盾御 guard 动画）
```

- **Context 增加 Time 引用**（`getTime()`），设置工具才能调游戏倍速
- **SessionData 增加 `getUnitData(name_id)`** 访问器，肖像 tooltip 按角色名哈希查数据
- **新增三个场景事件** `RestartEvent` / `BackToTitleEvent` / `SaveEvent`
- **GameScene 构造器依赖注入**：签名改为 `(Context&, shared_ptr<BlueprintManager>=nullptr, shared_ptr<SessionData>=nullptr, shared_ptr<UIConfig>=nullptr, shared_ptr<LevelConfig>=nullptr)`，成员由构造赋值、保留 `if(!mX)` 懒创建兜底——重开时用同一份数据
- **暂停系统**：引擎 GameState 只管状态，`GameScene::update` 才是执行者——暂停分支只跑 place_unit / ysort / selection / portrait + Scene::update
- **DebugUI 升级/撤退**：升级扣 `player.mCost`、COST 不足置灰（`BeginDisabled`）、快捷键 U；撤退返还 `cost * 0.5`、快捷键 R
- **盾御守卫动画**：skill_system 激活播 guard、持续结束播 idle；animation_state 玩家分支按 shield+SkillActiveTag 决定 guard/idle；attack_starter 玩家攻击时补上 `ActionLockTag`（守卫姿态期间攻击动画不被硬直打断）
- **修复两个既有 bug**：`GameEndEvent` 在启动时连上 onGameEnd（创建 handler 节点，规避 dispatcher 迭代器失效崩溃）；GameRuleSystem 加 `mIsGameOver` 标志，基地被毁后忽略后续到达的敌人

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| Context 增加 Time 引用 + `getTime()` | `src/engine/core/context.h`（前向声明/构造参数/getter/成员）+ `context.cpp` |
| GameApp 传 Time 实参 | `src/engine/core/game_app.cpp:initContext` |
| `getUnitData` 访问器 | `src/game/data/session_data.h:63` |
| 三个场景事件 | `src/game/defs/events.h:124-130`（RestartEvent / BackToTitleEvent / SaveEvent） |
| GameScene 构造器 DI + 场景回调 | `src/game/scene/game_scene.h` + `game_scene.cpp:60-63`（构造赋值）/ `351-376`（回调） |
| 暂停分支 | `src/game/scene/game_scene.cpp:133-140`（update 开头） |
| 事件连接 | `src/game/scene/game_scene.cpp:initEventConnections`（Restart/BackToTitle/Save/GameEnd） |
| 初始化进入 Playing | `src/game/scene/game_scene.cpp:122` |
| DebugUI 成员与回调声明 | `src/game/system/debug_ui_system.h`（mHoveredPortrait / mShowDebugUI / dtor） |
| DebugUI 四窗口 + 升级/撤退 | `src/game/system/debug_ui_system.cpp`（renderHoveredPortrait/InfoUI/SettingUI/DebugUI） |
| 盾御 guard 动画 | `skill_system.cpp:64-67,84-87` + `animation_state_system.cpp:46-60` + `attack_starter_system.cpp:69` |
| 游戏结束防重复 bug 修复 | `src/game/system/game_rule_system.h/.cpp`（mIsGameOver 标志） |

### 2. GameScene 构造器依赖注入：为"重开复用同一份数据"铺路

改造前 GameScene 的数据（蓝图/会话/UI/关卡配置）全部在 `init()` 里 `make_shared` + `loadFromFile` 懒创建，场景销毁后数据跟着没。本课把签名改成构造传入、空则兜底：

```cpp
GameScene(engine::core::Context& context,
    std::shared_ptr<game::factory::BlueprintManager> blueprint_manager = nullptr,
    std::shared_ptr<game::data::SessionData> session_data = nullptr,
    std::shared_ptr<game::data::UIConfig> ui_config = nullptr,
    std::shared_ptr<game::data::LevelConfig> level_config = nullptr);
```

成员由初始化列表赋值，`init()` 里保留 `if (!mX) { mX = ...; load...; }` 双保险。为什么要这么改？

- **`onRestart` 要能构造新场景**：`requestReplaceScene(make_unique<GameScene>(mContext, mBlueprintManager, mSessionData, mUIConfig, mLevelConfig))`——同一份 shared_ptr 传给新场景，重开本关时进度（会话）、蓝图、关卡配置全部复用，只有 registry 里的战斗实体重建。
- **`SceneManager` 不自己 new 场景**：`scene_manager.cpp` 只接管事件里的 `unique_ptr`（`scene_manager.h` 的 `scene_setup_func` / PushSceneEvent），所以 `requestReplaceScene(make_unique<...>)` 天然成立，无需改引擎。
- **共享语义明确**：蓝图/会话/UI/关卡配置是"跨场景存活的数据"用 shared_ptr；`GameStats` 是"关卡内临时状态"用值语义存 ctx，重开时自然重建。

这就是 m-prefix 注释里"管理数据的实例很可能同时被多个场景使用，因此使用共享指针"的落地。

### 3. 暂停系统的分层：引擎存状态，游戏层执行

暂停不是"某个开关"，而是**两层分工**：

- **引擎层 GameState**（`game_state.h`）只存枚举：`State::Playing / Paused`，`isPaused()` / `setState()`——本课前就绪，零改动。
- **游戏层 GameScene::update** 才是暂停的执行者。暂停分支放在 `mRemoveDeadSystem->update` 之后：

```cpp
if (mContext.getGameState().isPaused()) {
    mPlaceUnitSystem->update(delta_time);   // 幽灵仍跟随鼠标
    mYsortSystem->update(mRegistry);        // 排序照常（视觉正确）
    mSelectionSystem->update();             // 悬浮/选中仍可点
    mUnitsPortraitUI->update(delta_time);   // 肖像可滚动
    Scene::update(delta_time);              // 引擎基础更新
    return;                                 // 战斗/计时/寻路全冻结
}
```

要点：

- **哪些"仍要动"、哪些"全冻结"是人为划分的**：战斗（AttackStarter/Projectile/Movement）、计时（Timer）、规则（cost 恢复/通关计时）、寻路（FollowPath）、刷怪（EnemySpawner）全部不跑；而"交互预览"（鼠标放置幽灵、悬浮选中、肖像滚动）和"引擎基础"保留。因为暂停时要能用设置窗口调参，也希望能预演布阵。
- **渲染照常跑**：`render()` 不检查暂停——否则 ImGui 设置窗口就看不见、操作不了了。暂停只冻结 `update` 里的游戏逻辑。
- **init() 末尾 `setState(Playing)`**：新场景进来显式进入运行态（GameState 默认状态是 Playing，但显式设置更明确，也为暂停/恢复状态机提供稳定入口）。

### 4. DebugUI 四大窗口 + 升级/撤退：ImGui 交互技巧集

本课 DebugUISystem 从"两个只读窗口"扩成"四个窗口 + 两枚操作按钮"，`update()` 变成：

```cpp
beginFrame();
renderHoveredPortrait();
renderHoveredUnit();
renderSelectedUnit();
renderInfoUI();
renderSettingUI();
renderDebugUI();
endFrame();
```

**① 肖像悬浮 tooltip（renderHoveredPortrait）**——与悬浮单位 tooltip 的关键差异：肖像代表的是**角色档案**（会话数据），不是场上的实体。所以数据源是 `session_data->getUnitData(mHoveredPortrait)` + `blueprint_mgr->getPlayerClassBlueprint(class_id)`，再用 `statModify` 按等级/稀有度重算属性：

```cpp
const auto& unit_data = session_data->getUnitData(mHoveredPortrait);
const auto& class_blueprint = blueprint_mgr->getPlayerClassBlueprint(unit_data.mClassId);
const auto hp = engine::utils::statModify(class_blueprint.mStats.mHp, unit_data.mLevel, unit_data.mRarity);
```

`mHoveredPortrait` 由 `UIPortraitHoverEnterEvent`（置为 `event.mNameId`）/ `UIPortraitHoverLeaveEvent`（置 `entt::null`）驱动——DebugUISystem 构造时订阅、析构时 `disconnect`。

**② 升级/撤退（renderSelectedUnit）**——放在技能区块之前：

```cpp
const auto& player = mRegistry.get<game::component::PlayerComponent>(entity);
auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
ImGui::BeginDisabled(game_stats.mCost < player.mCost);          // COST 不足置灰
ImGui::SetNextItemShortcut(ImGuiKey_U, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
if (ImGui::Button("升级")) {
    mContext.getDispatcher().enqueue(game::defs::UpgradeUnitEvent{ entity, player.mCost });
}
ImGui::SameLine();
ImGui::Text("快捷键 U: COST消费: %d", player.mCost);
ImGui::EndDisabled();
// 撤退：返回 50% COST，无禁用条件（撤退永远允许）
auto return_cost = static_cast<int>(player.mCost * 0.5f);
```

- **升级扣费**与出击同价（`player.mCost`）；**撤退返半**（`cost * 0.5`）。数值决策在 DebugUI，执行在 GameRuleSystem（扣费/升级重算属性/返还/移除单位都是既有事件处理，本课零改动）。
- `BeginDisabled` 让 COST 不足时按钮置灰不可点，比"点了再失败"更友好；`SetNextItemShortcut` 给按钮绑快捷键 U/R。

**③ 关卡信息（renderInfoUI）**——纯读 ctx 展示：基地血量（`GameStats.mHomeHp`）、COST、剩余波次（`Waves.mWaves.size()`）、下一波倒计时（`Waves.mNextWaveCountDown`）、击杀（`mEnemyKilledCount/mEnemyCount`）、当前关卡（`SessionData`）。

**④ 设置工具（renderSettingUI）**——本课交互最丰富的窗口：
- **暂停/继续**：`SetNextItemShortcut(ImGuiKey_P)` + 按 `game_state.isPaused()` 切换文案与 `setState(Paused/Playing)`。**P 键语义从此从"清空所有玩家单位"改成"暂停/继续"**。
- **场景控制**：重新开始 → `RestartEvent`；返回标题 → `BackToTitleEvent`；保存 → `SaveEvent`（后两个是 TODO 桩）。
- **游戏倍速**：0.5/1/2 三按钮 + `SliderFloat("游戏速度", &time_scale, 0.5f, 2.0f)`，每帧 `time.setTimeScale(time_scale)`——这就是本课给 Context 加 Time 引用的原因。
- **音量**：`getMusicVolume/getSoundVolume` + `SliderFloat` + `setMusicVolume/setSoundVolume`（AudioPlayer 既有 API，零改动）。
- **显示调试工具开关**：`Checkbox("显示调试工具", &mShowDebugUI)`，控制 `renderDebugUI()` 是否绘制。

**⑤ 调试工具（renderDebugUI）**——`mShowDebugUI` 为假直接 return；COST+10 / COST+100 直接改 `game_stats.mCost`；通关发 `LevelClearEvent`（预期无可见后果，通关场景切换是后续课）。

### 5. 盾御守卫动画：ActionLockTag 的三处配合

盾御（shield）激活时角色应摆出"守卫"姿态（guard 动画），持续结束后回到 idle。动画系统收到 `AnimationFinishedEvent` 会按类型恢复循环动画，玩家分支原先一律回 idle——本课用**"盾御 + 技能激活中"特判**：

```cpp
// animation_state_system.cpp 玩家分支
const auto& skill = mRegistry.get<game::component::SkillComponent>(event.mEntity);
if (skill.mSkillId == "shield"_hs && mRegistry.any_of<game::defs::SkillActiveTag>(event.mEntity)) {
    mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "guard"_hs, true });
} else {
    mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "idle"_hs, true });
}
mRegistry.remove<game::defs::ActionLockTag>(event.mEntity);   // 解除硬直
```

三处配合才能让"守卫姿态"闭环：

| 位置 | 动作 | 作用 |
|------|------|------|
| `skill_system.cpp:64-67` | 盾御激活时（且未锁动作）enqueue `guard` | 施放即摆守卫姿势 |
| `skill_system.cpp:84-87` | 盾御持续结束时（且未锁动作）enqueue `idle` | 收招回待机 |
| `attack_starter_system.cpp:69` | **玩家攻击时 `emplace_or_replace<ActionLockTag>`** | 攻击动画播放期间动作锁定，守卫动画不被硬直打断 |
| `animation_state_system.cpp:46-60` | 玩家动画结束时按"盾御+激活中"回 guard / 否则 idle，随后 `remove<ActionLockTag>` | 动画结束恢复守卫姿态，并解除硬直 |

关键点是**本课给玩家补上了 `ActionLockTag`**。此前参考注释认为"玩家静止不动不需要"，导致玩家攻击动画结束时会立刻被动画系统切回 idle/guard，攻击动作还没播完就被顶掉。补上锁定后：玩家攻击 → emplace ActionLock（锁住）→ 攻击动画播完 → AnimationFinishedEvent → 动画状态系统按盾御状态回 guard/idle → remove ActionLock（解锁）。守卫期间的任何打断都被 ActionLock 挡住，guard 动画完整展示。

**为什么 `skill_system` 播 guard/idle 前要检查 `!any_of<ActionLockTag>`？** 因为如果盾御激活时玩家恰好正在攻击（动作锁定中），硬播 guard 会和攻击动画打架。跳过则等攻击动画结束后由 animation_state 特判回 guard——两处逻辑串成"最终回到守卫姿态"。

### 6. 冒烟测试暴露的两个既有 bug 修复

本课冒烟测试（临时把基地血量调到 1）暴露了两个预存 bug，已修复并验证：

**① `GameEndEvent` 无人消费 → dispatcher 迭代器失效崩溃（BEX64）**

根因：EnTT `dispatcher::update()`（dispatcher.hpp:385-389）迭代 handler dense_map，如果某个 handler 处理事件时 `enqueue` 了一个**尚无 handler 节点的新事件类型**，`assure<Type>()` 会向 dense_map 插入节点 → vector 重分配 → 正在迭代的迭代器失效 → 崩溃（0xc0000409, FAST_FAIL_INVALID_ARG, ucrtbased.dll）。

本场景里 `GameEndEvent` 是唯一"运行期才第一次被 enqueue 且无人消费"的事件（GameRuleSystem::onEnemyArriveHome 基地被毁时发出，而那时没有连接任何消费者）。修复：启动时就连上 `onGameEnd` 桩——**让 EnTT 在一开始就为 GameEndEvent 创建 handler 节点**，运行期 enqueue 时不再触发重分配。

```cpp
// initEventConnections
dispatcher.sink<game::defs::GameEndEvent>().connect<&GameScene::onGameEnd>(this);
```

**② 基地血量变负 + 重复触发游戏结束**

`onEnemyArriveHome` 每来一个敌人 `mHomeHp -= 1`，基地被毁后继续扣到负数，且每次到达都重发 `GameEndEvent`。修复：加 `mIsGameOver` 标志，基地被毁置位后，后续到达直接忽略：

```cpp
if (mIsGameOver) return;                     // 游戏结束后忽略后续到达
game_stats.mEnemyArrivedCount++;
game_stats.mHomeHp -= 1;
if (game_stats.mHomeHp <= 0) {
    mIsGameOver = true;
    mDispatcher.enqueue(game::defs::GameEndEvent{ false });
}
```

验证：基地血量=1 时跑 25 秒，恰好 1 次"基地被摧毁" + 1 次"游戏结束"，无崩溃、无 WER。

---

## 与参考实现（WispSnow/MonsterWar，commit 9a2283e）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段 `unit_map_` 等 | m-prefix：`mUnitMap/mHoveredPortrait/mShowDebugUI/mTime` | 本地 m-prefix 规范 |
| 2 | `Context` 无 Time、无 `getTime()` | 本课新增 `getTime()` + `mTime` 成员 + ctor 参数 | 设置工具倍速需要访问 Time（与参考一致的方向，本地补全） |
| 3 | `onClearAllPlayers()` + "pause" 输入连接 | 本课移除，P 改作暂停 | 本地早前把 P 绑到"清空玩家"，本课语义变更 |
| 4 | `GameEndEvent` 无人消费 | 本课连接 `onGameEnd` 桩（防 dispatcher 崩溃） | 冒烟测试发现的预存崩溃，参考实现同样有潜在 bug |
| 5 | 基地血量可跌负、重复触发结束 | 本课加 `mIsGameOver` 标志 | 冒烟测试发现的预存负数 bug |
| 6 | 缩进 2 空格 | game/scene 用 tab，其余 game 目录 4 空格 | 本地各目录缩进约定 |
| 7 | 大部分事件/逻辑（Upgrade/Retreat/肖像滚动/statModify/GameState/requestReplaceScene） | 本地早已实现 | 本地就绪度高，本课"空手接" |

**本地就绪度说明**：参考课需要新建的一大半东西（Upgrade/Retreat 事件与 GameRuleSystem 处理、肖像滚动、`Time::getTimeScale/setTimeScale`、AudioPlayer 音量 API、`requestReplaceScene`、`statModify`、GameState 状态机）本地在更早的课已就绪。本课真正新增的是：Context 的 Time 引用、`getUnitData`、三个场景事件、GameScene 构造器 DI + 场景回调、暂停分支、DebugUI 四窗口 + 升级/撤退按钮、盾御守卫动画，以及两个预存 bug 修复。

---

## 学习要点

### 1. 依赖注入（DI）让"场景重开"复用数据成为可能

GameScene 数据从"init 内部懒创建"改为"构造器传入 + 空则兜底"，表面是签名变化，实质是**数据所有权的转移**：数据不再属于某个场景实例，而是跨场景共享（shared_ptr），场景实例只是"借来用"。这样 `requestReplaceScene(make_unique<GameScene>(mContext, mBlueprintManager, ...))` 才能带着同一份进度重开。**面向"复用"设计数据所有权，而不是面向"创建"**。

### 2. 暂停是"状态 + 执行"两层：引擎只存状态，游戏层决定冻结范围

GameState 只提供一个 `isPaused()`/`setState()` 的开关；真正"哪些系统照跑、哪些冻结"是 GameScene::update 里一个 `if` 分支的人为划分。这暴露了一个通用原则：**状态枚举只管"是什么"，行为策略在更高层决策**。引擎保持通用（不知道"暂停时肖像还能滚"这种游戏规则），游戏层才有话语权。

### 3. EnTT dispatcher 的迭代器失效坑：enqueue 未消费事件会崩

`dispatcher::update()` 在迭代 handler dense_map 时，若 handler 内 enqueue 了**尚无节点的新类型**，`assure()` 插入会导致 vector 重分配、迭代器失效。规避手段：**启动时先连接该事件的一个空消费者**（`sink<GameEndEvent>().connect<...>(this)`），让节点预先存在。这是 EnTT 的使用者手册之外才踩得到的深坑——**"事件有没有人听"在 dispatcher 里不是运行期概念，而是容器结构的一部分**。

### 4. 单位操作走"事件驱动"而非"直接调用"：DebugUI 只发事件

升级/撤退按钮只做一件事：`enqueue(UpgradeUnitEvent/RetreatEvent)`。扣 cost、等级+1、重算属性、返还 cost、移除单位全在 GameRuleSystem 的事件回调里。DebugUI 不持有规则逻辑，**UI 只表达意图，规则系统只执行意图**——两个系统通过事件解耦，互不调用。这是本课最典型的 ECS 协作示范。

### 5. `BeginDisabled` + `SetNextItemShortcut`：ImGui 的交互三件套

`BeginDisabled(条件)` 置灰 + `SetNextItemShortcut(键)` 绑快捷键 + `Button()` 触发 + `SameLine()` 并排提示——一个可禁用、有快捷键、有提示的按钮就这么组合出来。注意 `SetNextItemShortcut` 只对"下一个 Item"生效，每个按钮前都要单独设置。

### 6. 动画恢复不是"一刀切 idle"，而是"状态特判"

AnimationStateSystem 收到动画结束事件后，玩家分支从"一律 idle"升级为"盾御+激活中 → guard，否则 idle"。**动画恢复逻辑要能感知单位当前状态（技能激活、被阻挡等）**——用 `SkillComponent.mSkillId` + `SkillActiveTag` 组合判定，而不是硬编码动画名。配合 ActionLockTag 保证攻击动画不被中途顶掉，形成"攻击→守卫→收招"的完整姿态链。

### 7. 冒烟测试的价值：最低成本暴露最深层的崩溃

把基地血量临时调到 1 的一次冒烟运行，暴露了"dispatcher 迭代器失效"这种只在特定时序（基地被毁瞬间）才触发的崩溃，以及负数血量逻辑错误。**用可复现的最小场景（调参 + 短时运行）比长期盲跑更能快速命中深层 bug**——这比本课任何功能代码都更值得记住的工程方法。
