# 014 Android 触屏适配方案（整合版）

> 状态：核心功能已实施，待真机/WASM验证
> 目标：让 MonsterWar 在 Android 设备（WASM 浏览器）上不依赖鼠标/键盘即可完整游玩。
> 原则：保留 PC 鼠标键盘流程，移动端通过“触摸输入适配层 + 交互模式状态机 + 移动端 HUD 按钮”实现同等操作。

---

## 1. 背景与问题

当前游戏在 Android 上能运行，但交互仍是 PC 的鼠标 + 键盘设计：

| 功能 | PC 现状 | Android 问题 |
|---|---|---|
| 放置单位 | 点肖像 → 幽灵单位跟随鼠标 → 左键放到格子上 | 没有鼠标 hover，触摸抬起/按下时序和 PC 不同，幽灵不会稳定跟随手指；点肖像后很难把单位“拖”到地图 |
| 选中单位 | 鼠标悬浮 + 左键点击选中 | 触摸没有 hover，`SelectionSystem` 依赖 `hovered_unit`，点选基本失效 |
| 升级 | 选中后 ImGui 窗口里点“升级”或按 `U` | 无键盘；ImGui 窗口在手机上偏小、偏 debug |
| 技能 | 选中后点技能按钮或按 `S` | 无键盘；按钮藏在 ImGui 面板里，不适合触屏 |
| 撤退 | 按 `R` 或 ImGui 按钮 | 无键盘 |
| 肖像滚动 | 键盘 `←` / `→` | 无键盘 |

核心问题不是“加几个按钮”，而是把整套 **hover/键盘交互** 转成 **触摸点击/拖拽交互**。

---

## 2. 总体设计

```
Android 触摸
   ↓
InputManager 触摸层（FINGER 事件 + 坐标转换 + 关闭合成鼠标）
   ↓
UI 点击消费机制（防止 UI 穿透）
   ↓
交互模式状态机（NONE / PLACING / SELECTING）
   ↓
PlaceUnitSystem 触摸放置
SelectionSystem 触摸点选
   ↓
MobileActionBar（升级/撤退/技能/取消）
```

关键决策：

1. **关闭触摸合成鼠标**，避免 FINGER 与鼠标双路径触发。
2. **FINGER 坐标归一化 → 窗口像素 → 逻辑坐标**，与现有鼠标坐标对齐。
3. **UI 点击消费**，点按钮不会同时触发放置/选中。
4. **显式交互模式状态机**，消除“取消放置/取消选中”歧义。
5. **移动端隐藏 ImGui**，正式操作入口统一走 MobileActionBar。

---

## 3. 输入层方案

### 3.1 触摸事件接入

在 `InputManager` 中增加：

- 处理 `SDL_EVENT_FINGER_DOWN / FINGER_MOTION / FINGER_UP`
- 记录：
  - `mIsTouchActive`
  - `mTouchPosition`（逻辑坐标）
  - `mTouchStartPosition`
- 注册动作：
  - `touch_begin`
  - `touch_move`
  - `touch_end`
  - `touch_cancel`
- 保留现有 `mouse_left` / `mouse_right` 给 PC 用。
- 提供查询：
  - `isTouchDevice()`
  - `getTouchPosition()`
  - `isTouchDown()`

### 3.2 关闭/屏蔽合成鼠标（关键）

当前 `game_app.cpp:190` 设置了：

```cpp
SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
```

如果继续开启，触摸会同时产生 FINGER 事件和合成鼠标事件，导致放置/选中触发两次。

处理方式：

- 移动端把该 hint 置为 `"0"`；
- 或 InputManager 在触摸激活时屏蔽合成鼠标事件路径。

### 3.3 FINGER 坐标转换（关键）

`SDL_EVENT_FINGER_*` 的 x/y 是归一化坐标（0~1），不是逻辑坐标。

转换链路：

1. 先乘窗口尺寸得到窗口像素：
   ```cpp
   glm::vec2 pixel = { x * window_w, y * window_h };
   ```
2. 再调用 `SDL_RenderCoordinatesFromWindow` 得到逻辑坐标：
   ```cpp
   SDL_RenderCoordinatesFromWindow(mSDLRenderer, pixel.x, pixel.y, &logical.x, &logical.y);
   ```
3. 与现有 `getLogicalMousePosition()` 的转换保持一致。

### 3.4 触摸设备检测

- WASM：`SDL_GetNumTouchDevices() > 0`
- 或复用 `web/games/monsterwar/js/game.js` 已有的 `isMobile` / `body.mobile` 检测结果

### 3.5 UI 点击消费机制（关键）

现状：`UIButton` 和 `PlaceUnitSystem` / `SelectionSystem` 订阅的是同一个 `mouse_left` 信号，点按钮会同时触发放置/选中。

