# Install SQL Server 2022 Express with LocalDB feature
$ErrorActionPreference = 'Continue'
$progressPreference = 'SilentlyContinue'

$installer = "$env:TEMP\SQL2022-SSEI-Expr.exe"
if (-not (Test-Path $installer)) {
    Write-Host "Downloading SQL Server 2022 Express installer..."
    try {
        Invoke-WebRequest -Uri 'https://go.microsoft.com/fwlink/?linkid=2215160' -OutFile $installer -UseBasicParsing
    } catch {
        Write-Host "ERROR: download failed"
        exit 1
    }
}

$size = (Get-Item $installer).Length
Write-Host "Installer size: $size bytes"
if ($size -lt 1000000) {
    Write-Host "ERROR: installer too small, download failed"
    exit 1
}

Write-Host "Installing SQL Server Express with LocalDB (silent)..."
Write-Host "This will take 5-15 minutes..."

$proc = Start-Process -FilePath $installer `
    -ArgumentList @(
        '/Q',
        '/ACTION=Install',
        '/FEATURES=LocalDB',
        '/INSTANCENAME=MSSQLLocalDB',
        '/IACCEPTSQLSERVERLICENSETERMS',
        '/SkipRules=RebootRequiredCheck'
    ) `
    -PassThru -NoNewWindow -Wait

Write-Host "Install exit code: $($proc.ExitCode)"
Write-Host "Done."