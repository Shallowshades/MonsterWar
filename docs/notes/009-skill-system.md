# 技能施放与显示：数据驱动的技能三态机

## 问题

上一课（008）之后，特效系统可以放 `skill_ready` / `skill_active` 特效了，但没有任何东西会放它们——**技能还不存在**。玩家角色虽然 player_data.json 里早就配了 `skill` 字段，代码里却没有"技能"这个概念：没有技能组件、没有技能系统、没有技能蓝图解析、没有技能计时器。

技能的形态是清晰且通用的：**角色配一个技能（数据驱动），技能有冷却 → 就绪 → 激活 → 持续结束 三态流转，激活时给角色加 Buff，角色头顶用循环特效标识当前状态，玩家通过 ImGui 选中面板 + 快捷键 S 施放**。本课就把这条完整的技能链路补上。

一句话：**让"技能"成为数据驱动的一等公民——组件存技能数据、系统管三态流转、蓝图管数值、特效标识状态、事件在系统间解耦通信。**

## 结论

新增一条"技能生产链"：

```
skill_data.json（技能蓝图：名字/描述/被动/冷却/持续/倍率Buff）
        │  BlueprintManager::loadSkillBlueprints() + parseBuff() 解析
        ▼
SkillBlueprint（技能蓝图：id + 名称 + 冷却/持续 + BuffBlueprint）
        │  EntityFactory::addSkillComponent() 挂到玩家单位（被动 → 直接就绪）
        ▼
SkillComponent（技能数据 + 显示实体ID + 两个计时器）
        ▲                              │
        │                              ▼
TimerSystem（冷却/持续两个计时器）    SkillSystem（事件驱动三态流转 + Buff增删）
        │  冷却结束发 SkillReadyEvent      │  就绪 → 显示 skill_ready 标识
        │  持续结束发 SkillDurationEndEvent │  激活 → 显示 skill_active 标识 + addBuff
        │                                   │  结束 → 删标识 + removeBuff
        ▼                                   ▼
    SkillReadyTag / SkillActiveTag      DebugUISystem（按钮 + S 快捷键 + 进度条）
```

- 新增**技能组件** `SkillComponent`：技能ID、显示特效实体ID、名称、描述、冷却/持续、两个计时器
- 新增**技能系统** `SkillSystem`：订阅 4 个事件，驱动三态流转，负责 Buff 增删与显示标识回收
- 新增**技能蓝图** `SkillBlueprint` + `BuffBlueprint`：倍率字段 + cost_regen，数据驱动技能数值
- 新增**三个技能事件**：`SkillReadyEvent` / `SkillActiveEvent` / `SkillDurationEndEvent`
- 新增**三个技能标签**：`SkillReadyTag` / `SkillActiveTag` / `PassiveSkillTag`
- **TimerSystem 改造**：从"只推进攻击冷却"升级为持有 registry/dispatcher 引用，加技能冷却/持续两个计时器
- **被动技能**：落子即放（PlaceUnitSystem 发 `SkillActiveEvent`）、永不过期（TimerSystem 排除 `PassiveSkillTag`）
- **显示标识**：复用第 8 课的 `skill_ready`/`skill_active` 特效蓝图，循环动画、`MAIN_LAYER+20`，由 SkillSystem 打 `DeadTag` 回收

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| 增益/技能蓝图 `BuffBlueprint`/`SkillBlueprint` | `src/game/data/entity_blueprint.h`（EffectBlueprint 后） |
| 技能组件 `SkillComponent` | `src/game/component/skill_component.h`（新建） |
| 三个技能事件 | `src/game/defs/events.h`（RemovePlayerUnitEvent 后） |
| 三个技能标签 | `src/game/defs/tags.h`（ShowRangeTag 后） |
| 技能显示偏移 `SKILL_DISPLAY_OFFSET` | `src/game/defs/constants.h`（RANGE_COLOR 后） |
| 技能蓝图容器 + 加载/查询/解析 | `src/game/factory/blueprint_manager.h/.cpp` |
| 技能显示工厂 + 技能组件挂载 | `src/game/factory/entity_factory.h/.cpp` |
| 技能三态流转 + Buff 增删 | `src/game/system/skill_system.h/.cpp`（新建，本课核心） |
| 技能冷却/持续计时 | `src/game/system/timer_system.h/.cpp` |
| 被动技能落子即放 | `src/game/system/place_unit_system.cpp:onPlaceUnit` |
| 选中面板技能区块 | `src/game/system/debug_ui_system.cpp:renderSelectedUnit` |
| 技能蓝图数据 | `assets/data/skill_data.json`（本地已存在） |
| 加载技能蓝图 | `src/game/scene/game_scene.cpp:initEntityFactory` |
| 系统注册/更新接线 | `src/game/scene/game_scene.cpp:initSystems/update` |

