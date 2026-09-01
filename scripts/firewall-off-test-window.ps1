# ============================================================
# firewall-off-test-window.ps1 — 临时关闭 Public 防火墙做连接测试
# 用途：验证 Android 手机无法访问本地 8000 端口是否由防火墙/火绒引起。
#       开启一个 240 秒测试窗口：防火墙关闭 → 手机测试 → 自动恢复。
# 用法：powershell -ExecutionPolicy Bypass -File scripts/firewall-off-test-window.ps1
# 说明：非管理员运行时自动弹出 UAC 提权窗口，点击"是"即可。
#       无论测试结果如何，240 秒后必定自动重新开启防火墙，不会长期暴露。
# ============================================================

$testWindowSeconds = 240

# 检查是否已是管理员，否则自我提权后重跑
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "请求管理员权限以临时关闭防火墙..."
    Start-Process powershell -Verb RunAs -ArgumentList `
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`""
    exit
}

$before = (netsh advfirewall show publicprofile state | Select-String "State").ToString()
Write-Host "关闭前: $before"

netsh advfirewall set publicprofile state off
Write-Host ""
Write-Host "======================================================"
Write-Host " Public 防火墙已临时关闭 ($testWindowSeconds 秒测试窗口)"
Write-Host " 请在手机上立即测试:"
Write-Host "   http://192.168.35.19:8000/games/monsterwar/"
Write-Host " 测试完成后本窗口会自动恢复防火墙并显示结果。"
Write-Host "======================================================"
Write-Host ""

for ($i = $testWindowSeconds; $i -ge 1; $i -= 30) {
    Write-Host "距自动恢复还有 $i 秒 ..."
    if ($i -le 30) {
        Start-Sleep -Seconds $i
    } else {
        Start-Sleep -Seconds 30
    }
}

netsh advfirewall set publicprofile state on
$after = (netsh advfirewall show publicprofile state | Select-String "State").ToString()
Write-Host ""
Write-Host "已恢复: $after"
if ($after -match "on") {
    Write-Host "OK: 防火墙已重新开启, 测试窗口结束。"
} else {
    Write-Host "警告: 防火墙未能恢复, 请手动检查!"
}
