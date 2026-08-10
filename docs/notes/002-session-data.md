# SessionData 会话数据系统：跨场景进度持久化

## 问题

战斗系统已完整（阻挡 / 目标锁定 / 弹道 / 死亡特效 / 血量条），但所有数据都是**静态 / 测试**的：

- 关卡数据靠 `EntityBuilderMW` 从 Tiled 地图硬解析
- 玩家单位靠 `createPlayerUnit("warrior"_hs, ...)` 在代码里写死职业
- 项目里根本没有"积分、通关状态、角色养成"这些概念

塔防游戏必须**跨关卡保存进度**：打到第几关、攒了多少积分、手上养了哪些角色。这个"进度"应该放在哪里？

## 结论

新增 `SessionData` 会话数据类，把**"游戏进度"从"一场战斗的临时状态"里独立出来**。它包含当前关卡、积分、通关状态、玩家角色池四类数据，用 `shared_ptr` 持有（供多个场景共享），通过 JSON 序列化落盘/载入。

```
┌─────────────────────────── 一次游戏会话（Session） ───────────────────────────┐
│  SessionData（常驻内存）                                                        │
│   ├── mLevelNumber   当前关卡号                                                  │
│   ├── mPoint         积分                                                        │
│   ├── mLevelClear    是否通关                                                    │
│   └── mUnitMap       玩家角色池（角色名哈希 → UnitData）                          │
│                                                                                │
│   ▲ 关卡开始前初始化              ▲ 通关/存盘/读档                               │
│   │  loadDefaultData()          │  saveToFile()                                 │
│   └──────────────┬────────────────┴──────────────┐                              │
│            GameScene（当前战斗，临时状态）           存档文件（持久化，磁盘）      │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| `UnitData` + `SessionData` 定义 | `src/game/data/session_data.h:22,36` |
| 加载/保存/角色操作实现 | `src/game/data/session_data.cpp:19,55` |
| GameScene 集成（初始化） | `src/game/scene/game_scene.cpp:129` |
| GameScene 集成（测试打印） | `src/game/scene/game_scene.cpp:223` |
| 默认进度模板 | `assets/data/default_session_data.json` |

### 2. 数据结构：`UnitData` + `SessionData`

```cpp
// session_data.h:22
struct UnitData {
    entt::id_type mNameId{ entt::null };   // 角色名哈希，作为 map 的 key
    entt::id_type mClassId{ entt::null };  // 职业名哈希，如 "warrior"
    std::string mName;                     // 显示名，"加尔隆"
    std::string mClass;                    // 职业字符串，"warrior"
    int mLevel{ 1 };                       // 等级
    int mRarity{ 1 };                      // 稀有度
};

