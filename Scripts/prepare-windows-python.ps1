param([string]$Archive)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Version = '3.13.15'
$Sha256 = 'd1f04d990aee1253d8569e8e5104e30fa9f5fa830899f14843448872d936a2cf'
if (!$Archive) {
    New-Item -ItemType Directory -Force "$Root\.tmp" | Out-Null
    $Archive = "$Root\.tmp\python-$Version-embed-amd64.zip"
    if (!(Test-Path $Archive)) {
        Invoke-WebRequest -UseBasicParsing "https://www.python.org/ftp/python/$Version/python-$Version-embed-amd64.zip" -OutFile $Archive
    }
}
if ((Get-FileHash $Archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Sha256) {
    throw 'Python runtime archive hash mismatch'
}
$Destination = "$Root\Runtime\Python"
if (Test-Path $Destination) {
    throw "Runtime already exists: $Destination. Preserve it before staging a replacement."
}
Expand-Archive -LiteralPath $Archive -DestinationPath $Destination
# The official ._pth isolates this runtime from user Python, site packages and
# environment variables. Leave it intact; the bridge adds only Randomizer/.
& "$Destination\python.exe" -I -c 'import sys; print(sys.version); assert sys.flags.isolated'
if ($LASTEXITCODE -ne 0) { throw 'Bundled Python validation failed' }
Write-Output "Prepared isolated Python $Version; retain LICENSE.txt when packaging."
