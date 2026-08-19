# MonsterWar

基于 SDL3 + EnTT ECS 的 2D 游戏引擎/框架。

## 项目概述

MonsterWar 是一个正在开发中的 2D 游戏，采用**引擎/游戏分离**的架构设计。引擎层（`engine/`）提供通用的游戏基础设施，游戏层（`game/`）构建具体的游戏逻辑。

### 核心架构

| 层级                       | 说明                                                         |
| -------------------------- | ------------------------------------------------------------ |
| **引擎层** (`src/engine/`) | ECS 框架、渲染、资源管理、UI、音频、输入、场景               |
| **游戏层** (`src/game/`)   | 游戏场景、敌人 AI（寻路+战斗）、实体工厂、蓝图系统、战斗系统 |

### 应用流程

```
main.cpp
  → GameApp::run()
    → init()
      → initDispatcher → Config → SDL → GameState → Time → ResourceManager
      → AudioPlayer → Renderer → TextRenderer → Camera → InputManager
      → Context → SceneManager (PushSceneEvent → GameScene)
    → loop:
      → handleEvents (SDL 事件 → InputManager → dispatcher)
      → update(delta)
        → RemoveDeadSystem (清理死亡实体)
        → TimerSystem (推进攻击冷却计时器，冷却结束添加 AttackReadyTag)
        → SetTargetSystem (为无目标角色寻找/刷新攻击目标)
        → OrientationSystem (根据目标/移动方向翻转精灵朝向)
        → FollowPathSystem (敌人路径跟随，触发 EnemyArriveHomeEvent)
        → BlockSystem (检测阻挡，设置速度 0 + 阻挡动画)
        → AttackStarterSystem (冷却完毕的角色触发攻击动画，锁定行动)
        → ProjectileSystem (投射物飞行弧线更新，到达后发出 AttackEvent)
        → MovementSystem (Velocity → Transform)
        → AnimationSystem (帧动画推进，响应 PlayAnimationEvent / AnimationFinishedEvent)
        → YSortSystem (Y 坐标排序)
      → render()
        → RenderSystem (按 layer+depth 排序渲染)
        → HealthBarSystem (绘制受伤单位的血量条)
    → close()
      → 逆序清理所有子系统
```

## ECS 组件系统（EnTT）

实体通过组合以下组件来定义行为：

### 引擎层组件（`engine::component`）

| 组件               | 文件                    | 说明                               |
| ------------------ | ----------------------- | ---------------------------------- |
| TransformComponent | `transform_component.h` | 位置、缩放、旋转                   |
| SpriteComponent    | `sprite_component.h`    | 精灵纹理、源矩形、翻转标志         |
| RenderComponent    | `render_component.h`    | 渲染图层与深度（Y-Sort 排序用）    |
| AnimationComponent | `animation_component.h` | 帧动画集合（Animation 对象映射表） |
| VelocityComponent  | `velocity_component.h`  | 移动速度（vec2）                   |
| ParallaxComponent  | `parallax_component.h`  | 视差滚动效果参数                   |
| NameComponent      | `name_component.h`      | 实体名称（字符串 + 哈希 ID）       |
| AudioComponent     | `audio_component.h`     | 音效集合（name_hash → sound_id）   |
| TileLayerComponent | `tilelayer_component.h` | 瓦片层 2D 网格数据                 |

### 游戏层组件（`game::component`）

| 组件                  | 文件                     | 说明                               |
| --------------------- | ------------------------ | ---------------------------------- |
| EnemyComponent        | `enemy_component.h`      | 敌人目标路径节点 ID + 移动速度     |
| StatsComponent        | `stats_component.h`      | RPG 属性（HP/ATK/DEF/射程/攻速等） |
| ClassNameComponent    | `class_name_component.h` | 职业名称或敌人类型（ID + 名称）    |
| PlayerComponent       | `player_component.h`     | 玩家单位，出击消耗                 |
| BlockerComponent      | `blocker_component.h`    | 阻挡者（最大/当前阻挡数量）        |
| BlockedByComponent    | `blocked_by_component.h` | 被阻挡者（记录阻挡自己的实体）     |
| TargetComponent       | `target_component.h`     | 攻击目标（锁定要攻击的实体）       |
| ProjectileComponent   | `projectile_component.h` | 投射物（目标、伤害、弧线飞行参数） |
| ProjectileIDComponent | `projectile_component.h` | 投射物ID，附加在远程角色上         |
| UnitPrepComponent     | `unit_prep_component.h`  | 预备出击单位（幽灵）：角色ID/类型/范围/cost |
| PlaceOccupiedComponent | `place_occupied_component.h` | 放置点占用（记录占用该放置点的单位） |

