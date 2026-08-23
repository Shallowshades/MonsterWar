# 特效载入与生成：数据驱动的通用特效系统

## 问题

上一课（007）之后，特效只有一个：**敌人死亡特效**。它走的是硬编码链路——`EnemyDeadEffectEvent` → `EffectSystem::onEnemyDeadEffectEvent` → `createEnemyDeadEffect`（复用敌人蓝图的 "damage" 动画）。想放一个治疗特效、升级特效、技能特效？没门——没有通用的特效数据、没有通用的特效事件、没有通用的特效工厂函数。

特效的本质是**一次性动画实体**：放出来、播一遍、自己消失。这种实体应当像单位、投射物一样**数据驱动**——从 JSON 加载多种特效蓝图，任何系统想说"这里放个 X 特效"就发一个事件，剩下的交给特效系统。本课就把这条链补全。

一句话：**把特效从"硬编码一个死亡特效"升级为"数据驱动的通用特效系统"，治疗特效是第一个使用者。**

## 结论

新增一条完整的"特效生产链"：

```
effect_data.json（特效蓝图数据，本地已存在：heal/level_up/skill_active/skill_ready）
        │  BlueprintManager::loadEffectBlueprints() 解析（parseSprite + parseOneAnimation）
        ▼
EffectBlueprint（特效蓝图：id + 精灵 + 单个动画）
        │  EntityFactory::createEffect() 组装组件
        ▼
特效实体 = Transform + Sprite + 单动画 + RenderComponent(MAIN_LAYER+10) + OneShotRemoveTag
        ▲
        │  EffectSystem::onEffectEvent（一个 sink 收所有通用特效）
        │  combat_resolve_system.cpp：治疗命中 → enqueue(EffectEvent{"heal"_hs, pos, false})
```

- 新增**通用事件** `EffectEvent{name_id, position, is_flipped}`：任何系统只要一句话就能请求一个特效
- 新增**通用工厂函数** `createEffect(effect_id, position, is_flipped)`：按特效蓝图组装特效实体
- 新增**单动画解析器** `parseOneAnimation`：特效的 `"animation"` 是单个对象，不能复用解析多动画 map 的 `parseAnimationsMap`
- 新增**蓝图容器** `mEffectBlueprints`：沿用"一个 map + load + get + parse 三件套"的既有规律
- 治疗事件接线：`onHealEvent` 里把 TODO 替换成发 `EffectEvent{"heal"_hs, transform.mPosition, false}`

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| 特效蓝图 `EffectBlueprint` | `src/game/data/entity_blueprint.h`（ProjectileBlueprint 后） |
| 通用特效事件 `EffectEvent` | `src/game/defs/events.h`（EnemyDeadEffectEvent 后） |
| 特效蓝图容器 + 加载/查询/解析 | `src/game/factory/blueprint_manager.h/.cpp` |
| 通用特效工厂函数 | `src/game/factory/entity_factory.h/.cpp` |
| 特效蓝图数据文件 | `assets/data/effect_data.json`（本地已存在） |
| 加载特效蓝图 | `src/game/scene/game_scene.cpp:initEntityFactory` |
| 治疗特效接线 | `src/game/system/combat_resolve_system.cpp:onHealEvent` |
| 通用特效事件回调 | `src/game/system/effect_system.h/.cpp` |

### 2. 蓝图管理器"按数据种类扩展"的规律

`BlueprintManager` 的结构是高度自相似的。每新增一类蓝图，只需照抄四件套：

| 件 | 作用 |
|----|------|
| 私有容器 `mEffectBlueprints` | `std::unordered_map<entt::id_type, EffectBlueprint>` |
| `loadEffectBlueprints(path)` | 读 JSON → `entt::hashed_string(name)` 当键 → 逐条解析 → 插入容器 |
| `getEffectBlueprint(id)` | 按 id 查询，未找到 log error + 返回首元素兜底 |
| 私有 `parseXxx(json)` | 把 JSON 子字段解析成对应蓝图结构体 |

```cpp
bool BlueprintManager::loadEffectBlueprints(std::string_view effect_json_path) {
    // ... 读文件 → json ...
    try {
        for (auto& [name, data_json] : json.items()) {
            entt::id_type id = entt::hashed_string(name.c_str());       // "heal" → FNV-1a 哈希
            data::SpriteBlueprint sprite = parseSprite(data_json);      // 复用：字段与单位完全一致
            data::AnimationBlueprint animation = parseOneAnimation(data_json);  // 新解析器：单动画
            mEffectBlueprints.emplace(id, data::EffectBlueprint{ id, name,
                std::move(sprite), std::move(animation) });
        }
    }
    catch (const std::exception& e) {
        spdlog::error("加载效果数据时出错: {}", e.what());
        return false;
    }
    return true;
}
```

