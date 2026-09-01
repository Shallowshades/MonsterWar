# 015 Android 触屏适配 - 当前状态与后续指引

> 记录时间：2026-09-01
> 分支：`feature-web`
> 目的：方便后续继续接手，避免重新排查已解决的问题。

---

## 1. 当前进展

### 已实现功能

- InputManager 增加触摸状态接口（`isTouchDevice` / `isTouchActive` / `getTouchPosition` / `getTouchStartPosition`）
- UI 点击消费机制（`setUiHitTester` / `isClickConsumed` / `consumeNextClick`）
- 放置单位支持触摸：点肖像 → 点地图放置，点空白/取消按钮取消
- 选中单位支持触摸：点单位选中，点空白取消
- 交互模式状态机：`NONE / PLACING / SELECTING`
- 移动端操作栏 `MobileActionBar`：升级 / 撤退 / 技能 / 取消
- 操作栏显示费用、冷却、禁用态（文字置灰）
- 放置模式取消按钮
- 移动端全局 HUD：暂停/继续、1x/2x 倍速
- 肖像栏左右箭头
- 移动端隐藏战斗内 ImGui
- 原生 Windows 构建通过

### 已部署线上

```
https://game.duckboobee.com/games/monsterwar/
```

当前缓存版本：
- `monsterwar.js?v=8`
- `game.js?v=14`
- wasm `?v=20260901h`

---

## 2. 已解决问题记录

### 问题 1：移动端完全无法点击
- 原因：曾关闭 `SDL_HINT_TOUCH_MOUSE_EVENTS` 并自行处理 FINGER，但 WASM 触摸没有可靠进入游戏。
- 解决：恢复 `SDL_HINT_TOUCH_MOUSE_EVENTS=1`，FINGER 处理改为 no-op，由 SDL 合成鼠标事件。
- 相关提交：`1635e0b`

### 问题 2：战斗内角色无法点击、无法放置
- 原因 1：ImGui 旧 `WantCaptureMouse=true` 导致 InputManager 丢弃所有鼠标事件。
  - 解决：触摸设备上忽略 ImGui 捕获拦截。
- 原因 2：UIButton 依赖 hover 状态，快速触摸在同一帧按下+抬起时点不到。
  - 解决：`UINormalState::update` 直接捕获同帧 `mouse_left` 释放并触发点击。
- 相关提交：`067d04a`

### 问题 3：手机画面比例不对
- 原因：`fitCanvas()` 太早执行，读到 canvas 默认尺寸 300×150，按错误比例缩放。
- 解决：`fitCanvas()` 等待真实 canvas 尺寸（<1000 时重试，最多 30 帧）。
- 相关提交：`7874e4d`
- **待用户验证**：横屏/竖屏是否已正常。

---

## 3. 后续继续时需要知道的事

### 3.1 WASM 构建

在 Git Bash 中 `emsdk_env.sh` 可能没有把 emcc 加进 PATH，需要手动指定：

```bash
export PATH="$HOME/emsdk/upstream/emscripten:$HOME/emsdk:$PATH"
cmake --build build-wasm --config Release
```

产物会自动同步到：

```
web/games/monsterwar/wasm/monsterwar.js
web/games/monsterwar/wasm/monsterwar.wasm
```

### 3.2 修改前端 JS/CSS 后

- 改 `web/games/monsterwar/js/game.js` 后要 bump：
  - `web/games/monsterwar/index.html` 里 `js/game.js?v=N`
- 改 wasm 后要 bump：
  - `index.html` 里 `wasm/monsterwar.js?v=N`
  - `game.js` 里 `?v=YYYYMMDDx`
- 然后重新部署。

### 3.3 远程部署

```bash
tar czf - -C web . | ssh root@110.40.223.66 \
  'mkdir -p /usr/local/aurora-vue/game-site && tar xzf - -C /usr/local/aurora-vue/game-site'
```

### 3.4 验证地址

```
https://game.duckboobee.com/games/monsterwar/
```

---

## 4. 当前待办 / 下一步

- [ ] 用户真机验证最新比例修复（game.js?v=14）
- [ ] 如果比例仍不对，收集手机 Console 中 `[fitCanvas]` 日志（gw/gh/scale）
- [ ] 确认点击/放置/升级/技能/撤退在真机全部可用
- [ ] 可清理不再使用的 FINGER 相关字段（`mActiveFingerId` 等）
- [ ] 可考虑竖屏自动旋转提示或竖屏裁剪方案

---

## 5. 相关文档

- 方案：`docs/notes/014-android-touch-controls.md`
- 部署记录：`docs/web-deployment-record.md`
- 部署计划：`docs/web-deployment-plan.md`