## ECS 系统

### 引擎层系统（`engine::system`）

| 系统            | 文件                   | 说明                                                                             |
| --------------- | ---------------------- | -------------------------------------------------------------------------------- |
| MovementSystem  | `movement_system.cpp`  | 遍历 Velocity + Transform 实体，`velocity * dt` 更新位置                         |
| AnimationSystem | `animation_system.cpp` | 遍历 Animation + Sprite 实体，推进帧计时器并切换帧；通过 PlayAnimationEvent 驱动 |
| RenderSystem    | `render_system.cpp`    | 遍历 Transform + Sprite + Render 实体，按 (layer, depth) 排序渲染                |
| YSortSystem     | `ysort_system.cpp`     | 遍历 RenderComponent 实体，将 `mDepth` 设为 Y 坐标                               |
| AudioSystem     | `audio_system.cpp`     | 监听 PlaySoundEvent，通过 AudioPlayer 播放音效                                   |

### 游戏层系统（`game::system`）

| 系统                 | 文件                         | 说明                                                                              |
| -------------------- | ---------------------------- | --------------------------------------------------------------------------------- |
| FollowPathSystem     | `follow_path_system.cpp`     | 敌人沿路径节点移动，到达终点触发 EnemyArriveHomeEvent                             |
| RemoveDeadSystem     | `remove_dead_system.cpp`     | 延迟清理标记 DeadTag 的死亡实体                                                   |
| BlockSystem          | `block_system.cpp`           | 检测阻挡距离，设置敌人速度为 0 并切换阻挡动画                                     |
| TimerSystem          | `timer_system.cpp`           | 推进攻击冷却计时器，冷却结束添加 AttackReadyTag                                   |
| SetTargetSystem      | `set_target_system.cpp`      | 为无目标角色寻找目标（玩家找敌人、远程敌人找玩家、治疗者找低血量盟友）            |
| AttackStarterSystem  | `attack_starter_system.cpp`  | AttackReadyTag 实体触发攻击动画，添加 ActionLockTag                               |
| OrientationSystem    | `orientation_system.cpp`     | 根据目标位置/阻挡者/移动方向翻转精灵朝向                                          |
| AnimationStateSystem | `animation_state_system.cpp` | 监听 AnimationFinishedEvent，恢复循环动画（idle/walk）                            |
| AnimationEventSystem | `animation_event_system.cpp` | 监听 AnimationEvent（动画帧事件），发出 AttackEvent / HealEvent / PlaySoundEvent  |
| CombatResolveSystem  | `combat_resolve_system.cpp`  | 监听 AttackEvent / HealEvent，计算伤害/治疗量，处理死亡和阻挡计数                 |
| ProjectileSystem     | `projectile_system.cpp`      | 响应 EmitProjectileEvent 创建投射物实体，更新飞行弧线轨迹，到达后发出 AttackEvent |
| EffectSystem         | `effect_system.cpp`          | 监听 EnemyDeadEffectEvent，通过实体工厂创建死亡特效                               |
| HealthBarSystem      | `health_bar_system.cpp`      | 渲染受伤单位的血量条（按血量百分比变色）                                          |
| PlaceUnitSystem      | `place_unit_system.cpp`      | 出击准备/放置：幽灵跟随鼠标、检测放置点、落子出兵、扣费、占用放置点、取消          |
| RenderRangeSystem    | `render_range_system.cpp`    | 渲染预备远程单位的攻击范围圆（半透明绿色）                                        |

## 关卡加载系统（Builder 模式）

重写了关卡加载系统，将 JSON 解析和实体创建分离。

- **LevelLoader** (`engine::loader`) — 负责解析 Tiled 地图文件（.tmj / .tsj）
  - 支持三种图层类型：图片图层、瓦片图层、对象图层
  - 支持 Tileset 的单一图片和瓦片集两种模式
  - 支持瓦片翻转标志（水平/垂直/对角线）
  - 自动解析地图背景色
  - 图层顺序追踪（支持从属性中读取自定义 `order`）
- **BasicEntityBuilder**（建造者模式）— 组件化构建游戏实体
  - 逐步构建：`buildBase()` → `buildSprite()` → `buildTransform()` → `buildRender()` → `buildAnimation()` → `buildAudio()`
  - 三个 `configure()` 重载分别处理自定义形状、图片对象、瓦片层
  - 可被子类继承扩展（虚函数 `build()`）
