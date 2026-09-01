# ============================================================
# allow-port-8000.ps1 — 放行 Windows 防火墙入站 TCP 8000
# 用途：本地开发服务器(localhost:8000)允许 Android 手机通过局域网访问
# 用法：powershell -ExecutionPolicy Bypass -File scripts/allow-port-8000.ps1
# 说明：非管理员运行时会自动弹出 UAC 提权窗口，点击"是"即可。
# ============================================================

$ruleName = "MonsterWar Dev 8000"

# 检查是否已是管理员，否则自我提权后重跑
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "请求管理员权限以添加防火墙规则..."
    Start-Process powershell -Verb RunAs -ArgumentList `
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`""
    exit
}

# 删除旧规则再添加（幂等）
netsh advfirewall firewall delete rule name=$ruleName | Out-Null
netsh advfirewall firewall add rule name=$ruleName dir=in action=allow protocol=TCP localport=8000
if ($LASTEXITCODE -eq 0) {
    Write-Host "OK: 已放行入站 TCP 8000 ($ruleName)"
} else {
    Write-Host "失败: netsh 返回 $LASTEXITCODE"
}
