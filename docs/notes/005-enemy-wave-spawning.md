# 敌人按波次生成：数据驱动的关卡配置 + 双队列刷怪

## 问题

上一课的敌人是怎么来的？`createTestEnemy()` 在每个起点**一次性**硬编码刷 12 个测试敌人（slime/wolf/goblin/dark_witch 各一）。这不是真正的游戏刷怪：

- **敌人与关卡无关**——每个起点刷同样 4 种敌人，所有关卡一模一样，谈不上难度递进
- **没有节奏**——一波全出，没有"准备期 → 一波一波来 → 波间喘息"的塔防节奏
- **敌人属性写死**——等级/稀有度固定，关卡配置（`enemy_level/enemy_rarity`）根本没用上
- **顺序固定**——敌人类型和出生起点都写死在代码里，毫无随机性

一句话：**刷怪逻辑是测试代码，不是游戏系统。**

## 结论

把"刷什么、什么时候刷、刷在哪"全部搬到**数据文件**里，运行时由两个新模块驱动：

```
assets/data/level_config.json  ← 关卡/波次/敌人组成（数据）
        │ LevelConfig::loadFromFile() 解析
        ▼
  LevelConfig → vector<LevelData>（每关一个：地图、准备时间、波次队列、敌人总数…）
        │ 游戏启动时取当前关
        ▼
  Waves（关卡波次队列 + 倒计时）── 存入 registry.ctx() ──┐
                                                        ▼
                        EnemySpawner::update(每帧)
    ├─ 波次倒计时走完 → 队首波弹出 → 敌人逐个进当前波队列 → shuffle 打乱
    └─ 生成计时走完 → 弹出队首敌人类型 → 随机起点 → createEnemyUnit()
```

`EnemySpawner` 用一个**双队列**模型：`std::queue<Wave>` 存整关的波次，`std::deque<entt::id_type>` 存当前波要出的敌人（双端队列方便洗牌）。每波内部按 `spawn_interval` 间隔逐个生成，起点随机抽。

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| 关卡/波次数据结构 `Wave`/`Waves`/`LevelData` | `src/game/data/level_data.h` |
| 关卡配置类 `LevelConfig` | `src/game/data/level_config.h/.cpp` |
| 关卡配置 JSON | `assets/data/level_config.json` |
| 敌人生成器 `EnemySpawner` | `src/game/spawner/enemy_spawner.h/.cpp` |
| 洗牌算法 `shuffle` | `src/engine/utils/math.h:136` |
| ctx 服务定位器注册新键 | `src/game/scene/game_scene.cpp:initRegistryContext` |
| 关卡配置初始化 / 生成器挂载 | `src/game/scene/game_scene.cpp:initLevelConfig/initEnemySpawner` |

### 2. 数据层三结构体：JSON 怎么落成内存

`level_config.json` 是一个数组，数组里每个对象是一关。`LevelConfig::loadFromFile` 逐关解析出 `LevelData`：

```cpp
struct Wave {                                  // 一波
    float mNextWaveInterval{};                 // 本波结束后，下一波要等多少秒
    float mSpawnInterval{};                    // 本波内，相邻两个敌人的生成间隔（秒）
    std::vector<std::pair<entt::id_type, int>> mEnemyTypes;  // <敌人名哈希, 数量>
};
struct Waves {                                 // 整关的波次集合
    float mNextWaveCountDown{};                // 距离下一波开始的倒计时
    std::queue<Wave> mWaves;                   // 还没开始的波次（先进先出）
};
struct LevelData {                             // 一关
    int mLevelNumber{ 1 };
    int mEnemyLevel{ 1 };                      // 本关敌人等级 → 决定属性
    int mEnemyRarity{ 1 };                     // 本关敌人稀有度
    std::string mName;
    std::string mMapPath;                      // 本关地图（loadLevel 用它替代写死的 level1.tmj）
    float mPrepTime{ 5.0f };                   // 开局准备时间（第一波前）
    int mTotalEnemyCount{ 0 };                 // 所有波次敌人数量总和
    Waves mWavesData;
};
```

