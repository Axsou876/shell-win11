param(
    [string]$QtDir = "C:\Users\axsou\Qt\6.9.3\msvc2022_64",
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [switch]$Clean,
    [switch]$Deploy,
    [switch]$SmokeTest
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninja = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

foreach ($path in @($cmake, $ninja, $vcvars, (Join-Path $QtDir "bin\qmake.exe"))) {
    if (-not (Test-Path $path)) {
        throw "Missing prerequisite: $path"
    }
}

if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir
}
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$batch = Join-Path $BuildDir "build-msvc.bat"
@"
@echo off
call "$vcvars"
if errorlevel 1 exit /b %errorlevel%
"$cmake" -S "$root" -B "$BuildDir" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DCMAKE_PREFIX_PATH="$QtDir" -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%
"$cmake" --build "$BuildDir"
if errorlevel 1 exit /b %errorlevel%
"@ | Set-Content -Encoding Ascii $batch

& cmd.exe /c $batch
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exe = Join-Path $BuildDir "bin\caelestia-shell.exe"
Write-Host "Built: $exe"

if ($Deploy) {
    $windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
    & $windeployqt --debug --qmldir (Join-Path $root "src\qml") $exe
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if ($SmokeTest) {
    Push-Location $BuildDir
    try {
        $env:PATH = "$QtDir\bin;$env:PATH"
        $env:QT_SCALE_FACTOR = "1"
        & $exe --smoke-test
        exit $LASTEXITCODE
    } finally {
        Pop-Location
    }
}
