# 出击准备与出击完成：放置点实体 + 幽灵单位 + 事件流

## 问题

上一课把"规则层"建好了——cost 经济、胜负判定、出击面板 UI 都有了。但点一下肖像之后呢？**出击流程断在了半路**：

- **地图上没有"可放置点"**——塔防里单位只能放在指定的位置，这个"哪里能放"的信息完全不存在
- **没有"准备出击"的中间态**——点击肖像应该出现一个跟随鼠标的幽灵单位（半透明、绿/红提示能否放置），而不是直接凭空生成
- **没有放置交互**——幽灵拖到合法位置点左键才真正出兵、扣 cost、占住位置；右键取消
- **远程单位没有攻击范围提示**——拖幽灵时该画一个范围圆，让玩家知道放这能不能打到路
- **渲染不支持改颜色**——幽灵绿/红、范围圆半透明，之前的 `drawSprite` 根本没有颜色参数
- **击杀侧通关判据是坏的**——`GameStats.mEnemyCount` 从没初始化（一直是 0），`(到达数+击杀数) >= 敌人总数` 这个判断形同虚设

一句话：**规则层有了，但"出击"还点不动。**

## 结论

用一条完整的事件流把"点肖像 → 幽灵拖拽 → 落子出兵"串起来，核心是新增 **`PlaceUnitSystem`**（放置单位系统）：

```
点击肖像 ── PrepUnitEvent ──┐
                            ▼
              PlaceUnitSystem（订阅事件 + 绑定鼠标按键）
   ├─ onPrepUnitEvent  检查 cost → 清掉旧幽灵 → 在鼠标处创建幽灵实体（createUnitPrep）
   ├─ update（每帧）   幽灵跟随鼠标 → checkTargetPlace 检测是否落在合法放置点上
   │                    → 幽灵染绿（可放置）/ 染红（不可放置）
   ├─ onPlaceUnit(左键) 幽灵有效 → 按蓝图+等级+稀有度建真实单位 → 放置点加 PlaceOccupiedComponent
   │                    → 扣 cost → 幽灵标记死亡 → 发 RemoveUIPortraitEvent → 播放置音效
   └─ onCancelPrepUnit(右键)  幽灵标记死亡
```

支撑它的三块地基：

1. **放置点实体**：`buildPlace()` 从 tileset 瓦片属性里读到 `place="melee"` / `"range"`，给关卡对象实体打上 `MeleePlaceTag` / `RangedPlaceTag`
2. **幽灵单位**：`UnitPrepComponent` 承载预备数据（角色 id / 职业类型 / 范围 / cost），`createUnitPrep()` 创建
3. **渲染变色**：`RenderComponent.mColor` + `drawSprite` 颜色参数（SDL 纹理颜色调制），新增 `drawFilledCircle` 画范围圆

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| 放置点标签 `MeleePlaceTag`/`RangedPlaceTag` | `src/game/defs/tags.h` |
| 预备单位组件 `UnitPrepComponent` | `src/game/component/unit_prep_component.h` |
| 放置点占用组件 `PlaceOccupiedComponent` | `src/game/component/place_occupied_component.h` |
| 常量 `PLACE_RADIUS` / `RANGE_COLOR` | `src/game/defs/constants.h` |
| 幽灵单位工厂方法 `createUnitPrep` | `src/game/factory/entity_factory.cpp` |
| 放置点识别 `buildPlace` | `src/game/loader/entity_builder_mw.cpp:69` |
| **放置单位系统 `PlaceUnitSystem`** | `src/game/system/place_unit_system.h/.cpp` |
| 范围圆渲染 `RenderRangeSystem` | `src/game/system/render_range_system.h/.cpp` |
| 颜色字段 `RenderComponent::mColor` | `src/engine/component/render_component.h` |
| 变色绘制 `drawSprite` / 圆形 `drawFilledCircle` | `src/engine/render/renderer.cpp` |
| 击杀侧通关判定 | `src/game/system/combat_resolve_system.cpp:79` |
| GameScene 挂载新系统 | `src/game/scene/game_scene.cpp:260-261` |

### 2. 放置点实体：地图数据怎么变成可放置区域

塔防的"哪里能放单位"是关卡地图的一部分，应该由地图数据驱动，而不是硬编码。流程：