### 2. 技能三态机：事件驱动 + 计时器，系统间零直接调用

技能不是简单的"有冷却就放"，而是**三个状态**随时间流转：

```
冷却中 ──(TimerSystem 计时够了)──▶ 就绪 ──(SkillActiveEvent)──▶ 激活 ──(TimerSystem 持续够了)──▶ 冷却中
```

每个状态迁移的触发者与处理者不同，靠**标签 + 事件**在 TimerSystem 和 SkillSystem 之间协作，谁都不直接调用谁：

| 状态 | 判定 | 迁移触发 | 迁移副作用（SkillSystem 处理） |
|------|------|---------|-------------------------------|
| **冷却中** | 有 `SkillComponent`，无 `SkillReadyTag` | TimerSystem 推进 `mCooldownTimer`，够了 → `emplace SkillReadyTag` + `enqueue(SkillReadyEvent)` | 显示 `skill_ready` 标识 |
| **就绪** | 有 `SkillReadyTag` | DebugUI 按钮/快捷键 S 点击 → `enqueue(SkillActiveEvent)` | 删就绪标识 → 建 `skill_active` 标识 → `remove SkillReadyTag` + `emplace SkillActiveTag` → `addBuff()` |
| **激活** | 有 `SkillActiveTag` | TimerSystem 推进 `mDurationTimer`，够了 → `remove SkillActiveTag` + `enqueue(SkillDurationEndEvent)` | 删激活标识 → `removeBuff()` |

要点拆解：

- **计时器归 TimerSystem，状态迁移归 SkillSystem**：TimerSystem 只管"时间到了没"，到了发事件；SkillSystem 只管"收到事件怎么改状态"。分工干净。
- **`SkillActiveEvent` 没有对应计时器**，它是"人为指令"事件（玩家按 S 或被动落子）——这也是为什么 SkillSystem 里要先检查 `any_of<SkillReadyTag>`：**不是就绪状态收到激活指令就直接忽略**，防止冷却中强行施放。
- **`SkillReadyTag` 的删除是 SkillSystem 做的**：TimerSystem 只负责 emplace（冷却够了变就绪），SkillSystem 负责在施放时 remove（就绪用完）。每个标签的增删只有一个职责方，不重复。

### 3. TimerSystem 改造：从单一攻击冷却到"计时器管理器"

改造前的 TimerSystem 是纯函数式：`update(registry, delta_time)` 每次传 registry 进来，只管攻击冷却。改造后变成**持引用 + 多计时器**：

```cpp
class TimerSystem {
    entt::registry& mRegistry;
    entt::dispatcher& mDispatcher;
public:
    TimerSystem(entt::registry& registry, entt::dispatcher& dispatcher);
    void update(float delta_time);
private:
    void updateAttackTimer(float delta_time);          // 原有：攻击冷却
    void updateSkillCooldownTimer(float delta_time);   // 新增：技能冷却
    void updateSkillDurationTimer(float delta_time);   // 新增：技能持续
};
```

三个计时器用**同一种 view 筛选 + 推进 + 判满 + 迁移**的模板：

