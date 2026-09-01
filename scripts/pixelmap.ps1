# pixelmap.ps1 — 把图片降采样成网格，输出每格平均颜色，判断内容布局
# 用法: powershell -ExecutionPolicy Bypass -File scripts\pixelmap.ps1 <图片路径> [网格宽] [网格高]
param(
    [Parameter(Mandatory = $true)][string]$Path,
    [int]$Cols = 32,
    [int]$Rows = 16
)

Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap($Path)
$W = $bmp.Width; $H = $bmp.Height
Write-Output "IMG: ${W}x${H}  grid ${Cols}x${Rows}"

$cellW = [double]$W / $Cols
$cellH = [double]$H / $Rows

for ($r = 0; $r -lt $Rows; $r++) {
    $line = ''
    for ($c = 0; $c -lt $Cols; $c++) {
        # 每格采样中心像素颜色
        $x = [int](($c + 0.5) * $cellW)
        $y = [int](($r + 0.5) * $cellH)
        if ($x -ge $W) { $x = $W - 1 }
        if ($y -ge $H) { $y = $H - 1 }
        $p = $bmp.GetPixel($x, $y)
        $bright = ($p.R + $p.G + $p.B) / 3
        if ($bright -lt 30) { $line += '.' }        # 近黑
        elseif ($bright -lt 80) { $line += ':' }     # 暗
        elseif ($bright -lt 150) { $line += 'o' }    # 中
        elseif ($bright -lt 210) { $line += 'O' }    # 亮
        else { $line += '#' }                        # 很亮
    }
    Write-Output $line
}
$bmp.Dispose()