```
buildings.tsj（tileset）
  瓦片 id=10 定义属性 place="melee"（melee_place.png）
  瓦片 id=11 定义属性 place="range"（range_place.png）
        ↓  LevelLoader::getTileInfoByGid 把瓦片属性读进 TileInfo.mProperties
level1.tmj 的 placement 图层
  17 个对象对象，gid=719/720（指向上述瓦片），每个都是 64×64 的一个格子
        ↓  loadObjectLayer 对有 gid 的对象走 configure + build
BasicEntityBuilder::build()  已生成精灵/变换/渲染实体
   ↓  EntityBuilderMW::buildPlace() 追加打标签
实体 + MeleePlaceTag / RangedPlaceTag（放置点）
```

`buildPlace()` 核心（`entity_builder_mw.cpp:69`）：

```cpp
if (mTileInfo && mTileInfo->mProperties) {
    for (auto& property : mTileInfo->mProperties.value()) {
        if (property.value("name", "") == "place") {
            auto type = property.value("value", "");
            if (type == "melee") mRegistry.emplace<game::defs::MeleePlaceTag>(mEntityId);
            else if (type == "range") mRegistry.emplace<game::defs::RangedPlaceTag>(mEntityId);
        }
    }
}
```

**为什么标签而不是组件带数据？** 放置点只需要"区分近战/远程"这一个维度，不需要额外数据字段——空结构体标签（tag）正好。要查某类放置点就 `view<MeleePlaceTag, Transform, Sprite>(exclude<PlaceOccupiedComponent>)`。以后想区分更多类型，加标签即可。

**放置点占用**用 `PlaceOccupiedComponent{ mEntity }` 记录"谁占了这个点"，配合 `view(entt::exclude<PlaceOccupiedComponent>)` 实现"已占用的点不可再放"。移除单位时（`onRemoveUnitEvent`）要把对应占用组件删掉，否则位置永远锁死。

### 3. 幽灵单位 `UnitPrepComponent`：把"即将发生的事"做成实体

点击肖像后，先不生成真正的单位，而是生成一个**预备实体**：

```cpp
// entity_factory.cpp:createUnitPrep
entt::entity EntityFactory::createUnitPrep(entt::id_type name_id, entt::id_type class_id, int cost, const glm::vec2& position) {
    auto entity = mRegistry.create();
    const auto& blueprint = mBlueprintManager.getPlayerClassBlueprint(class_id);
    addTransformComponent(entity, position);
    addSpriteComponent(entity, blueprint.mSprite);
    mRegistry.emplace<game::component::UnitPrepComponent>(entity, name_id, blueprint.mPlayer.mType, blueprint.mStats.mRange, cost);
    mRegistry.emplace<engine::component::RenderComponent>(entity, 100);       // 幽灵层压在最上
    if (blueprint.mPlayer.mType == game::defs::PlayerType::RANGED) {
        mRegistry.emplace<game::defs::ShowRangeTag>(entity);                 // 远程幽灵要画范围圆
    }
    return entity;
}
```

`UnitPrepComponent` 存的都是**预备态数据**：`mNameId`（会话数据里查角色）、`mType`（判断找近战/远程放置点）、`mRange`（画范围圆）、`mCost`（落子时扣）。

**为什么用实体做幽灵，而不是直接存一个"预备中"标志？** 幽灵有精灵、有变换、要每帧跟着鼠标动、要按状态变色、要画范围圆——这些全是 ECS 组件的活。让幽灵成为实体，`PlaceUnitSystem::update()` 只用 `view<UnitPrepComponent, TransformComponent>` 就能统一驱动它，最后落子/取消时打个 `DeadTag` 交给既有的 `RemoveDeadSystem` 清理，**复用整套清理管线**。这就是 ECS 里"状态也实体化"的思路——过渡状态不再是一个孤立的标志，而是一个可被系统处理的实体。

### 4. `PlaceUnitSystem` 事件流：从肖像到落子

系统在构造器里同时绑定**鼠标按键**和**事件**（`place_unit_system.cpp:35`）：

```cpp
input_manager.onAction("mouse_left"_hs).connect<&PlaceUnitSystem::onPlaceUnit>(this);
input_manager.onAction("mouse_right"_hs).connect<&PlaceUnitSystem::onCancelPrepUnit>(this);
dispatcher.sink<game::defs::PrepUnitEvent>().connect<&PlaceUnitSystem::onPrepUnitEvent>(this);
dispatcher.sink<game::defs::RemovePlayerUnitEvent>().connect<&PlaceUnitSystem::onRemoveUnitEvent>(this);
```

