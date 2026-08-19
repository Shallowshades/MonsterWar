# 出击面板与关卡规则：registry.ctx() 服务定位器 + 事件驱动规则层

## 问题

战斗系统（阻挡 / 目标锁定 / 弹道 / 死亡特效）已经完整，但塔防游戏最核心的**"规则层"还是空的**：

- **没有经济系统**——出击要消耗的 `cost` 资源不存在，角色肖像只是摆在面板上的一排图，点不动
- **没有胜负判定**——基地血量、敌人到达数量、敌人总数量这些"关卡状态"完全没有
- **出击 UI 死代码化**——`game_scene.cpp` 里内联写了 90 行建面板的代码，没有点击/悬停交互，没有 cost 不足时的遮盖
- **数据不知道该放哪**——规则系统要读 cost、UI 要读 cost、战斗系统要统计击杀数，这些数据放成员变量？全局？怎么共享？

一句话：**战斗有了，但"游戏"还没有。**

## 结论

用三个手段补齐规则层，核心是引入 **`registry.ctx()` 作为"服务定位器"**：

1. 关卡内统计数据 **`GameStats`** 存进 `registry.ctx()`，规则系统、出击 UI、战斗系统各自从 ctx 取，不用层层传参
2. 新增 **`GameRuleSystem`** 事件驱动规则层——cost 恢复、敌人到达基地、单位升级/撤退、通关/失败判定
3. 出击面板抽成 **`UnitsPortraitUI`** 类，`UIButton` 支持三回调（点击 / 悬停进 / 悬停出），用 **10 个新事件**与规则层通信

```
┌───────────────────────── registry.ctx()（服务定位器） ─────────────────────────┐
│   GameStats（关卡统计，值语义）   shared_ptr<SessionData/UIConfig/BlueprintManager> │
│   ▲          ▲           ▲                                                       │
│   │          │           │                                                       │
│ GameRuleSystem      UnitsPortraitUI      CombatResolveSystem                     │
│ （cost恢复/胜负判定）   （遮盖/滚动/点击）        （敌人死亡→击杀数+1）              │
└──────────────────────────────────────────────────────────────────────────────────┘
```

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| `GameStats` 关卡统计数据 | `src/game/data/game_stats.h:22` |
| `CostRegenComponent` 额外cost恢复 | `src/game/component/cost_regen_component.h` |
| 10 个新事件（预备出击/悬停/升级/撤退/通关/失败…） | `src/game/defs/events.h:51-99` |
| `GameRuleSystem` 规则层 | `src/game/system/game_rule_system.h/.cpp` |
| `UnitsPortraitUI` 出击面板 | `src/game/ui/units_portrait_ui.h/.cpp` |
| GameScene 把数据塞进 ctx | `src/game/scene/game_scene.cpp:222` |
| GameScene 创建出击 UI | `src/game/scene/game_scene.cpp:231` |
| GameScene 挂载 GameRuleSystem | `src/game/scene/game_scene.cpp:258` |
| update 中规则层/UI 的调用位置 | `src/game/scene/game_scene.cpp:111-123` |
| 敌人死亡→击杀数+1 | `src/game/system/combat_resolve_system.cpp:74` |

### 2. 核心思想：`registry.ctx()` 服务定位器

EnTT 的每个 `registry` 自带一个**按类型索引的上下文容器**。存：`ctx().emplace<T>(...)`，取：`ctx().get<T&>()`。

```cpp
// game_scene.cpp:222
bool GameScene::initRegistryContext() {
    mRegistry.ctx().emplace<game::data::GameStats>(mGameStats);                                    // 值语义：存的是 GameStats 本体
    mRegistry.ctx().emplace<std::shared_ptr<game::factory::BlueprintManager>>(mBlueprintManager);  // 共享语义：存的是 shared_ptr
    mRegistry.ctx().emplace<std::shared_ptr<game::data::SessionData>>(mSessionData);
    mRegistry.ctx().emplace<std::shared_ptr<game::data::UIConfig>>(mUIConfig);
    return true;
}
```

**两种存储语义要分清**（这是本课最容易混的点）：

| 语义 | 存什么 | 怎么取 | 谁拥有数据 |
|------|--------|--------|-----------|
| **值语义** | `GameStats` 本体 | `get<GameStats&>()` → 引用 | 存进 ctx 的就是唯一一份，所有 getter 拿引用看到同一份 |
| **共享语义** | `std::shared_ptr<T>` | `get<std::shared_ptr<T>>()` → 拷贝指针 | 数据归属会话/全局，多个场景共享同一实例 |

为什么用 `GameStats` 值语义？因为它就是"这一场战斗的临时状态"，生命期和场景绑定——注册表销毁它就销毁，天然正确。为什么数据用 `shared_ptr`？因为 SessionData 之类**跨关卡存活**，上一课已经论证过（见 002-session-data.md）。

**对比：为什么不用成员变量 / 全局 / 单例？**

