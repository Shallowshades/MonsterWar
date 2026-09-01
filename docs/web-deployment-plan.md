# game.duckboobee.com 游戏门户 + MonsterWar WebAssembly 部署方案

> 落盘日期：2026-08-31 · 分支：feature-web · 状态：已批准，实施中
> 原始计划：`C:\Users\29230\.claude\plans\duckboobee-com-www-duckboobee-com-admin-fluffy-wilkinson.md`

## 背景（Context）

云服务器 110.40.223.66（宝塔面板，nginx 以 Docker 单文件挂载运行）现有站点：
- `www.duckboobee.com` — 博客（Aurora：SpringBoot 后端 8080 + Vue 前端 blog）
- `admin.duckboobee.com` — 后台管理（Vue admin）

目标：新增 `game.duckboobee.com` 作为**多项目游戏门户**（MonsterWar 是第一个项目）：
1. 把本仓库 C++ 游戏（SDL3 + EnTT ECS + ImGui）用 Emscripten 编译为 WebAssembly，在浏览器运行
2. 在门户里展示 `docs/diagrams/` 下的架构/流程图
3. 不触碰现有部署（后端不可改；nginx 只追加新 server 块，不重建现有站点逻辑）
4. 门户设计成可扩展：以后加更多游戏/项目 = 加一个子目录 + projects.json 加一条

### 已确认决策（用户拍板）
- 博客跳转：**本期不做**（以后可改 aurora-blog Vue 导航栏）
- 存档/配置：**需要 IndexedDB (IDBFS) 持久化**
- 静态资源：**直接用服务器 nginx 服务**，接受首次加载慢（约 18MB，1M 带宽约 2 分钟，之后走浏览器缓存）
- 多平台：**暂定 Windows 桌面浏览器 + Android 移动浏览器**，保留扩展（iOS 等）

### 分支
当前在 **feature-web**（与 master 完全一致，0 ahead / 0 behind），所有改动直接提交到该分支，无需新建。

### 可行性结论（已探索验证）
游戏代码可移植性良好：
- 所有资源相对路径 `assets/...`，无绝对路径、无硬编码 Windows 路径
- 仅 `_WIN32` 守卫的 Windows.h（main.cpp）与 resources.rc（CMake 已按平台排除）
- 无线程/动态库/系统调用；渲染走 SDL3 Renderer 自动后端（wasm 下为 WebGL2/GLES2）
- 触摸输入：SDL3 Emscripten 默认把触摸映射为鼠标事件（SDL_HINT_TOUCH_MOUSE_EVENTS=1）→ 移动端点击复用现有 InputManager，无需改游戏逻辑

三大改造点（否则 wasm 必崩）：
1. **主循环**：`GameApp::run()` 阻塞 while → `emscripten_set_main_loop_arg` 每帧回调
2. **`std::filesystem::canonical`**（level_loader.cpp resolvePath）→ `lexically_normal()` 纯词法折叠
3. **浏览器音频 autoplay**：SDL3 有自动 resume，JS 壳再加一次 pointerdown 解锁兜底

依赖现状（已核实 `external/` 目录）：
- 本地已有：`SDL-release-3.4.2`、`SDL_mixer-release-3.2.0`、glm、json、spdlog、entt、imgui
- **缺：SDL_image-3.2.4、SDL_ttf-3.2.2** → 原生构建走 prebuilt（find_package 命中 COFF 库）
- wasm 构建**必须跳过 find_package/prebuilt**（COFF 无法被 emcc 链接），改用本地源码/入仓源码

## 目标架构

```
game.duckboobee.com (nginx root: /usr/local/game-site)
├─ index.html              门户：项目卡片（渲染自 projects.json）
├─ projects.json           项目清单（新增项目 = 加一条 JSON + 一个子目录）
├─ css/ js/ img/           门户共享资源
└─ games/monsterwar/       第一个项目
   ├─ index.html           游戏壳（canvas + 加载屏 + 返回门户 + 全屏）
   ├─ wasm/monsterwar.js + .wasm   Emscripten 产物
   ├─ css/ js/             游戏壳样式与引导（平台检测、IDBFS、音频解锁）
   └─ diagrams/            MonsterWar 流程图（3 个 archify 独立 HTML）
```