注意：`mouse_left/right` 之前在 GameScene 里注册，本课**迁到了 PlaceUnitSystem**——按键的所有权跟着"谁处理它"走，场景不再管鼠标左/右键。`update()` 里幽灵每帧跟随鼠标：

```cpp
// place_unit_system.cpp:58
mTargetPlaceEntity = entt::null;                        // 每帧先置空
for (auto entity : view<UnitPrepComponent, TransformComponent>) {
    transform.mPosition = mouse_pos_world;              // 幽灵 = 鼠标位置
    checkTargetPlace(transform.mPosition, unit_prep.mType);
    render.mColor = (mTargetPlaceEntity != entt::null) ? FColor::green() : FColor::red();
}
```

**`checkTargetPlace` 的几何中心问题**（`place_unit_system.cpp:82`）：Tiled 里对象参照点在**左上角**，而"是否靠近放置点"应该按**中心**算。所以：

```cpp
auto center_position = place_transform.mPosition + place_sprite.mSize * place_transform.mScale / 2.0f;
if (distanceSquared(position, center_position) < PLACE_RADIUS * PLACE_RADIUS)
    mTargetPlaceEntity = place_entity;
```

用**距离平方**比较避免开根号。`PLACE_RADIUS = 40.0f`，精灵 64×64，半对角线 ≈ 45，所以鼠标落在格子上方偏一点也能吸附到。

**落子 `onPlaceUnit`**（`place_unit_system.cpp:146`）的完整动作序列：

```
① mTargetPlaceEntity 有效？（无效直接 return）
② 取放置点中心作为出生位置
③ 从 SessionData.unit_map 按 mNameId 查角色数据
④ createPlayerUnit(class_id, pos, level, rarity) 建真实单位 + NameComponent
⑤ 放置点 emplace PlaceOccupiedComponent(unit_entity)  ← 占住
⑥ game_stats.mCost -= mCost                            ← 扣费
⑦ 幽灵 emplace_or_replace DeadTag                      ← 交给清理
⑧ enqueue RemoveUIPortraitEvent                        ← 面板移除该肖像
⑨ 图层修正：若放置点图层超过 MAIN_LAYER，玩家图层 = 放置点图层+1
⑩ playSound("unit_placed")
```

**第⑤步的语义**：占用组件挂在**放置点实体**上、记录**占用它的单位**。这样 `checkTargetPlace` 用 `exclude<PlaceOccupiedComponent>` 一眼跳过已占用的点；而 `onRemoveUnitEvent` 遍历所有占用组件、比对 `mEntity == event.mEntity` 找到对应放置点再解除占用。

**右键取消 `onCancelPrepUnit`** 返回 `false`，注释写着"让鼠标右键可以穿透"——即取消动作消费掉这次右键，但返回值不阻塞其他订阅者（如果将来有右键菜单之类）。

**为什么落子也走 `RemovePlayerUnitEvent`？** 之前 `onClearAllPlayers`（暂停键）直接 `destroy()` 实体。本课改成对所有玩家 `enqueue(RemovePlayerUnitEvent)`，由 `PlaceUnitSystem::onRemoveUnitEvent` 统一处理——标记死亡 **并且** 解除放置点占用。一个事件同时协调"清理单位"和"释放位置"，比两处各写各的更一致。

### 5. 渲染变色：给绘制管线加颜色

**`RenderComponent` 加 `mColor`**（`render_component.h`）：

```cpp
engine::utils::FColor mColor{ engine::utils::FColor::white() };
```

**`drawSprite` 加颜色参数**（`renderer.cpp`）：SDL 纹理本身是白底带透明度（用颜色调制在着色），所以用 `SDL_SetTextureColorModFloat` 把白色纹理染成目标色：

```cpp
SDL_SetTextureColorModFloat(texture, color.r, color.g, color.b);
SDL_SetTextureAlphaModFloat(texture, color.a);
```

`RenderSystem` 把 `render.mColor` 一路传进 `drawSprite`。**幽灵的绿/红只是改一个成员变量，不用换贴图**——同一张角色图，`mColor = green()` 就是可放置，`red()` 就是不可放置。