- **EntityBuilderMW** (`game::loader`) — MonsterWar 扩展构建器，从 Tiled 对象图层解析路径节点和起点，并从瓦片属性识别放置区域（`buildPlace` 打 `MeleePlaceTag`/`RangedPlaceTag`）
- 删除了旧的 `scene::LevelLoader`（逻辑耦合严重）

## Y-Sort 渲染排序系统

实现了基于 y 坐标的渲染排序，保证角色在屏幕上的正确遮挡关系。

- **YSortSystem** — 每帧更新 `RenderComponent::mDepth` 为实体的 y 坐标，然后按 `(layer, depth)` 排序
- **RenderComponent** — 存储 `mLayer` 和 `mDepth`，通过 `operator<` 定义排序规则
- **RenderSystem** — 改用 `RenderComponent` 驱动遍历顺序，确保按排序结果渲染

## 路径节点系统

定义敌人从起点到终点的寻路路径网络。

- **WaypointNode** (`game::data`) — 路径节点数据结构：ID、位置坐标、下一节点 ID 列表
- **EntityBuilderMW** — 加载关卡时从 Tiled 对象图层解析路径节点和起点
  - 识别 `point=true` 的对象作为路径节点
  - 解析 `next*` 属性建立节点间的连接关系
  - 解析 `start=true` 属性标记起点
- **FollowPathSystem** — 敌人沿路径移动，到达节点后随机选择下一节点分支，到达终点触发 `EnemyArriveHomeEvent`

## 蓝图与实体工厂系统

实现了蓝图驱动的实体创建机制，将实体数据定义与创建逻辑解耦。

- **BlueprintManager** (`game::factory`) — 从 JSON 加载敌人/玩家蓝图数据并解析为结构化蓝图
  - 支持子蓝图分别解析：Stats、Sprite、Animation、Sound、Enemy、Player、DisplayInfo
  - 提供 `getEnemyClassBlueprint()` / `getPlayerClassBlueprint()` 按 ID 查询蓝图
- **EntityFactory** (`game::factory`) — 根据蓝图数据创建 ECS 实体并组装组件
  - `createEnemyUnit()` — 按蓝图自动添加 Transform、Sprite、Animation、Stats、Enemy 等组件
  - `createPlayerUnit()` — 按蓝图创建玩家单位，添加 Player、Blocker 等组件
  - `createEnemyDeadEffect()` — 根据敌人蓝图创建死亡特效实体（复用 "damage" 动画，播完自动移除）
  - `addOneAnimationComponent()` — 创建只含单个动画的组件（`loop=false`），用于特效实体
  - 提供独立的 `addXxxComponent()` 方法供子类扩展
- **蓝图数据结构** (`entity_blueprint.h`) — 定义了一系列子蓝图结构体
  - `EnemyClassBlueprint` — 聚合所有子蓝图，作为完整敌人类型定义
  - `PlayerClassBlueprint` — 玩家职业蓝图，包含 `PlayerBlueprint`（类型、技能、阻挡、消耗）
  - `PlayerBlueprint` 通过 `PlayerType` 枚举（MELEE / RANGED / MIXED）区分单位类型
  - `ProjectileBlueprint` — 投射物蓝图（弧线高度、飞行时间、精灵、音效）
  - `PlayerClassBlueprint` / `EnemyClassBlueprint` 包含 `mProjectileId` 字段，关联远程单位的投射物类型
- **玩家单位组件**：`PlayerComponent`（出击消耗）、`BlockerComponent`（阻挡计数）、`BlockedByComponent`（被阻挡引用）

## 会话数据系统（SessionData）

实现了跨场景的游戏进度持久化，把"当前关卡、积分、通关状态、玩家角色池"从一场战斗的临时状态中独立出来。

- **SessionData** (`game::data`) — 会话数据类，持有游戏进度，用 `shared_ptr` 供多个场景共享
  - 关卡信息：`mLevelNumber`（当前关卡）/ `mPoint`（积分）/ `mLevelClear`（是否通关）
  - 角色池：`mUnitMap`（角色名哈希 → UnitData）
  - `loadDefaultData()` — 从 `assets/data/default_session_data.json` 加载默认进度
  - `saveToFile()` — 序列化为 JSON 存档（自动创建父目录、4 空格缩进）
  - 角色操作：`addUnit()` / `removeUnit()` / `addUnitLevel()` / `addUnitRarity()` / `clearUnits()`
  - 进度操作：`addPoint()` / `addOneLevel()` / `setLevelClear()` / `clear()`
