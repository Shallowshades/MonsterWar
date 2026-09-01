# game.duckboobee.com 部署记录

> 部署完成时间：2026-09-01
> 本文记录**实际执行过程、踩到的坑与解决办法、服务器现状、后续修改指南**，供后续改动时查询。
> 部署前方案见 `docs/web-deployment-plan.md`。

---

## 1. 部署结果

| 项 | 状态 |
|---|---|
| 域名 | `https://game.duckboobee.com`（http/https 双入口，http 直出不跳转） |
| 证书 | TrustAsia DV（云厂商免费证书），有效期 **2026-09-01 ~ 2026-11-30** |
| 站点根 | 服务器 `/usr/local/aurora-vue/game-site/`（blog/admin 的兄弟目录，零容器改动方案） |
| 游戏入口 | `https://game.duckboobee.com/games/monsterwar/` |
| 受影响站点 | 无。www.duckboobee.com（博客）、admin.duckboobee.com 全程正常 |

### 服务器侧文件布局
```
/usr/local/aurora-vue/game-site/          # 站点根（= 仓库 web/）
├── index.html / projects.json / css/ / js/
└── games/monsterwar/
    ├── index.html          # 游戏壳（wasm/monsterwar.js?v=5 + js/game.js?v=10）
    ├── wasm/monsterwar.js (222KB) + monsterwar.wasm (21.5MB)
    ├── css/ js/
    └── diagrams/           # 8 个流程图 HTML

/usr/local/nginx/cert/game.duckboobee.com_bundle.pem + .key   # 证书
/usr/local/nginx/nginx.conf                                     # nginx 配置（末尾追加 game 两个 server 块）
```

---

## 2. 部署流程（实际执行）

### 2.1 DNS
用户在域名服务商控制台添加 A 记录：`game → 110.40.223.66`。验证：
```bash
nslookup game.duckboobee.com   # 应返回 110.40.223.66
```

### 2.2 SSH 访问
本机公钥 `~/.ssh/id_ed25519.pub` 已加入服务器 `root` 的 `authorized_keys`，之后全程免密：
```bash
ssh root@110.40.223.66
```

### 2.3 上传站点（tar 流式，避免大量小文件逐个 scp）
```bash
tar czf - -C web . | ssh root@110.40.223.66 'mkdir -p /usr/local/aurora-vue/game-site && tar xzf - -C /usr/local/aurora-vue/game-site'
du -sh /usr/local/aurora-vue/game-site   # 27M
```

### 2.4 证书
1. 等云厂商证书期间，先生成**自签占位证书**（让 https 立即可用，浏览器有安全提示）：
   ```bash
   openssl req -x509 -newkey rsa:2048 -nodes \
     -keyout /usr/local/nginx/cert/game.duckboobee.com.key \
     -out /usr/local/nginx/cert/game.duckboobee.com_bundle.pem -days 365 \
     -subj "/C=CN/.../CN=game.duckboobee.com"
   ```
2. 用户从云厂商下载 **Nginx 格式** 证书 zip，本地解压后覆盖上传：
   ```bash
   scp game.duckboobee.com_bundle.pem game.duckboobee.com.key root@110.40.223.66:/usr/local/nginx/cert/
   docker exec nginx openssl x509 -in /etc/nginx/cert/game.duckboobee.com_bundle.pem -noout -subject -dates
   ```

### 2.5 nginx 配置
本地写好两个 server 块文件（可留档复用）：
- `scripts/nginx-game-http.conf` — 80 端口直出 + wasm immutable 缓存
- `scripts/nginx-game-https.conf` — 443 ssl，镜像现有 www/admin 的 ssl 写法

**关键：必须放进容器实际读取的文件，并加在 http{} 内部**（详见第 3 节问题 4/5）。最终执行：
```bash
# 备份容器当前配置
docker exec nginx cp /etc/nginx/nginx.conf /etc/nginx/nginx.conf.bak.pre-game

# 把宿主机最新配置写入容器（绕过 bind mount 旧 inode 问题）
cat /usr/local/nginx/nginx.conf | docker exec -i nginx sh -c "cat > /etc/nginx/nginx.conf"

# 验证 + 重载
docker exec nginx nginx -t
docker exec nginx nginx -s reload
```

### 2.6 验证
```bash
curl -s -o /dev/null -w "%{http_code}\n" https://game.duckboobee.com/                  # 200
curl -s -o /dev/null -w "%{http_code}\n" https://game.duckboobee.com/games/monsterwar/ # 200
curl -s -D - -o /dev/null https://game.duckboobee.com/games/monsterwar/wasm/monsterwar.wasm \
  | grep -iE "content-type|cache-control"    # application/wasm + immutable
curl -s -o /dev/null -w "%{http_code}\n" https://www.duckboobee.com/  # 200（博客未受影响）
curl -s -o /dev/null -w "%{http_code}\n" https://admin.duckboobee.com/ # 200
```

