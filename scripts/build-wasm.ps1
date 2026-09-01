# ============================================================
# build-wasm.ps1 — 用 Emscripten 把 MonsterWar 编译为 WebAssembly
#
# 前置：已安装 Emscripten SDK（emsdk install latest + emsdk activate latest）
# 产物：web/games/monsterwar/wasm/monsterwar.js + .wasm
#
# 用法：powershell -ExecutionPolicy Bypass -File scripts/build-wasm.ps1 [-Emsdk "C:\emsdk"]
# ============================================================
param(
    [string]$Emsdk = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

# 1. 定位并激活 Emscripten 环境（emcmake 不在 PATH 时）
if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
    if (-not $Emsdk) {
        $candidates = @(
            "$env:EMSDK",
            "$HOME\emsdk",
            "$env:USERPROFILE\emsdk",
            "C:\emsdk",
            "D:\emsdk"
        ) | Where-Object { $_ }
        foreach ($c in $candidates) {
            if (Test-Path "$c\emsdk_env.ps1") { $Emsdk = $c; break }
        }
    }
    if (-not $Emsdk -or -not (Test-Path "$Emsdk\emsdk_env.ps1")) {
        throw "未找到 Emscripten SDK。请先安装（https://emscripten.org/docs/getting_started/downloads.html）并 emsdk activate latest，或用 -Emsdk 指定路径。"
    }
    Write-Host "应用 Emscripten SDK 环境: $Emsdk"
    # 新版 emsdk_env.ps1 只打印 construct_env 输出而不实际应用环境变量，emcmake 不会进 PATH。
    # 这里直接从 emsdk 目录结构计算所需环境（上游 emscripten + 最新版 node/python）。
    $env:EMSDK = $Emsdk
    $nodeDir = Get-ChildItem "$Emsdk\node" -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
    $pyDir   = Get-ChildItem "$Emsdk\python" -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
    if ($nodeDir) { $env:EMSDK_NODE = Join-Path $nodeDir.FullName "node.exe" }
    if ($pyDir)   { $env:EMSDK_PYTHON = Join-Path $pyDir.FullName "python.exe" }
    $env:PATH = "$Emsdk;$Emsdk\upstream\emscripten;$env:PATH"
}

# 2. 校验工具链已安装（未 install/activate 时 emcc 不在 PATH）
if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
    throw "emcmake 不可用。请先执行: emsdk install latest  &&  emsdk activate latest"
}

# 3. 配置 + 构建
Set-Location $Root
Write-Host "==> emcmake 配置 (build-wasm) ..."
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败 (exit $LASTEXITCODE)" }

Write-Host "==> 构建 ..."
cmake --build build-wasm --config Release
if ($LASTEXITCODE -ne 0) { throw "构建失败 (exit $LASTEXITCODE)" }

# 4. 汇总产物
$wasmDir = Join-Path $Root "web\games\monsterwar\wasm"
Write-Host ""
Write-Host "构建完成，产物："
Get-ChildItem $wasmDir -ErrorAction SilentlyContinue | Select-Object Name, @{N="SizeMB";E={[math]::Round($_.Length/1MB,1)}}