**范围圆 `drawFilledCircle`**（`renderer.cpp`）：复用 `assets/textures/UI/circle.png`，`worldToScreen` 后以半径画个正方形目标矩形，同样用颜色调制染成 `RANGE_COLOR = {0,1,0,0.3}`（半透明绿）。`RenderRangeSystem` 只遍历 `view<ShowRangeTag, TransformComponent, UnitPrepComponent>`，用 `prep.mRange` 当半径——**只有远程幽灵才带 `ShowRangeTag`**，近战不画。

### 6. 击杀侧通关判定修复

`GameRuleSystem` 里本来就有 `(到达数+击杀数) >= 敌人总数 → 通关` 的判定，但 `mEnemyCount` 从没被赋值（一直是 0），导致敌人一来就通关。本课在 `createTestEnemy()` 里补上（`game_scene.cpp:292`）：

```cpp
mGameStats.mEnemyCount = enemy_count;
mRegistry.ctx().get<game::data::GameStats&>().mEnemyCount = enemy_count;
```

**注意第二行不能省**：`initRegistryContext()` 把 `mGameStats` **拷贝**进 ctx（值语义），`createTestEnemy()` 在它之后运行，改成员 `mGameStats` 不会反映到 ctx 里那份。必须**再写一次 ctx 里的实例**，各系统（读 ctx）才看得到真实总数。这正是值语义容易踩的坑——"我改了成员，系统怎么没反应？"答案：系统读的是 ctx 里那份拷贝。

### 7. 与参考实现（WispSnow/MonsterWar）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 幽灵用半透明渲染 | 本地 `mColor` 直接染绿/红（不透明度仍 1.0） | 本地幽灵贴图尚无非透明底色，先保可见性；将来可调 `FColor` 的 a 通道 |
| 2 | `checkTargetPlace` 按距离平方+中心点 | 与参考一致 | — |
| 3 | `RemovePlayerUnitEvent` 由移除系统处理 | 本地由 `PlaceUnitSystem::onRemoveUnitEvent` 处理（标记死亡+解除占用） | 解除占用和清理单位必须原子化，放同一系统保证顺序 |
| 4 | 命名 `cost_`/`registry_`（trailing underscore） | 本地 m-prefix | 遵循本地编码规范 |
| 5 | `onCancelPrepUnit` 返回 true 拦截右键 | 本地返回 false 穿透 | 鼠标按键可被多个订阅者监听，返回值 false 不阻塞其他处理 |

---

## 学习要点

### 1. 过渡状态实体化

"预备出击"这种中间态，与其用标志位，不如直接造一个实体（`UnitPrepComponent`）。好处：**复用现有系统的驱动和清理**——跟鼠标动靠 `TransformComponent`，变色靠 `RenderComponent.mColor`，取消/落子靠打 `DeadTag` 交给 `RemoveDeadSystem`。ECS 里"状态"也是一等公民。

### 2. 数据属于谁：成员变量 vs ctx 拷贝

`GameStats` 存进 `registry.ctx()` 是**值语义的拷贝**。改成员变量不自动同步到 ctx。谁改数据，谁就要负责同步 ctx 里的实例（或干脆只改 ctx）。排查"系统没反应"时先想：**系统读的是不是 ctx 里那份？**

### 3. 按键所有权跟着处理者走

`mouse_left/right` 的绑定从 GameScene 迁到 PlaceUnitSystem，是因为"谁处理左键落子"就归谁注册。绑定与处理逻辑放同一处，避免"输入在 A 注册、逻辑在 B 处理"的割裂。析构时也要记得 `disconnect`，防止悬空回调。

### 4. 几何参照点：Tiled 左上角 vs 逻辑中心

地图对象坐标是左上角（原点是左上角、y 向下），但游戏逻辑判断（放置吸附、碰撞）通常要中心。**凡是从地图数据拿坐标参与逻辑判断，先想清楚参照点**。`transform + sprite.size * scale / 2` 这个中心公式在本项目多次出现。

### 5. 渲染变色 = 颜色调制，不是换图

白色底 + 透明度的纹理可以用 `SDL_SetTextureColorModFloat` 染任意色，同一个精灵图零成本切状态（绿=可、红=不可）。需要半透明就调 `AlphaMod`。这条思路也让"范围圆"这类纯色形状可以直接复用一张圆形贴图。

### 6. 别忽略"看起来已存在但从未初始化"的坏判断

`mEnemyCount` 从 0 开始的通关判定 bug 说明：**一个看似完整的胜负逻辑，可能因为某个字段从未赋值而完全失效**。排查这种"条件永远为真/假"的问题，先确认数据来源有没有被真正初始化。