仓库内新增 `web/` 目录即站点根，可整目录上传部署。

## 实施阶段

### 阶段 0：方案落盘
本文件即方案存档。

### 阶段 1：Emscripten 工具链 + 依赖准备
1. 安装 Emscripten SDK（emsdk install latest / 3.1.5x+，SDL3 需要较新版本）
2. 把 SDL_image、SDL_ttf 源码补进 `external/`（wasm 构建需要；网络受限，避免 FetchContent 在线拉取）。沿用既有做法（SDL_mixer 也是手动补齐子模块）：
   - `git clone --branch release-3.2.4 --recursive https://github.com/libsdl-org/SDL_image.git external/SDL_image-release-3.2.4`
   - `git clone --branch release-3.2.2 --recursive https://github.com/libsdl-org/SDL_ttf.git external/SDL_ttf-release-3.2.2`
   - 网络受限则用 github archive 端点 + `curl --retry`，再手动补子模块（libpng / freetype）
3. 核对 CMakeLists 的 `LOCAL_PATH` 与真实目录一致（SDL-release-3.4.2、SDL_mixer-release-3.2.0、SDL_image-release-3.2.4、SDL_ttf-release-3.2.2），否则会意外走 FetchContent

### 阶段 2：CMakeLists 加 EMSCRIPTEN 分支
1. `find_or_fetch_dependency` 宏内：
   - EMSCRIPTEN 下强制 `_LIB_IS_SHARED=OFF`（wasm 只能静态）
   - EMSCRIPTEN 下跳过 find_package（prebuilt 是 COFF，绝不能命中）
   - `set(CMAKE_PREFIX_PATH .../prebuilt)` 用 `if(NOT EMSCRIPTEN)` 包起来
2. `add_executable` 之后加 `if(EMSCRIPTEN)` 块：
   - `SDLIMAGE_VENDORED / SDLMIXER_VENDORED / SDLTTF_VENDORED` 置 ON（解码器用仓库 vendored 源码）
   - 缩减解码器：`SDLMIXER_MP3_MPG123 OFF`（用 dr_mp3）、`SDLMIXER_VORBIS_VORBISFILE OFF`（用 stb_vorbis），砍掉 FLAC/MOD/OPUS/MIDI/GME
   - emcc 链接参数：
     ```
     -O3
     -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2
     -sALLOW_MEMORY_GROWTH=1
     -sINITIAL_MEMORY=134217728
     -sMODULARIZE=1 -sEXPORT_NAME=createMonsterWar
     -sEXPORTED_RUNTIME_METHODS=[FS,IDBFS,callMain]
     -sFORCE_FILESYSTEM=1
     -sINVOKE_RUN=0
     -sDISABLE_EXCEPTION_CATCHING=0
     --embed-file ${CMAKE_SOURCE_DIR}/assets@/assets
     ```
   - POST_BUILD 把产物归位到 `web/games/monsterwar/wasm/monsterwar.js` / `.wasm`
3. `copy_assets` / `copy_imgui_ini` 的 PRE_BUILD 用 `if(NOT EMSCRIPTEN)` 包起来

**embed-file 而非 preload-file**：游戏全部同步 `std::ifstream` 读取；embed 在 `createMonsterWar()` resolve 后 FS 立即完整可用，与 IDBFS 引导顺序无竞态（代价是 .wasm 约 20MB）。preload 备选，但持久化竞态难处理。

