[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $repo 'modern\data\PlayDH\Resource\MapChange.bin'
$expected = '66ACA3CCCA86469E4F8EC1F0DADA451FD2422D5C4B0C2698408E56C0101B7048'
if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $expected) {
    throw 'MapChange.bin differs from the audited playdh-current baseline.'
}
$target = Join-Path $repo 'unity\MoxiangClient\Assets\StreamingAssets\Gameplay'
New-Item -ItemType Directory -Path $target -Force | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $target 'MapChange.bin')
if ((Get-FileHash -LiteralPath (Join-Path $target 'MapChange.bin') -Algorithm SHA256).Hash -ne $expected) {
    throw 'Packaged MapChange.bin integrity check failed.'
}
Write-Output "MapChange.bin staged and verified: $expected"
