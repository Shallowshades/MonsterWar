# 通关场景与结束场景：LevelClearScene / EndScene / 通关判定链

## 问题

第 11 课（011）把标题场景、存档/读档面板、角色表格都补齐了，游戏的"外圈流程"（标题 → 战斗 → 返回标题/保存）闭环。但战斗的**结局没有归宿**：

1. **赢了没去处**——`GameRuleSystem` 和 `CombatResolveSystem` 在全歼敌人时只发 `LevelClearDelayedEvent`，延迟计时结束后发 `LevelClearEvent`，但**没有场景监听它**（本地 `GameScene::onLevelClear` 还是 TODO 桩），游戏会卡在"敌人全死光了但没有通关界面"的僵局。
2. **输了没去处**——基地血量归零时发 `GameEndEvent`，本地 `onGameEnd()` 也是桩，同样僵住。
3. **没有奖励反馈**——通关该发多少积分、该不该进下一关，完全没有逻辑。
4. **没有胜负大结局**——最后一关通关 / 游戏失败的"终局画面"（胜利/失败 + 退出入口）不存在。
5. **下层场景会跟上层抢渲染**——如果直接 push 一个结算场景到栈顶，下层的 GameScene 每帧还在渲染自己的 ImGui 调试窗口，两套窗口会叠在一起打架。

一句话：**给"通关"和"游戏结束"各造一个场景，把通关判定事件链接通，并解决"场景栈里上下两层如何协同渲染"的门控问题**——新增 LevelClearScene（通关结算：奖励积分 + 可排序角色表 + 下一关/保存/返回标题）与 EndScene（游戏结束：胜利/失败大字 + 返回标题/退出游戏）。

## 结论

```
战斗中 → GameRuleSystem/CombatResolveSystem 判定
    │ 全歼敌人 或 敌人全到齐（剩余敌人已为零）
    ▼
LevelClearDelayedEvent{2.0s} → GameRuleSystem 延迟计时（mIsLevelClear + mLevelClearTimer）
    │ 计时归零
    ▼
LevelClearEvent → GameScene::onLevelClear
    │ 奖励积分 = 击杀数 + 基地血量×5；setLevelClear(true)；addPoint
    ├── isFinalLevel(当前关) ? ──▶ push EndScene(ctx, true)      （胜利终局）
    └── 否则 ────────────────▶ push LevelClearScene(ctx, BP, UI, LC, SD, gameStats)
                                    ├── 下一关 → addOneLevel + setLevelClear(false) → replace GameScene
                                    ├── 保存 → 存档面板 toggle
                                    └── 返回标题 → replace TitleScene

战斗中 → GameRuleSystem::onEnemyArriveHome：基地血量 ≤ 0
    ▼
GameEndEvent{mIsWin=false} → GameScene::onGameEndEvent → push EndScene(ctx, false)  （失败终局）
```

