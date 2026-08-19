$root = "d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】"

$dirs = Get-ChildItem $root -Directory
foreach ($d in $dirs) {
    $files = Get-ChildItem $d.FullName -Recurse -Include '*.cpp','*.h'
    $count = $files.Count
    if ($count -gt 0) {
        $sum = ($files | Measure-Object -Property Length -Sum).Sum
        $kb = [math]::Round($sum / 1024, 1)
        Write-Output "$($d.Name): $count files, $kb KB"
    } else {
        Write-Output "$($d.Name): 0 files"
    }
}

Write-Output ""
Write-Output "=== TOTAL ==="
$allFiles = Get-ChildItem $root -Recurse -Include '*.cpp','*.h'
$totalCount = $allFiles.Count
$totalKB = [math]::Round(($allFiles | Measure-Object -Property Length -Sum).Sum / 1024, 1)
Write-Output "Total: $totalCount files, $totalKB KB"

# Count lines for major directories
Write-Output ""
Write-Output "=== LINE COUNTS ==="
$majorDirs = @(
    "[Client]MH",
    "4DYUCHIGX_RENDER",
    "4DyuchiGXGeometry",
    "4DYUCHIGXEXECUTIVE",
    "4DyuchiGRX_myself97",
    "4DyuchiNET_Latest",
    "4DyuchiGRX_common",
    "[CC]Ability",
    "[CC]BattleSystem",
    "[CC]Quest",
    "[CC]Skill",
    "[CC]ServerModule",
    "[CC]Suryun",
    "[Lib]BaseNetwork",
    "[Lib]YHLibrary",
    "[Lib]HSEL",
    "SoundLib"
)

foreach ($dirname in $majorDirs) {
    $dirPath = Join-Path $root $dirname
    if (Test-Path $dirPath) {
        $cppFiles = Get-ChildItem $dirPath -Recurse -Include '*.cpp','*.h'
        $lineCount = 0
        foreach ($f in $cppFiles) {
            $lines = (Get-Content $f.FullName -ErrorAction SilentlyContinue).Count
            $lineCount += $lines
        }
        $fileCount = $cppFiles.Count
        Write-Output "${dirname}: $fileCount files, $lineCount lines"
    }
}