注意 load 函数的返回值要**接进 GameScene 的 initEntityFactory**——那里已经连了三条 load（enemy/player/projectile），用 `||` 串起来，任一条失败整体返回 false：

```cpp
if (!mBlueprintManager->loadEnemyClassBlueprints(...) ||
    !mBlueprintManager->loadPlayerClassBlueprints(...) ||
    !mBlueprintManager->loadProjectileBlueprints(...) ||
    !mBlueprintManager->loadEffectBlueprints("assets/data/effect_data.json")) {
    spdlog::error("加载蓝图失败");
    return false;
}
```

### 3. `parseOneAnimation` vs `parseAnimationsMap`：为什么不能复用

这是本课最容易踩的坑。单位蓝图的 `"animation"` 是**动画名 → 动画数据**的 map：

```json
"animation": { "idle": {...}, "walk": {...}, "attack": {...} }
```

所以 `parseAnimationsMap` 用 `json["animation"].items()` 遍历（每个元素是一对键值，键是动画名）。

而特效蓝图的 `"animation"` 是**单个对象**：

```json
"animation": {"duration":50, "row":0, "frames":[0,1,2,3,4,5,6,7,8,9,10]}
```

如果拿 `parseAnimationsMap` 硬套，`items()` 会遍历出 `duration/row/frames` 三对——把 `duration` 当成动画名去哈希，然后 `anim_data["frames"]` 对 50 这个数字取字段直接崩掉。所以新增 `parseOneAnimation`，直接读 `json["animation"]` 这一个对象：

```cpp
data::AnimationBlueprint BlueprintManager::parseOneAnimation(const nlohmann::json& json) {
    auto anim_data = json["animation"];
    std::vector<int> frames = anim_data["frames"].get<std::vector<int>>();
    // 处理可能存在的事件信息（与 parseAnimationsMap 相同）
    std::unordered_map<int, entt::id_type> events;
    if (anim_data.contains("events")) { ... }
    return data::AnimationBlueprint{ anim_data.value("duration", 100.0f),
        anim_data.value("row", 0),
        std::move(frames),
        std::move(events) };
}
```

这里有个优雅的细节：`skill_ready` 特效的 `"animation": {"frames":[0]}`——**没有 duration 也没有 row**。`anim_data.value("duration", 100.0f)` 的默认值 100 和 `value("row", 0)` 的默认值 0 正好兜住，不用为它写特判。`value(key, default)` 是 nlohmann::json 的"取键或给默认值"，比 `[]`（必须存在，不存在抛异常）宽容得多。

### 4. 特效 = 一次性动画实体

`createEffect` 是 `createEnemyDeadEffect` 的通用化，组装完全相同的骨架：

```cpp
entt::entity EntityFactory::createEffect(entt::id_type effect_id, const glm::vec2& position, const bool is_flipped) {
    auto entity = mRegistry.create();
    const auto& blueprint = mBlueprintManager.getEffectBlueprint(effect_id);
    // 添加Transform组件
    addTransformComponent(entity, position);
    // 添加Sprite组件
    addSpriteComponent(entity, blueprint.mSprite, is_flipped);
    // 添加Animation组件, 只有一个动画，名称为特效id
    addOneAnimationComponent(entity, blueprint.mAnimation, blueprint.mSprite, effect_id);

    // 补充其他必要组件（特效盖在单位上层）
    mRegistry.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 10);
    mRegistry.emplace<game::defs::OneShotRemoveTag>(entity);
    return entity;
}
```

三个关键点：

- **`addOneAnimationComponent`**：特效只有一个动画，不走 `addAnimationComponent` 的多动画 map 流程。这个函数早就在（007 之前为死亡特效准备的），组件里只有一个动画条目，`AnimationComponent` 的默认动画 ID 直接传特效 ID。
- **`RenderComponent(MAIN_LAYER + 10)`**：渲染层比单位（`MAIN_LAYER` = 10）高 10，保证特效盖在单位上面。特效在 Y-sort 系统里不受"角色互遮"影响，永远在最上层飘出来。
- **`OneShotRemoveTag`**：动画播完标记死亡，下一帧由 `RemoveDeadSystem` 清理。特效不用管"什么时候消失"——动画播完自己走，`RemoveDeadSystem` 已存在无需新增。这正是"特效 = 一次性动画实体"的 ECS 表达。

### 5. 通用 `EffectEvent` 取代硬编码专用事件

以前只有 `EnemyDeadEffectEvent`（专用，字段是敌人 class_id + 位置 + 翻转）。新增的 `EffectEvent` 字段几乎一样（特效 name_id + 位置 + 翻转），但语义泛化了——**任何系统都能发**：

```cpp
// combat_resolve_system.cpp — 治疗命中
const auto& transform = mRegistry.get<engine::component::TransformComponent>(event.mTarget);
mDispatcher.enqueue(game::defs::EffectEvent{ "heal"_hs, transform.mPosition, false });
```