- **UnitData** — 单个角色的数据档案：角色名哈希、职业哈希、名字、职业、等级、稀有度
- **GameScene 集成** (`game_scene.cpp`) — `initSessionData()` 在 `init()` 最前面初始化，从默认数据加载后缓存 `mLevelNumber`；`testSessionData()` 打印验证

```
GameScene（当前战斗）── 读写 ──▶ SessionData（内存进度）── saveToFile() ──▶ 存档 JSON
                                      │
                                      └── loadDefaultData() ◀── default_session_data.json
```

## 出击面板与关卡规则系统

实现了塔防的"规则层"：cost 经济、基地血量 / 胜负判定、出击面板 UI，以及把共享数据放进 `registry.ctx()` 的服务定位器模式。

- **GameStats** (`game::data`) — 关卡内统计数据：cost / cost生成速率 / 基地血量 / 敌人总数 / 到达数 / 击杀数，存入 `registry.ctx()` 共享
- **CostRegenComponent** (`game::component`) — 额外 cost 恢复速率（如建筑）
- **GameRuleSystem** (`game::system`) — 事件驱动规则层，订阅 4 个事件
  - `update()` — cost 恢复（基础速率 + CostRegenComponent 叠加）、通关延迟计时
  - `onEnemyArriveHome` — 到达数+1、基地血量-1；基地被毁 → `GameEndEvent{false}`，全歼 → 延迟 2 秒发 `LevelClearEvent`
  - `onUpgradeUnitEvent` — 扣 cost、等级+1、按蓝图重算属性、播升级音效
  - `onRetreatEvent` — 返还 cost、发 `RemovePlayerUnitEvent`
- **UnitsPortraitUI** (`game::ui`) — 出击面板类：cost 遮盖刷新、左右滚动、按钮三回调（点击 → `PrepUnitEvent`、悬停进/出 → 事件）、出战后移除肖像
- **registry.ctx() 服务定位器** (`game_scene.cpp:222`) — GameStats（值语义）+ 三个 shared_ptr（共享语义）存入注册表上下文，系统/UI 各自取用
- **UIButton 三回调** (`engine::ui`) — 构造器支持 点击 / 悬停进入 / 悬停离开 三个回调

```
registry.ctx()（服务定位器）
   ├── GameStats（值语义，关卡临时状态）
   ├── shared_ptr<SessionData/UIConfig/BlueprintManager>（共享语义，跨场景）
   ├── GameRuleSystem ── 事件驱动（EnemyArriveHome/UpgradeUnit/Retreat/LevelClearDelayed）
   └── UnitsPortraitUI ── 出击面板（遮盖 / 滚动 / 点击发 PrepUnitEvent）
```

## 出击准备与出击完成系统

实现了"点击肖像 → 幽灵跟随鼠标 → 落子出兵"的完整出击流程：地图放置点识别、预备单位（幽灵）、放置交互、攻击范围圆渲染。

### 放置单位流程

```
点击肖像 ── PrepUnitEvent ──┐
                            ▼
              PlaceUnitSystem（订阅事件 + 绑定鼠标按键）
   ├─ onPrepUnitEvent   检查 cost → 清掉旧幽灵 → 鼠标处创建幽灵（createUnitPrep）
   ├─ update（每帧）     幽灵跟随鼠标 → checkTargetPlace 检测合法放置点
   │                     → 幽灵染绿（可放置）/ 染红（不可放置）
   ├─ onPlaceUnit(左键)  幽灵有效 → 建真实单位（蓝图+等级+稀有度）
   │                     → 放置点加 PlaceOccupiedComponent → 扣 cost
   │                     → 幽灵标记死亡 → RemoveUIPortraitEvent → 播放置音效
   └─ onCancelPrepUnit(右键)  幽灵标记死亡（右键穿透）
```

- **放置点实体** — `EntityBuilderMW::buildPlace()` 从 tileset 瓦片属性读 `place="melee"/"range"`，给地图对象实体打 `MeleePlaceTag` / `RangedPlaceTag`；已占用点用 `PlaceOccupiedComponent` 标记，view 用 `exclude` 跳过
- **UnitPrepComponent** (`game::component`) — 幽灵单位预备数据（角色ID / 职业类型 / 范围 / cost），`EntityFactory::createUnitPrep()` 创建，远程类型额外带 `ShowRangeTag`
- **PlaceUnitSystem** (`game::system`) — 放置单位系统：`mouse_left/right` 与 `PrepUnitEvent` / `RemovePlayerUnitEvent` 的注册与处理集中于此
  - `checkTargetPlace` — 按放置点中心（左上角 + size×scale/2）距离平方 < `PLACE_RADIUS²` 判定可放
  - 落子动作序列：建单位 → 占用放置点 → 扣 cost → 幽灵死亡 → 移除肖像 → 图层修正 → 音效
  - `onRemoveUnitEvent` — 标记死亡 + 解除对应放置点占用（暂停清除也走此事件）