### 阶段 3：源码最小改动（wasm 相关均 `#ifdef __EMSCRIPTEN__` 内或可移植化）
1. `src/engine/core/game_app.h` / `game_app.cpp`：
   - 抽 `frame()`（桌面循环体 / wasm 回调共用）
   - `run()`：wasm 下 `emscripten_set_main_loop_arg(cb, this, 0, 1)`（第三参必须 1，否则 main 返回后 GameApp 析构、回调悬垂）；桌面保持 while + `close()`
   - `frame()` 尾部 wasm 下检测 `!mIsRunning` → `emscripten_cancel_main_loop()` + `close()`
   - `initConfig()`：wasm 下路径重定向到 `assets/save/config.json`（写进 IDBFS 持久化目录）
   - `initImGui()`：wasm 下 `io.IniFilename = nullptr`
2. `src/engine/loader/level_loader.cpp` resolvePath：`canonical` → `lexically_normal()`（wasm libc++ 不支持 canonical；原生保留 canonical 并顺手修正 fallback 返回未折叠路径的 bug）
3. `src/main.cpp`：开头加 `SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1")`
4. 音频兜底（可选增强）：AudioManager 设备创建失败不 throw，延后到用户交互后重试；JS 壳同时做 `audioContext.resume()`

### 阶段 4：web/ 门户 + 游戏壳（静态站点）
1. `web/projects.json`：MonsterWar 条目（id/name/desc/icon/href/platforms:["desktop","mobile"]/featured）
2. `web/index.html` + `js/site.js` + `css/site.css`：fetch projects.json → 渲染卡片网格；按平台过滤；新增项目零代码改动
3. `web/games/monsterwar/index.html`：
   - `<canvas id="canvas">`（SDL3 默认选择器，硬要求）
   - viewport meta（禁缩放、适配刘海屏）、加载屏、顶部栏（返回门户 + 全屏按钮）
4. `web/games/monsterwar/css/game.css`：响应式——canvas `100vw/100vh`，SDL LETTERBOX 自动黑边+坐标映射，无需 JS 算宽高比；`touch-action:none` 防滚动；`body.mobile` 适配
5. `web/games/monsterwar/js/game.js`：
   - 平台检测 `isMobile`（UA + ontouchstart + maxTouchPoints），body 加 `.mobile`，扩展点收敛于此
   - 首次 pointerdown/keydown 解锁音频（`Module.SDL3.audioContext.resume()`）
   - `boot()`：`await createMonsterWar()` → `FS.mkdir('/assets/save')` + `FS.mount(IDBFS,{},'/assets/save')` → `await syncfs(true)` 回灌存档 → 播种默认 config → `Module.callMain()`
   - 持久化回写：visibilitychange / pagehide / 每 5s 定时调 `FS.syncfs(false)`
   - 全屏按钮（移动端优先显示）
6. `web/games/monsterwar/diagrams/`：从 `docs/diagrams/` 拷贝 3 个独立 HTML（runtime-architecture / frame-sequence / unit-lifecycle）。注意 skill-workflow 目前只有 .json 源、无 .html，本期不包含
7. `scripts/build-wasm.ps1`：emsdk_env + `emcmake cmake -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release` + `cmake --build build-wasm`

### 阶段 5：本地构建 + 浏览器验证
1. 跑 `scripts/build-wasm.ps1` → 检查 `web/games/monsterwar/wasm/` 产物
2. 本地静态服务器（`python -m http.server`）起 `web/` 根
3. 桌面浏览器（Chrome/Edge）：标题场景 → 开始游戏 → 施放技能 → 通关/结束场景；**刷新页面验证存档保留（IDBFS）**
4. Android Chrome / DevTools 设备模拟：触屏点击、canvas 缩放、无键盘场景
5. PNG 纹理冒烟验证 SDL_image 解码；中文场景验证 SDL_ttf/freetype

### 阶段 6：服务器部署（不影响现有站点）
1. **DNS**：`game.duckboobee.com` 加 A 记录 → `110.40.223.66`（已备案主域名下的子域名，无需新备案）
2. **SSL 证书**：为 game.duckboobee.com 申请（宝塔面板 SSL / 阿里云·腾讯免费 DV 证书），放 `/usr/local/nginx/cert/game.duckboobee.com_bundle.pem` + `.key`（沿用现有命名）
3. **上传**：web/ 内容 → 服务器 `/usr/local/game-site/`
4. **nginx 容器访问 game-site**：按部署文档第 788 行原 `docker run` 命令重建容器，追加 `-v /usr/local/game-site:/usr/local/game-site` 挂载。恢复手段：保留原命令，出错即 `docker rm` + 原命令重跑。
   - 备选（零容器改动）：把文件放 `/usr/local/aurora-vue/game-site`（已挂载，但注意博客重部署可能覆盖该目录）
