# ============================================================
# unblock-python-firewall.ps1 — 删除 python.exe 的入站 Block 防火墙规则
# 用途：本地开发服务器(HTTP 8000)无法被局域网手机访问，原因是
#       Public 网络配置下存在两条 python.exe 的"阻止"入站规则
#       (火绒"网络入侵拦截"自动添加)。
# 用法：powershell -ExecutionPolicy Bypass -File scripts/unblock-python-firewall.ps1
# 说明：非管理员运行时自动弹出 UAC 提权窗口，点击"是"即可。
#       注意：本机 netsh 不支持 action= 过滤参数(会报 'action' is not a valid
#       argument)，因此必须用纯名字过滤删除，否则命令静默失败。
# ============================================================

$ruleName = "python.exe"

# 检查是否已是管理员，否则自我提权后重跑
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "请求管理员权限以删除 python.exe 阻止规则..."
    Start-Process powershell -Verb RunAs -ArgumentList `
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`""
    exit
}

# 纯名字过滤删除（本机 action= 参数不可用，见文件头说明）
netsh advfirewall firewall delete rule name=$ruleName
Write-Host "已执行删除，剩余 python.exe 规则："
netsh advfirewall firewall show rule name=$ruleName
