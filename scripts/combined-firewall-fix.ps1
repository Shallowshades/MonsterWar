# ============================================================
# combined-firewall-fix.ps1 — 一键防火墙修复(最终版)
# 用途：
#   1) 确保 Public 防火墙开启
#   2) 删除火绒添加的 python.exe 入站 Block 规则
#   3) 添加 python.exe 入站 Allow 规则(兜底)
#   4) 展示最终状态并暂停, 窗口不会一闪而过
# 用法：powershell -ExecutionPolicy Bypass -File scripts/combined-firewall-fix.ps1
# 说明：非管理员时自动提权。请务必盯住屏幕, 点 UAC 的"是"。
# ============================================================

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "请求管理员权限..."
    Start-Process powershell -Verb RunAs -ArgumentList `
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`""
    exit
}

Write-Host "=============================================="
Write-Host " 开始执行防火墙修复"
Write-Host "=============================================="

# 1) 确保 Public 防火墙开启
netsh advfirewall set publicprofile state on | Out-Null
$state = (netsh advfirewall show publicprofile state | Select-String "State").ToString()
Write-Host "`n[1] 防火墙: $state"

# 2) 删除 python.exe 的入站 Block 规则
$ruleName = "python.exe"
$before = (netsh advfirewall firewall show rule name=$ruleName dir=in action=block | Select-String "Rule Name").Count
netsh advfirewall firewall delete rule name=$ruleName dir=in action=block
$after = (netsh advfirewall firewall show rule name=$ruleName dir=in action=block | Select-String "Rule Name").Count
Write-Host "[2] python.exe Block 规则: $before -> $after 条"

# 3) 添加 python.exe 入站 Allow 规则(兜底, 防 Block 卷土重来)
netsh advfirewall firewall add rule name="python.exe Allow Dev" dir=in action=allow program="C:\Users\29230\AppData\Local\Programs\Python\Python312\python.exe" enable=yes profile=any
Write-Host "[3] 已添加 python.exe Allow Dev 规则"

# 4) 确认 8000 端口放行规则仍在
$portAllow = (netsh advfirewall firewall show rule name="MonsterWar Dev 8000" | Select-String "Rule Name").Count
Write-Host "[4] MonsterWar Dev 8000 放行规则: $portAllow 条"

Write-Host "`n=============================================="
Write-Host " 完成! 请在手机上测试:"
Write-Host "   http://192.168.35.19:8000/games/monsterwar/"
Write-Host "=============================================="
Write-Host ""
pause
