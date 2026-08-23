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
      → Context → SceneManager (PushSceneEvent → TitleScene)
    → loop:
      → handleEvents (SDL 事件 → InputManager → dispatcher)
      → update(delta)
        → RemoveDeadSystem (清理死亡实体)
        → [暂停分支] 若 isPaused()：仅跑 PlaceUnit(幽灵跟随鼠标) + YSort + Selection + UnitsPortraitUI(肖像滚动) + 场景基础更新，战斗/计时/寻路全冻结并 return
        → TimerSystem (推进攻击冷却 + 技能冷却/持续计时器，冷却结束添加对应就绪标签)
        → GameRuleSystem (cost 恢复 + 通关延迟计时)
        → SetTargetSystem (为无目标角色寻找/刷新攻击目标)
        → OrientationSystem (根据目标/移动方向翻转精灵朝向)
        → FollowPathSystem (敌人路径跟随，触发 EnemyArriveHomeEvent)
        → BlockSystem (检测阻挡，设置速度 0 + 阻挡动画)
        → AttackStarterSystem (冷却完毕的角色触发攻击动画，锁定行动)
        → ProjectileSystem (投射物飞行弧线更新，到达后发出 AttackEvent)
        → MovementSystem (Velocity → Transform)
        → AnimationSystem (帧动画推进，响应 PlayAnimationEvent / AnimationFinishedEvent)
        → PlaceUnitSystem (出击准备/放置：幽灵跟随鼠标、检测放置点、落子出兵)
        → YSortSystem (Y 坐标排序)
        → SelectionSystem (鼠标悬浮单位检测：优先玩家后敌人，写入 hovered_unit ctx)
        → EnemySpawner (按波次生成敌人：波次倒计时 + 生成间隔 + 随机起点)
        → UnitsPortraitUI (肖像遮盖刷新、左右滚动)
      → render()
        → RenderSystem (按 layer+depth 排序渲染)
        → HealthBarSystem (绘制受伤单位的血量条)
        → RenderRangeSystem (预备/已放置远程单位的攻击范围圆)
        → DebugUISystem (ImGui 调试 UI：悬浮/肖像 tooltip + 角色状态(升级/撤退/技能) + 关卡信息 + 设置工具 + 调试工具，最后渲染盖最上面)
    → close()
      → 逆序清理所有子系统