---

## 3. 遇到的问题与解决

### 问题 1：宿主机 nginx.conf 与容器实际加载的配置不一致（最重要的坑）
- **症状**：追加 game server 块后 `docker exec nginx nginx -t` 通过，但 `http://game` 返回 **301 跳 https**、`https://game` 返回的是 **blog 内容（906B）**。
- **根因**：`/usr/local/nginx/nginx.conf` 是**单文件 bind mount**（`-v /usr/local/nginx/nginx.conf:/etc/nginx/nginx.conf`）。宿主机文件中途被**替换过（换了 inode）**，Docker 的 bind mount 仍指向**旧 inode**。结果：容器一直读旧文件，宿主机文件的任何修改 reload 都不生效。容器跑的是旧版配置（80 块带裸 `rewrite ^(.*)$ https://$host$1 permanent;` → 所以 http 请求全 301；game 块不存在 → https 落到第一个 443 块(www) 出 blog 内容）。
- **诊断手段**：
  ```bash
  docker exec nginx nginx -T                      # 导出"实际生效"配置，发现无 game、rewrite 生效
  docker exec nginx grep -c game.duckboobee.com /etc/nginx/nginx.conf   # = 0
  docker exec nginx wc -l /etc/nginx/nginx.conf   # 94 行 ≠ 宿主机 115 行 → 确认是不同文件
  ```
- **解决（零停机）**：不重启容器（避免影响 blog），把配置**写进容器内**的文件再 reload：
  ```bash
  cat /usr/local/nginx/nginx.conf | docker exec -i nginx sh -c "cat > /etc/nginx/nginx.conf"
  docker exec nginx nginx -t && docker exec nginx nginx -s reload
  ```
- **遗留**：宿主机与容器文件是两个 inode，之后**编辑宿主机文件 reload 不生效**。根治方案：重建 nginx 容器（原命令见第 5 节），让 bind mount 重新绑定到宿主机当前文件——代价是 www/admin 闪断约 2-5 秒，需用户确认。

### 问题 2：宿主机 nginx.conf 原本就缺 http{} 闭合 `}`
- **症状**：最初用 `cat >>` 追加 game 块后 nginx -t 报 `"server" directive is not allowed here`。
- **根因**：宿主机 nginx.conf（115 行）**本身就没有 http{} 的闭合 `}`**——文件结尾是最后一个 server 块的 `}`，http 一直没关。这是个**预先存在的残缺文件**，因为容器不读它所以一直没暴露；**一旦直接重启/重建容器用这份文件，nginx 会起不来**。
- **解决**：`cp nginx.conf.bak.<时间戳>` 恢复备份 → `cat` 追加两个 game 块 → **末尾补一行 `}` 闭合 http**。`nginx -t` 验证通过。
- **教训**：追加 server 块必须放在 **http{} 内部**（最后一个 `}` 之前），不能直接 append 到文件末尾。

### 问题 3：`nginx -t` 测试时 include 相对路径解析
- **症状**：`nginx -t -c /tmp/xxx.conf` 报 `open() "/tmp/mime.types" failed`。
- **根因**：配置文件里的 `include mime.types;` 是相对路径，以**配置文件所在目录**解析；放 /tmp 就找不到。
- **解决**：把测试配置写到 `/etc/nginx/` 目录下再 `-t`（相对 include 解析到同目录的 mime.types），或加 `-p /etc/nginx/`（本机验证 `-p` 不生效，用前者可靠）。

### 问题 4：SSH host key changed（服务器重建过）
- **症状**：`REMOTE HOST IDENTIFICATION HAS CHANGED`，ssh 拒绝连接。
- **根因**：服务器被重装/重建过，主机密钥换新，本机 known_hosts 还是旧的。
- **解决**：`ssh-keyscan -t ed25519 110.40.223.66` 拿当前指纹，用户确认是同一台机后 `ssh-keygen -R 110.40.223.66` 清旧条目，再用 `StrictHostKeyChecking=accept-new` 连接。

### 问题 5：PowerShell 解压证书 zip 失败
- **症状**：bash 里嵌套 PowerShell 执行 `Expand-Archive -DestinationPath "$env:TEMP\game-cert"` 报 `The path ':TEMP\game-cert' ... not a valid file system path`。
- **根因**：外层 bash 先把 `$env:TEMP` 当 bash 变量展开成空。
- **解决**：用硬编码路径 `C:\Users\29230\AppData\Local\Temp\game-cert`。