- **RenderRangeSystem** (`game::system`) — 遍历 `ShowRangeTag + UnitPrepComponent`，用 `drawFilledCircle` 画半透明攻击范围圆（`RANGE_COLOR`）
- **渲染变色** (`engine::render`) — `RenderComponent::mColor` + `drawSprite` 颜色参数（`SDL_SetTextureColorModFloat` 调制），幽灵绿/红即时切换；`drawFilledCircle` 复用 `UI/circle.png`
- **击杀侧通关修复** (`combat_resolve_system.cpp:79`) — `createTestEnemy` 补上 `GameStats.mEnemyCount`（含 ctx 实例），全歼敌人触发 `LevelClearDelayedEvent`

## 战斗系统

实现了基于 ECS 标签和冷却计时的自动战斗循环。

### 战斗流程

```
TimerSystem (冷却计时)
    → 冷却结束 → 添加 AttackReadyTag
SetTargetSystem (目标锁定)
    → 玩家(近战/远程) → 范围内最近的敌人
    → 远程敌人 → 范围内最近的玩家
    → 治疗者 → 范围内血量百分比最低的受伤盟友
    → 目标超出范围/死亡 → 清除目标
OrientationSystem (朝向)
    → 有目标 → 面朝目标
    → 被阻挡 → 面朝阻挡者
    → 移动中 → 面朝移动方向
AttackStarterSystem (攻击触发)
    → 筛选 AttackReadyTag 实体
    → 触发 PlayAnimationEvent
    → 添加 ActionLockTag (锁定行动)
    → 移除 AttackReadyTag (重置冷却)
AnimationSystem (帧推进 → 帧事件)
    → 推进到有 mEvents 的帧时发出 AnimationEvent
AnimationEventSystem (动画帧事件处理)
    → 玩家命中 → 对目标发出 AttackEvent 或 HealEvent，附带 PlaySoundEvent
    → 敌人命中 → 对阻挡者发出 AttackEvent
    → 角色在"emit"帧 → 发出 EmitProjectileEvent
ProjectileSystem (投射物飞行)
    → 响应 EmitProjectileEvent，根据 projectile_data.json 蓝图创建投射物实体
    → 每帧更新位置：水平线性插值 + 垂直正弦弧线（mArcHeight 控制弧度高度）
    → 到达目标位置（mTotalFlightTime 耗尽）→ 发出 AttackEvent + PlaySoundEvent
    → 投射物标记 DeadTag，下一帧清理
CombatResolveSystem (战斗结算)
    → AttackEvent: damage = atk - def (最小 10% atk)，扣血 → 死亡 DeadTag 或受伤 InjuredTag
    → 敌人死亡 → 减少阻挡者 BlockerComponent.mCurrentCount + 发出 EnemyDeadEffectEvent
    → HealEvent: 回血 → 满血移除 InjuredTag
EffectSystem (特效)
    → 监听 EnemyDeadEffectEvent
    → createEnemyDeadEffect：复用敌人蓝图的 "damage" 动画创建一次性特效实体
AudioSystem (音效播放)
    → 监听 PlaySoundEvent，通过 AudioPlayer 播放
AnimationStateSystem (动画状态恢复)
    → 监听 AnimationFinishedEvent
    → 被阻挡敌人 → 恢复 idle 循环动画
    → 未被阻挡敌人 → 恢复 walk 循环动画
    → 玩家 → 恢复 idle 循环动画
    → 一次性特效实体 (OneShotRemoveTag) → 标记 DeadTag，交由 RemoveDeadSystem 清理
    → 移除 ActionLockTag (解除行动锁定)
HealthBarSystem (血量条渲染)
    → 遍历 HasHealthBarTag + InjuredTag 的受伤单位
    → 血量百分比 >70% 绿色 / >30% 橙色 / 其余红色
```

### 游戏标签（`game::defs`）

ECS 空标签（tag），用于标记实体状态，配合 view 的 `exclude` 做筛选：