```

> **标题场景（TitleScene）循环更精简**：`Scene::update` → `AnimationSystem` → `MovementSystem` → `YSortSystem`，渲染最后挂 `DebugUISystem::updateTitle(*this)`（标题 Logo / 标题按钮 / 角色信息 / 读档面板）。两个场景切换点：**开始游戏** → 带全部共享数据（蓝图/会话/UI/关卡配置）进 `GameScene`；**返回标题** → 只传 `mContext`（丢弃未保存进度，进度走存档通道）。

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
| SkillComponent        | `skill_component.h`        | 技能（技能ID、显示特效实体、名称/描述、冷却/持续、两个计时器） |
| CostRegenComponent    | `cost_regen_component.h`   | COST 恢复速率（被动技能等额外回 COST） |

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
| TimerSystem          | `timer_system.cpp`           | 推进攻击冷却 + 技能冷却/持续计时器，冷却结束添加对应就绪标签、发技能事件          |
| SetTargetSystem      | `set_target_system.cpp`      | 为无目标角色寻找目标（玩家找敌人、远程敌人找玩家、治疗者找低血量盟友）            |
| AttackStarterSystem  | `attack_starter_system.cpp`  | AttackReadyTag 实体触发攻击动画，添加 ActionLockTag                               |
| OrientationSystem    | `orientation_system.cpp`     | 根据目标位置/阻挡者/移动方向翻转精灵朝向                                          |
| AnimationStateSystem | `animation_state_system.cpp` | 监听 AnimationFinishedEvent，恢复循环动画（idle/walk）                            |
| AnimationEventSystem | `animation_event_system.cpp` | 监听 AnimationEvent（动画帧事件），发出 AttackEvent / HealEvent / PlaySoundEvent  |
| CombatResolveSystem  | `combat_resolve_system.cpp`  | 监听 AttackEvent / HealEvent，计算伤害/治疗量，处理死亡和阻挡计数                 |
| ProjectileSystem     | `projectile_system.cpp`      | 响应 EmitProjectileEvent 创建投射物实体，更新飞行弧线轨迹，到达后发出 AttackEvent |
| EffectSystem         | `effect_system.cpp`          | 监听 EnemyDeadEffectEvent / EffectEvent，通过实体工厂创建死亡/通用特效            |
| HealthBarSystem      | `health_bar_system.cpp`      | 渲染受伤单位的血量条（按血量百分比变色）                                          |
| GameRuleSystem       | `game_rule_system.cpp`       | 游戏规则：cost 恢复、敌人到达基地扣血/胜负判定、单位升级/撤退、通关延迟切换场景     |
| PlaceUnitSystem      | `place_unit_system.cpp`      | 出击准备/放置：幽灵跟随鼠标、检测放置点、落子出兵、扣费、占用放置点、取消          |
| RenderRangeSystem    | `render_range_system.cpp`    | 渲染预备/已放置远程单位的攻击范围圆（半透明绿色，`ShowRangeTag`）                 |
| DebugUISystem        | `debug_ui_system.cpp`        | 调试 UI：ImGui 每帧逻辑+渲染；`update()`（战斗分支：悬浮/肖像 tooltip + 角色状态[升级U/撤退R/技能] + 关卡信息 + 设置工具[暂停P/重开/倍速/音量] + 调试工具[加钱/通关] + 存档面板）+ `updateTitle(TitleScene&)`（标题分支：Logo + 4 按钮 + 角色信息 + 读档面板） |
| SelectionSystem      | `selection_system.cpp`       | 选择单位：每帧悬浮检测（写 `hovered_unit` ctx）+ 左键选中玩家单位 + 右键清除选中 |
| SkillSystem          | `skill_system.cpp`          | 技能系统：事件驱动三态流转（就绪/激活/持续结束），显示标识增删、Buff 乘除、被动清理 |

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

- **BlueprintManager** (`game::factory`) — 从 JSON 加载敌人/玩家/投射物/特效/技能蓝图数据并解析为结构化蓝图
  - 支持子蓝图分别解析：Stats、Sprite、Animation（map）、Sound、Enemy、Player、DisplayInfo、单个 Animation、Buff
  - 提供 `getEnemyClassBlueprint()` / `getPlayerClassBlueprint()` / `getProjectileBlueprint()` / `getEffectBlueprint()` / `getSkillBlueprint()` 按 ID 查询蓝图
- **EntityFactory** (`game::factory`) — 根据蓝图数据创建 ECS 实体并组装组件
  - `createEnemyUnit()` — 按蓝图自动添加 Transform、Sprite、Animation、Stats、Enemy 等组件
  - `createPlayerUnit()` — 按蓝图创建玩家单位，添加 Player、Blocker、Skill 等组件
  - `createEnemyDeadEffect()` — 根据敌人蓝图创建死亡特效实体（复用 "damage" 动画，播完自动移除）
  - `createEffect()` — 根据特效蓝图创建通用特效实体（Transform + Sprite + 单动画 + 上层渲染 + 播完移除）
  - `createSkillDisplay()` — 创建技能显示标识实体（循环动画 + `MAIN_LAYER+20`，不加移除标签，由 SkillSystem 打 `DeadTag` 回收）
  - `addSkillComponent()` — 给玩家单位挂技能组件（初始冷却 = 冷却时间一半；被动技能直接打 `PassiveSkillTag` + `SkillReadyTag` 落子即放）
  - `addOneAnimationComponent()` — 创建只含单个动画的组件（`loop=false` 用于特效 / `loop=true` 用于技能标识），用于特效实体
  - 提供独立的 `addXxxComponent()` 方法供子类扩展
- **蓝图数据结构** (`entity_blueprint.h`) — 定义了一系列子蓝图结构体
  - `EnemyClassBlueprint` — 聚合所有子蓝图，作为完整敌人类型定义
  - `PlayerClassBlueprint` — 玩家职业蓝图，包含 `PlayerBlueprint`（类型、技能、阻挡、消耗）
  - `PlayerBlueprint` 通过 `PlayerType` 枚举（MELEE / RANGED / MIXED）区分单位类型
  - `ProjectileBlueprint` — 投射物蓝图（弧线高度、飞行时间、精灵、音效）
  - `EffectBlueprint` — 特效蓝图（精灵 + 单个动画），由通用 `EffectEvent` 触发
  - `BuffBlueprint` — 增益蓝图（HP/ATK/DEF/射程/攻速倍率 + COST 恢复），技能激活时给角色加 Buff
  - `SkillBlueprint` — 技能蓝图（名称/描述/是否被动/冷却/持续 + Buff），数据驱动技能数值
  - `PlayerClassBlueprint` / `EnemyClassBlueprint` 包含 `mProjectileId` 字段，关联远程单位的投射物类型
- **玩家单位组件**：`PlayerComponent`（出击消耗）、`BlockerComponent`（阻挡计数）、`BlockedByComponent`（被阻挡引用）

## 会话数据系统（SessionData）

实现了跨场景的游戏进度持久化，把"当前关卡、积分、通关状态、玩家角色池"从一场战斗的临时状态中独立出来。

- **SessionData** (`game::data`) — 会话数据类，持有游戏进度，用 `shared_ptr` 供多个场景共享
  - 关卡信息：`mLevelNumber`（当前关卡）/ `mPoint`（积分）/ `mLevelClear`（是否通关）
  - 角色池：`mUnitMap`（角色名哈希 → UnitData）+ `mUnitDataList`（`vector<UnitData*>`，与 map 平行的可排序迭代列表，addUnit/removeUnit/clearUnits/clear 四路同步维护）
  - `mapUnitDataList()` — 用 `mUnitMap` 重建 `mUnitDataList`（load 后调用）
  - `loadDefaultData()` — 从 `assets/data/default_session_data.json` 加载默认进度
  - `loadFromFile(path)` — 从存档文件加载（内部别名到 `loadDefaultData(path)`，解析失败不破坏旧数据）
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
- **RenderRangeSystem** (`game::system`) — 遍历 `ShowRangeTag + UnitPrepComponent`（预备幽灵）与 `ShowRangeTag + Transform + StatsComponent`（已放置单位），用 `drawFilledCircle` 画半透明攻击范围圆（`RANGE_COLOR`）
- **渲染变色** (`engine::render`) — `RenderComponent::mColor` + `drawSprite` 颜色参数（`SDL_SetTextureColorModFloat` 调制），幽灵绿/红即时切换；`drawFilledCircle` 复用 `UI/circle.png`
- **击杀侧通关修复** (`combat_resolve_system.cpp:79`) — `createTestEnemy` 补上 `GameStats.mEnemyCount`（含 ctx 实例），全歼敌人触发 `LevelClearDelayedEvent`

## 敌人生成与关卡配置系统

实现了数据驱动的波次刷怪：敌人不再是测试代码硬编码，而是从 `level_config.json` 按波次、间隔、敌人组成、随机起点生成。核心是把"内容"（关卡/波次/敌人）搬进数据文件，运行时用 `LevelConfig` 解析、`EnemySpawner` 驱动。

```
assets/data/level_config.json  ← 关卡/波次/敌人组成（数据）
        │ LevelConfig::loadFromFile() 解析
        ▼
  LevelConfig → vector<LevelData>（每关：地图、准备时间、波次队列、敌人等级/稀有度、总数）
        │ 游戏启动时取当前关
        ▼
  Waves（关卡波次队列 + 倒计时）── 存入 registry.ctx() ──┐
                                                        ▼
                        EnemySpawner::update(每帧)
    ├─ 波次倒计时走完 → 队首波弹出 → 敌人逐个进当前波队列 → shuffle 打乱
    └─ 生成计时走完 → 弹出队首敌人类型 → 随机起点 → createEnemyUnit()
