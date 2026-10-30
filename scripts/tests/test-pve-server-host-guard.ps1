param([string]$RepoRoot='')
if(-not $RepoRoot){$RepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))}
$ErrorActionPreference='Stop'
if([Environment]::MachineName -eq 'MOXIANG-SERVER'){throw 'These negative tests run only on the client PC'}
$script=Join-Path $RepoRoot 'deploy\scripts\start_modern.ps1'
$tokens=$null;$parseErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($script,[ref]$tokens,[ref]$parseErrors)
if($parseErrors.Count){throw 'Server helper does not parse'}
$mode=@($ast.ParamBlock.Parameters|Where-Object {$_.Name.VariablePath.UserPath -eq 'Mode'})
if($mode.Count -ne 1 -or $mode[0].DefaultValue.Value -ne 'status'){throw 'Implicit launch is not read-only'}
$results=@()
foreach($case in @(@{mode='start';dry=$false},@{mode='restart';dry=$false},@{mode='stop';dry=$false},@{mode='restart';dry=$true},@{mode='stop';dry=$true})){
 $p=New-Object Diagnostics.Process
 $p.StartInfo.FileName='C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe'
 $p.StartInfo.Arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$script+'" -Mode '+$case.mode+$(if($case.dry){' -DryRun'}else{''})
 $p.StartInfo.UseShellExecute=$false;$p.StartInfo.CreateNoWindow=$true
 $p.StartInfo.RedirectStandardOutput=$true;$p.StartInfo.RedirectStandardError=$true
 try{
  if(-not $p.Start()){throw 'Negative test process did not start'}
  $null=$p.Handle;$outTask=$p.StandardOutput.ReadToEndAsync();$errTask=$p.StandardError.ReadToEndAsync()
  if(-not $p.WaitForExit(8000)){throw 'Negative test timed out; no repeated invocation'}
  $out=$outTask.GetAwaiter().GetResult();$err=$errTask.GetAwaiter().GetResult()
  $expected=if($case.dry){'DryRun cannot stop or restart services'}else{'Server mutation is limited to PVE VM112'}
  if($p.ExitCode -eq 0 -or -not $err.Contains($expected)){throw ('Expected early refusal absent: '+$case.mode)}
  $results+=[ordered]@{mode=$case.mode;dryRun=$case.dry;exitCode=$p.ExitCode;expectedRefusal=$true}
 }finally{$p.Dispose()}
}
$receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');script=$script;sha256=(Get-FileHash -LiteralPath $script).Hash.ToLowerInvariant();parserErrors=0;defaultMode='status';cases=$results;scope='Five actual early-refusal executions on PC; no PVE success-path or DB acceptance'}
$receipt|ConvertTo-Json -Depth 6