| 标签             | 文件     | 说明                                                  |
| ---------------- | -------- | ----------------------------------------------------- |
| DeadTag          | `tags.h` | 标记死亡实体，下一帧由 RemoveDeadSystem 清理          |
| AttackReadyTag   | `tags.h` | 攻击冷却结束，可发起攻击                              |
| ActionLockTag    | `tags.h` | 行动锁定（播放攻击动画时禁止移动）                    |
| InjuredTag       | `tags.h` | 生命值未满，供治疗者检测                              |
| FaceLeftTag      | `tags.h` | 默认朝左的角色类型（朝向右时翻转精灵）                |
| HealerTag        | `tags.h` | 治疗单位类型                                          |
| MeleeUnitTag     | `tags.h` | 近战单位类型                                          |
| RangedUnitTag    | `tags.h` | 远程单位类型                                          |
| OneShotRemoveTag | `tags.h` | 一次性动画实体（死亡特效），播完标记 DeadTag 自动移除 |
| HasHealthBarTag  | `tags.h` | 需要显示血量条的实体（玩家/敌人单位）                 |
| MeleePlaceTag    | `tags.h` | 近战放置区域（地图放置点）                            |
| RangedPlaceTag   | `tags.h` | 远程放置区域（地图放置点）                            |
| ShowRangeTag     | `tags.h` | 预备远程单位：显示攻击范围圆                          |

### 游戏层常量（`game::defs`）

| 常量                | 文件          | 说明                                                         |
| ------------------- | ------------- | ------------------------------------------------------------ |
| BLOCK_RADIUS        | `constants.h` | 阻挡检测半径（40.0），小于此距离视为被阻挡                   |
| UNIT_RADIUS         | `constants.h` | 角色自身半径（20.0），用于计算攻击范围（射程 + UNIT_RADIUS） |
| HEALTH_BAR_SIZE     | `constants.h` | 血量条大小（48.0 × 8.0）                                     |
| HEALTH_BAR_OFFSET_Y | `constants.h` | 血量条竖直方向偏移（8.0，水平方向居中）                      |
| PLACE_RADIUS        | `constants.h` | 放置吸附半径（40.0），鼠标靠近放置点中心小于此距离即可放置   |
| RANGE_COLOR         | `constants.h` | 攻击范围圆颜色（半透明绿 {0,1,0,0.3}）                        |

### 玩家类型枚举（`game::defs::PlayerType`）

| 值      | 说明                               |
| ------- | ---------------------------------- |
| UNKNOWN | 未定义（默认值）                   |
| MELEE   | 近战型，只能放置在近战区域         |
| RANGED  | 远程型，只能放置在远程区域         |
| MIXED   | 混合型（暂不实现，未来可扩展）     |

### 输入绑定（当前状态）

| 输入       | 快捷键         | 行为                                                           |
| ---------- | -------------- | -------------------------------------------------------------- |
| 鼠标左键   | `mouse_left`   | 放置幽灵单位（由 `PlaceUnitSystem` 注册，落在合法放置点才生效）|
| 鼠标右键   | `mouse_right`  | 取消放置（移除幽灵单位，由 `PlaceUnitSystem` 注册）            |
| A / D 键   | `move_left`/`move_right` | 出击面板左右滚动（由 `UnitsPortraitUI` 处理）      |
| P / Escape | `pause`        | 清除所有已出击的玩家单位（`GameScene` 发 `RemovePlayerUnitEvent`） |

### 动画帧事件

动画数据结构（`Animation`）中的 `mEvents` 映射表将帧索引映射到事件名称 ID：

- 在 JSON 蓝图中配置，如 `"events": { "0": "hit" }` 表示第 0 帧触发 "hit" 事件
- `AnimationSystem` 推进到有事件标记的帧时发出 `AnimationEvent`
- `AnimationEventSystem` 接收后处理为攻击/治疗/音效等具体逻辑

## 资源配置与数据定义

建立了标准化的游戏数据配置体系：

- 玩家数据 (`player_data.json`) — 角色属性与初始状态
- 敌人数据 (`enemy_data.json`) — 敌人类型与参数
- 技能数据 (`skill_data.json`) — 技能效果与冷却
- 弹道数据 (`projectile_data.json`) — 投射物属性
- 关卡配置 (`level_config.json`) — 关卡参数
- 特效数据 (`effect_data.json`) — 视觉效果
- UI 配置 (`ui_config.json`) — 界面布局
- 资源映射 (`resource_mapping.json`) — 资源路径映射表
- 存档系统 (`assets/save/`) — JSON 格式存档

## 事件系统

基于 `entt::dispatcher` 的事件分发机制，支持场景切换和游戏逻辑通信。

### 引擎事件（`engine::utils`）