```

- **关卡数据结构** (`level_data.h`) — `Wave`（波次间隔/生成间隔/敌人类型列表）/ `Waves`（波次队列 + 倒计时）/ `LevelData`（关卡属性 + 波次数据 + 敌人总数）
- **LevelConfig** (`game::data`) — 读取 `level_config.json` → `vector<LevelData>`；敌人名用 `entt::hashed_string` 哈希，与蓝图 `"wolf"_hs` 键对齐；解析时累加 `mTotalEnemyCount`；getters 按关卡号（-1 角标）取数据
- **EnemySpawner** (`game::spawner`) — 双队列模型：`std::queue<Wave>` 管波间节奏（`next_wave_interval`），`std::deque<entt::id_type>` 管波内节奏（`spawn_interval`，双端队列支持 `shuffle` 洗牌）
  - `update()` — 两段：波次倒计时走完开启新一波（灌入敌人 + 打乱）；生成计时走完 `spawnEnemy()`
  - `spawnEnemy()` — `randomInt` 随机起点、等级/稀有度从 LevelConfig 读、`createEnemyUnit(type, pos, start_index, level, rarity)`
- **shuffle** (`engine::utils::math`) — Fisher-Yates 洗牌（`std::shuffle` + thread_local mt19937）
- **ctx 引用语义** (`game_scene.cpp`) — 新存入 `Waves&` / `waypoint_nodes&` / `start_points&` / `level_number&`（就地修改的共享状态用引用语义）；`GameStats` 保持值语义（`initLevelConfig` 先于 `initRegistryContext`，拷贝带正确的 `mEnemyCount`）
- **初始化顺序** (`game_scene.cpp`) — `initSessionData` → `initLevelConfig`（复制当前关波次 + 设置 `mGameStats.mEnemyCount`）→ … → `initRegistryContext` → `initEnemySpawner`；`loadLevel` 地图路径改用 `mLevelConfig->getMapPath(mLevelNumber)`

## ImGui 调试 UI 系统

把 ImGui（调试 GUI）接入引擎的完整三件套：初始化 / 事件 / 渲染，并新增 `DebugUISystem` 每帧绘制调试窗口。用于运行时查看状态、调整参数、快速开发 UI 原型。

```
GameApp::initImGui()         ① 初始化：CreateContext → 配置/缩放/透明度 → 中文字体 → SDL3 后端
InputManager::update()       ② 事件：SDL_PollEvent 里 ImGui_ImplSDL3_ProcessEvent + WantCaptureMouse 拦截
DebugUISystem::update()      ③ 渲染（GameScene）：beginFrame → 战斗窗口 → 存档面板 → endFrame
DebugUISystem::updateTitle()   渲染（TitleScene）：beginFrame → Logo → 标题按钮 → 角色信息 → 读档面板 → endFrame
```

- **initImGui** (`engine::core`) — ImGui 初始化：`CreateContext`、键盘/手柄导航、`StyleColorsDark`、系统 DPI 缩放（`SDL_GetDisplayContentScale`）、窗口/弹窗透明度、中文字体 `VonwaonBitmap-16px.ttf`（`GetGlyphRangesChineseSimplifiedCommon`，失败回退默认字体）、`ImGui_ImplSDL3_InitForSDLRenderer` + `ImGui_ImplSDLRenderer3_Init`；在 `initSceneManager` 后、首个场景创建前调用；`close()` 里 Shutdown 三件套（在 `SDL_DestroyRenderer` 之前）
- **逻辑分辨率开关** (`game_state.h/.cpp`) — `disableLogicalPresentation()` / `enableLogicalPresentation()`：读当前逻辑尺寸后用 `SDL_LOGICAL_PRESENTATION_DISABLED` / `LETTERBOX` 重设。ImGui 对 letterbox 支持差，画 ImGui 前临时关闭（鼠标 1:1 到物理像素）、画完恢复
- **输入接线** (`input_manager.cpp`) — 轮询循环里 `ImGui_ImplSDL3_ProcessEvent`；`processEvent` 开头 `ImGui::GetIO().WantCaptureMouse` 拦截，ImGui 捕获鼠标时游戏不响应（调试 UI 不穿透到放置/战斗操作）
- **DebugUISystem** (`game::system`) — 双分支：`update()`（战斗分支，挂 `GameScene::render` 最后）每帧 `beginFrame` → `renderHoveredUnit` + `renderSelectedUnit` + 角色信息 + 设置/调试工具 + 存档面板 → `endFrame`；`updateTitle(TitleScene&)`（标题分支，挂 `TitleScene::render` 最后）→ `renderTitleLogo` + `renderTitleButtons`（4 按钮直接调 TitleScene 私有回调，`friend`）+ `renderUnitInfoUI`（14 列可排序角色表格 + 升级按钮）+ `renderLoadPanelUI`（3 个 SLOT 读档按钮）→ `endFrame`
- **渲染顺序** — 挂在 `GameScene::render()` 最后（`Scene::render()` 之后），盖在最上层，在 `present()` 前写进 SDL 渲染器

## 单位信息显示与选择系统

把上一课的调试 GUI 从"演示窗口"升级为**运行时单位侦察工具**：鼠标悬浮单位弹 tooltip、左键选中玩家单位弹「角色状态」面板并画攻击范围圆、右键清除选中。核心是新增 `SelectionSystem` 做悬浮/选中判定，并引入 **`emplace_as` 命名上下文**让场景与多个系统共享同一份"当前悬浮/选中"状态。

```
GameScene 持有 mSelectedUnit / mHoveredUnit（entt::entity）
    │ registry.ctx().emplace_as<entt::entity&>("selected_unit"_hs, mSelectedUnit)
    │ registry.ctx().emplace_as<entt::entity&>("hovered_unit"_hs, mHoveredUnit)
    ▼
