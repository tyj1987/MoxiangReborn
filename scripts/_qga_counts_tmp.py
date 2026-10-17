import base64
import subprocess

HOST = "root@192.168.2.200"
script = r"""
$rows=@()
foreach($root in @('C:\Moxiang\data\PlayDH','C:\Moxiang\original\stage-canonical-birth-20261017\modern\data\PlayDH')) {
  $files=@(Get-ChildItem -LiteralPath $root -File -Recurse -ErrorAction SilentlyContinue)
  $rows += [ordered]@{root=$root;exists=(Test-Path -LiteralPath $root);files=$files.Count;bytes=[int64](($files | Measure-Object Length -Sum).Sum)}
}
$rows | ConvertTo-Json -Compress
"""
encoded = base64.b64encode(script.encode("utf-16le")).decode("ascii")
command = ["ssh", "-T", HOST, "qm", "guest", "exec", "112", "--", "powershell.exe", "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded]
completed = subprocess.run(command, capture_output=True, check=False)
print(completed.stdout.decode(errors="replace"))
print(completed.stderr.decode(errors="replace"))
raise SystemExit(completed.returncode)
