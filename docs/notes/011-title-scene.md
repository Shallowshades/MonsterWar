# 标题场景：TitleScene / 存档读档面板 / 可排序单位表格

## 问题

第 10 课（010）把主场景 GameScene 完善成了"可玩向"：暂停、重开、四大调试窗口、升级/撤退都齐了。但游戏**没有入口**——`main.cpp` 一启动就直接 Push 进 GameScene，玩家没有标题界面，也就没有"开始游戏 / 确认角色 / 载入游戏 / 退出游戏"这几个最基本的流程：

1. **没有标题场景**——启动即战斗，没有游戏名 LOGO、没有主菜单入口。
2. **开始游戏没有"流程"**——直接进 GameScene，不经过任何选择/确认。
3. **没有存档/读档**——010 里 `onSave` / `onBackToTitle` 还是 TODO 桩，进度没法落盘，也就没有"退出后接着玩"的通道。
4. **没有角色总览**——玩家手里的角色池只能靠战斗里的肖像面板看，缺一个"确认角色/查看档案"的入口。

一句话：**把"直接进战斗"改成"标题场景 →（开始游戏/读档）→ 战斗 →（返回标题/保存）"的完整流程闭环**——新增 TitleScene 与四个入口按钮，补上存档/读档面板，并加一个可排序的角色信息表格。

## 结论

本课补上游戏"外圈"流程：

```
main.cpp 初始场景
    │ PushSceneEvent
    ▼
TitleScene（标题场景）
    ├── 渲染：RenderSystem 画 title.tmj 地图 + DebugUI（TitleLogo 图 + 4 个大按钮）
    ├── init：setState(State::Title) + setTimeScale(1.0f)
    │
    ├── 开始游戏 onStartGameClick → requestReplaceScene(新 GameScene，带上 4 份共享数据)
    ├── 确认角色 onConfirmRoleClick → 角色信息面板（14 列可排序表格 + 升级按钮）
    ├── 载入游戏 onLoadGameClick → 读档面板（SLOT 1/2/3 → session_data->loadFromFile）
    └── 退出游戏 onQuitClick → quit()（QuitEvent 退出循环）
              │
              ▼ 战斗中
GameScene
    ├── 返回标题 onBackToTitle → requestReplaceScene(新 TitleScene，只传 mContext)
    └── 保存 onSave → 存档面板（SLOT 1/2/3 → session_data->saveToFile）
```

- **TitleScene 与 GameScene 构造器完全对称**：`(Context&, shared_ptr<BlueprintManager>=nullptr, shared_ptr<SessionData>=nullptr, shared_ptr<UIConfig>=nullptr, shared_ptr<LevelConfig>=nullptr)`，空则懒创建——两个场景可以互相用同一份数据接力。
- **`friend class game::system::DebugUISystem`**：标题按钮回调（onStartGameClick 等）是 TitleScene 私有方法，DebugUI 通过 friend 直接调用，无需为每个回调暴露公有接口。
- **DebugUISystem 拆分两条路径**：`update()`（GameScene：战斗窗口 + 存档面板）与 `updateTitle(TitleScene&)`（标题 Logo/按钮/角色信息/读档面板）。
- **SessionData 新增 `mUnitDataList`**（与 `mUnitMap` 平行的 `vector<UnitData*>`，add/remove/clear/load 四路同步）+ `getUnitDataList()` + `loadFromFile(path)`（`loadDefaultData(path)` 别名）。
- **存档/读档面板走 ctx 传显示标志**：GameScene 的 `mShowSavePanel` 经 `ctx().emplace_as<bool&>("show_save_panel"_hs, ...)` 暴露给 DebugUI；TitleScene 的两个标志则因 friend 直接读私有成员。
- **14 列可排序单位表格**：`ImGuiTableFlags_Sortable` + `SpecsDirty` 脏标识 + `std::stable_sort`，每行有肖像悬浮 tooltip 和扣积分升级按钮。

---

## 原理分析

### 1. 代码位置