SelectionSystem::update (每帧，先玩家后敌人 view)
    │ distanceSquared(transform.mPosition, mouse) <= HOVER_RADIUS² → 写 hovered_unit ctx
    ▼
onMouseLeftClick  → 悬浮的是玩家单位 → 清除旧选中 + 设 selected_unit + 加 ShowRangeTag → return true
onMouseRightClick → 清除选中 + return false（穿透，让取消放置照常响应）
    ▼
DebugUISystem 只读两个 ctx：hovered → BeginTooltip 显示属性；selected → 左上角「角色状态」窗口
RenderRangeSystem 第二段 view：ShowRangeTag + Transform + Stats → 画已放置单位的范围圆
```

- **`emplace_as` 命名上下文** (`game_scene.cpp`) — `registry.ctx().emplace_as<T>("key"_hs, value)`：按 FNV-1a 哈希名注册上下文项。这里注册 `entt::entity&` 引用，让 SelectionSystem（写）与 DebugUISystem（读）通过 `ctx().get<entt::entity&>("hovered_unit"_hs)` 访问**同一份** GameScene 成员状态，跨模块解耦
- **SelectionSystem** (`game::system`) — 构造时 `connect` 输入信号 `mouse_left`/`mouse_right`（析构 `disconnect`）；`update()` 每帧用 `distanceSquared` 检测悬浮（玩家优先，`HOVER_RADIUS` 见常量表）；左键选中玩家单位（`clearCurrentSelection` + 加 `ShowRangeTag`，返回 true 阻止其他订阅者）；右键清选（返回 false 穿透给取消放置）
- **DebugUISystem 单位信息** (`debug_ui_system.cpp`) — `renderHoveredUnit` 用 `BeginTooltip` 显示姓名（`try_get<NameComponent>`，只有玩家有）+ 职业/等级/稀有度/生命值/攻击力/防御力/攻击范围/攻击间隔；`renderSelectedUnit` 用 `SetNextWindowPos(ImVec2(10,10))` + `Begin("角色状态", NoTitleBar)` 显示同样信息 + 阻挡数量（`try_get<BlockerComponent>`）
- **悬浮/选中语义分离** — hovered 每帧重算（纯查询预览）；selected 只在左键点击变更（持久选择），`ShowRangeTag` 是其副作用信号，驱动范围圆绘制
- **与 `WantCaptureMouse` 协同** — 鼠标悬停/拖动 ImGui 窗口时事件被拦截，不会误选单位；代价是调试窗口上点不到游戏（预期行为）

## 技能系统

实现了数据驱动的技能三态机：每个玩家职业配一个技能（蓝图数据驱动），技能在**冷却 → 就绪 → 激活 → 持续结束**间流转，激活时给角色加 Buff（攻/防/射程/攻速倍率、被动回 COST），角色头顶用循环特效标识当前状态，通过 ImGui 选中面板 + 快捷键 S 施放。

```
TimerSystem (计时)
    → 技能冷却够了 → SkillReadyTag + enqueue SkillReadyEvent
    → 技能持续够了 → remove SkillActiveTag + enqueue SkillDurationEndEvent