```cpp
// 技能冷却计时器
auto view_skill = mRegistry.view<game::component::SkillComponent>(
    entt::exclude<game::defs::SkillReadyTag, game::defs::PassiveSkillTag>);  // 冷却中 = 没就绪 + 非被动
for (auto entity : view_skill) {
    auto& skill = view_skill.get<game::component::SkillComponent>(entity);
    skill.mCooldownTimer += delta_time;
    if (skill.mCooldownTimer >= skill.mCooldown) {
        mRegistry.emplace_or_replace<game::defs::SkillReadyTag>(entity);
        skill.mCooldownTimer = 0.0f;
        mDispatcher.enqueue(game::defs::SkillReadyEvent{ entity });   // 就绪事件让 SkillSystem 显示标识
    }
}
```

两个关键筛选条件：

- **技能冷却**排除 `SkillReadyTag`：已经就绪的不用再计时冷却；**排除 `PassiveSkillTag`**：被动技能一开始就 `SkillReadyTag`，永不进冷却。
- **技能持续**用 `view<SkillComponent, SkillActiveTag>`（交集），排除 `PassiveSkillTag`：被动技能激活后持续计时永不触发 → **被动永久生效**。

### 4. 被动技能：落子即放 + 永不过期，两处代码配合

`rest`（休整）是被动技能：`passive: true`，每秒回 0.3 COST。它不经过"冷却 → 就绪 → 施放"的正常流程，而是**落子就生效、永远生效**：

1. **创建时直接就绪**（`entity_factory.cpp:addSkillComponent`）：
   ```cpp
   if (skill.mPassive) {
       mRegistry.emplace<game::defs::PassiveSkillTag>(entity);
       mRegistry.emplace<game::defs::SkillReadyTag>(entity);   // 天生就绪
   }
   ```
2. **落子即放**（`place_unit_system.cpp:onPlaceUnit`，渲染图层修正后）：
   ```cpp
   if (mRegistry.all_of<game::defs::PassiveSkillTag>(unit_entity)) {
       mContext.getDispatcher().enqueue(game::defs::SkillActiveEvent{ unit_entity });
   }
   ```
3. **永不过期**：TimerSystem 的持续计时排除 `PassiveSkillTag`，`mDurationTimer` 永不推进，`SkillDurationEndEvent` 永不发出 → `SkillActiveTag` 和 Buff 永久挂着。

`cost_regen` 走的是 **CostRegenComponent**：`addBuff` 里 `emplace_or_replace<CostRegenComponent>(entity, mCostRegen)`，而 **GameRuleSystem 早就实现了每帧回 COST 的循环**（`game_rule_system.cpp:37-41`，遍历所有 `CostRegenComponent` 累加 `mRate * dt`）——本课对规则系统**零改动**。被动回 COST 是"往注册表里塞一个已有组件就自动生效"，又一次组合优于继承。

### 5. Buff 增删：乘除对称，restore 靠除法

`BuffBlueprint` 的六个倍率字段全是乘数语义（默认 1.0 表示"不变"）：

```cpp
struct BuffBlueprint {
    float mHpMultiplier{ 1.0f };
    float mAtkMultiplier{ 1.0f };
    float mDefMultiplier{ 1.0f };
    float mRangeMultiplier{ 1.0f };
    float mAtkIntervalMultiplier{ 1.0f };
    float mCostRegen{ 0.0f };   // 注意：不是倍率，是绝对值（每秒恢复量）
};
```

`addBuff` 用 `*=`，`removeBuff` 用 `/=`——**乘除对称**，同一个倍率乘上去、除回来就是原值：

```cpp
// addBuff
stats.mAtk *= buff_blueprint.mAtkMultiplier;
// removeBuff
stats.mAtk /= buff_blueprint.mAtkMultiplier;
```

为什么不用"存原值"？因为**乘除对称免去了记录基线**：只要技能持续期间没有别的修改，除回同一倍率必然还原。代价是倍率必须非零（skill_data 里没有 0 倍率，且除 0 不会发生——数值是数据配的，约定非零）。

### 6. 显示标识：循环特效实体 + 显式回收

`skill_ready` / `skill_active` 标识是**角色头顶的循环特效**。对比第 8 课的一次性特效：

| | 一次性特效（`createEffect`） | 技能标识（`createSkillDisplay`） |
|--|--|--|
| 动画循环 | `loop=false` | `loop=true` |
| 移除标签 | `OneShotRemoveTag`（播完自动走） | **无**（要一直显示） |
| 渲染层 | `MAIN_LAYER + 10` | `MAIN_LAYER + 20` |
| 回收方式 | 播完自己 DeadTag | SkillSystem 状态切换时打 `DeadTag` |