- **成员变量传参**：`GameRuleSystem`、`UnitsPortraitUI`、`CombatResolveSystem` 都要读这些数据。逐个传引用 → 构造参数爆炸，新系统加入就要改所有调用点。
- **全局变量 / 单例**：脱离 ECS 生命周期，多个场景共存时互相污染，且无法测试。
- **ctx()**：数据生命期天然跟注册表走，系统/UI 手里本来就攥着 `registry&`，`ctx().get<T>()` 一行拿到。这是 ECS 生态的标准做法——**数据放注册表，谁需要谁取**。

### 3. `GameRuleSystem`：事件驱动的规则层

构造函数里订阅 4 个事件（`entt::sink`），之后**不是每帧轮询逻辑，而是被动等事件**：

```cpp
// game_rule_system.cpp:19
GameRuleSystem::GameRuleSystem(entt::registry& registry, entt::dispatcher& dispatcher)
    : mRegistry(registry), mDispatcher(dispatcher) {
    mDispatcher.sink<game::defs::EnemyArriveHomeEvent>().connect<&GameRuleSystem::onEnemyArriveHome>(this);
    mDispatcher.sink<game::defs::UpgradeUnitEvent>().connect<&GameRuleSystem::onUpgradeUnitEvent>(this);
    mDispatcher.sink<game::defs::RetreatEvent>().connect<&GameRuleSystem::onRetreatEvent>(this);
    mDispatcher.sink<game::defs::LevelClearDelayedEvent>().connect<&GameRuleSystem::onLevelClearDelayedEvent>(this);
}
```

只有 `update()` 是每帧调用的，做两件**周期性**的事：cost 恢复、通关倒计时。其余全部事件驱动。

**cost 恢复**（`game_rule_system.cpp:31`）：

```cpp
game_stats.mCost += game_stats.mCostGenPerSecond * delta_time;   // 基础速率
for (auto entity : view_cost_regen) {                             // 额外速率（建筑等）
    game_stats.mCost += cost_regen.mRate * delta_time;
}
```

**敌人到达基地**（`game_rule_system.cpp:53`）——这是胜负判定的关键：

```cpp
game_stats.mEnemyArrivedCount++;   // 到达数+1
game_stats.mHomeHp -= 1;           // 基地血量-1
if (game_stats.mHomeHp <= 0) {
    mDispatcher.enqueue(game::defs::GameEndEvent{ false });   // 基地没了 → 失败
}
else if ((arrived + killed) >= enemyCount) {
    mDispatcher.enqueue(game::defs::LevelClearDelayedEvent{ 2.0f });  // 全歼 → 延迟2秒通关
}
```

注意"**延迟 2 秒通关**"这个设计：全歼敌人的瞬间直接切场景太生硬，留 2 秒让玩家看清战场。实现是 `onLevelClearDelayedEvent` 设标志 + 计时器，在 `update()` 里倒计时归零后再发 `LevelClearEvent`——**用状态 + 计时器把"瞬间事件"变"延迟事件"**。

**升级 / 撤退**：`onUpgradeUnitEvent` 扣 cost、`mLevel++`、按蓝图+等级+稀有度用 `statModify()` 重算属性，最后 `PlaySoundEvent` 播升级音效。`onRetreatEvent` 返还 cost、发 `RemovePlayerUnitEvent` 让移除系统清理实体。

**为什么规则层用事件而不是轮询？** 规则的触发点分散在别的系统里——敌人到基地发生在 `FollowPathSystem`、敌人死亡发生在 `CombatResolveSystem`。如果让规则层每帧去扫描"有没有敌人到基地"，要么侵入别人的系统加钩子，要么重复遍历。事件解耦了**触发方与响应方**：触发方只管 `enqueue(事件)`，规则层只订阅感兴趣的信号。这正是 `entt::dispatcher` 的价值。

### 4. `UnitsPortraitUI`：UI 类封装 + 三回调按钮

`game_scene.cpp` 里那 90 行建面板代码抽成一个类，职责收进一个对象：

```
anchor_panel (id="anchor_panel", 位于屏幕底部)
 └─ frame_panel × N（每个角色一个，id=角色名哈希，order_index=cost）
     ├─ UIImage   portrait  角色肖像图
     ├─ UIButton  frame     交互层（三回调）
     ├─ UIImage   icon      职业图标
     ├─ UILabel   cost      出击费用
     └─ UIPanel   cover     灰色遮盖（cost 不够时盖住）
```

**两个设计点值得记：**

**① cost 直接当排序键 + 遮盖判断键。** `frame_panel` 以 `cost` 作为 `addChild(child, cost)` 的 order_index，这样面板按费用自动排序；而"cost 不够要遮盖"的判断直接写成 `cover->setVisible(game_stats.mCost < frame_panel->getOrderIndex())`（`units_portrait_ui.cpp:53`）。**排序、遮盖共用同一个数值**，不用额外存一份 cost。