5. **nginx.conf 追加 server 块**（改前备份 nginx.conf）：
   - 80 → 301 到 https；443 ssl，`server_name game.duckboobee.com`，`root /usr/local/game-site`，`index index.html`
   - `types { application/wasm wasm; }`（老 nginx 需手动补 MIME）
   - wasm/js 加 `Cache-Control "public, max-age=31536000, immutable"`
6. **生效**：`docker exec nginx nginx -t` → `docker exec nginx nginx -s reload`（reload 不断现有连接，www/admin 不受影响）

## 验证清单（端到端）
- [ ] 本地 build-wasm 成功，产物在 web/games/monsterwar/wasm/
- [ ] 门户首页可渲染 MonsterWar 卡片
- [ ] 桌面浏览器游戏可玩（完整流程），刷新后存档保留
- [ ] Android 触屏可玩、canvas 自适应
- [ ] 流程图 3 个 HTML 可访问
- [ ] 服务器 `nginx -t` 通过，www / admin / game 三域名均正常
- [ ] 刷新不丢档（IDBFS syncfs 生效）

## 风险与缓解
1. **SDL_image PNG 解码（最高风险）**：必须 vendored ON + 本地源码含 libpng 子模块；构建后用 PNG 冒烟
2. **imgui_impl_sdlrenderer3 在 wasm 的兼容性**：理论可用（走 RenderGeometry→WebGL2）；首帧验证，出问题换 imgui_impl_opengl3
3. **WebGL2 可用性**：`-sMIN_WEBGL_VERSION=2` 放弃极老设备，可接受
4. **simulate_infinite_loop 必须为 1**，否则 main 返回后悬垂崩溃
5. **-sEXPORTED_RUNTIME_METHODS 漏 FS/IDBFS** → bootstrap 里 `FS is not defined`；`-sINVOKE_RUN=0` 必须配 callMain 否则黑屏
6. **C++ 异常必须开着**（`-sDISABLE_EXCEPTION_CATCHING=0`），仓库全链路 throw/catch
7. **18MB 首屏加载慢**（已接受）；后续可 OSS+CDN 或 preload-file 优化
8. SDL3 未来升级 wasm 渲染后端可能变 WebGPU，本期锁 `release-3.4.2`

## 后续可扩展（本期不做）
- 博客跳转：改 aurora-blog Vue 导航栏加"游戏"入口，重新 npm build 后覆盖 /usr/local/aurora-vue/blog
- OSS + CDN 加速静态资源
- skill-workflow 流程图生成 HTML（用 archify）
- 更多项目：projects.json 加条目 + `web/games/<id>/` 子目录，门户 JS 零改动

## 当前进度（2026-08-31 记录）

