# MonsterWar

基于 SDL3 + EnTT ECS 的 2D 游戏引擎/框架。

## 项目概述

MonsterWar 是一个正在开发中的 2D 游戏，采用**引擎/游戏分离**的架构设计。引擎层（`engine/`）提供通用的游戏基础设施，游戏层（`game/`）构建具体的游戏逻辑。

### 核心架构

| 层级                       | 说明                                           |
| -------------------------- | ---------------------------------------------- |
| **引擎层** (`src/engine/`) | ECS 框架、渲染、资源管理、UI、音频、输入、场景 |
| **游戏层** (`src/game/`)   | 游戏场景、敌人 AI、实体工厂、蓝图系统          |

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
        → FollowPathSystem (敌人路径跟随，触发 EnemyArriveHomeEvent)
        → MovementSystem (Velocity → Transform)
        → AnimationSystem (帧动画推进)
        → YSortSystem (Y 坐标排序)
      → render()
        → RenderSystem (按 layer+depth 排序渲染)
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

| 组件               | 文件                     | 说明                               |
| ------------------ | ------------------------ | ---------------------------------- |
| EnemyComponent     | `enemy_component.h`      | 敌人目标路径节点 ID + 移动速度     |
| StatsComponent     | `stats_component.h`      | RPG 属性（HP/ATK/DEF/射程/攻速等） |
| ClassNameComponent | `class_name_component.h` | 职业名称或敌人类型（ID + 名称）    |

## ECS 系统

### 引擎层系统（`engine::system`）

| 系统            | 文件                   | 说明                                                              |
| --------------- | ---------------------- | ----------------------------------------------------------------- |
| MovementSystem  | `movement_system.cpp`  | 遍历 Velocity + Transform 实体，`velocity * dt` 更新位置          |
| AnimationSystem | `animation_system.cpp` | 遍历 Animation + Sprite 实体，推进帧计时器并切换帧                |
| RenderSystem    | `render_system.cpp`    | 遍历 Transform + Sprite + Render 实体，按 (layer, depth) 排序渲染 |
| YSortSystem     | `ysort_system.cpp`     | 遍历 RenderComponent 实体，将 `mDepth` 设为 Y 坐标                |

### 游戏层系统（`game::system`）

| 系统             | 文件                     | 说明                                                  |
| ---------------- | ------------------------ | ----------------------------------------------------- |
| FollowPathSystem | `follow_path_system.cpp` | 敌人沿路径节点移动，到达终点触发 EnemyArriveHomeEvent |
| RemoveDeadSystem | `remove_dead_system.cpp` | 延迟清理标记 DeadTag 的死亡实体                       |

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
- **EntityBuilderMW** (`game::loader`) — MonsterWar 扩展构建器，从 Tiled 对象图层解析路径节点和起点
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

- **BlueprintManager** (`game::factory`) — 从 JSON 加载敌人蓝图数据并解析为结构化蓝图
  - 支持子蓝图分别解析：Stats、Sprite、Animation、Sound、Enemy、DisplayInfo
  - 提供 `getEnemyClassBlueprint()` 接口按 ID 查询蓝图
- **EntityFactory** (`game::factory`) — 根据蓝图数据创建 ECS 实体并组装组件
  - `createEnemyUnit()` — 按蓝图自动添加 Transform、Sprite、Animation、Stats 等组件
  - 提供独立的 `addXxxComponent()` 方法供子类扩展
- **蓝图数据结构** (`entity_blueprint.h`) — 定义了 Stats、Sprite、Animation 等子蓝图结构体
  - `EnemyClassBlueprint` 聚合所有子蓝图，作为完整敌人类型定义
- **GameScene** 整合：`initEntityFactory()` 初始化工厂，`createTestEnemy()` 生成测试敌人

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

| 事件              | 说明               |
| ----------------- | ------------------ |
| QuitEvent         | 退出游戏           |
| PushSceneEvent    | 压入新场景到场景栈 |
| PopSceneEvent     | 弹出当前场景       |
| ReplaceSceneEvent | 替换当前场景       |

### 游戏事件（`game::defs`）

| 事件                 | 说明                         |
| -------------------- | ---------------------------- |
| EnemyArriveHomeEvent | 敌人到达基地，触发扣血等逻辑 |

## 引擎基础设施

- **GameApp** (`engine::core`) — 应用生命周期管理，持有 SDL 窗口/渲染器和所有子系统
- **Config** (`engine::core`) — 基于 JSON 的配置系统，自动创建默认配置
- **Context** (`engine::core`) — 依赖注入容器，持有所有引擎模块引用（Renderer、Camera、ResourceManager 等）
- **Time** (`engine::core`) — 高性能时间管理，提供 delta time 和时间缩放功能
- **GameState** (`engine::core`) — 游戏状态枚举（运行/暂停），封装 SDL_Window 和 SDL_Renderer
- **Renderer** (`engine::render`) — SDL3 渲染封装，支持纹理绘制、混合模式和 alpha 调制
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
├── main.cpp                          # 入口：初始化 spdlog → 创建 GameApp → 注册 GameScene → 运行
├── engine/                           # 引擎层 — 通用 2D 游戏引擎
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
│   │   └── state/                    #     状态模式：Normal / Hover / Pressed
│   └── utils/                        #   工具（Math, Events, Alignment）
└── game/                             # 游戏层 — MonsterWar 游戏逻辑
    ├── component/                    #   游戏组件（Enemy, Stats, ClassName）
    ├── data/                         #   数据结构（WaypointNode, EntityBlueprint）
    ├── defs/                         #   标签与事件定义（Tags, Events）
    ├── factory/                      #   工厂（BlueprintManager, EntityFactory）
    ├── loader/                       #   关卡扩展构建器（EntityBuilderMW）
    ├── scene/                        #   游戏场景（GameScene — 主游戏场景）
    │   └── game_scene.cpp/h
    └── system/                       #   游戏系统（FollowPath, RemoveDead）
        └── follow_path_system.cpp/h
```

```
assets/
├── config.json                       # 全局配置
├── data/                             # 游戏数据（JSON 配置）
│   ├── enemy_data.json               #   敌人蓝图数据
│   ├── player_data.json              #   玩家数据
│   ├── skill_data.json               #   技能数据
│   └── ...
├── maps/                             # Tiled 地图文件
│   ├── level1.tmj / level2.tmj       #   关卡地图
│   ├── title.tmj                     #   标题画面
│   └── tileset/                      #   瓦片集（.tsj）
├── textures/                         # 纹理资源
│   └── Enemy/                        #   敌人精灵图
└── save/                             # 存档文件
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
