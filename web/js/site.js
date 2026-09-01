// 门户前端：projects.json 驱动项目卡片 + 流程图列表
// 新增项目 = projects.json 加一条 + 对应子目录，本文件零改动
(function () {
  'use strict';

  var grid = document.getElementById('projects');
  var diagramSection = document.getElementById('diagrams');
  var projects = [];

  // 自动检测当前设备：触屏优先判定移动端；此逻辑与游戏壳 game.js 保持一致，
  // 后续新增平台只需在此扩展并给项目打对应平台标签。
  var isMobile =
    /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) ||
    ('ontouchstart' in window) ||
    navigator.maxTouchPoints > 0;
  var current = isMobile ? 'mobile' : 'desktop';

  // 简单 HTML 转义（projects.json 为站方维护的受信数据，这里仅作兜底）
  function esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
  }

  function renderProjects() {
    // 只显示当前设备可运行的项目（多平台项目两端都显示）
    var list = projects.filter(function (p) {
      return (p.platforms || []).indexOf(current) !== -1;
    });

    grid.innerHTML = '';
    if (!list.length) {
      grid.innerHTML = '<p class="error">当前设备暂无可运行的项目</p>';
      return;
    }
    list.forEach(function (p) {
      var card = document.createElement('a');
      card.className = 'project-card' + (p.featured ? ' featured' : '');
      card.href = p.href;
      var pf = (p.platforms || []).map(function (x) {
        return '<span>' + (x === 'mobile' ? '📱' : '🖥️') + '</span>';
      }).join('');
      var tags = (p.tags || []).map(function (t) {
        return '<span class="tag">' + esc(t) + '</span>';
      }).join('');
      card.innerHTML =
        '<div class="card-icon">' + (p.icon || '🎮') + '</div>' +
        '<div class="card-body">' +
          '<h2 class="card-name">' + esc(p.name) + '</h2>' +
          '<p class="card-desc">' + esc(p.desc || '') + '</p>' +
          '<div class="card-tags">' + tags + '</div>' +
        '</div>' +
        '<div class="card-platforms">' + pf + '</div>';
      grid.appendChild(card);
    });
  }

  function renderDiagrams() {
    // 只列当前设备可运行项目的流程图（与 renderProjects 同一过滤标准）
    var withDiagrams = projects.filter(function (p) {
      return (p.platforms || []).indexOf(current) !== -1 &&
        p.diagrams && p.diagrams.length;
    });
    if (!withDiagrams.length) {
      diagramSection.classList.add('hidden');
      return;
    }
    diagramSection.classList.remove('hidden');
    var html = '<h2>📐 架构与流程图</h2><div class="diagram-list">';
    withDiagrams.forEach(function (p) {
      p.diagrams.forEach(function (d) {
        html += '<a class="diagram-item" href="' + esc(d.file) + '" target="_blank" rel="noopener">' +
          esc(p.name) + ' · ' + esc(d.name) + '</a>';
      });
    });
    html += '</div>';
    diagramSection.innerHTML = html;
  }

  fetch('projects.json')
    .then(function (res) { return res.json(); })
    .then(function (data) {
      projects = data.projects || [];
      renderProjects();
      renderDiagrams();
    })
    .catch(function () {
      grid.innerHTML = '<p class="error">加载项目列表失败，请刷新重试</p>';
    });
})();