| 事件                   | 说明                                         |
| ---------------------- | -------------------------------------------- |
| QuitEvent              | 退出游戏                                     |
| PushSceneEvent         | 压入新场景到场景栈                           |
| PopSceneEvent          | 弹出当前场景                                 |
| ReplaceSceneEvent      | 替换当前场景                                 |
| PlayAnimationEvent     | 请求播放动画（实体 + 动画名 + 循环标志）     |
| AnimationFinishedEvent | 动画播放完毕（供 AnimationStateSystem 使用） |
| AnimationEvent         | 动画帧事件（在特定帧触发，如 hit）           |
| PlaySoundEvent         | 播放音效（可指定目标实体或全局播放）         |

### 游戏事件（`game::defs`）

| 事件                      | 说明                                            |
| ------------------------- | ----------------------------------------------- |
| EnemyArriveHomeEvent      | 敌人到达基地                                    |
| AttackEvent               | 攻击命中（攻击者 + 目标 + 原始伤害）            |
| HealEvent                 | 治疗命中（治疗者 + 目标 + 治疗量）              |
| EmitProjectileEvent       | 发射投射物（投射物ID + 目标 + 起止位置 + 伤害） |
| EnemyDeadEffectEvent      | 敌人死亡特效（敌人ID + 位置 + 翻转标志）        |
| PrepUnitEvent             | 预备出击（角色名ID + 职业ID + 费用）            |
| UIPortraitHoverEnterEvent | 单位肖像悬停进入（角色名ID）                    |
| UIPortraitHoverLeaveEvent | 单位肖像悬停离开                                |
| RemoveUIPortraitEvent     | 移除单位肖像（单位出战后，角色名ID）            |
| RemovePlayerUnitEvent     | 移除玩家单位（撤退/死亡时，实体）               |
| UpgradeUnitEvent          | 单位升级（实体 + 费用）                         |
| RetreatEvent              | 单位撤退（实体 + 返还费用）                     |
| LevelClearEvent           | 关卡通关（延迟计时结束，用于切场景）            |
| LevelClearDelayedEvent    | 关卡通关延迟（进入通关倒计时）                  |
| GameEndEvent              | 游戏结束（是否胜利）                            |

## 引擎基础设施

- **GameApp** (`engine::core`) — 应用生命周期管理，持有 SDL 窗口/渲染器和所有子系统
- **Config** (`engine::core`) — 基于 JSON 的配置系统，自动创建默认配置
- **Context** (`engine::core`) — 依赖注入容器，持有所有引擎模块引用（Renderer、Camera、ResourceManager 等）
- **Time** (`engine::core`) — 高性能时间管理，提供 delta time 和时间缩放功能
- **GameState** (`engine::core`) — 游戏状态枚举（运行/暂停），封装 SDL_Window 和 SDL_Renderer
- **Renderer** (`engine::render`) — SDL3 渲染封装，支持纹理绘制、世界坐标矩形（填充/边框）绘制、混合模式和 alpha 调制
- **Camera** (`engine::render`) — 相机位置、视口管理、世界坐标与屏幕坐标转换
- **TextRenderer** (`engine::render`) — 字体加载与文本渲染（基于 SDL_ttf）
- **ResourceManager** (`engine::resource`) — 纹理/字体/音频的统一资源管理门面类
- **Scene / SceneManager** (`engine::scene`) — 场景栈管理，支持 push/pop/replace 切换
- **UI 系统** (`engine::ui`) — 基于状态的交互式 UI（按钮、标签、面板、图片）
  - 使用 State 模式：Normal → Hover → Pressed 状态切换
  - UIInteractive 基类扩展自 UIElement，支持三态图片和点击回调
- **InputManager** (`engine::input`) — 输入事件处理，通过 entt::dispatcher 分发
- **AudioPlayer** (`engine::audio`) — 音频播放（音效 Mix_Chunk + 音乐 Mix_Music）

## 项目结构