SkillSystem (事件驱动三态流转，订阅 4 事件)
    → SkillReadyEvent      → 显示 skill_ready 标识（循环特效，头顶 +SKILL_DISPLAY_OFFSET）
    → SkillActiveEvent     → 删就绪标识 → 建 skill_active 标识 → SkillActiveTag → addBuff
                            （要求 SkillReadyTag，冷却中收到直接忽略；被动落子即放）
    → SkillDurationEndEvent → 删激活标识 → removeBuff
    → RemovePlayerUnitEvent → 清理显示标识（单位死亡兜底）
addBuff/removeBuff (乘除对称)
    → Stats.mHp/mAtk/mDef/mRange/mAtkInterval *= /= 倍率
    → mCostRegen > 0 → emplace_or_replace / remove CostRegenComponent（GameRuleSystem 已有每帧回 COST 循环）
DebugUISystem 选中面板技能区块
    → BeginDisabled(!SkillReadyTag) + 按钮(技能名) + SetNextItemShortcut(S) → enqueue SkillActiveEvent
    → 激活中：被动 → "被动技能激活中"；否则 "激活中，剩余时间: %.1f 秒"
    → 冷却中：SkillReadyTag → "技能准备就绪"；否则 ProgressBar(cooldownTimer / cooldown)
    → TextWrapped 技能描述