方案：

- InputManager 增加：
  - `consumeNextClick()`
  - `isClickConsumed()`
- UI 命中并处理触摸/点击后调用 `consumeNextClick()`；
- `PlaceUnitSystem` / `SelectionSystem` 在动作回调开头检查：
  - 已消费 → 直接返回，不处理本次点击。

或者由 UIManager 在 UI 命中时拦截动作分发，不让 `mouse_left` / `touch_end` 继续广播到游戏系统。

---

## 4. 交互模式状态机

在 GameScene 或新增的 `TouchInputController` 中维护模式：

| 模式 | 触发 | 触摸行为 |
|---|---|---|
| `NONE` | 默认 | 点空白 = 取消选中；点玩家单位 = 选中 |
| `PLACING` | 点肖像进入 | 触摸拖动 = 移动幽灵；触摸抬起合法 = 放置；点空白/取消 = 取消放置 |
| `SELECTING` | 点单位进入 | 点单位 = 选中；点空白 = 取消选中 |

一次触摸事件只能按当前模式分派，避免“取消放置/取消选中”歧义。

---

## 5. 放置单位

### 5.1 推荐流程：两步点击（先做）

1. 点击下方角色肖像 → 进入 `PLACING`
   - 创建幽灵单位（复用 `UnitPrepComponent`）
   - 幽灵初始出现在屏幕中央或上一次放置位置，而不是肖像位置
2. 点地图上的合法放置点
   - 自动吸附到最近的合法放置点（近战点/远程点）
   - 合法显示绿色，非法显示红色
3. 松手/确认：
   - 在合法放置点上松手 → 放置成功
   - 在非法位置松手 → 不放置，幽灵保留
4. 取消：
   - 点地图空白处
   - 或点“取消放置”按钮
   - 或再次点同一个肖像

### 5.2 后续增强：拖拽放置

在两步点击跑通后，再叠加：

- 点肖像后手指不抬起，直接拖到地图；
- 幽灵跟随手指；
- 松手在合法点则放置。

### 5.3 代码改动点

- `PlaceUnitSystem::update()`
  - 不再每帧跟随“鼠标位置”，改为跟随“触摸位置”
  - 触摸移动时更新幽灵位置 + 吸附检测
- `PlaceUnitSystem::onPlaceUnit()`
  - 从“鼠标按下时放置”改为“触摸抬起且目标合法时放置”
- 新增 `onTouchEnd()` / `onTouchCancel()`
- 放置成功后自动退出 `PLACING`

---

## 6. 选中单位

当前 `SelectionSystem` 依赖 hover 状态，Android 上没有 hover，需要改成：

- 触摸抬起时，根据触摸位置查找玩家单位：
  - 命中玩家单位 → 选中该单位
  - 命中空白/敌人 → 取消选中
- 如果触摸点在 UI 上，则不触发选中（依赖 UI 点击消费机制）

代码改动点：

- `SelectionSystem::update()` 增加触摸模式分支
- 新增 `onTouchEnd()`：用 `getTouchPosition()` 做点选
- PC 鼠标 hover 逻辑保留

---

## 7. 移动端操作栏 MobileActionBar

### 7.1 显示逻辑

- 未选中单位：不显示操作栏，或只显示暂停/速度等全局按钮
- 选中玩家单位后，在屏幕底部/单位旁边显示操作栏：

```
[ 升级  -100 ]  [ 撤退 +50 ]  [ 技能 ⚡ ]  [ ✕ 取消 ]
```

### 7.2 按钮规则

| 按钮 | 行为 | 置灰/禁用条件 |
|---|---|---|
| 升级 | 发送 `UpgradeUnitEvent{ entity, player.mCost }` | `COST < player.mCost` |
| 撤退 | 先计算 `return_cost = (int)(player.mCost * 0.5f)`，再发送 `RetreatEvent{ entity, return_cost }` | 不需要（或按需） |
| 技能 | 发送 `SkillActiveEvent{ entity }` | 技能未就绪 / 冷却中 |
| 取消 | 清除选中单位 | 无 |

### 7.3 设计要点

- 按钮尺寸：至少 `48x48` 逻辑像素，建议 `64x64`
- 按钮上直接显示费用/冷却：
  - 升级按钮显示 `升级 -100`
  - 技能按钮显示技能名 + 冷却倒计时或“就绪”
- 位置：屏幕底部，避开下方肖像栏；可放在选中单位上方的小浮层
- 使用现有 `UIButton` 的点击回调，发送事件即可
- 按钮点击必须消费触摸事件，防止穿透到放置/选中

### 7.4 新增文件