// session_data.h:36
class SessionData {
    int mLevelNumber{ 1 };
    int mPoint{ 0 };
    bool mLevelClear{ false };
    std::unordered_map<entt::id_type, UnitData> mUnitMap;   // 角色名哈希 : 角色数据
};
```

`UnitData` 是"一个角色"的最小档案，`SessionData` 是"整局游戏"的进度档案。

### 3. 为什么用 `hashed_string` 哈希当 key？

`entt::hashed_string("加尔隆")` 把 UTF-8 字节序列通过 **FNV-1a 算法**变成 32 位整数 ID，等价于蓝图系统里的 `"warrior"_hs` 写法。

- **优点**：查找是 O(1) 哈希查找，不用反复做字符串比较
- **代价**："哈希即身份"，两个名字若撞哈希会互相覆盖（FNV-1a 理论上有碰撞可能，实际不同名字几乎不会）
- **一致性**：同一个名字在任意地方哈希结果相同，所以 SessionData 里的 `name_id` 能和实体工厂、蓝图管理器里的职业 ID 对上

这是 EnTT 生态的核心思想——**一切身份都用编译期/运行期字符串哈希，而非裸字符串**。

### 4. 加载 / 保存机制

**加载（JSON → 内存）`session_data.cpp:19`：**

```cpp
bool SessionData::loadDefaultData(std::string_view path) {
    if (!std::filesystem::exists(path)) { ... return false; }
    std::ifstream file{ std::filesystem::path(path) };   // 花括号：避开 most-vexing-parse
    if (!file.is_open()) { ... return false; }
    nlohmann::json json;
    try {
        file >> json;                    // nlohmann 反序列化，非法 JSON 会抛异常 → 必须进 try
        clear();                         // 解析成功后再清旧数据
        mLevelNumber = json["level"].get<int>();
        mPoint        = json["point"].get<int>();
        mLevelClear   = json["level_clear"].get<bool>();
        for (const auto& [name, data] : json["unit"].items()) { ... mUnitMap.emplace(...); }
    } catch (const std::exception& e) { ... return false; }
    return true;
}
```

**保存（内存 → JSON 磁盘）`session_data.cpp:55`：**

```cpp
bool SessionData::saveToFile(std::string_view path) {
    // 父目录不存在时自动创建，避免 ofstream 打开失败
    if (... && !std::filesystem::exists(file_path.parent_path())) {
        std::filesystem::create_directories(file_path.parent_path());
    }
    std::ofstream file{ file_path };
    ...
    file << json.dump(4);   // 4 空格缩进序列化，与 assets/save/SLOT_1.json 格式一致
}
```

三个值得注意的点：

- **中文 key 直接落盘**：nlohmann 默认 `dump()` 不转义非 ASCII，配合 MSVC `/utf-8` 标志，中文角色名能正确读写
- **`hashed_string(name.data(), name.size())`**：传长度避免依赖 `string_view::data()` 的 NUL 结尾（参考实现用 `.data()` 是隐患）
- **文件路径用 `std::filesystem::path`**：Windows 下 path 内部用 UTF-16 存储，`parent_path().string()` 转回 UTF-8 打印日志

### 5. GameScene 集成：为什么是 `shared_ptr`？

```cpp
// game_scene.cpp:129
bool GameScene::initSessionData() {
    if (!mSessionData) {                       // 为空才创建，支持外部传入共享实例
        mSessionData = std::make_shared<game::data::SessionData>();
        if (!mSessionData->loadDefaultData()) { ... return false; }
    }
    mLevelNumber = mSessionData->getLevelNumber();   // 缓存关卡号到场景
    return true;
}
```

**核心决策：`std::shared_ptr` 而不是 `unique_ptr` 或值拷贝。**

SessionData 的**所有权属于整个游戏会话，不属于某个场景**。关卡切换时（主菜单 → GameScene → 结算场景），多个场景要读写**同一个**进度实例；若用 `unique_ptr`，场景销毁时数据也跟着销毁，进度就丢了。这也正是类注释里写的"数据实例很可能同时被多个场景使用"。

**初始化时机**：`initSessionData()` 放在 `GameScene::init()` 的**最前面**（`game_scene.cpp:52`），因为它是"关卡数据"的地基，后续 `loadLevel()`、实体工厂都可能要读它。

### 6. 与参考实现（WispSnow/MonsterWar）的差异

参考提交 `6b5e7a5 完成SessionData` 实现了同样的功能，但本地"自主重写"时修正了 6 处问题：

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | `clear()` 漏重置 `point_`、`level_clear_` | 全部重置 | 重开游戏会残留上次积分/通关状态 |
| 2 | `loadFromFile()` 只是调 `loadDefaultData()` | 删除该函数 | "读档"≠"加载默认"，语义欺骗，等真正做存档槽再加 |
| 3 | `std::ifstream file(path)` 圆括号 | 花括号 `file{ path }` | 圆括号被解析成函数声明（most-vexing-parse） |
| 4 | `file >> json` 在 try 外 | 整体入 try | 非法 JSON 会裸抛异常 |
| 5 | 开头就 `clear()` | 解析成功后才 `clear()` | 避免解析失败时误清已有进度 |
| 6 | `hashed_string(name.data())` | `hashed_string(name.data(), name.size())` | `data()` 不保证 NUL 结尾 |

---

## 学习要点

### 内存态 vs 持久态（SessionData 的核心设计思想）

| 维度 | 内存态（SessionData） | 持久态（JSON 文件） |
|------|----------------------|---------------------|
| **位置** | 常驻内存，一次游戏会话生命周期 | 磁盘 `assets/save/*.json` |
| **访问** | O(1) 哈希查找 | 需要反序列化 |
| **格式** | `unordered_map<id_type, UnitData>` | `{"unit": {"角色名": {...}}}` |
| **同步时机** | 实时 | 仅 saveToFile / loadDefaultData 时 |

游戏逻辑只读写内存态（快），需要"存档/读档"时才做一次性序列化/反序列化（慢）。**把"频繁访问的进度"和"磁盘格式"解耦**，是这类系统的通用模式。

### 两个 C++ 陷阱

1. **most-vexing-parse**：`std::ifstream file(path)` 会被编译器当作函数声明。统一用花括号 `file{ path }`。
2. **`string_view::data()` 不保证 NUL 结尾**：只有从字符串字面量构造时才碰巧是。做哈希、传 C 接口时用 `(data(), size())` 显式传长度。