### 问题 6（前置阶段）：手机连不上本地开发服务器（火绒）
- 火绒"网络入侵拦截"自动给 python.exe 添加入站 **Block** 防火墙规则，程序级 Block 优先级高于端口放行 → 手机访问超时。
- 本机 netsh **不支持 `action=` 过滤参数**（报 `'action' is not a valid argument`），`delete rule ... action=block` 静默失败。
- 解决：`netsh advfirewall firewall delete rule name=python.exe`（纯名字过滤）。详见 `scripts/unblock-python-firewall.ps1` 与记忆 `dev-env-firewall-huorong.md`。

---

## 4. 服务器现状速查

| 项 | 值 |
|---|---|
| 服务器 | 110.40.223.66，主机名 VM-4-14-centos，CentOS 7，SSH root 免密 |
| nginx | Docker 容器（名 `nginx`），`--privileged=true`，-p 80/443，`--restart=always` |
| 挂载 | `nginx.conf`(单文件)、`/usr/local/nginx/cert`、`/usr/local/aurora-vue` 三个 |
| nginx 版本 | 容器内 `nginx/1.31.3`，mime.types 自带 `application/wasm`（第 55 行，无需另加） |
| game server 块 | nginx.conf 末尾 http{} 内，共两个（80 + 443 ssl），wasm 加 `Cache-Control: public, max-age=31536000, immutable` |
| 证书 | `/usr/local/nginx/cert/game.duckboobee.com_bundle.pem` + `.key`，TrustAsia DV，**2026-11-30 到期** |

---

## 5. 后续修改指南

### 5.1 更新游戏（重建 wasm / 改 js / 换资源）
1. 本地 emcmake 重新构建 → 产物替换 `web/games/monsterwar/wasm/`。
2. **同步 bump 缓存版本号**（三者必须一起 +1，否则"旧胶水+新 wasm"报 ASM_CONSTS 错，或浏览器缓存旧 wasm 画面顶格）：
   - `web/games/monsterwar/index.html`：`wasm/monsterwar.js?v=N` 和 `js/game.js?v=N`
   - `web/games/monsterwar/js/game.js`：`locateFile` 里 `path + '?v=YYYYMMDDx'`
3. 上传：可全量 `tar czf - -C web . | ssh ...`（27M），或只传改动文件。
4. 验证：浏览器硬刷新（Ctrl+Shift+R）→ 游戏可玩 → 刷新页面确认存档保留（IDBFS）。

### 5.2 改 nginx 配置
1. 改宿主机 `/usr/local/nginx/nginx.conf`（**先 `cp` 备份**）。
2. **必须写进容器**（bind mount 旧 inode，reload 不读宿主机文件）：
   ```bash
   cat /usr/local/nginx/nginx.conf | docker exec -i nginx sh -c "cat > /etc/nginx/nginx.conf"
   docker exec nginx nginx -t   # 通过才继续
   docker exec nginx nginx -s reload
   ```
3. 验证 blog/admin/game 三域名均 200。

### 5.3 证书续期
- **2026-11-30 到期**。云厂商重新申请 → 下载 Nginx 格式 → 覆盖 `/usr/local/nginx/cert/game.duckboobee.com_bundle.pem` + `.key` → 执行 5.2 的第 2 步 reload（server 块路径不变，配置不用改）。

### 5.4 可选：根治 bind mount 旧 inode（重建容器）
- 当前 nginx 容器原 docker run 命令：
  ```
  docker run --name nginx --restart=always -p 443:443 -p 80:80 -d \
    -v /usr/local/nginx/nginx.conf:/etc/nginx/nginx.conf \
    -v /usr/local/nginx/cert:/etc/nginx/cert \
    -v /usr/local/aurora-vue:/usr/local/aurora-vue \
    --privileged=true nginx
  ```
- 重建后 bind mount 重新绑定到宿主机当前 nginx.conf，改配置 reload 直接生效。
- **注意**：重建会使 www/admin 闪断约 2-5 秒；重建前确保宿主机 nginx.conf 已修复完整（问题 2），否则容器起不来。需用户确认后执行。

---

## 6. 相关文件与记忆
- 方案：`docs/web-deployment-plan.md`
- server 块模板：`scripts/nginx-game-http.conf`、`scripts/nginx-game-https.conf`
- 防火墙脚本：`scripts/unblock-python-firewall.ps1`、`scripts/delete-python-block-rules.ps1`
- 记忆：`feedback-no-touch-blog.md`（绝不影响博客）、`project-server-duckboobee-ops.md`（服务器运维要点）、`dev-env-firewall-huorong.md`（火绒防火墙）