`EffectSystem` 只需要在构造函数里多连一个 sink：

```cpp
mDispatcher.sink<game::defs::EnemyDeadEffectEvent>().connect<&EffectSystem::onEnemyDeadEffectEvent>(this);
mDispatcher.sink<game::defs::EffectEvent>().connect<&EffectSystem::onEffectEvent>(this);
```

回调一行搞定：

```cpp
void EffectSystem::onEffectEvent(const game::defs::EffectEvent& event) {
    mEntityFactory.createEffect(event.mNameId, event.mPosition, event.mIsFlipped);
}
```

以后升级特效、技能特效来了，其他系统 `enqueue(EffectEvent{"level_up"_hs, ...})` 即可，特效系统一个 sink 全收，不需要再为每个特效加专门的系统或事件。**通用事件把"特效的种类"从代码下沉到了数据**——蓝图上多一条就多一种特效。

### 6. 验证：治疗特效整条链打通的日志证据

冒烟测试跑约 20 秒，日志里最有力的证据是这一行：

```
[debug] 成功加载并缓存纹理: assets/textures/FX/Heal_Effect.png
```

它出现在治疗事件日志（`治疗者 ID: 520, 治疗目标 ID: 509, 治疗量: 100`，每 1.5 秒一次）之后——说明 `EffectEvent{"heal"_hs, ...}` 发出后，`createEffect` 找到了 heal 特效蓝图，特效实体的 Sprite 组件带上了 `Heal_Effect.png` 贴图并被渲染系统首次加载。全链无 ERROR/WARN，`EntityFactory 加载完成`（4 类蓝图全加载成功）。

---

## 与参考实现（WispSnow/MonsterWar，commit c62ae52）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段 `id_/name_/sprite_/animation_` | m-prefix：`mId/mName/mSprite/mAnimation` | 遵循本地 m-prefix 规范 |
| 2 | `EffectEvent` 字段 `name_id_/position_/is_flipped_` | `mNameId/mPosition/mIsFlipped` | 同上 |
| 3 | 容器 `effect_blueprints_` | `mEffectBlueprints` | 同上 |
| 4 | 缩进 2 空格 | game/factory + game/system 用 4 空格，game/scene 用 tab | 本地各目录缩进约定 |
| 5 | `addOneAnimationComponent` 需要新建 | 本地已存在（`entity_factory.h:73`，死亡特效已在用） | 上一课就绪，直接复用 |
| 6 | `effect_data.json` 需新增 | 本地已存在且内容一致 | 上一课就绪 |
| 7 | 逻辑、结构 | 完全照搬 | 本课无本地设计差异 |

---

## 学习要点

### 1. 蓝图管理器是"可预测的样板"：一个 map + load + get + parse 三件套

每新增一类数据驱动实体，几乎不用动脑：加容器、加 load、加 get、加 private parse。这个"自相似结构"的好处是——读一个 `loadProjectileBlueprints` 就能猜出 `loadEffectBlueprints` 长什么样；代价是样板代码重复。取舍上：数据驱动的扩展收益远大于样板成本，因为实体种类会越来越多。

### 2. JSON 结构决定解析器，不能"一个解析器通吃"

`parseAnimationsMap` 处理 `"animation"` 为 map 的结构，`parseOneAnimation` 处理 `"animation"` 为单对象的结构——**解析器的形态必须跟数据形态一一对应**。写解析代码前先看清 JSON 结构（对象还是数组、map 还是标量），这是复用与新增判断的第一步。

### 3. `value(key, default)`：宽容取键，让数据可选字段不爆雷

`anim_data.value("duration", 100.0f)` 在键缺失时给默认值，`[]` 直接抛异常。`skill_ready` 只有 `frames` 没有 `duration/row`，就是靠它安然通过。**可选字段一律用 `value`，必选字段才用 `[]`**。

### 4. 通用事件把"种类"从代码下沉到数据

专用事件（`EnemyDeadEffectEvent`）每加一种特效就要改系统；通用事件（`EffectEvent` + 蓝图）加特效只改数据。系统里"种类"越多，越该往数据驱动走——本课治疗是 `EffectEvent` 的第一个使用者，下一课技能施放还会继续用同一套。

### 5. 一次性实体的 ECS 表达：单动画 + 移除标签，生命周期零管理

特效实体不需要任何"何时消失"的逻辑：单动画播完，`OneShotRemoveTag` 标记死亡，`RemoveDeadSystem` 清理。**把生命周期交给既有的清理管道，而不是每个特效自己计时**——这是组合优于继承的又一次体现。

### 6. 渲染层是视觉正确性的最后一环

同样的实体骨架，渲染层决定"盖不盖得住"。特效要盖住单位就 `MAIN_LAYER + 10`（参考里投射物是 `MAIN_LAYER + 1`）。调渲染优先级时想清楚"这个实体要叠在谁上面"。