```cpp
entt::entity EntityFactory::createSkillDisplay(entt::id_type effect_id, const glm::vec2& position) {
    auto entity = mRegistry.create();
    const auto& effect_blueprint = mBlueprintManager.getEffectBlueprint(effect_id);
    addTransformComponent(entity, position);
    addSpriteComponent(entity, effect_blueprint.mSprite);
    // 循环播放（loop=true），且不加 OneShotRemoveTag —— 技能标识要一直显示
    addOneAnimationComponent(entity, effect_blueprint.mAnimation, effect_blueprint.mSprite, effect_id, true);
    mRegistry.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 20);
    return entity;
}
```

标识的**生命周期由 SkillSystem 接管**：状态切换时把旧标识打 `DeadTag`、建新标识；单位死亡（`onRemoveUnitEvent`）也清理标识兜底。**"从哪来"（数据 + 工厂）和"何时消失"（状态机 + 系统）分离**——特效实体自己完全不知道自己在表达什么，纯粹是一个会循环播放的动画。

`SKILL_DISPLAY_OFFSET = {0, -96}` 是标识相对角色的显示偏移（头顶上方）。

### 7. ImGui 技能面板：禁用态按钮 + 快捷键 + 进度条

`renderSelectedUnit` 的技能区块（`debug_ui_system.cpp`）展示了几个 ImGui 交互技巧：

```cpp
if (auto skill = mRegistry.try_get<game::component::SkillComponent>(entity); skill) {
    auto ready = mRegistry.all_of<game::defs::SkillReadyTag>(entity);
    ImGui::BeginDisabled(!ready);                                       // 冷却中按钮置灰
    ImGui::SetNextItemShortcut(ImGuiKey_S,                              // 绑定快捷键 S
        ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
    if (ImGui::Button(skill->mName.c_str())) {
        mContext.getDispatcher().enqueue<game::defs::SkillActiveEvent>(entity);  // 点按钮 = 发激活事件
    }
    ImGui::EndDisabled();
    // 激活中：显示"剩余时间"或"被动技能激活中"
    // 冷却中：SkillReadyTag → "技能准备就绪"；否则 ProgressBar(cooldown_timer / cooldown)
    ImGui::TextWrapped("%s", skill->mDescription.c_str());              // 描述自动换行
}
```

- **`BeginDisabled(!ready)`**：冷却中按钮置灰不可点，避免非法施放（SkillSystem 里也有 `any_of<SkillReadyTag>` 双保险）。
- **`SetNextItemShortcut(ImGuiKey_S, ...)`**：给下一个 Item（按钮）注册快捷键 S。`RouteAlways` 让快捷键全局生效，`Tooltip` 在悬浮时提示。
- **`ProgressBar(cooldown_timer / cooldown)`**：用冷却进度填进度条，比文字倒计时直观。
- **面板只读组件不读蓝图**：技能名/描述/计时器全在 `SkillComponent` 里，面板不依赖 BlueprintManager，只管展示与发事件。

### 8. 初始冷却 = 冷却时间的一半

`addSkillComponent` 里一个容易被忽略的细节：

```cpp
mRegistry.emplace<game::component::SkillComponent>(entity,
    skill_id, entt::null, skill.mName, skill.mDescription,
    skill.mCooldown, skill.mDuration,
    skill.mCooldown / 2.0f,   // 初始技能冷却时间为技能冷却时间的一半
    0.0f);
```

角色刚出场时 `mCooldownTimer = cooldown / 2`，不是 0 也不是满冷却——**让玩家不用等满一个完整冷却就能第一次放技能**，节奏更好。这是数据之外塞在组件初始化里的"手感调参"，参考实现同款。

---

