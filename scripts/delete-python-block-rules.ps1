# ============================================================
# delete-python-block-rules.ps1 — 真正删除 python.exe 阻止规则
# 关键修正：本机 netsh 不支持 show/delete rule 的 action= 过滤参数,
#           会报 "'action' is not a valid argument"。因此这里用纯名字过滤。
# 用法：powershell -ExecutionPolicy Bypass -File scripts/delete-python-block-rules.ps1
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
Write-Host " 删除前 python.exe 相关规则:"
netsh advfirewall firewall show rule name=python.exe

Write-Host "`n执行删除 (delete rule name=python.exe):"
netsh advfirewall firewall delete rule name=python.exe

Write-Host "`n删除后 python.exe 规则:"
netsh advfirewall firewall show rule name=python.exe

Write-Host "`n(注意: 单独命名的 'python.exe Allow Dev' 放行规则不受影响)"
netsh advfirewall firewall show rule name="python.exe Allow Dev" | Select-String "Rule Name|Action"

Write-Host "`n请先在手机上测试: http://192.168.35.19:8000/games/monsterwar/"
Write-Host ""
pause