| 内容 | 位置 |
|------|------|
| TitleScene 声明 | `src/game/scene/title_scene.h`（新建） |
| TitleScene 实现 | `src/game/scene/title_scene.cpp`（新建） |
| 场景加入构建 | `CMakeLists.txt`（SOURCES 加 title_scene.cpp） |
| `mUnitDataList` / `getUnitDataList` / `loadFromFile` | `src/game/data/session_data.h/.cpp` |
| GameScene `mShowSavePanel` + onBackToTitle/onSave | `src/game/scene/game_scene.h`（成员）+ `game_scene.cpp` |
| ctx 暴露存档面板标志 | `src/game/scene/game_scene.cpp:initRegistryContext` |
| DebugUI fwd TitleScene + updateTitle 声明 | `src/game/system/debug_ui_system.h` |
| DebugUI 标题渲染函数 | `src/game/system/debug_ui_system.cpp`（renderTitleLogo/Buttons/UnitInfoUI/LoadPanelUI/SavePanelUI/UnitTable） |
| 初始场景切 TitleScene | `src/main.cpp` |

### 2. TitleScene 的 DI 构造器：与 GameScene 对称，让"数据接力"成立

TitleScene 的构造器签名与 GameScene 一字不差，且都保留"空则懒创建"的兜底。为什么要完全对称？因为**两个场景要互相当对方的入口**：

- **标题 → 游戏**：`onStartGameClick` 把 `mBlueprintManager / mSessionData / mUIConfig / mLevelConfig` 四份 shared_ptr 全部传给新 GameScene——玩家在标题读档或选好的进度原样带进战斗。
- **游戏 → 标题**：`onBackToTitle` 却**只传 `mContext`**，不传任何共享数据——**刻意丢弃未保存进度**。为什么？因为本课起有存档系统了：战斗中的进度靠"保存"落盘，回标题就是"放弃当前局"，从盘上的档重新开始。这是参考的明确设计（存档成为进度的持久通道，返回标题不再负责搬运内存态进度）。

```cpp
void TitleScene::onStartGameClick() {
    // 读档载入的数据可能已经通关，此时进入下一关
    if (mSessionData->isLevelClear()) {
        mSessionData->setLevelClear(false);
        mSessionData->addOneLevel();
    }
    requestReplaceScene(std::make_unique<game::scene::GameScene>(
        mContext, mBlueprintManager, mSessionData, mUIConfig, mLevelConfig));
}

void GameScene::onBackToTitle() {
    requestReplaceScene(std::make_unique<game::scene::TitleScene>(mContext));  // 只传 mContext
}
```

两个 `requestReplaceScene(make_unique<...>)` 之所以成立，是因为引擎层 `Scene::requestReplaceScene` 只发 `ReplaceSceneEvent`，`SceneManager` 只接管事件里的 `unique_ptr`——场景之间的切换是"事件驱动的换手"，数据跟随 shared_ptr 走。

### 3. `friend DebugUISystem`：ImGui 调试系统穿透场景私有接口

标题按钮的四个回调是 TitleScene 的私有方法。参考没有为它们各写一个公有包装，而是直接声明：

```cpp
class TitleScene final : public engine::scene::Scene {
    friend class game::system::DebugUISystem;   // 允许DebugUISystem访问私有成员变量及方法
```

于是 DebugUI 的 `renderTitleButtons` 里可以 `title_scene.onStartGameClick()` 直接点私有回调，`updateTitle` 里也能 `renderUnitInfoUI(title_scene.mShowUnitInfo)` 直接读私有标志。好处：**调试 UI 与场景"穿透式"耦合，省掉一堆公有接口**；代价：调试系统要知道场景内部结构——参考明确注释"先用 ImGui 快速实现，未来再完善游戏内 UI"，这种耦合是可接受的过渡手段。工程上，**调试工具允许侵入，正式玩法接口保持克制**。

### 4. DebugUISystem 两条更新路径：update() 与 updateTitle()

同一个人 DebugUISystem 要服务两个场景，且渲染逻辑完全不同。参考拆成两条：