```

- **技能组件** (`skill_component.h`) — `SkillComponent`：技能ID、显示特效实体ID、名称/描述、冷却/持续、`mCooldownTimer`/`mDurationTimer` 两个计时器
- **技能/增益蓝图** (`entity_blueprint.h`) — `SkillBlueprint`（名称/描述/被动/冷却/持续/Buff）+ `BuffBlueprint`（六个倍率字段，`mCostRegen` 是绝对值非倍率），解析见 `BlueprintManager::loadSkillBlueprints()` / `getSkillBlueprint()` / `parseBuff()`
- **挂技能** (`entity_factory.cpp:addSkillComponent`) — 创建时初始冷却 = 冷却时间一半（免等满整个冷却）；被动技能直接打 `PassiveSkillTag` + `SkillReadyTag`
- **被动技能** — 落子即放（`PlaceUnitSystem::onPlaceUnit` 检测 `PassiveSkillTag` → enqueue `SkillActiveEvent`）；永不过期（TimerSystem 持续计时排除 `PassiveSkillTag`，`SkillDurationEndEvent` 永不发出）
- **显示标识** (`entity_factory.cpp:createSkillDisplay`) — 复用第 8 课 `skill_ready`/`skill_active` 特效蓝图，`loop=true` 循环动画、`MAIN_LAYER+20`、**不加** `OneShotRemoveTag`；由 SkillSystem 状态切换时打 `DeadTag` 回收
- **EnTT remove vs erase** — TimerSystem 和 SkillSystem 都 remove 过 `SkillActiveTag`：`registry.remove<T>` 缺失返回 0 不抛（`erase` 才抛），双重移除安全
- **技能数据** (`skill_data.json`) — 盾御(shield)/威能(power_up)/疾速(speed_up) 主动技能 + 休整(rest) 被动回 COST

## 主场景完善

把 GameScene 从"测试向"完善为"可玩向"：引入**暂停系统**、**场景重开/返回/保存事件**、**GameScene 构造器依赖注入**（重开复用同一份共享数据）、**DebugUI 四大窗口**、**升级/撤退按钮**与**盾御守卫动画**。

### 暂停系统：引擎存状态，游戏层执行

- **GameState**（`engine::core`）只存状态枚举（Playing/Paused）与 `isPaused()` / `setState()`，本课前就绪、零改动
- **GameScene::update** 才是暂停的执行者——`mRemoveDeadSystem` 之后加暂停分支：`isPaused()` 时仅跑 `PlaceUnitSystem`（幽灵跟随鼠标）+ `YSortSystem` + `SelectionSystem` + `UnitsPortraitUI`（肖像滚动）+ `Scene::update`，战斗/计时/寻路/刷怪全冻结并 `return`；渲染照常跑（ImGui 设置窗口要能操作）
- `init()` 末尾显式 `setState(Playing)` 进入运行态

### GameScene 构造器依赖注入（DI）

数据从"init 内部懒创建"改为**构造传入 + 空则兜底**，为场景重开铺路：

```cpp
GameScene(Context& context,
    shared_ptr<BlueprintManager> = nullptr,
    shared_ptr<SessionData> = nullptr,
    shared_ptr<UIConfig> = nullptr,
    shared_ptr<LevelConfig> = nullptr);