**② 按钮三回调取代"一层 if"。** 之前 UIButton 只有点击回调，悬停状态变化要靠外层轮询 `isHovered()`。现在构造器直接收 3 个 lambda：

```cpp
frame_panel->addChild(std::make_unique<engine::ui::UIButton>(mContext,
    frame, frame, frame, glm::vec2(0,0), frame_size,
    [this, name_id, class_id, cost]() {   // 点击 → 预备出击
        mContext.getDispatcher().enqueue(game::defs::PrepUnitEvent{ name_id, class_id, cost });
    },
    [this, name_id]() {                   // 悬停进入 → 事件
        mContext.getDispatcher().enqueue(game::defs::UIPortraitHoverEnterEvent{ name_id });
    },
    [this]() {                            // 悬停离开 → 事件
        mContext.getDispatcher().enqueue(game::defs::UIPortraitHoverLeaveEvent{});
    }
));
```

注意 lambda **按值捕获** `name_id / class_id / cost`——`unit_map` 是 `unordered_map`，循环变量 `unit_data` 是引用，捕获它会在插入/排序后悬垂。这是和参考实现的一个差异（见下）。

滚动（`move_left`/`move_right` 已让给肖像面板左右移，不再造测试兵）、出战后移除肖像（`onRemoveUIPortraitEvent`）也都封装在类里。

### 5. `game_scene.cpp` 瘦身：依赖变成了谁？

重构后 `game_scene.cpp` 净减 ~140 行。原来的建面板代码删掉，换成两个新 init 函数 + 一个系统挂载：

```
init() 调用链（game_scene.cpp:58）
 initSessionData → initUIConfig → loadLevel → initEventConnections → initInputConnections
 → initEntityFactory → initRegistryContext → initUnitsPortraitUI → initSystems
                       └ 数据入 ctx            └ 建出击面板         └ 挂 GameRuleSystem

update() 调用顺序（game_scene.cpp:111-123）
 mTimerSystem → mGameRuleSystem → mBlockSystem → ... → mUnitsPortraitUI->update
               └ cost/通关计时      └ 战斗逻辑         └ 遮盖刷新/滚动
```

`mGameRuleSystem` 放 `mTimerSystem` 之后：先走冷却计时，再恢复 cost，规则层看到的是本帧最新状态。

### 6. 与参考实现（WispSnow/MonsterWar）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 升级时发**通用**特效事件 | 本地 `EffectSystem` 只支持敌人死亡特效，升级先只发 `PlaySoundEvent`，特效留 `NOTE` 注释延后 | 不为一个未支持的特性造轮子，等特效系统扩展再做 |
| 2 | 按钮回调捕获 `&unit_data`（map 元素引用） | 按值捕获 `name_id/class_id/cost` | `unordered_map` 的引用在后续插入/排序后可能失效，捕获按值更安全 |
| 3 | 命名 `cost_`/`registry_`（trailing underscore） | 本地 m-prefix（`mCost`/`mRegistry`） | 遵循本地编码规范 |
| 4 | 保留造治疗者的测试入口 | 删除 `onCreateTestPlayerHealer` | `move_left` 让给面板滚动后该函数成死代码，删干净 |

---

## 学习要点

### 1. 服务定位器（Service Locator）什么时候用

`registry.ctx()` 本质是服务定位器模式。它和"依赖注入"的区别：

| 方式 | 优点 | 缺点 | 适用 |
|------|------|------|------|
| 构造参数传引用 | 依赖显式、可测试 | 链条长时参数爆炸 | 依赖少且固定 |
| **ctx() 服务定位器** | 一行取用、不传参、生命周期跟注册表 | 依赖"隐式"、编译期查不出缺失 | 多个系统共享的关卡状态、全局数据 |

ECS 里 `ctx()` 是标准答案——系统本来就持有 `registry&`，取数据零成本。

### 2. 值语义 vs 共享语义（存进 ctx 前先想清楚）

- **关卡内状态（GameStats）→ 值语义**：就是这一场战斗的临时数据，注册表销毁即销毁
- **跨关卡数据（SessionData 等）→ shared_ptr**：生命周期比场景长，多个场景要共享同一份

判断标准一句话：**"这个数据跟着关卡走，还是跟着游戏会话走？"**

### 3. 事件驱动 vs 轮询

- 触发点分散在多个系统 → **事件**（触发方 `enqueue`，规则层订阅）
- 每帧都要做且逻辑独立 → **update 轮询**（cost 恢复、倒计时）

### 4. 延迟事件的小技巧

"瞬间想延迟触发"（全歼后 2 秒再切场景）→ 设标志 + 计时器，在 `update()` 里归零后发真正的事件。用状态把事件从"触发时刻"解耦到"期望时刻"。

### 5. 死代码要删干净

`onCreateTestPlayerHealer` 失去绑定后就是死代码，声明 + 定义一起删。留着会让后来者困惑"这函数绑哪了？"
