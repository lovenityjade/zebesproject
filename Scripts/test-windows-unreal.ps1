param(
    [Parameter(Mandatory=$true)][string]$Root,
    [string]$Engine = 'D:\Epic Games\UE_5.8',
    [ValidateSet('Rom','SeedSharing','Profile','Tracker','Flow','Title')]
    [string[]]$Cases = @('Rom','SeedSharing','Profile','Tracker','Flow')
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path $Root).Path
if (!(Test-Path "$Root\ISOLATED_TEST_DIRECTORY")) { throw 'Missing isolated test marker' }
$Exe = "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Project = "$Root\Unreal\SMUnreal.uproject"
$Directory = "$Root\SMTests"
New-Item -ItemType Directory -Force $Directory | Out-Null
$Results = @()
foreach ($Test in $Cases) {
    $Log = "$Directory\unreal-$Test.log"
    $Output = "$Directory\unreal-$Test-stdout.log"
    $Arguments = @("`"$Project`"", '/Engine/Maps/Entry', '-game', '-unattended', '-NoZenService',
                   "`"-abslog=$Log`"", '-stdout', '-FullStdOutLogOutput')
    $Marker = switch ($Test) {
        'SeedSharing' { 'SM_SEED_SHARING_SELF_TEST PASS' }
        'Flow' { 'SM_MENU_FLOW_TEST PASS' }
        'Title' { 'SM_TITLE_TEST PASS' }
        default { "SM_$($Test.ToUpperInvariant())_SELF_TEST PASS" }
    }
    if ($Test -eq 'Title') {
        # Run this case from an interactive Windows session for real RHI output.
        $Arguments += @('-d3d11','-windowed','-ResX=1280','-ResY=720','-nosplash',
                        '-ExecCmds="t.MaxFPS 60"','-SMTitleTest','-SMTitleTestCase=2')
    } else {
        $Arguments += @('-nullrhi','-nosound')
        if ($Test -eq 'Flow') { $Arguments += '-SMMenuFlowTest' }
        else { $Arguments += "`"-SM${Test}SelfTest=$Root`"" }
    }
    Remove-Item $Log -ErrorAction SilentlyContinue
    $Process = Start-Process -FilePath $Exe -ArgumentList $Arguments -WorkingDirectory $Root `
        -RedirectStandardOutput $Output -RedirectStandardError "$Output.err" -PassThru
    # Retain the process handle so PowerShell 5 can read ExitCode after exit.
    $Handle = $Process.Handle
    if (!$Process.WaitForExit(600000)) {
        Stop-Process -Id $Process.Id
        throw "Timeout in $Test; stopped only that test process"
    }
    if ($Process.ExitCode -ne 0 -or !(Test-Path $Log) -or
        !(Select-String -Path $Log -SimpleMatch $Marker -Quiet)) {
        if (Test-Path $Log) { Get-Content $Log -Tail 25 }
        throw "$Test failed with exit $($Process.ExitCode); see $Log"
    }
    Write-Output $Marker
    $Results += @{case=$Test; passed=$true; exitCode=$Process.ExitCode; rendered=($Test -eq 'Title')}
}
@{platform='Win64'; host='UnrealEditor game mode'; packagedGameTested=$false; cases=$Results} |
    ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 "$Directory\unreal-windows-$($Cases -join '-').json"