三个关键点：

**① `mNextWaveInterval` 的语义。** 它不是"第一波的等待"，而是"**这一波结束后**到下一波的间隔"。所以初始化时第一波的等待来自 `mPrepTime`，之后的波间隔才用每波的 `mNextWaveInterval`。`loadFromFile` 末尾 `level_data.mWavesData.mNextWaveCountDown = level_data.mPrepTime` 就把开局倒计时设成了准备时间。

**② 敌人名用哈希匹配蓝图。** JSON 里写的是字符串 `"slime"`，解析时用 `entt::hashed_string("slime")`（FNV-1a）转成 `entt::id_type`，正好和本地蓝图的 `"slime"_hs` 键对上。这样 JSON 敌人名 → 蓝图的查找是 `O(1)` 且编译期/运行期同源。

**③ `mTotalEnemyCount` 解析时累加。** 每波 `enemy_types` 里 `count.get<int>()` 加进总和。这个总数就是通关判定 `(到达数+击杀数) >= 敌人总数` 的分母，由 `initLevelConfig` 写进 `mGameStats.mEnemyCount`。

### 3. `EnemySpawner` 双队列：波次节奏怎么流出来

`update(delta_time)` 是核心，分两段，每段各守一个队列：

```cpp
void EnemySpawner::update(float delta_time) {
    auto& waves = mRegistry.ctx().get<game::data::Waves&>();

    // ① 关卡波次队列：倒计时走完 → 开启新一波
    if (!waves.mWaves.empty()) {
        waves.mNextWaveCountDown -= delta_time;
        if (waves.mNextWaveCountDown <= 0.0f) {
            auto& wave = waves.mWaves.front();
            waves.mNextWaveCountDown = wave.mNextWaveInterval;  // 重置为"这波之后等多久"
            mSpawnInterval = wave.mSpawnInterval;
            mSpawnTimer = 0.0f;
            for (auto& [class_id, count] : wave.mEnemyTypes)    // 把这一波敌人灌进当前波队列
                for (int i = 0; i < count; ++i)
                    mEnemyTypes.push_back(class_id);
            engine::utils::shuffle(mEnemyTypes.begin(), mEnemyTypes.end());  // 打乱顺序
            waves.mWaves.pop();                                 // 这波处理完，弹出
            spdlog::info("开始新一波敌人生成");
        }
    }

    // ② 当前波队列：生成间隔走完 → 出一个敌人
    if (!mEnemyTypes.empty()) {
        mSpawnTimer += delta_time;
        if (mSpawnTimer >= mSpawnInterval) {
            mSpawnTimer = 0.0f;
            spawnEnemy();
        }
    }
}
```

**为什么是两个队列？** 这是把"波次节奏"和"波内节奏"拆成了两个独立的计时状态机：

- `Waves`（关卡级）管**波之间**：什么时候开下一波，由 `mNextWaveCountDown` + 每波的 `mNextWaveInterval` 决定
- `mEnemyTypes`（波内级）管**波之内**：这一波怎么出，由 `mSpawnTimer` + 每波的 `mSpawnInterval` 决定

如果只用一个队列，波间隔和生成间隔会搅在一起。拆成两级后，新增"某波同时出多个起点"或"某波无限刷"这类需求，只改数据或只在某级动手即可。

**为什么当前波队列用 `std::deque` 而波次用 `std::queue`？** `queue` 是严格的先进先出，正好表达"波次只能按顺序来"；但**当前波内部要洗牌**——`engine::utils::shuffle` 需要随机访问迭代器（`std::shuffle` 要求 RandomAccessIterator），`deque` 支持而 `queue` 不支持。所以一层管顺序、一层管随机，容器的选择跟着**操作需求**走。

### 4. `spawnEnemy`：起点随机 + 属性从配置读