```
src/game/ui/mobile_action_bar.h
src/game/ui/mobile_action_bar.cpp
```

类似 `UnitsPortraitUI`，在 `GameScene::initUnitsPortraitUI()` 附近初始化，并在 `GameScene::update/render` 中通过 UIManager 更新渲染。

---

## 8. 肖像栏滚动

现在肖像栏用键盘左右移动，Android 需要：

- 在肖像栏两侧加 `◀` / `▶` 触摸按钮
- 或支持在肖像栏区域左右滑动滚动

建议先加两个箭头按钮，简单可靠。

---

## 9. 移动端其他处理

- **隐藏 ImGui 调试窗**：
  - 升级/撤退/技能入口不再依赖 ImGui
  - 移动端不渲染 DebugUISystem 的 ImGui 窗口，避免两套 UI 叠加
- **防止误触**：
  - UI 点击优先于地图操作
  - 触摸开始点在 UI 内时，不进入放置/选中逻辑
- **适配安全区**：
  - 按钮和肖像栏避开刘海/底部导航条
- **可选**：
  - 暂停、1x/2x 倍速按钮
  - 全屏/横屏提示（已有 JS 壳支持）

---

## 10. 实施步骤

0. **最小可行验证**
   - 先把触摸位置映射成虚拟鼠标位置，触摸按下映射为 `mouse_left`
   - 在 Android 真机验证现有“点肖像 → 点地图”两步流程可玩
1. 关闭/屏蔽触摸合成鼠标
2. 加 FINGER 坐标转换
3. 加 UI 点击消费机制
4. 加交互模式状态机
5. 改 `SelectionSystem` 支持触摸点选
6. 改 `PlaceUnitSystem` 支持触摸放置
7. 新增 `MobileActionBar`
8. 移动端隐藏 ImGui 调试窗
9. 肖像栏加左右箭头 / 滑动
10. 真机/浏览器设备模拟验证

---

## 11. 涉及文件（预估）

| 文件 | 改动 |
|---|---|
| `src/engine/input/input_manager.h/.cpp` | 增加触摸事件、触摸状态、触摸动作、坐标转换、UI 消费 |
| `src/engine/core/game_app.cpp` | 移动端关闭 `SDL_HINT_TOUCH_MOUSE_EVENTS` |
| `src/game/system/place_unit_system.h/.cpp` | 触摸放置流程、吸附、取消 |
| `src/game/system/selection_system.h/.cpp` | 触摸点选 |
| `src/game/ui/mobile_action_bar.h/.cpp` | 新增移动端操作栏 |
| `src/game/scene/game_scene.h/.cpp` | 初始化/更新移动端 HUD、交互模式状态机 |
| `src/game/ui/units_portrait_ui.h/.cpp` | 肖像栏左右箭头/滑动 |
| `src/engine/ui/state/*` | 可选：让 UIButton 支持无 hover 的触摸按下/抬起 |
| `src/game/system/debug_ui_system.cpp` | 移动端隐藏 ImGui 调试窗 |

---

## 12. 验收标准

- [ ] Android 浏览器上可以点肖像、拖拽/点击地图放置单位
- [ ] 点选单位后出现升级/撤退/技能按钮
- [ ] 升级、撤退、技能按钮可正常触发，费用/冷却显示正确
- [ ] 点击 UI 按钮不会误触发放置/选中
- [ ] 肖像栏可通过箭头或滑动滚动
- [ ] 移动端不显示 ImGui 调试窗
- [ ] PC 鼠标键盘流程不受影响

---

## 附录：审核结论与修订记录

- Claude Code 审核结论：方案整体成立、方向正确，事实引用基本准确；但有 7 处缺口必须补齐。
- 已吸收的关键修订：
  1. 关闭/屏蔽 `SDL_HINT_TOUCH_MOUSE_EVENTS`，防止双重输入
  2. 补全 FINGER 坐标转换链路
  3. 增加 UI 点击消费机制
  4. 增加交互模式状态机
  5. 移动端隐藏 ImGui
  6. 撤退按钮按 `cost * 0.5` 计算返还
  7. 明确 `isTouchDevice()` 检测方式

---

## 实施记录（2026-09-01）

- `3a81c40` Android 触摸输入层 + 移动端操作栏
- `b4b2ec3` MobileActionBar 费用/冷却显示 + 禁用不可用按钮
- `6ba0799` 交互模式状态机 NONE / PLACING / SELECTING
- `cc45e4f` 放置模式增加取消放置按钮
- `f9767a0` 移动端全局 HUD 增加暂停/倍速按钮
- `fe625ef` 操作栏按钮禁用时文字置灰

当前原生 Windows 构建通过；WASM 构建验证在本环境超时，待后续执行。
