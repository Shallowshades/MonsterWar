// MonsterWar 浏览器引导壳：
//   平台检测 → 音频解锁 → 加载 wasm → IDBFS 挂载存档 → callMain 启动 → 定时持久化
(function () {
  'use strict';

  var $ = function (sel) { return document.querySelector(sel); };
  var canvas = $('#canvas');
  var loadingEl = $('#loading');
  var statusEl = $('#loading-status');
  var fullscreenBtn = $('#fullscreen-btn');

  // ---- 平台检测：后续平台扩展的收敛点（UA + 触屏能力） ----
  var isMobile =
    /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) ||
    ('ontouchstart' in window) ||
    navigator.maxTouchPoints > 0;
  if (isMobile) {
    document.body.classList.add('mobile');
  }

  // ---- 全屏 + 横屏锁定 ----
  // 游戏是 1600x1216 横屏，竖屏下只能居中缩成一小条。全屏时把屏幕锁为横屏，
  // 旋转后 resize 事件触发 fitCanvas 重新计算，游戏填满横屏。
  // Android Chrome 支持全屏内 screen.orientation.lock；iOS Safari 不支持（catch 兜底，仅全屏不旋转）。
  function toggleFullscreen() {
    if (document.fullscreenElement) {
      document.exitFullscreen().catch(function () {});
      if (screen.orientation && screen.orientation.unlock) {
        screen.orientation.unlock().catch(function () {});
      }
    } else {
      document.documentElement.requestFullscreen()
        .then(function () {
          if (screen.orientation && screen.orientation.lock) {
            screen.orientation.lock('landscape').catch(function () {});
          }
        })
        .catch(function () {});
    }
  }
  fullscreenBtn.addEventListener('click', toggleFullscreen);

  // ---- 浏览器音频 autoplay 解锁 ----
  // 首次交互时恢复 SDL3 的 AudioContext（SDL3 在 Emscripten 下暴露 Module.SDL3.audioContext）
  function resumeAudio() {
    if (window.Module && window.Module.SDL3 && window.Module.SDL3.audioContext) {
      var ctx = window.Module.SDL3.audioContext;
      if (ctx.state === 'suspended') {
        ctx.resume().catch(function () {});
      }
    }
  }
  document.addEventListener('pointerdown', resumeAudio);
  document.addEventListener('keydown', resumeAudio);

  // ---- 画面适配：canvas 固定为游戏原生分辨率，用 CSS transform 等比缩放并居中到视口 ----
  // 背景：SDL 的 letterbox（逻辑分辨率）在 wasm 的 GLES2 后端不缩放，游戏会以原生像素顶在左上角。
  // 因此保持 canvas 原生尺寸，由浏览器按 min(视口宽/游戏宽, 可用高/游戏高) 统一缩放。
  // 缩放后的鼠标坐标 SDL 会通过 getBoundingClientRect 自动换算，点击位置不受影响。
  // 注意：必须在 module.callMain() 之后调用——CreateWindow 时 SDL 会探测 canvas 的 CSS 尺寸，
  // 若当时已设宽高会误判为"外部接管尺寸"而覆盖窗口大小。
  function fitCanvas(attempt) {
    attempt = attempt || 0;
    var gw = canvas.width;
    var gh = canvas.height;
    if (!gw || !gh || (gw < 1000 && gh < 1000)) {
      // SDL 尚未设置 canvas 尺寸（如 callMain 尚未完成），下一帧重试
      // 注意：canvas 默认是 300x150，如果太早执行会把游戏按 300x150 缩放，导致比例错误
      if (attempt < 30) requestAnimationFrame(function () { fitCanvas(attempt + 1); });
      return;
    }
    try {
      var topbar = document.getElementById('topbar');
      var topbarH = topbar ? topbar.offsetHeight : 0;
      var availH = Math.max(window.innerHeight - topbarH, 1);
      var scale = Math.min(window.innerWidth / gw, availH / gh);
      var centerY = topbarH + availH / 2;   // 避开顶栏，在剩余区域垂直居中
      canvas.style.position = 'fixed';
      canvas.style.left = '50%';
      canvas.style.top = centerY + 'px';
      canvas.style.width = gw + 'px';
      canvas.style.height = gh + 'px';
      canvas.style.transform = 'translate(-50%, -50%) scale(' + scale + ')';
      canvas.style.transformOrigin = 'center';
      var r = canvas.getBoundingClientRect();
      console.log('[fitCanvas] gw', gw, 'gh', gh, 'scale', scale, 'centerY', centerY,
        '-> transform=', canvas.style.transform,
        'box=', Math.round(r.width) + 'x' + Math.round(r.height),
        '@' + Math.round(r.left) + ',' + Math.round(r.top));
    } catch (e) {
      console.error('[fitCanvas] 异常:', e);
    }
  }
  window.addEventListener('resize', fitCanvas);
  window.addEventListener('orientationchange', fitCanvas);

  // ---- 启动 ----
  function boot() {
    // createMonsterWar 由 wasm/monsterwar.js 暴露（MODULARIZE + EXPORT_NAME），返回 Promise<Module>
    if (typeof createMonsterWar !== 'function') {
      statusEl.textContent = '引擎脚本未加载，请刷新重试';
      return;
    }
    createMonsterWar({
      // 缓存破坏：21MB 的 monsterwar.wasm 极易被浏览器缓存成旧构建（症状：画面仍左上角顶格）。
      // 用构建标记做查询串强制重取（本地 http.server / 生产 nginx 均忽略查询串）。
      locateFile: function (path, prefix) {
        if (/\.wasm$/.test(path)) return prefix + path + '?v=20260901h';
        return prefix + path;
      }
    })
      .then(function (module) {
        window.Module = module;
        return mountSaveDir(module);
      })
      .then(function (module) {
        statusEl.textContent = '启动游戏...';
        module.callMain();          // -sINVOKE_RUN=0：必须手动调用，main() 内部注册 rAF 主循环
        fitCanvas();
        loadingEl.classList.add('hidden');
        return module;
      })
      .then(function (module) { schedulePersist(module); })
      .catch(function (err) {
        console.error('MonsterWar 启动失败:', err);
        statusEl.textContent = '启动失败：' + (err && err.message ? err.message : err);
      });
  }

  // ---- IDBFS：把 /assets/save 挂载到 IndexedDB，刷新页面不丢档 ----
  function mountSaveDir(module) {
    var FS = module.FS;
    statusEl.textContent = '准备存档...';
    try { FS.mkdir('/assets/save'); } catch (e) { /* 已存在则忽略 */ }
    FS.mount(module.IDBFS, {}, '/assets/save');
    return new Promise(function (resolve, reject) {
      // syncfs(true)：把 IndexedDB 里已持久化的存档回灌到内存 FS
      FS.syncfs(true, function (err) { return err ? reject(err) : resolve(module); });
    });
  }

  // ---- 持久化回写：定时 + 页面隐藏/退出 ----
  function schedulePersist(module) {
    var FS = module.FS;
    function persist() { FS.syncfs(false, function () {}); }
    setInterval(persist, 5000);                 // 每 5s 静默回写
    document.addEventListener('visibilitychange', function () {
      if (document.visibilityState === 'hidden') persist();
    });
    window.addEventListener('pagehide', persist);
  }

  boot();
})();
