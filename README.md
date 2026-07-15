# MonsterWar

基于 SDL3 + EnTT ECS 的 2D 游戏引擎/框架。

## 项目概述

MonsterWar 是一个正在开发中的 2D 游戏，采用引擎/游戏分离的架构设计。引擎层（`engine`）提供通用的游戏基础设施，游戏层（`game`）构建具体的游戏逻辑。

### 核心架构

| 层级                       | 说明                                       |
| -------------------------- | ------------------------------------------ |
| **引擎层** (`src/engine/`) | ECS 框架、渲染、资源管理、UI、音频、输入等 |
| **游戏层** (`src/game/`)   | 游戏场景、游戏逻辑                         |

### ECS 组件系统（EnTT）

实体通过组合以下组件来定义行为：

| 组件               | 文件                    | 说明                        |
| ------------------ | ----------------------- | --------------------------- |
| TransformComponent | `transform_component.h` | 位置、缩放、旋转            |
| SpriteComponent    | `sprite_component.h`    | 精灵纹理与源矩形            |
| RenderComponent    | `render_component.h`    | 渲染图层与深度（Y-Sort 用） |
| AnimationComponent | `animation_component.h` | 帧动画                      |
| VelocityComponent  | `velocity_component.h`  | 移动速度                    |
| ParallaxComponent  | `parallax_component.h`  | 视差滚动                    |
| NameComponent      | `name_component.h`      | 实体名称                    |
| AudioComponent     | `audio_component.h`     | 音频                        |
| TileLayerComponent | `tilelayer_component.h` | 瓦片层集合                  |

### 1. 关卡加载系统（Builder 模式重构）

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
- 删除了旧的 `scene::LevelLoader`（逻辑耦合严重）

### 2. Y-Sort 渲染排序系统

实现了基于 y 坐标的渲染排序，保证角色在屏幕上的正确遮挡关系。

- **YSortSystem** — 每帧更新 `RenderComponent.mDepth` 为实体的 y 坐标，然后按 `(layer, depth)` 排序
- **RenderComponent** — 存储 `mLayer` 和 `mDepth`，通过 `operator<` 定义排序规则
- **RenderSystem** — 改用 `RenderComponent` 驱动遍历顺序，确保按排序结果渲染

### 3. 资源配置与数据定义

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

### 4. 引擎基础设施

- **Config** — 基于 JSON 的配置系统，自动创建默认配置
- **GameApp** — 应用生命周期管理（初始化 → 更新循环 → 清理）
- **Renderer** — SDL3 渲染封装，支持纹理、文本、颜色设置
- **ResourceManager** — 纹理/字体/音频的统一资源管理
- **Scene / SceneManager** — 场景栈管理，支持场景切换
- **UI 系统** — 基于状态的交互式 UI（按钮、标签、面板、图片），支持状态模式（正常/悬浮/按下）
- **InputManager** — 输入事件处理
- **AudioPlayer** — 音频播放

## 项目结构

```
src/
├── main.cpp                          # 入口
├── engine/                           # 引擎层
│   ├── core/                         #   核心（Config, GameApp, Context, Time）
│   ├── component/                    #   ECS 组件定义
│   ├── system/                       #   ECS 系统（Render, Movement, Animation, YSort）
│   ├── loader/                       #   关卡加载（LevelLoader, BasicEntityBuilder）
│   ├── render/                       #   渲染（Renderer, Camera, TextRenderer, Image）
│   ├── resource/                     #   资源管理（Texture, Font, Audio）
│   ├── scene/                        #   场景管理（Scene, SceneManager）
│   ├── input/                        #   输入管理
│   ├── audio/                        #   音频播放
│   ├── ui/                           #   UI 系统
│   └── utils/                        #   工具（Math, Events, Alignment）
└── game/
    └── scene/                        # 游戏场景
        └── game_scene.cpp/h          #   GameScene 主场景
```

```
assets/
├── config.json                       # 全局配置
├── data/                             # 游戏数据（JSON 配置）
├── maps/                             # Tiled 地图文件
│   ├── level1.tmj / level2.tmj       #   关卡地图
│   ├── title.tmj                     #   标题画面
│   └── tileset/                      #   瓦片集（.tsj）
└── save/                             # 存档文件
```

## 技术栈

- **渲染**: SDL3
- **ECS**: EnTT
- **JSON**: nlohmann/json
- **数学**: GLM
- **日志**: spdlog
- **音频**: SDL3_audio
- **构建**: CMake
