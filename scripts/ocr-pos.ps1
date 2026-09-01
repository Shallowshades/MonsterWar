# ocr-pos.ps1 — 用 Windows.Media.Ocr 提取图片文字及位置（规范化坐标）
# 用法: powershell.exe -ExecutionPolicy Bypass -File scripts\ocr-pos.ps1 <图片路径>
param(
    [Parameter(Mandatory = $true)][string]$Path
)

Add-Type -AssemblyName System.Runtime.WindowsRuntime

$null = [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType = WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Foundation, ContentType = WindowsRuntime]
$null = [Windows.Storage.StorageFile, Windows.Foundation, ContentType = WindowsRuntime]
$null = [Windows.Storage.Streams.RandomAccessStream, Windows.Foundation, ContentType = WindowsRuntime]
$null = [Windows.Globalization.Language, Windows.Foundation, ContentType = WindowsRuntime]

$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
})[0]

function Await($WinRtTask, $ResultType) {
    $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
    $netTask = $asTask.Invoke($null, @($WinRtTask))
    $netTask.Wait() | Out-Null
    $netTask.Result
}

$file = Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync($Path)) ([Windows.Storage.StorageFile])
$stream = Await ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
$decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
$bitmap = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
$W = $bitmap.PixelWidth
$H = $bitmap.PixelHeight

$engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
if ($null -eq $engine) { Write-Output "OCR_ENGINE_UNAVAILABLE"; $stream.Dispose(); exit 1 }

$result = Await ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
foreach ($line in $result.Lines) {
    $text = ($line.Words | ForEach-Object { $_.Text }) -join ' '
    $r = $line.Words[0]
    # 用整行词矩形范围
    $minX = [double]::MaxValue; $minY = [double]::MaxValue; $maxX = 0; $maxY = 0
    foreach ($w in $line.Words) {
        if ($w.BoundingRect.X -lt $minX) { $minX = $w.BoundingRect.X }
        if ($w.BoundingRect.Y -lt $minY) { $minY = $w.BoundingRect.Y }
        $rx = $w.BoundingRect.X + $w.BoundingRect.Width
        $ry = $w.BoundingRect.Y + $w.BoundingRect.Height
        if ($rx -gt $maxX) { $maxX = $rx }
        if ($ry -gt $maxY) { $maxY = $ry }
    }
    $px = [math]::Round([double]$minX / [double]$W, 3)
    $py = [math]::Round([double]$minY / [double]$H, 3)
    $pw = [math]::Round(([double]$maxX - [double]$minX) / [double]$W, 3)
    $ph = [math]::Round(([double]$maxY - [double]$minY) / [double]$H, 3)
    Write-Output ("{0:F3},{1:F3} {2:F3}x{3:F3}  {4}" -f $px, $py, $pw, $ph, $text)
}
$stream.Dispose()