- **场景栈语义**：通关/结束用 **push**（压到 GameScene 上层），不是 replace——下层 GameScene 仍留在栈里。`SceneManager::update` 只更新栈顶（战斗已冻结），`render` 却**渲染整个栈**（战斗画面垫底、结算 UI 盖顶）。
- **渲染门控**：`GameScene::render()` 里 `if (isPlaying() || isPaused()) debug_ui_system_->update()`——结算/结束场景把状态切成 `LevelClear`/`GameOver` 后，GameScene 的调试 UI 就不再渲染，避免两层 ImGui 窗口叠加冲突；战斗精灵层照常垫底。
- **LevelClearScene**：DI 构造器 `(Context&, BP, UI, LC, SessionData, GameStats&)` 复用四份共享数据 + 引用关卡内统计；`init()` 判空 → `setState(LevelClear)` → ctx emplace 三份共享数据 → 播 `win` 音乐。
- **EndScene**：构造器 `(Context&, bool is_win=false)`；`init()` 按 is_win 播 `win`/`lose` + `setState(GameOver)`。
- **DebugUISystem 四分支**：`update()`（战斗）/ `updateTitle()`（标题）/ `updateLevelClear()`（结算）/ `updateEnd()`（结束），各渲染各的窗口。
- **BGM 四场景启用**：标题 `title_bgm` / 战斗 `battle_bgm` / 结算 `win` / 结束 `win|lose`——`resource_mapping.json` 早已映射好音频路径，本课真正把音乐放出来。

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| `State::LevelClear` + `isLevelClear()` | `src/engine/core/game_state.h` |
| LevelClearScene 声明/实现 | `src/game/scene/level_clear_scene.h/.cpp`（新建） |
| EndScene 声明/实现 | `src/game/scene/end_scene.h/.cpp`（新建） |
| 场景加入构建 | `CMakeLists.txt`（SOURCES 加 end_scene.cpp + level_clear_scene.cpp） |
| GameScene 接通事件 + onLevelClear/onGameEndEvent 实现 | `src/game/scene/game_scene.h/.cpp` |
| GameScene::render 调试UI门控 | `src/game/scene/game_scene.cpp:render` |
| 标题/战斗 BGM 启用 | `src/game/scene/title_scene.cpp` / `game_scene.cpp` |
| DebugUI 四分支声明 | `src/game/system/debug_ui_system.h` |
| DebugUI 结算/结束渲染函数 | `src/game/system/debug_ui_system.cpp`（updateLevelClear/updateEnd + 5 个渲染函数） |
| 通关判定（延迟事件入口） | `src/game/system/game_rule_system.cpp` / `combat_resolve_system.cpp`（本课零改动，本地已就绪） |

### 2. 通关判定链：双路判定 → 延迟计时 → 一次切换

通关判定有**两个入口**，因为"敌人清空"可以由两种方式达成：

- **全歼**（`CombatResolveSystem::onAttackEvent`）：最后一个敌人死亡 → `mEnemyKilledCount + mEnemyArrivedCount >= mEnemyCount` → 发 `LevelClearDelayedEvent{2.0f}`。
- **清场**（`GameRuleSystem::onEnemyArriveHome` 的 else 分支）：所有敌人都已经到过基地（击杀 + 到达 = 总数，基地还没被摧毁）→ 同样发 `LevelClearDelayedEvent{2.0f}`。

两条路殊途同归，都进 **GameRuleSystem 的延迟计时器**：

```cpp
void GameRuleSystem::onLevelClearDelayedEvent(const LevelClearDelayedEvent& event) {
    mIsLevelClear = true;
    mLevelClearTimer = event.mDelayTime;
}
void GameRuleSystem::update(float delta_time) {
    ...
    if (mIsLevelClear) {
        mLevelClearTimer -= delta_time;
        if (mLevelClearTimer <= 0.0f) {
            mDispatcher.enqueue(game::defs::LevelClearEvent{});   // 延迟结束才真正切换
            mIsLevelClear = false;    // 重置标志，避免重复触发
        }
    }
}
```

**为什么要延迟 2 秒？** 最后一个敌人死掉的特效/死亡动画要播完，玩家需要"看一眼战果"。用"标志 + 计时器"而不是"直接 sleep"——不阻塞游戏循环，倒计时期间战斗画面照常渲染。`mIsLevelClear = false` 是**一次性标志**，防止计时器归零那帧被反复触发。

`LevelClearEvent` 最终被 **GameScene::onLevelClear** 接住（本课在 `initEventConnections` 里补上 `sink<LevelClearEvent>().connect<&GameScene::onLevelClear>`）：

```cpp
void GameScene::onLevelClear() {
    const auto point = mGameStats.mEnemyKilledCount + mGameStats.mHomeHp * 5;  // 奖励 = 击杀数 + 基地血量×5
    mSessionData->setLevelClear(true);
    mSessionData->addPoint(point);
    if (mLevelConfig->isFinalLevel(mLevelNumber)) {
        requestPushScene(std::make_unique<game::scene::EndScene>(mContext, true));      // 末关 → 胜利终局
    } else {
        requestPushScene(std::make_unique<game::scene::LevelClearScene>(mContext,
            mBlueprintManager, mUIConfig, mLevelConfig, mSessionData, mGameStats));      // 否则 → 结算场景
    }
}
```