```cpp
void DebugUISystem::update() {          // GameScene 专用
    beginFrame();
    renderHoveredPortrait(); renderHoveredUnit(); renderSelectedUnit();
    renderInfoUI(); renderSettingUI(); renderDebugUI();
    auto& show_save_panel = mRegistry.ctx().get<bool&>("show_save_panel"_hs);   // 存档面板
    renderSavePanelUI(show_save_panel);
    endFrame();
}

void DebugUISystem::updateTitle(game::scene::TitleScene& title_scene) {   // TitleScene 专用
    beginFrame();
    renderTitleLogo();
    renderTitleButtons(title_scene);
    renderUnitInfoUI(title_scene.mShowUnitInfo);      // friend 直接读私有标志
    renderLoadPanelUI(title_scene.mShowLoadPanel);
    endFrame();
}
```

**存档面板标志怎么传给 DebugUI？** 两条路径用了两种传法：

- GameScene 走 **ctx**：`mShowSavePanel` 经 `ctx().emplace_as<bool&>("show_save_panel"_hs, mShowSavePanel)` 放入 registry 上下文，DebugUI 用 `mRegistry.ctx().get<bool&>("show_save_panel"_hs)` 拿到**同一块 bool 的引用**——改的是同一个变量。为什么走 ctx？因为 DebugUI 持有的是 `mRegistry`，ctx 是它俩共同的"黑板"；而且 GameScene 每次重开都新建 registry，存档面板标志自然随场景重置。
- TitleScene 走 **friend**：两个标志直接读私有成员，不需要 ctx。

两种都合法，选择标准是"数据已经放在哪"。`emplace_as<bool&>` 的 `_as` 后缀值得注意：它告诉 EnTT 存的是**引用**，而不是拷贝——所以 DebugUI 改标志，GameScene 的 `mShowSavePanel` 同步变。

### 5. SessionData::mUnitDataList：平行指针列表为"可排序遍历"服务

`mUnitMap` 是 `unordered_map`，遍历顺序是哈希序，没法做"按列排序"。参考为表格加了一条**与 map 平行的指针列表**：

```cpp
std::vector<UnitData*> mUnitDataList;   // 与 mUnitMap 同步更新，用于排序遍历

void SessionData::mapUnitDataList() {   // load 后重建
    mUnitDataList.clear();
    mUnitDataList.reserve(mUnitMap.size());
    for (auto& [id, data] : mUnitMap) mUnitDataList.push_back(&data);
}
```

**维护纪律**：加角色 `push_back(&mUnitMap[name_id])`、删角色先 `std::remove` 再从列表删、清空一起清、load 完重建。指针指向 map 的 value，不拷贝数据，排序只挪指针。**"map 管存取、list 管有序遍历"**——两种容器各司其职，靠同步维护保持一致。

**`loadFromFile` 为什么只是别名？** 参考的 loadFromFile 语义是"从文件覆盖当前数据"，而本地 `loadDefaultData(std::string_view path)` 恰好已支持任意路径（且有"解析失败不破坏旧数据"的容错）。所以：

```cpp
bool SessionData::loadFromFile(std::string_view path) { return loadDefaultData(path); }
```

复用而不是复制——本地 API 设计恰好覆盖了读档需求。

### 6. renderUnitTable：ImGui 可排序表格的完整套路

14 列（姓名/职业/类型/等级/稀有度/COST/生命值/攻击力/防御力/攻击范围/攻击间隔/阻挡数量/技能/升级）是本课最长的函数，拆开是三块：

**① 表头 + 数据**：`ImGui::BeginTable("角色信息", 14, SizingFixedFit | Sortable)` + 14 个 `TableSetupColumn` + `TableHeadersRow`。`SizingFixedFit` 让列宽按内容自适应，`Sortable` 打开表头点击排序。

**② SpecsDirty 脏标识排序**：

```cpp
if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
    if (sort_specs->SpecsDirty && !unit_data_list.empty()) {
        const ImGuiTableColumnSortSpecs& spec = sort_specs->Specs[0];
        const int col = spec.ColumnIndex;
        const bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
        std::stable_sort(unit_data_list.begin(), unit_data_list.end(), [&](const UnitData* lhs, const UnitData* rhs) {
            // 按 col 号 switch，产生 delta(-1/0/1)，返回 ascending ? delta<0 : delta>0
        });
        sort_specs->SpecsDirty = false;    // 关键：置 false，下轮跳过
    }
}
```