```cpp
void EnemySpawner::spawnEnemy() {
    auto& start_points = mRegistry.ctx().get<std::vector<int>&>();
    auto& waypoint_nodes = mRegistry.ctx().get<std::unordered_map<int, game::data::WaypointNode>&>();
    auto& level_config = mRegistry.ctx().get<std::shared_ptr<game::data::LevelConfig>&>();
    auto& level_number = mRegistry.ctx().get<int&>();

    auto random_index = engine::utils::randomInt(0, static_cast<int>(start_points.size()) - 1);
    auto start_index = start_points[random_index];
    auto position = waypoint_nodes[start_index].mPosition;
    auto level = level_config->getEnemyLevel(level_number);
    auto rarity = level_config->getEnemyRarity(level_number);

    auto enemy_type = mEnemyTypes.front();
    mEnemyTypes.pop_front();
    mEntityFactory.createEnemyUnit(enemy_type, position, start_index, level, rarity);
}
```

两处随机/数据驱动的设计点：

- **起点随机**：`randomInt(0, size-1)` 从 `start_points` 里抽一个。之前 `createTestEnemy` 是"每个起点都刷"，现在是"随机一个起点出"。配合波内洗牌，同一波敌人散布在不同出生点、以随机顺序登场。
- **敌人属性数据化**：`level`/`rarity` 不再写死，从 `level_config->getEnemyLevel/getEnemyRarity(mLevelNumber)` 读。这些值顺着 `EntityFactory::createEnemyUnit(type, pos, start_index, level, rarity)` → `addStatsComponent` 里的 `statModify(base, level, rarity)` 影响实际 HP/ATK——**改 JSON 的 `enemy_level` 就能让整关敌人变强**，零代码改动。

### 5. ctx 服务定位器：值语义 vs 引用语义，一次讲清

`initLevelConfig` 把三样东西放进了 `registry.ctx()`（服务定位器），加上原有的，现在上下文里有两类存法：

```cpp
mRegistry.ctx().emplace<game::data::GameStats>(mGameStats);                       // 值语义（拷贝）
mRegistry.ctx().emplace<std::shared_ptr<...>>(mBlueprintManager / mSessionData / mUIConfig / mLevelConfig);  // 共享语义
mRegistry.ctx().emplace<std::unordered_map<int, WaypointNode>&>(mWaypointNodes);  // 引用语义
mRegistry.ctx().emplace<std::vector<int>&>(mStartPoints);                         // 引用语义
mRegistry.ctx().emplace<game::data::Waves&>(mWaves);                              // 引用语义
mRegistry.ctx().emplace<int&>(mLevelNumber);                                      // 引用语义
```

**为什么新加的这几个用引用语义（`emplace<T&>`）？** 因为 `EnemySpawner` 要**就地修改**它们：

- `Waves&` —— 每帧倒计时递减、波次弹出，都是改成员 `mWaves` 本体
- `waypoint_nodes&` / `start_points&` / `level_number&` —— 生成敌人要读场景成员

如果存的是**拷贝**，生成器改的是 ctx 里那份，`GameScene::update` 里读的还是成员 `mWaves`（没改），两边各说各话。**凡是"写入方"要共享给"读取方"且需要就地改的，用引用语义；只有不可变快照才用值语义。** `shared_ptr` 则是"多个场景共享同一份生命周期"的第三个选项。

**`GameStats` 为什么反而坚持值语义？** 本地在 003 课明确选了值语义（`emplace<GameStats>(mGameStats)` 拷贝）。本课 `initLevelConfig` 在 `initRegistryContext` **之前**执行，`mGameStats.mEnemyCount = 31` 先写进成员，随后 ctx 拷贝时把正确的总数带进去——所以值语义在这里依然正确。而参考实现此课把 `GameStats` 改成了引用语义（`emplace<GameStats&>`），那是它的选择，本地**不改**：保持值语义的论证是——副本是"关卡开始时的快照"，各系统只读不改，拷贝无害且隔离清楚。**这个顺序依赖（先赋成员、后拷贝 ctx）就是值语义能工作的前提，一旦 init 顺序乱了就得换引用语义。**