奖励公式 `击杀数 + 基地血量×5` 是参考的设计：**打得快、守得稳，积分就高**——既奖励进攻效率，也奖励基地健康度。`setLevelClear(true)` 让后续"开始游戏"（011 的 `onStartGameClick`）检测到通关状态后自动进下一关。

### 3. push 而非 replace：SceneManager 的"只更栈顶、全栈渲染"

这是本课最关键的引擎语义。`SceneManager::update` 只更新栈顶场景，`render` 却渲染整个场景栈：

```cpp
void SceneManager::update(float delta_time) { current_scene->update(delta_time); processPendingActions(); }
void SceneManager::render() { for (const auto& scene : scene_stack_) scene->render(); }
```

所以通关/结束用 **push** 压入结算场景后：

- **update 只跑栈顶**（LevelClearScene/EndScene）→ 下层的 GameScene 战斗逻辑全部冻结，不需要自己判断暂停。
- **render 全栈都跑** → 下层的战斗画面（RenderSystem 画的地图/单位）继续垫底，结算/结束 UI 画在最上层。
- **问题**：如果 GameScene 每帧还渲染自己的 ImGui 调试窗口，就会和上层结算窗口**叠在一起抢绘制**。所以 `GameScene::render()` 加门控：

```cpp
if (mContext.getGameState().isPlaying() || mContext.getGameState().isPaused()) {
    mDebugUISystem->update();    // 只有战斗/暂停态才渲染战斗调试UI
}
```

`LevelClearScene::init()` 把状态切到 `LevelClear`、`EndScene::init()` 切到 `GameOver`——GameScene 的门控判定立刻失效，调试 UI 不画了；而战斗精灵层（不走 ImGui）照常画。**"状态机门控调试 UI + 场景栈门控更新"** 两条机制配合，实现"下层垫底、上层盖顶、互不打架"。

上层场景**不 override update()**——基类 `Scene::update` 只更新 UI 管理器；按钮回调（`onNextLevelClick` 等）通过 `requestReplaceScene` 发事件，`SceneManager` 下一帧 `processPendingActions` 才真正切换。场景切换永远延迟一帧，避免在渲染/事件处理中途改栈。

### 4. LevelClearScene：引用 GameStats + ctx 共享数据 + friend

```cpp
class LevelClearScene final : public engine::scene::Scene {
    friend class game::system::DebugUISystem;
    std::shared_ptr<...> mBlueprintManager / mUIConfig / mLevelConfig / mSessionData;  // 四份共享数据
    game::data::GameStats& mGameStats;   // 引用下层 GameScene 的成员，只读显示
    std::unique_ptr<game::system::DebugUISystem> mDebugUISystem;
    bool mShowSavePanel{ false };
};
```

三个设计点：

- **`GameStats&` 引用成员**：结算窗口要显示击杀数/基地血量/奖励，而这些统计原本在 GameScene 的 `mGameStats` 成员里。LevelClearScene 构造器**收引用**，push 期间 GameScene 不 update、统计值稳定；DebugUI 通过 friend 直接读 `level_clear_scene.mGameStats`。因为 push 不销毁 GameScene，引用生命周期安全；结算里"下一关"替换掉整个栈时，两个场景一起销毁，也不会悬垂。
- **ctx 重新 emplace**：结算窗口里要复用 `renderUnitTable()`（可排序角色表）和 `renderSavePanelUI()`（存档面板），它们都从 `registry.ctx()` 读 `SessionData/BlueprintManager/UIConfig`。LevelClearScene 是**新的 registry**，所以 `init()` 里把三份 shared_ptr 重新 emplace 进自己的 ctx。
- **friend**：沿用 011 的模式，按钮回调是私有方法，DebugUI 直接 `level_clear_scene.onNextLevelClick()`。

`onNextLevelClick` 和 011 的 `onStartGameClick` 几乎一样，只是**主动清掉通关标志**再进下一关：

```cpp
void LevelClearScene::onNextLevelClick() {
    mSessionData->addOneLevel();
    mSessionData->setLevelClear(false);   // 进入下一关，清除通关状态
    requestReplaceScene(std::make_unique<game::scene::GameScene>(mContext,
        mBlueprintManager, mSessionData, mUIConfig, mLevelConfig));   // 四份共享数据带进新战斗
}
```

