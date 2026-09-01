# cropmap.ps1 — 裁剪图片区域并输出像素网格（相对裁剪区坐标）
# 用法: powershell -ExecutionPolicy Bypass -File scripts\cropmap.ps1 <图片路径> <x> <y> <w> <h> [网格宽] [网格高]
param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][int]$CX,
    [Parameter(Mandatory = $true)][int]$CY,
    [Parameter(Mandatory = $true)][int]$CW,
    [Parameter(Mandatory = $true)][int]$CH,
    [int]$Cols = 48,
    [int]$Rows = 30
)

Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap($Path)
$W = $bmp.Width; $H = $bmp.Height
Write-Output "CROP: ${CW}x${CH} @ ${CX},${CY}  grid ${Cols}x${Rows}  (img ${W}x${H})"

for ($r = 0; $r -lt $Rows; $r++) {
    $line = ''
    for ($c = 0; $c -lt $Cols; $c++) {
        $x = $CX + [int](($c + 0.5) * $CW / $Cols)
        $y = $CY + [int](($r + 0.5) * $CH / $Rows)
        if ($x -lt 0) { $x = 0 } elseif ($x -ge $W) { $x = $W - 1 }
        if ($y -lt 0) { $y = 0 } elseif ($y -ge $H) { $y = $H - 1 }
        $p = $bmp.GetPixel($x, $y)
        $bright = ($p.R + $p.G + $p.B) / 3
        if ($bright -lt 30) { $line += '.' }
        elseif ($bright -lt 80) { $line += ':' }
        elseif ($bright -lt 150) { $line += 'o' }
        elseif ($bright -lt 210) { $line += 'O' }
        else { $line += '#' }
    }
    Write-Output $line
}
$bmp.Dispose()