- `SpecsDirty` 在用户点击表头后的**下一帧**为 true，`ImGui::TableGetSortSpecs()` 每帧都会返回同一份 specs——**必须排序后立刻置 false**，否则每帧都重排（脏标识模式）。
- 每列的比较器从蓝图中取对应属性，再 `statModify` 重算（COST 用等级=1 只算稀有度）。**COST 列用 int（round 后）比较**，避免 float 相等判断的坑。
- `std::stable_sort` 保证同 key 的行保持原有相对顺序，视觉上稳定。

**③ 行渲染 + 交互**：每行 `TableNextRow` + 若干 `TableNextColumn`。要点：

- **姓名列悬浮显肖像 tooltip**：`ui_config->getPortrait(mNameId)` 取 `Image`，用 `getSourceRect()`（源矩形）+ `getTextureSize()`（精灵图尺寸）算 UV 坐标，`ImGui::Image` 画 128×128 大头像。**UV 是"源矩形位置/精灵图尺寸"的比例**，把雪碧图的子区域贴到 UI 上。
- **`PushID(unit->mName.c_str())`**：ImGui 的按钮默认以文本为 ID，两个同名的"升级"按钮会 ID 冲突。`PushID` 用角色名做作用域前缀，保证每个按钮 ID 唯一。
- **升级按钮扣积分**：`BeginDisabled(!(getPoint() >= cost))` 置灰 + 点击 `addPoint(-cost); mLevel += 1`——这是标题场景里"消耗积分养角色"的入口，与战斗内"消耗 COST"升级是两套资源体系。

### 7. main.cpp 初始场景 + 标题状态

```cpp
auto titleScene = std::make_unique<game::scene::TitleScene>(context);
context.getDispatcher().trigger<engine::utils::PushSceneEvent>(...);
```

TitleScene::init 里 `setState(engine::core::State::Title)` + `setTimeScale(1.0f)`——前者把全局状态切到标题（方便其它系统查询 `isInTitle()`），后者**重置游戏速度**（从战斗 2 倍速返回标题后，倍速不再残留）。引擎 `game_app.cpp` 的循环对 State 无门控，所以标题场景天然能启动、能渲染 ImGui。

### 8. 本地适配中遇到的三个环境差异

| # | 差异 | 本地处理 |
|---|------|---------|
| 1 | ImGui 1.91 的 `ImTextureID` 默认是 `ImU64`（参考项目可能是 `void*`/`SDL_Texture*`） | `ImGui::Image((ImTextureID)(intptr_t)texture, ...)` 显式把 `SDL_Texture*` 转成整数指针 |
| 2 | `Rect` 用 m-prefix（`mPosition/mSize`，参考用 `position/size`） | UV 计算改成 `portrait_rect->mPosition.x` 等 |
| 3 | `PlayerBlueprint.mCost` 是 `int`，传给 `statModify(float base)` 触发 C4244 | `statModify(static_cast<float>(mCost), ...)` 显式转换 |

---

## 与参考实现（WispSnow/MonsterWar，commit 0b863f8）的差异

| # | 参考实现 | 本地修正 | 原因 |
|---|---------|---------|------|
| 1 | 字段后缀 `unit_data_list_/name_/level_` 等 | m-prefix：`mUnitDataList/mName/mLevel/mPosition/mSize` | 本地 m-prefix 规范 |
| 2 | `ui_manager_` 在头文件里**未声明**（依赖基类继承成员） | 直接用基类 `mUIManager->init(...)` | 编译合法性：本地 Scene 基类已有 `mUIManager` |
| 3 | `ImGui::Image(texture, ...)` 直接传 `SDL_Texture*` | `(ImTextureID)(intptr_t)texture` | 本地 ImGui 1.91 `ImTextureID=ImU64`，直接传指针编译不过 |
| 4 | `constexpr glm::vec2 DISPLAY_SIZE` | `const glm::vec2 DISPLAY_SIZE` | 本地 glm 1.0.1 的 vec 构造非 constexpr，`constexpr` 编译不过 |
| 5 | `statModify(player_.cost_, ...)`（float base） | `statModify(static_cast<float>(mCost), ...)` | 本地 `mCost` 是 int，直接传触发 C4244 警告 |
| 6 | `loadFromFile` 独立实现 | `loadDefaultData(path)` 别名 | 本地 loadDefaultData 已支持任意路径且语义相同 |
| 7 | 缩进 2 空格 | game/scene 用 tab，其余 game 目录 4 空格 | 本地各目录缩进约定 |
| 8 | `assets/maps/title.tmj` / `title.png` / `State::Title` / `Scene::quit/requestReplaceScene` / `getPortrait` / `getTextureSize` 等 | 本地早已就绪 | 本地就绪度高，本课"空手接" |