### 5. EndScene：is_win 决定一切

EndScene 极简——一个 `bool mIsWin` 就够：`init()` 按 is_win 决定播 `win` 还是 `lose` 音乐、切 `GameOver` 状态；`render()` 挂 `updateEnd`；按钮只有"返回标题"和"退出游戏"。它由两个入口构造：

- `GameScene::onLevelClear` 在 `isFinalLevel` 时 `EndScene(ctx, true)`（胜利终局）
- `GameScene::onGameEndEvent` 收到 `GameEndEvent{mIsWin}` 时 `EndScene(ctx, event.mIsWin)`（目前实际只有失败 `{false}`）

本地把 `onGameEnd()` 改成 **`onGameEndEvent(const GameEndEvent& event)`** 带参，就是为了拿到 `event.mIsWin`——虽然现在只发 `false`，但接口上支持胜利/失败两种终局。

### 6. DebugUISystem 四分支：一个系统服务四个场景

```
update()          GameScene    战斗窗口 + 存档面板（show_save_panel 走 ctx）
updateTitle()     TitleScene   Logo + 4 按钮 + 角色信息 + 读档面板（friend 读私有成员）
updateLevelClear() LevelClearScene  通关文本 + 结算统计/角色表 + 3 按钮 + 存档面板（friend）
updateEnd()       EndScene     胜利/失败大字 + 2 按钮（friend）
```

每条路径都 `beginFrame() → 若干 render 函数 → endFrame()`。`renderLevelClearTable` 里先 `renderUnitTable()` 复用可排序角色表，再画一行统计（关卡/击杀/基地血量/奖励/剩余积分）——`SetWindowFontScale` 控制字号放大。这是 ImGui 调试 UI 服务多场景的标准做法：**一个系统、多入口，每个入口渲染自己的那套窗口**。

### 7. BGM 四场景启用：resource_mapping 早已就位

本地 `assets/data/resource_mapping.json` 从更早的课就配好了四首音乐：

```json
"music": {
    "title_bgm":  "assets/audio/HEROICCC(chosic.com).mp3",
    "battle_bgm": "assets/audio/4 Battle Track INTRO TomMusic.ogg",
    "win":        "assets/audio/level-win.mp3",
    "lose":       "assets/audio/violin-lose-4.mp3"
}
```

`GameApp::init` 启动时 `loadResources("assets/data/resource_mapping.json")` 就把音乐预载进 `AudioManager`（`AudioManager::loadMusic` 内部 `Mix_OpenAudio`）。`AudioPlayer::playMusic(hashed, loops, fadeInMs)` 里 `mCurrentMusicId` 去重（同一首不重复播）、`Mix_HaltMusic` 停旧播新。所以本课只是**取消注释**四处 `playMusic(...)` 调用——011 里 `title_scene.cpp` 还写着"BGM资源待后续补充"，现在资源/设备齐了，真正把音乐放出来。结算/结束用 `playMusic("win"_hs, 0)` 的 **loops=0**：一次性播放，不是循环。

---

## 与参考实现（WispSnow/MonsterWar，commit b83f956）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段后缀 `game_stats_/session_data_/is_win_/show_save_panel_` | m-prefix：`mGameStats/mSessionData/mIsWin/mShowSavePanel` | 本地 m-prefix 规范 |
| 2 | `LevelClearDelayedEvent{}` 默认 `delay_time_=3.0f` | 本地显式 `LevelClearDelayedEvent{ 2.0f }` | 本地延迟 2 秒，参考 3 秒；保留本地值不追赶 |
| 3 | `onGameEndEvent(const GameEndEvent&)` | 本地原 `onGameEnd()` 无参 → 本课对齐改名加参 | 结算要读 `event.mIsWin`；原无参桩也顺带修好 |
| 4 | 本课新写 `GameRuleSystem` 的 is_level_clear_ 计时 + GameEndEvent 分发 | 本地已在 010 课实现（`mIsLevelClear/mLevelClearTimer/mIsGameOver` + 双路判定） | 本地就绪度高，本课只接通 `LevelClearEvent` 监听 |
| 5 | 本课新增 `assets/save/SLOT_1.json` | 本地已存在且逐字节一致 | 本地此前存档测试已生成 |
| 6 | `events.h` 本课加 `LevelClearDelayedEvent/GameEndEvent` | 本地 010 已有 | 同上，就绪度高 |
| 7 | 缩进 2 空格 | game/scene 用 tab，debug_ui_system 4 空格 | 本地各目录缩进约定 |
| 8 | 结算/结束场景不 init UI 管理器 | 相同（`Scene::render` 的 UIManager 未初始化也安全） | `UIManager::render` 对根面板判空，无 UI 元素则不渲染 |