## 与参考实现（WispSnow/MonsterWar，commit 20e190c）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段 `id_/name_/cooldown_/buff_/hp_multiplier_` | m-prefix：`mId/mName/mCooldown/mBuff/mHpMultiplier` | 遵循本地 m-prefix 规范 |
| 2 | 三个技能事件字段 `entity_` | `mEntity` | 同上 |
| 3 | `registry_/context_/entity_factory_` 成员 | `mRegistry/mContext/mEntityFactory` | 同上 |
| 4 | `TimerSystem` 用 `rate_`（CostRegenComponent） | 本地已为 `mRate` | 上上课就绪，本课零改动 |
| 5 | `skill_data.json` / `cost_regen_component.h` 需新增 | 本地已存在且内容一致 | 之前就绪，直接复用 |
| 6 | `GameRuleSystem` 需新增回 COST 逻辑 | 本地已实现（`game_rule_system.cpp:31-41`） | 之前就绪，本课零改动 |
| 7 | 缩进 2 空格 | game/factory + game/system 用 4 空格，game/scene 用 tab | 本地各目录缩进约定 |
| 8 | 结构、逻辑、时序 | 完全照搬 | 本课无本地设计差异 |

本地就绪度高到几乎"空手接"：技能蓝图、CostRegenComponent、规则系统回 COST 循环、player_data 的技能字段、特效蓝图全在上课前就存在了。本课新增的就是整条技能链路本身。

---

## 学习要点

### 1. 三态机用"标签 + 事件"在两个系统间流转，零直接调用

技能的三态（冷却/就绪/激活）不是某个系统里一个 `switch` 写死的，而是**分散在 TimerSystem（计时、发事件）和 SkillSystem（收事件、改标签、加 Buff）两个系统里，靠 tag 表示状态、靠事件传递迁移**。每加一个状态就加一个 tag + 一个事件 + 一个回调，改一个状态不动另一个。ECS 的"数据 + 系统"范式下，**状态机的正确写法是把状态当数据、迁移当事件**。

### 2. `emplace_or_replace` vs `emplace`：同一实体可能有同组件

TimerSystem 冷却结束 `emplace_or_replace<SkillReadyTag>`——为什么不用 `emplace`？因为 view 已经排除 `SkillReadyTag`，理论上没有重复 emplace 的机会；但 `emplace_or_replace` 更稳：万一逻辑漏洞导致重复，`emplace` 会抛异常，`emplace_or_replace` 静默替换。**面向"不该发生但发生了也别崩"写防御性代码**。

### 3. `remove` vs `erase`：EnTT 的"安静 vs 抛错"

SkillSystem 的 `onSkillDurationEndEvent` 和 TimerSystem 的持续计时**都 remove 了 `SkillActiveTag`**——看起来会重复移除。但 EnTT 的 `registry.remove<T>` 在组件缺失时**返回 0 不抛**（只有 `erase` 是抛错版）。时序上 TimerSystem 先 remove + 发事件，SkillSystem 收到事件再 remove 一次，第二次就是个安全的 no-op。**知道 API 的失败语义，才敢放心双删**。

### 4. 事件派发时机决定了"跨帧"协作是安全的

`GameApp::run` 每帧末尾 `mDispatcher->update()`（game_app.cpp:45）：帧内 enqueue 的事件当帧末统一派发。所以 `SkillActiveEvent` 发出 → 当帧末 SkillSystem 收到 → 建标识 + 加 Buff；`DeadTag` 的标识实体 → **下一帧开头** RemoveDeadSystem 清理。技能系统完全不用关心"哪帧清理"，时序由引擎的既有管道闭环保证。

### 5. 被动技能是"组合即效果"的示范

被动技能不需要单独的"被动系统"：落子事件 + 既有的 cost 恢复组件 + 计时器排除条件，三处既有机制组合出"落子即放、永不过期、持续回 COST"。**新玩法不一定需要新系统，往往是既有系统的组合方式不同**。

### 6. 数据驱动的边界：数值在数据、结构在代码

技能的所有"参数"（名字、描述、被动与否、冷却、持续、倍率）都在 `skill_data.json`；代码里只有"结构"（组件有哪些字段、状态怎么流转、Buff 怎么乘除）。改技能数值只动 JSON，加技能类型在 JSON 加一条。这正是 008 特效系统"种类下沉到数据"的延续——技能种类同样下沉。