```

- **场景控制事件**：`RestartEvent`（重新开始当前关卡）/ `BackToTitleEvent`（返回标题）/ `SaveEvent`（保存）——本课新增三个事件，由设置工具按钮发出
- **onRestart**：用同一份共享数据 `requestReplaceScene(make_unique<GameScene>(mContext, mBlueprintManager, mSessionData, mUIConfig, mLevelConfig))` 重开，会话/蓝图/关卡配置复用，仅战斗实体重建
- **共享语义**：蓝图/会话/UI/关卡配置是"跨场景存活"的数据用 shared_ptr；`GameStats` 是"关卡内临时状态"用值语义存 ctx

### DebugUI 四大窗口

| 窗口 | 内容 |
|------|------|
| 悬浮 tooltip（原） | 场上悬浮单位属性（玩家/敌人实体） |
| 肖像 tooltip（新） | 出击面板肖像悬浮 → `getUnitData(name_id)` + 蓝图 `statModify` 重算属性（角色档案，非场上实体） |
| 角色状态 + 升级/撤退（新） | 升级 U：扣 `player.mCost`、COST 不足 `BeginDisabled` 置灰；撤退 R：返还 `cost * 0.5` |
| 关卡信息（新） | 基地血量 / COST / 剩余波次 / 下一波倒计时 / 击杀 / 当前关卡 |
| 设置工具（新） | 暂停/继续（P 键）、重新开始 / 返回标题 / 保存、游戏倍速（0.5/1/2 按钮 + SliderFloat → `Time::setTimeScale`）、音乐/音效音量、显示调试工具开关 |
| 调试工具（新） | COST+10 / COST+100、通关（发 `LevelClearEvent`），由设置工具勾选控制显隐 |

- **Context 增加 Time 引用**（`getTime()`，`game_app.cpp` 传 `*mTime` 实参）——设置工具倍速需要访问 Time
- **SessionData 增加 `getUnitData(name_id)`**——肖像 tooltip 按角色名哈希查数据（`operator[]` 不存在则默认构造）
- **P 键语义变更**：原"pause"输入（清空所有玩家单位）移除，P 改由设置工具 `SetNextItemShortcut(ImGuiKey_P)` 暂停/继续；单位回收靠撤退按钮 R

### 盾御守卫动画

盾御（shield）激活时角色摆守卫姿态，三处配合闭环：

- `skill_system.cpp` — 盾御激活时（且未锁动作）enqueue `guard`；持续结束时（且未锁动作）enqueue `idle`
- `attack_starter_system.cpp:updatePlayer` — 玩家攻击时 `emplace_or_replace<ActionLockTag>`（守卫/攻击动画期间锁定行动，不被硬直打断）
- `animation_state_system.cpp` — 玩家动画结束时按 `SkillComponent.mSkillId == "shield" && SkillActiveTag` 特判回 `guard`，否则回 `idle`，随后 `remove<ActionLockTag>` 解除硬直

## 标题场景

实现**标题场景（TitleScene）**：开始游戏 / 确认角色 / 载入游戏 / 退出游戏四流程，`main.cpp` 初始场景由 GameScene 改为 TitleScene，应用启动先进标题画面。

- **TitleScene**（`game::scene`）— 与 GameScene 对称的 5 参数 DI 构造器 `(Context&, shared_ptr<BlueprintManager/SessionData/UIConfig/LevelConfig> = nullptr)`；`friend class DebugUISystem`（ImGui 系统直接访问私有成员与回调）；`init()` 顺序 `initSessionData → initLevelConfig → initBlueprintManager → initUIConfig → loadTitleLevel`（title.tmj 用默认 BasicEntityBuilder）`→ initSystems → initRegistryContext → initUI`，末尾 `setState(Title)` + `setTimeScale(1.0f)`（从战斗倍速回来也重置）
- **四按钮回调** — `onStartGameClick`（`isLevelClear()` 则 `setLevelClear(false)+addOneLevel()` 进下一关 → 带全部共享数据 `requestReplaceScene(GameScene)`）/ `onConfirmRoleClick`（toggle 角色信息面板）/ `onLoadGameClick`（toggle 读档面板）/ `onQuitClick`（`quit()` 关窗口）
- **场景切换数据语义** — 标题 → 游戏传全部 4 份共享数据（把进度带进战斗）；游戏 → 标题只传 `mContext`（丢弃未保存进度——本课起有存档系统，进度走 save/load 通道，这是刻意设计）
- **可排序角色表格**（`renderUnitTable`）— 14 列（姓名/职业/类型/等级/稀有度/COST/生命值/攻击力/防御力/攻击范围/攻击间隔/阻挡数量/技能/升级），`ImGuiTableFlags_Sortable` + `SpecsDirty` 脏标识模式（排序后必须置 false 防重复排序）驱动 `std::stable_sort` 于 `mUnitDataList`；数值列用 `statModify` 重算、`static_cast<int>(round)` 防 float 相等比较问题；升级按钮 `PushID(unit->mName.c_str())` 保证按钮 ID 唯一、COST 不足 `BeginDisabled` 置灰
- **存档/读档面板** — 各 3 个 `SLOT 1/2/3` 按钮 → `loadFromFile("assets/save/SLOT_x.json")` / `saveToFile(...)`；底部按 `isLevelClear()` 显示「下一关/当前关卡」；GameScene 的存档面板经 `ctx().emplace_as<bool&>("show_save_panel"_hs, mShowSavePanel)` 暴露给 `update()` 分支（ECS/registry 惯例），TitleScene 两个标志因 `friend` 直接读私有成员、不走 ctx

详见 `docs/notes/011-title-scene.md`。

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
    → HealEvent: 回血 → 满血移除 InjuredTag + 发 EffectEvent 治疗特效
EffectSystem (特效)
    → 监听 EnemyDeadEffectEvent：createEnemyDeadEffect（复用敌人蓝图的 "damage" 动画）
    → 监听 EffectEvent：createEffect（按 effect_data.json 特效蓝图创建通用特效实体）
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
| OneShotRemoveTag | `tags.h` | 一次性动画实体（特效/死亡特效），播完标记 DeadTag 自动移除 |
| HasHealthBarTag  | `tags.h` | 需要显示血量条的实体（玩家/敌人单位）                 |
| MeleePlaceTag    | `tags.h` | 近战放置区域（地图放置点）                            |
| RangedPlaceTag   | `tags.h` | 远程放置区域（地图放置点）                            |
| ShowRangeTag     | `tags.h` | 预备远程单位：显示攻击范围圆                          |
| SkillReadyTag    | `tags.h` | 技能冷却结束，可以施放                                 |
| SkillActiveTag   | `tags.h` | 技能激活中（施放后到持续时间结束）                     |
| PassiveSkillTag  | `tags.h` | 被动技能（落子即放、永不过期，计时器排除）            |

### 游戏层常量（`game::defs`）

| 常量                | 文件          | 说明                                                         |
| ------------------- | ------------- | ------------------------------------------------------------ |
| BLOCK_RADIUS        | `constants.h` | 阻挡检测半径（40.0），小于此距离视为被阻挡                   |
| UNIT_RADIUS         | `constants.h` | 角色自身半径（20.0），用于计算攻击范围（射程 + UNIT_RADIUS） |
| HEALTH_BAR_SIZE     | `constants.h` | 血量条大小（48.0 × 8.0）                                     |
| HEALTH_BAR_OFFSET_Y | `constants.h` | 血量条竖直方向偏移（8.0，水平方向居中）                      |
| PLACE_RADIUS        | `constants.h` | 放置吸附半径（40.0），鼠标靠近放置点中心小于此距离即可放置   |
| HOVER_RADIUS        | `constants.h` | 鼠标悬浮检测半径（30.0），鼠标与单位中心距离小于此值视为悬浮 |
| RANGE_COLOR         | `constants.h` | 攻击范围圆颜色（半透明绿 {0,1,0,0.3}）                        |
| SKILL_DISPLAY_OFFSET | `constants.h` | 技能显示标识相对角色的偏移（{0,-96}，角色头顶上方）           |

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
| 鼠标左键   | `mouse_left`   | 放置幽灵单位 / 选中悬浮的玩家单位（`PlaceUnitSystem` + `SelectionSystem`）|
| 鼠标右键   | `mouse_right`  | 取消放置（移除幽灵单位）；清除单位选中（`SelectionSystem`，右键穿透）      |
| A / D 键   | `move_left`/`move_right` | 出击面板左右滚动（由 `UnitsPortraitUI` 处理）      |
| P 键       | `ImGui`        | 暂停 / 继续游戏（`DebugUISystem` 设置工具 `SetNextItemShortcut`） |
| U 键       | `ImGui`        | 升级选中单位（`DebugUISystem` 选中面板 `SetNextItemShortcut`）     |
| R 键       | `ImGui`        | 撤退选中单位，返还 50% COST（`DebugUISystem` 选中面板 `SetNextItemShortcut`） |
| S 键       | `ImGui`        | 施放选中单位的技能（`DebugUISystem` 选中面板 `SetNextItemShortcut`） |

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
| EffectEvent               | (通用)特效（特效ID + 位置 + 翻转标志）          |
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
| RestartEvent              | 重新开始当前关卡                                |
| BackToTitleEvent          | 返回标题场景                                    |
| SaveEvent                 | 保存游戏                                        |
| SkillReadyEvent           | 技能冷却结束（实体）                            |
| SkillActiveEvent          | 技能激活/施放（实体，玩家按 S 或被动落子触发）  |
| SkillDurationEndEvent     | 技能持续时间结束（实体）                        |

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
├── main.cpp                          #   入口：初始化 spdlog → 创建 GameApp → 注册 TitleScene → 运行
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
    ├── component/                    #   游戏组件（Enemy, Stats, ClassName, Player, Blocker, Target, Projectile, UnitPrep, Skill, CostRegen）
    ├── data/                         #   数据结构（WaypointNode, EntityBlueprint, SessionData, LevelConfig, LevelData）
    ├── defs/                         #   标签与事件定义 + 常量（Tags, Events, Constants）
    ├── factory/                      #   工厂（BlueprintManager, EntityFactory）
    ├── loader/                       #   关卡扩展构建器（EntityBuilderMW）
    ├── scene/                        #   游戏场景（GameScene — 主游戏场景；TitleScene — 标题场景）
    │   ├── game_scene.cpp/h
    │   └── title_scene.cpp/h
    ├── spawner/                      #   敌人生成器（EnemySpawner — 按波次生成）
    └── system/                       #   游戏系统（战斗、寻路、阻挡、放置等）
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
        ├── effect_system.cpp/h             # 特效创建（通用特效 + 死亡特效）
        ├── health_bar_system.cpp/h         # 血量条渲染
        ├── game_rule_system.cpp/h          # 游戏规则（cost 恢复、基地血量/胜负判定、升级/撤退、通关延迟切换）
        ├── debug_ui_system.cpp/h           # 调试 UI（ImGui 每帧逻辑+渲染，update 战斗分支 + updateTitle 标题分支）
        ├── selection_system.cpp/h          # 选择单位系统（悬浮检测 + 左键选中 + 右键清除）
        └── skill_system.cpp/h              # 技能系统（三态流转 + 显示标识 + Buff 管理）
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