**本地就绪度说明**：参考本课要新写的部分——`GameRuleSystem` 通关延迟计时、`CombatResolveSystem` 全歼判定、`events.h` 的两个新事件、`SLOT_1.json`、`LevelConfig::isFinalLevel`、四首音乐的 `resource_mapping`——本地全在更早的课就位。本课真正新增的是：`State::LevelClear`、`LevelClearScene`/`EndScene` 两个场景、GameScene 的事件接通 + 奖励实现 + render 门控 + battle_bgm、DebugUI 的 `updateLevelClear`/`updateEnd` + 5 个渲染函数、CMakeLists 两个源文件。

---

## 学习要点

### 1. 结局 = 场景栈里的"覆盖层"：update 只更栈顶、render 全栈叠画

通关/结束场景用 push 压在 GameScene 上层，靠 `SceneManager` 的"只更栈顶、渲染全栈"语义实现"战斗冻结、画面垫底、UI 盖顶"。给战斗加结算/暂停/菜单覆盖层时，**优先用场景栈而不是状态分支**——上层场景天然隔离逻辑，下层自动冻结。这是"场景栈"设计最典型的用法。

### 2. 一次判定、延迟切换：标志 + 计时器，不 sleep

"最后一个敌人死了"到"切到结算场景"之间有 2 秒表演时间。用 `mIsLevelClear` 标志 + `update` 里倒计时，而不是 `sleep`——**不阻塞主循环，倒计时期间画面照常渲染**。切换动作只做一次（重置标志），保证不会重复 push。

### 3. 上下层都渲染 ImGui 会打架 → 状态机门控调试 UI

结算场景盖在战斗场景上层，GameScene 的调试 UI 仍会每帧画 → 两套窗口叠加。解法：**渲染门控**——`GameScene::render()` 只在 `isPlaying()||isPaused()` 时画调试 UI，结算/结束场景把状态切成 `LevelClear`/`GameOver` 后自动让路。**"调试 UI 跟随状态显隐"，比"每帧判栈深"更贴合 ECS 的状态机思路**。

### 4. 引用 GameStats：push 期间下层冻结，引用才安全

LevelClearScene 用 `GameStats&` 引用下层 GameScene 的成员。这依赖一个前提：**push 期间 GameScene 不更新，统计值不变**。引用传递共享可变状态时，一定要想清楚"被引用对象的生命周期"——这里是场景栈保证的。

### 5. 奖励公式即产品语义：击杀数 + 基地血量×5

"打得快、守得稳，积分就高"——奖励公式不是随便写的，它同时奖励进攻效率（击杀）和防守质量（基地血量）。**游戏数值的设计意图，往往藏在公式里**。

### 6. 一处改动、四处 BGM：资源映射提前做，启用只是一行

`resource_mapping.json` + `AudioManager::loadMusic` + `Mix_OpenAudio` 这套音频管线在更早的课就绪，本课"启用 BGM"不过是取消注释 + 加一行。**基础设施提前搭好，功能上线就只剩接线**——这也是分课学习的主线：引擎能力先行，游戏玩法后接。

### 7. 事件链的"最后一公里"：判定早已就绪，缺的只是监听

本地 010 课就把通关判定、延迟计时、`GameEndEvent`/`LevelClearDelayedEvent` 都实现了，但没人监听 `LevelClearEvent`，所以胜利后游戏僵住。**本课真正的关键改动是一行 `sink<LevelClearEvent>().connect<&GameScene::onLevelClear>(this)`**——事件系统的价值就在这：发送方和接收方解耦，补齐监听即补齐流程。