### 6. `shuffle`：Fisher-Yates + thread_local 随机源

`math.h` 新增（`<algorithm>` 里有 `std::shuffle`，`<random>` 提供引擎）：

```cpp
template<typename RandomIt>
void shuffle(RandomIt first, RandomIt last) {
    static thread_local std::mt19937 generator{ std::random_device{}() };
    std::shuffle(first, last, generator);
}
```

`static thread_local` 是关键：随机数引擎**只初始化一次**（`std::random_device` 真随机种子），之后每次调用复用，避免反复播种让随机序列退化；`thread_local` 让每个线程各有一个独立引擎，多线程下不会竞争。同文件里的 `randomInt` 也是同一模式——项目里两处随机用同一套规范。

### 7. 与参考实现（WispSnow/MonsterWar，commit e8c40e5）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | `GameStats` 改成 `emplace<GameStats&>` 引用语义 | **保持值语义**（`emplace<GameStats>`） | 003 课本地已选定值语义；本课 `initLevelConfig` 先于 `initRegistryContext`，拷贝会带上正确的 `mEnemyCount`，值语义依然正确（见上文 §5） |
| 2 | 命名 trailing underscore（`level_data_` 等） | m-prefix | 遵循本地编码规范 |
| 3 | 参考没有"打乱" | 本地 `createTestEnemy` 本就从多个起点刷，改生成器后保留 `shuffle` 洗牌 | 参考实现此课只按顺序生成；洗牌让同波敌人顺序随机，更接近塔防手感 |

---

## 学习要点

### 1. 数据驱动：把"内容"和"逻辑"分开

`createTestEnemy` 是"逻辑里写死内容"，`LevelConfig + JSON` 是"逻辑读内容"。改敌人组成、节奏、难度（等级/稀有度）、地图，全部改 JSON 而不碰代码。**判断一个东西该不该数据化的标准：它是不是"这一关的内容"？** 是，就进数据文件。塔防里波次、敌人、地图、属性全是内容，所以全该数据化。

### 2. 双队列 = 两层节奏的天然建模

波次节奏（波间隔）和波内节奏（生成间隔）是两个独立维度，用两个容器+两个计时器分开表达。**给逻辑分层时，先看它是不是在回答两个不同的问题**——"下一波什么时候来"和"这波里现在出谁"是两回事，硬塞一个队列会糊成一团。

### 3. 容器跟着操作选，不跟着直觉选

"波次"直觉上用 `queue`（顺序出），但当前波要**洗牌**，`std::shuffle` 要随机访问迭代器，所以用 `deque`。**选容器的依据是你要对它做哪些操作**：先进先出→queue；插入删除两端→deque；洗牌/随机访问→vector/deque。

### 4. ctx 三种语义，别混用

- **值语义**（`emplace<T>`）：快照、只读，谁改了都不会乱（各系统读它不会意外改到别人）
- **共享语义**（`emplace<shared_ptr<T>>`）：跨场景共生命周期的资源
- **引用语义**（`emplace<T&>`）：要**就地修改**且各方必须看到同一份的共享状态

判断口诀：**要改 → 引用；共享生命周期 → shared_ptr；只读快照 → 值。** 值语义下有个陷阱：改成员不等于改 ctx（上上节课踩过），必须保证"先改成员、后拷贝 ctx"的顺序。

### 5. 随机源规范：`static thread_local`

随机引擎"一次播种、复用引擎"，且每个线程独立，是 C++ 里正确的随机用法。写随机代码先想这两点，而不是每调一次都 `std::random_device{}()` 现造引擎。

### 6. 哈希作为跨文件键

JSON 里的字符串和代码里的 `"wolf"_hs` 用同一个 FNV-1a 哈希（`entt::hashed_string`）对齐，字符串键在运行时以整数 id 形式比较——**数据文件的键和代码的键同源**，是"数据驱动"能成立的粘合剂。