```
src/
├── main.cpp                          #   入口：初始化 spdlog → 创建 GameApp → 注册 GameScene → 运行
├── engine/                           #   引擎层 — 通用 2D 游戏引擎
│   ├── core/                         #   核心（GameApp, Config, Context, Time, GameState）
│   ├── component/                    #   ECS 组件定义（9 个组件）
│   ├── system/                       #   ECS 系统（Render, Movement, Animation, YSort）
│   ├── loader/                       #   关卡加载（LevelLoader, BasicEntityBuilder）
│   ├── render/                       #   渲染（Renderer, Camera, TextRenderer, Image）
│   ├── resource/                     #   资源管理（ResourceManager + Texture/Font/Audio 子管理器）
│   ├── scene/                        #   场景管理（Scene 基类, SceneManager）
│   ├── input/                        #   输入管理（InputManager）
│   ├── audio/                        #   音频播放（AudioPlayer）
│   ├── ui/                           #   UI 系统（Button, Label, Panel, Image + State 模式）
│   │   └── state/                    #   状态模式：Normal / Hover / Pressed
│   └── utils/                        #   工具（Math, Events, Alignment）
└── game/                             #   游戏层 — MonsterWar 游戏逻辑
    ├── component/                    #   游戏组件（Enemy, Stats, ClassName, Player, Blocker, Target, Projectile）
    ├── data/                         #   数据结构（WaypointNode, EntityBlueprint, SessionData）
    ├── defs/                         #   标签与事件定义 + 常量（Tags, Events, Constants）
    ├── factory/                      #   工厂（BlueprintManager, EntityFactory）
    ├── loader/                       #   关卡扩展构建器（EntityBuilderMW）
    ├── scene/                        #   游戏场景（GameScene — 主游戏场景）
    │   └── game_scene.cpp/h
    └── system/                       #   游戏系统（战斗、寻路、阻挡等）
        ├── follow_path_system.cpp/h
        ├── block_system.cpp/h
        ├── remove_dead_system.cpp/h
        ├── timer_system.cpp/h              # 攻击冷却计时
        ├── set_target_system.cpp/h         # 自动锁定攻击目标
        ├── attack_starter_system.cpp/h     # 触发攻击动画
        ├── orientation_system.cpp/h        # 精灵朝向控制
        ├── animation_state_system.cpp/h    # 动画状态恢复
        ├── animation_event_system.cpp/h    # 动画帧事件
        ├── combat_resolve_system.cpp/h     # 伤害/治疗计算与结算
        ├── projectile_system.cpp/h         # 投射物飞行与命中
        ├── effect_system.cpp/h             # 特效创建（死亡特效）
        └── health_bar_system.cpp/h         # 血量条渲染
```

```
assets/
├── config.json                       #   全局配置
├── data/                             #   游戏数据（JSON 配置）
│   ├── enemy_data.json               #   敌人蓝图数据
│   ├── player_data.json              #   玩家数据
│   ├── skill_data.json               #   技能数据
│   ├── default_session_data.json     #   默认会话数据
│   └── ...
├── maps/                             #   Tiled 地图文件
│   ├── level1.tmj / level2.tmj       #   关卡地图
│   ├── title.tmj                     #   标题画面
│   └── tileset/                      #   瓦片集（.tsj）
├── textures/                         #   纹理资源
│   └── Enemy/                        #   敌人精灵图
└── save/                             #   存档文件（SLOT_x.json）
```

## 构建与运行

依赖库自动管理（预编译 > 系统库 > 本地 `external/` > 在线 FetchContent），首次构建会自动下载依赖。

```powershell
# 配置 — Ninja 生成器，并行编译速度快
cmake -S . -B build -G Ninja

# 构建
cmake --build build --config Debug

# 运行
./build/Debug/MonsterWar-Windows.exe

# 清理重建
cmake --build build --config Debug --clean-first
```

> 也可使用 VS Code CMake Tools 插件进行图形化构建（需要选择 Ninja 生成器）。

## 技术栈

| 类别     | 技术          | 版本   | 说明                         |
| -------- | ------------- | ------ | ---------------------------- |
| 渲染     | SDL3          | 3.2.x  | 底层图形窗口与硬件加速渲染   |
| 图像加载 | SDL3_image    | 3.2.x  | 支持 PNG/JPG 等格式加载      |
| 音频     | SDL3_mixer    | 开发版 | OGG/MP3 音效与音乐播放       |
| 字体     | SDL3_ttf      | 3.2.x  | TrueType 字体渲染            |
| ECS      | EnTT          | 3.15   | 轻量级实体组件系统           |
| JSON     | nlohmann/json | 3.12   | JSON 解析与序列化            |
| 数学     | GLM           | 1.0.1  | OpenGL 风格数学库（vec2 等） |
| 日志     | spdlog        | 1.15   | 高性能日志系统               |
| UI调试   | ImGui         | 最新   | 调试用即时模式 GUI           |
| 构建     | CMake         | 3.10+  | 跨平台构建系统               |
| 编译     | Ninja / MSVC  | —      | 并行构建（Ninja 推荐）       |
| 标准     | C++20         | —      | 使用 C++20 标准              |