| 阶段 | 状态 | 说明 |
| --- | --- | --- |
| 0 方案落盘 | ✅ 完成 | 本文件 |
| 1 工具链 + 依赖 | 🟡 进行中 | SDL_image-3.2.4 / SDL_ttf-3.2.2 源码已入 `external/`，libpng/zlib/freetype 子模块已补（github archive + curl --retry）。**剩余**：emsdk 已下载到 `C:\Users\29230\emsdk`，待执行 `emsdk install latest` + `emsdk activate latest` |
| 2 CMake 加 EMSCRIPTEN 分支 | ✅ 完成 | 详见 CMakeLists 各 `if(EMSCRIPTEN)` 块；**已修复** imgui.ini 块缺失 `if(NOT EMSCRIPTEN)` 开头的嵌套错误 |
| 3 源码最小改动 | ✅ 完成 | `game_app` 抽 `frame()` + wasm 主循环 + config 路径重定向到 `assets/save/` + touch hint + ImGui ini 禁用；`level_loader` canonical→lexically_normal（wasm）/保留 canonical（原生）+ 修 fallback 返回未折叠路径 bug；**已通过原生构建验证**（MonsterWar-Windows.exe 编译链接成功） |
| 4 web/ 门户 + 游戏壳 | ✅ 文件已建 | 见下方「已建文件」；**待浏览器验证**（依赖阶段 5） |
| 5 本地构建 + 浏览器验证 | 🟡 进行中 | ✅ emsdk 已装（Emscripten 6.0.8）；✅ `emcmake cmake -B build-wasm` + 构建成功，产物 21.5MB wasm + 222KB js 已归位 `web/games/monsterwar/wasm/`；**已修复** `--embed-file` 参数缺失 `=` 导致资产未内嵌的 bug（修后 wasm 3.4MB→21.5MB，PNG 签名 56 处 ≈55 张资产）；**已修复** 游戏画面不适配：SDL 的 letterbox 在 wasm GLES2 后端不缩放（游戏以原生 1280x720 顶在左上角），改为 wasm 窗口固定原生分辨率 + JS `fitCanvas()` 用 CSS transform 等比缩放居中（避开顶栏），已核实 SDL 鼠标坐标经 `getBoundingClientRect` 自动换算不受缩放影响；**已修复** `scripts/build-wasm.ps1` 在新版 emsdk 下 emcmake 找不到的问题（新版 emsdk_env.ps1 只打印不应用环境，脚本改为直接从 emsdk 目录结构计算 env）。⏳ 待浏览器验证（本地 http.server 已起 8000 端口） |
| 6 服务器部署 | ⏳ 待办 | DNS A 记录 / SSL 证书 / 上传 web/ / nginx server 块 |

### 已建文件（阶段 4）
```
web/projects.json                项目清单（含 8 张流程图条目）
web/index.html + css/site.css + js/site.js   门户（projects.json 驱动卡片 + 流程图区）
web/games/monsterwar/index.html  游戏壳（canvas#canvas + 加载屏 + 顶栏）
web/games/monsterwar/css/game.css
web/games/monsterwar/js/game.js  IDBFS 挂载 /assets/save + 音频解锁 + 定时持久化
web/games/monsterwar/diagrams/   8 个流程图 HTML（自 docs/diagrams 复制）
scripts/build-wasm.ps1           自动激活 emsdk → emcmake → build → 产物归位
```

### 与原始计划的差异（已核实后调整）
- `docs/diagrams/` 实际已有 **8 个**独立 HTML（不止计划里的 3 个，skill-workflow 也已生成 HTML），**全部**复制进 `web/games/monsterwar/diagrams/`
- SDL_image 解码器**只保留 PNG**（关 AVIF/JPG/TIF/WEBP），资产核实为 55 张 png；音频只留 wav/ogg/mp3（关其余）——均在 CMake EMSCRIPTEN 块配置
- SDL_ttf **只保留 freetype**（关 HARFBUZZ / PLUTOSVG），中文栅格化不需要复杂整形/彩色 emoji
- 存档路径核实：config → `assets/save/config.json`，游戏存档 → `assets/save/SLOT_*.json`，全部落在 IDBFS 挂载点 `/assets/save`；首次运行无档时 Config/SessionData 自动写默认，无需 JS 播种
- touch hint 放在 `game_app.cpp initSDL()`（SDL_Init 前），而非计划中的 main.cpp

### 下一步（阻塞项）
1. 浏览器打开 `http://localhost:8000/` 验证：门户卡片 → 游戏壳 → 标题场景 → 开始游戏 → 施放技能 → 通关/结束场景；**刷新页面验证存档保留（IDBFS）**；DevTools 设备模拟验证触屏
2. 验证 PNG 纹理（SDL_image 解码）与中文场景（SDL_ttf/freetype）
3. 全部通过后进入阶段 6：服务器部署（DNS / SSL / 上传 / nginx server 块）
