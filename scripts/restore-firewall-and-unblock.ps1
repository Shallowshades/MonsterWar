# ============================================================
# restore-firewall-and-unblock.ps1 — 收尾脚本
# 用途：测试窗口结束后恢复防火墙，并删除火绒添加的 python.exe 阻止规则。
#       （前提：已在火绒中关闭"网络入侵拦截"，否则规则会被重新添加）
# 用法：powershell -ExecutionPolicy Bypass -File scripts/restore-firewall-and-unblock.ps1
# 说明：非管理员运行时自动弹出 UAC 提权窗口。
# ============================================================

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "请求管理员权限以恢复防火墙并删除阻止规则..."
    Start-Process powershell -Verb RunAs -ArgumentList `
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`""
    exit
}

# 1) 确保 Public 防火墙开启
netsh advfirewall set publicprofile state on | Out-Null
$state = (netsh advfirewall show publicprofile state | Select-String "State").ToString()
Write-Host "防火墙状态: $state"

# 2) 删除 python.exe 的入站 Block 规则（TCP + UDP）
$ruleName = "python.exe"
$before = (netsh advfirewall firewall show rule name=$ruleName dir=in action=block | Select-String "Rule Name").Count
netsh advfirewall firewall delete rule name=$ruleName dir=in action=block
$after = (netsh advfirewall firewall show rule name=$ruleName dir=in action=block | Select-String "Rule Name").Count
Write-Host "python.exe Block 规则: 删除前 $before 条, 删除后 $after 条"

# 3) 确认放行规则仍在
$allow = (netsh advfirewall firewall show rule name="MonsterWar Dev 8000" | Select-String "Rule Name").Count
Write-Host "MonsterWar Dev 8000 放行规则: $allow 条"

if ($after -eq 0 -and $allow -ge 1) {
    Write-Host "OK: 防火墙已开启, python 阻止规则已清空, 8000 放行规则在位。请在手机上再次测试。"
} else {
    Write-Host "警告: 有异常, 请检查上述输出。"
}
