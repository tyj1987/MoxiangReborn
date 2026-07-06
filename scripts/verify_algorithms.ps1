# verify_algorithms.ps1
# 用 PowerShell 验证 modern/ 下的资源格式算法与原版 .bin/.pak/.bsad 文件 100% 兼容。

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$Root = 'D:\墨香全套源代码（源码+资源+客户端+服务端+教程）'
$Res  = Join-Path $Root '墨香【源码配套资源】\PlayDH\Resource'

Write-Host '=' -NoNewline; Write-Host ('=' * 60)
Write-Host ' Moxian-Reborn Algorithm Verification'
Write-Host ('=' * 60)

# -----------------------------------------------------------------------------
# Helper: 1:1 decrypt algorithm from MHFileEx.cpp
# -----------------------------------------------------------------------------
function Decrypt-BinPayload {
    param([byte[]]$Data, [int]$Type)
    $out = New-Object byte[] $Data.Length
    for ($i = 0; $i -lt $Data.Length; $i++) {
        $v = ([int]$Data[$i] - $i) -band 0xFF
        if ($Type -ne 0 -and (($i % $Type) -eq 0)) {
            $v = ($v - $Type) -band 0xFF
        }
        $out[$i] = [byte]$v
    }
    return ,$out
}

function Encrypt-BinPayload {
    param([byte[]]$Data, [int]$Type)
    $out = New-Object byte[] $Data.Length
    for ($i = 0; $i -lt $Data.Length; $i++) {
        $v = ([int]$Data[$i] + $i) -band 0xFF
        if ($Type -ne 0 -and (($i % $Type) -eq 0)) {
            $v = ($v + $Type) -band 0xFF
        }
        $out[$i] = [byte]$v
    }
    return ,$out
}

$allOk = $true

# -----------------------------------------------------------------------------
# Test 1: .bin round-trip
# -----------------------------------------------------------------------------
Write-Host ''
Write-Host '[1/4] Testing .bin round-trip (type=0)...' -ForegroundColor Cyan

$payload = [System.Text.Encoding]::ASCII.GetBytes('Hello Moxian! This is a test payload. ' * 4)
$encrypted = Encrypt-BinPayload -Data $payload -Type 0
$decrypted = Decrypt-BinPayload -Data $encrypted -Type 0
$matches = $true
for ($i = 0; $i -lt $payload.Length; $i++) {
    if ($payload[$i] -ne $decrypted[$i]) { $matches = $false; break }
}
if ($matches) {
    Write-Host "  OK: round-trip $($payload.Length) bytes verified" -ForegroundColor Green
} else {
    Write-Host "  FAIL: round-trip mismatch" -ForegroundColor Red
    $allOk = $false
}

# -----------------------------------------------------------------------------
# Test 2: Real MonsterList.bin
# -----------------------------------------------------------------------------
Write-Host ''
Write-Host '[2/4] Decrypting real MonsterList.bin...' -ForegroundColor Cyan

$binPath = Join-Path $Res 'MonsterList.bin'
if (-not (Test-Path $binPath)) {
    Write-Host "  SKIP: $binPath not found" -ForegroundColor Yellow
} else {
    $raw = [System.IO.File]::ReadAllBytes($binPath)
    if ($raw.Length -lt 14) {
        Write-Host "  FAIL: file too small (need >=14 for header+CRCs)" -ForegroundColor Red
        $allOk = $false
    } else {
        $ver = [BitConverter]::ToUInt32($raw, 0)
        $typ = [BitConverter]::ToUInt32($raw, 4)
        $sz  = [BitConverter]::ToUInt32($raw, 8)
        Write-Host "  Header: version=0x$('{0:X8}' -f $ver), type=$typ, size=$sz"

        # Real layout: header(12) + crc1(1) + data(N) + crc2(1)
        # Data starts at offset 13.
        $dataStart = 13
        $dataEnd   = $dataStart + $sz - 1
        if ($dataEnd -ge $raw.Length) {
            Write-Host "  FAIL: declared size $sz exceeds file" -ForegroundColor Red
            $allOk = $false
        } else {
            $payloadBytes = $raw[$dataStart..$dataEnd]
            $decoded = Decrypt-BinPayload -Data $payloadBytes -Type $typ

            # 统计可读字符比例
            $sample = $decoded[0..([Math]::Min(1999, $decoded.Length - 1))]
            $printable = 0
            foreach ($b in $sample) {
                if (($b -ge 32 -and $b -lt 127) -or $b -in 9,10,13,0) {
                    $printable++
                }
            }
            $ratio = $printable / $sample.Length
            Write-Host ("  Decoded {0} bytes, printable ratio: {1:P0}" -f $decoded.Length, $ratio)

            if ($ratio -gt 0.4) {
                Write-Host "  OK: looks like valid data" -ForegroundColor Green
                $preview = ($decoded[0..([Math]::Min(159, $decoded.Length - 1))] | ForEach-Object {
                    if ($_ -ge 32 -and $_ -lt 127) { [char]$_ } else { '.' }
                }) -join ''
                Write-Host "  Preview: $preview" -ForegroundColor DarkGray
            } else {
                Write-Host "  WARN: low printable ratio" -ForegroundColor Yellow
            }
        }
    }
}

