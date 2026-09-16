param(
    [string]$Engine = 'D:\Epic Games\UE_5.8',
    [string]$Msys = 'C:\msys64',
    [ValidateSet('SMUnreal', 'SMUnrealEditor')][string]$Target = 'SMUnreal',
    [switch]$NativeOnly,
    [switch]$GameOnly
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
if ($NativeOnly -and $GameOnly) { throw 'Choose NativeOnly or GameOnly, not both' }
if (!$GameOnly) {
    $CompilerBin = Join-Path $Msys 'ucrt64\bin'
    $Cmake = Join-Path $CompilerBin 'cmake.exe'
    $Python = Join-Path $CompilerBin 'python.exe'
    $Ninja = Join-Path $CompilerBin 'ninja.exe'
    foreach ($Tool in @($Cmake, $Python, $Ninja, (Join-Path $CompilerBin 'gcc.exe'))) {
        if (!(Test-Path $Tool)) { throw "Required development tool missing: $Tool" }
    }
    $PreviousPath = $env:PATH
    try {
        $env:PATH = "$CompilerBin;$PreviousPath"
        & $Cmake -S "$Root\Native" -B "$Root\Native\build" -G Ninja `
            "-DCMAKE_C_COMPILER=$CompilerBin\gcc.exe" "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DPython3_EXECUTABLE=$Python"
        if ($LASTEXITCODE -ne 0) { throw 'Native CMake configuration failed' }
        & $Cmake --build "$Root\Native\build" --parallel 2
        if ($LASTEXITCODE -ne 0) { throw 'Native DLL build failed' }
    } finally { $env:PATH = $PreviousPath }
}
if (!$NativeOnly) {
    & "$Engine\Engine\Build\BatchFiles\Build.bat" $Target Win64 Development `
        "$Root\Unreal\SMUnreal.uproject" -MaxParallelActions=2 -NoUBA -NoUBALocal
    if ($LASTEXITCODE -ne 0) { throw 'Unreal Windows game build failed' }
}
