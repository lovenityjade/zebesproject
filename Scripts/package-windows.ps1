param(
    [string]$Engine = 'D:\Epic Games\UE_5.8',
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Python=Join-Path $Root 'Runtime\Python\python.exe'
if(!(Test-Path $Python)){throw 'Run Scripts/prepare-windows-python.ps1 first'}
if(Test-Path $Output){throw 'Output must be a new directory, never an installed game'}
& $Python "$PSScriptRoot\check-distribution.py"
if($LASTEXITCODE -ne 0){throw 'Distribution gate failed'}
& "$PSScriptRoot\build-windows.ps1" -Engine $Engine -NativeOnly
if($LASTEXITCODE -ne 0){throw 'Native build failed'}
$Output=[IO.Path]::GetFullPath($Output)
& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$Root\Unreal\SMUnreal.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Engine/Maps/Entry -stage -pak "-stagingdirectory=$Output\Stage" -unattended -utf8output '-ubtargs=-MaxParallelActions=2 -NoUBA'
if($LASTEXITCODE -ne 0){throw 'Cook failed'}
$Stage="$Output\Stage\Windows"
& $Python "$PSScriptRoot\stage-runtime.py" $Stage --platform Windows
if($LASTEXITCODE -ne 0){throw 'Staging audit failed'}
Copy-Item "$Engine\Engine\Source\ThirdParty\Licenses" "$Stage\Licenses\Unreal-third-party" -Recurse
foreach($Group in @('crt','gcc-libs','mingw-w64-libraries')){
 $Source="C:\msys64\ucrt64\share\licenses\$Group"
 if(!(Test-Path $Source)){throw "Missing native runtime notices: $Source"}
 New-Item -ItemType Directory -Force "$Stage\Licenses\MinGW" | Out-Null
 Copy-Item $Source "$Stage\Licenses\MinGW\$Group" -Recurse
}
$Redist="$Stage\Engine\Extras\Redist\en-us"
New-Item -ItemType Directory -Force $Redist | Out-Null
Copy-Item "$Engine\Engine\Extras\Redist\en-us\vc_redist.x64.exe" $Redist
Get-ChildItem $Stage -Recurse -File | Where-Object { $_.Extension -in '.pdb','.debug','.sym' } | Remove-Item
$Version=(Get-Content "$Root\VERSION" -Raw).Trim()
if($Version -notmatch '^ALPHA-\d+\.\d+$'){throw 'Invalid VERSION'}
$Zip="$Output\The_Zebes_Project-$Version-Windows-x64.zip"
Compress-Archive -Path "$Stage\*" -DestinationPath $Zip -CompressionLevel Optimal
$Hash=(Get-FileHash -Algorithm SHA256 $Zip).Hash.ToLower()
Set-Content -Encoding ASCII "$Zip.sha256" "$Hash  $([IO.Path]::GetFileName($Zip))"
@{version=$Version;sha256=$Hash;packagedGameTested=$false} | ConvertTo-Json | Set-Content -Encoding UTF8 "$Zip.build.json"
Write-Output "Built $Zip. Clean-install and rendered tests are required before publication."