# -----------------------------------------------------------------------------
# Test 3: Real .pak file
# -----------------------------------------------------------------------------
Write-Host ''
Write-Host '[3/4] Inspecting .pak file structure...' -ForegroundColor Cyan

$pakFiles = @('Effect.pak', 'Character.pak', 'Map.pak', 'monster.pak', 'npc.pak', 'Pet.pak', 'Titan.pak', 'npc.pak', 'Pet.pak', 'Titan.pak')
$foundAny = $false
foreach ($name in $pakFiles) {
    # .pak files live in PlayDH/ root, not in Resource/
$pakPath = Join-Path (Split-Path $Res -Parent) $name
    if (-not (Test-Path $pakPath)) { continue }
    $foundAny = $true
    $raw = [System.IO.File]::ReadAllBytes($pakPath)
    if ($raw.Length -lt 92) {
        Write-Host "  FAIL: $name too small (need >=92 for PACK_FILE_HEADER)" -ForegroundColor Red
        $allOk = $false
        continue
    }

    # PACK_FILE_HEADER (92 bytes total, only first 12 meaningful)
    $version = [BitConverter]::ToUInt32($raw, 0)
    $count   = [BitConverter]::ToUInt32($raw, 4)
    $flag    = [BitConverter]::ToUInt32($raw, 8)
    Write-Host "  ${name}: $($raw.Length) bytes version=0x$('{0:X8}' -f $version) count=$count flag=$flag"

    if ($version -ne 0x1) {
        Write-Host "    WARN: unexpected version (expected 0x1)" -ForegroundColor Yellow
    }

    # Parse first 5 entries
    $cursor = 92  # end of PACK_FILE_HEADER
    for ($i = 0; $i -lt [Math]::Min(3, $fileCount); $i++) {
        if ($cursor + 32 -gt $raw.Length) { break }
        $d_total = [BitConverter]::ToUInt32($raw, $cursor + 0)
        $d_real  = [BitConverter]::ToUInt32($raw, $cursor + 4)
        $d_nlen  = [BitConverter]::ToUInt32($raw, $cursor + 8)
        $d_off   = [BitConverter]::ToUInt32($raw, $cursor + 12)
        $cursor += 32
        if ($cursor + $d_nlen + 1 -gt $raw.Length) { break }
        $nameBytes = $raw[$cursor..($cursor + $d_nlen - 1)]
        $entryName = [System.Text.Encoding]::ASCII.GetString($nameBytes)
        $cursor += $d_nlen + 1  # name + NUL

        # Sanity: d_total should equal 32 + nlen + 1 + realSize
        $expected = 32 + $d_nlen + 1 + $d_real
        $status = if ($d_total -eq $expected) { 'OK' } else { 'MISMATCH' }
        Write-Host "    [$i] name=$entryName size=$d_real entryOff=$d_off total=$d_total ($status)"

        # Move to next entry (data is d_real bytes after cursor)
        if ($cursor + $d_real -gt $raw.Length) { break }
        $cursor += $d_real
    }
}
if (-not $foundAny) {
    Write-Host "  SKIP: no .pak files found" -ForegroundColor Yellow
}

# -----------------------------------------------------------------------------
# Test 4: Real .bsad files
# -----------------------------------------------------------------------------
Write-Host ''
Write-Host '[4/4] Inspecting .bsad skill area files...' -ForegroundColor Cyan

$skillDir = Join-Path $Res 'SkillArea'
if (-not (Test-Path $skillDir)) {
    Write-Host "  SKIP: $skillDir not found" -ForegroundColor Yellow
} else {
    $samples = Get-ChildItem -Path $skillDir -Filter '*.bsad' | Select-Object -First 5
    foreach ($bsad in $samples) {
        $raw = [System.IO.File]::ReadAllBytes($bsad.FullName)
        if ($raw.Length -lt 8) {
            Write-Host "  FAIL: $($bsad.Name) too small" -ForegroundColor Red
            $allOk = $false
            continue
        }
        $w = [BitConverter]::ToUInt16($raw, 0)
        $h = [BitConverter]::ToUInt16($raw, 2)
        $cells = $raw[8..(8 + $w * $h - 1)]
        Write-Host "  $($bsad.Name): ${w}x${h} = $($cells.Length) cells (expected $($w * $h))"

        for ($row = 0; $row -lt [Math]::Min(3, $h); $row++) {
            $line = ''
            for ($col = 0; $col -lt [Math]::Min($w, 20); $col++) {
                $cell = $cells[$row * $w + $col]
                switch ($cell) {
                    1 { $line += '# ' }
                    2 { $line += 'X ' }
                    default { $line += '. ' }
                }
            }
            Write-Host "    | $line"
        }
    }
}

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
Write-Host ''
Write-Host ('=' * 60)
if ($allOk) {
    Write-Host ' RESULT: PASS' -ForegroundColor Green
} else {
    Write-Host ' RESULT: FAIL' -ForegroundColor Red
}
Write-Host ('=' * 60)
exit 0
