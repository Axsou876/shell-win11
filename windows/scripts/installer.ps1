param(
    [string]$QtDir = "C:\Users\axsou\Qt\6.9.3\msvc2022_64",
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build-release"),
    [string]$Iscc,
    [switch]$SkipBuild,
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"

$windowsRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = [IO.Path]::GetFullPath($BuildDir)

function Find-Tool([string]$name, [string]$fallback) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    if ($fallback -and (Test-Path $fallback)) {
        return $fallback
    }
    throw "Missing prerequisite: $name"
}

$cmakeLists = Get-Content (Join-Path $windowsRoot "CMakeLists.txt") -Raw
$version = [regex]::Match($cmakeLists, "project\(caelestia-shell-windows VERSION ([0-9.]+)").Groups[1].Value
if (-not $version) {
    throw "Cannot read the version from windows\CMakeLists.txt"
}

if (-not $SkipBuild) {
    $cmake = Find-Tool "cmake.exe" "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ninja = Find-Tool "ninja.exe" "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    # Locally the MSVC environment comes from vcvars; on a CI runner the
    # compiler is already on PATH, so the call is optional.
    $vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    $batch = Join-Path $BuildDir "build-msvc.bat"
    $lines = @("@echo off")
    if (Test-Path $vcvars) {
        $lines += "call `"$vcvars`""
        $lines += "if errorlevel 1 exit /b %errorlevel%"
    }
    $lines += "`"$cmake`" -S `"$windowsRoot`" -B `"$BuildDir`" -G Ninja -DCMAKE_MAKE_PROGRAM=`"$ninja`" -DCMAKE_PREFIX_PATH=`"$QtDir`" -DCMAKE_BUILD_TYPE=Release"
    $lines += "if errorlevel 1 exit /b %errorlevel%"
    $lines += "`"$cmake`" --build `"$BuildDir`""
    $lines += "if errorlevel 1 exit /b %errorlevel%"
    Set-Content -Encoding Ascii -Path $batch -Value $lines

    & cmd.exe /c $batch
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$exe = Join-Path $BuildDir "bin\caelestia-shell.exe"
if (-not (Test-Path $exe)) {
    throw "Missing $exe; build it first or drop -SkipBuild"
}

if (-not $SkipDeploy) {
    $windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
    if (-not (Test-Path $windeployqt)) {
        throw "Missing prerequisite: $windeployqt"
    }
    & $windeployqt --release --qmldir (Join-Path $windowsRoot "src\qml") $exe
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if (-not $Iscc) {
    $Iscc = Find-Tool "ISCC.exe" (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe")
}

& $Iscc "/DMyAppVersion=$version" (Join-Path $windowsRoot "installer\caelestia-shell.iss")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Installer: $(Join-Path $windowsRoot "dist\Caelestia-Shell-$version-Setup.exe")"
