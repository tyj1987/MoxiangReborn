[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $repo 'modern\build-unity-x64\mxh_unity_core.dll'
$targetDir = Join-Path $repo 'unity\MoxiangClient\Assets\Plugins\x86_64'
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw 'Run scripts/build-unity-core.cmd first.' }
New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
# A running Editor may hold this DLL open; close that project before staging.
Copy-Item -LiteralPath $source -Destination (Join-Path $targetDir 'mxh_unity_core.dll')
Get-FileHash -LiteralPath (Join-Path $targetDir 'mxh_unity_core.dll') -Algorithm SHA256 | Select-Object Hash