**本地就绪度说明**：参考课需要新建的 title.tmj（逐字节一致）、title.png、`State::Title`、`Scene::quit/requestReplaceScene`、`UIConfig::getPortrait`、`Image/ResourceManager` 相关 API、`BlueprintManager`、`statModify`、`PlayerType`、`DebugUISystem` fwd 声明等，本地全在更早的课就位。本课真正新增的是：TitleScene 两个文件、SessionData 的平行列表 + loadFromFile、GameScene 的 onBackToTitle/onSave + mShowSavePanel、DebugUI 的 updateTitle + 6 个渲染函数、main.cpp 初始场景切换。

---

## 学习要点

### 1. 场景流程 = "对称构造器 + 事件驱动换手"

TitleScene 和 GameScene 能互相成为对方入口，靠的是**一模一样的构造器签名**——两场景对同一批 shared_ptr 有相同的"接住"能力，数据才能在它们之间接力。配合引擎层 `requestReplaceScene(make_unique<...>)`（只发事件、不直接建场景），**场景切换就是"事件里的指针换手"**。设计多场景游戏时，先统一场景构造器，再谈流程。

### 2. 返回标题 = 放弃进度：数据所有权就是产品决策

`onBackToTitle` 只传 mContext、`onStartGameClick` 传全部数据——同一个 `requestReplaceScene`，传不传共享数据体现了**产品语义**：开始游戏要带着进度进去，返回标题要清空当前局（因为存档系统兜底）。**"传给新场景什么数据"不是技术细节，而是流程设计的一部分**。

### 3. friend：调试系统的"通行证"，游戏玩法的"禁区"

`friend DebugUISystem` 让 ImGui 调试系统直接访问场景私有成员/回调，省掉大量公有包装。但要警惕：**friend 让依赖反向（场景知道调试系统）**，只适合调试工具这种"注定要删/要换"的临时实现。正式玩法接口应保持克制，不要因为调试方便就全开公有一遍。

### 4. ctx 的 `emplace_as<bool&>`：registry 上下文存"引用"而非"拷贝"

`mRegistry.ctx().emplace_as<bool&>("show_save_panel"_hs, mShowSavePanel)` 存的是**对成员变量的引用**，DebugUI 用 `get<bool&>` 拿到的还是那块内存——改标志即改成员，双方永远一致。相比"拷贝进 ctx"，引用型上下文是**跨系统共享可变状态的轻量通道**；代价是生命周期要小心（标志归属的场景必须比 ctx 活得久）。

### 5. 平行列表维护：map 管存取、list 管有序遍历

`unordered_map` 无序、`vector` 有序，两个容器服务两种需求，靠**四路同步**（add/remove/clear/load）保持一致。这是"需要同时支持哈希存取与有序遍历"时的通用解。指针列表存 `UnitData*` 而非拷贝，排序只挪指针、数据零拷贝。

### 6. ImGui 表格排序 = SpecsDirty 脏标识 + stable_sort

`SpecsDirty` 只在点击表头后为真一帧，**排序完必须置 false**（脏标识模式），否则每帧重排。列比较器从蓝图中取属性并 `statModify` 重算，数值列用 int 比较避免浮点相等坑。这是 ImGui `BeginTable` + `Sortable` 的标准用法。

### 7. 环境差异也是"学习对象"：ImTextureID / glm constexpr

参考代码能 `ImGui::Image(texture)` 直接编译，本地要 `(ImTextureID)(intptr_t)texture`——不是参考错了，是 ImGui 1.91 把 `ImTextureID` 从 `void*` 改成了 `ImU64`。**"照抄参考编译不过"时，先看依赖版本**，通常不是代码问题而是 API 演进。同样 `constexpr glm::vec2` 编译不过是因为 glm 1.0.1 的构造器非 constexpr——退一步用 `const` 即可。
