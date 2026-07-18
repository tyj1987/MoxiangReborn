# Moxian dev sync: ROG -> WINSERVER
$server = "192.168.2.200"
$src = "D:\Moxian"
$dst = "\\$server\C$\Moxian"

Write-Host "Syncing Moxian to WINSERVER..."
net use \\$server\C$ /user:52TRZ\Administrator Tyj_198729 /persistent:no 2>&1 | Out-Null

robocopy $src $dst /MIR /R:2 /W:2 /MT:16 /NP /NDL `
  /XD build _deps node_modules .cache temp `
  /XF *.obj *.pdb *.ilk *.exp *.lib *.dll

Write-Host "Done. Exit: $LASTEXITCODE"
net use \\$server\C$ /delete 2>&1 | Out-Null
