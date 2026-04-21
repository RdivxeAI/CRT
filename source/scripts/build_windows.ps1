param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$QtRoot = "",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "x64",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [string]$BuildDir = "",
    [switch]$SkipDeploy
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $buildDir = Join-Path $projectRoot "build"
}
else {
    $buildDir = [System.IO.Path]::GetFullPath($BuildDir)
}

if ([string]::IsNullOrWhiteSpace($QtRoot)) {
    foreach ($candidate in @($env:QT6_ROOT, $env:QT_ROOT, $env:CMAKE_PREFIX_PATH)) {
        if (-not [string]::IsNullOrWhiteSpace($candidate)) {
            $QtRoot = $candidate.Split(';')[0]
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($QtRoot)) {
    throw "Qt root was not provided. Pass -QtRoot C:\Qt\6.x.x\msvc2022_64 or set QT6_ROOT."
}

$qtRoot = [System.IO.Path]::GetFullPath($QtRoot)
if (-not (Test-Path $qtRoot)) {
    throw "Qt root was not found: $qtRoot"
}

$distDir = Join-Path $projectRoot "dist"
$portableDir = Join-Path $distDir "portable"
if (-not $portableDir.StartsWith($projectRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean a directory outside the project root: $portableDir"
}

[System.IO.Directory]::CreateDirectory($buildDir) | Out-Null
[System.IO.Directory]::CreateDirectory($distDir) | Out-Null

$configureArgs = @(
    "-S", $projectRoot,
    "-B", $buildDir,
    "-DCMAKE_PREFIX_PATH=$qtRoot"
)

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs += @("-G", $Generator)
}

if ($Generator -like "Visual Studio*" -and -not [string]::IsNullOrWhiteSpace($Platform)) {
    $configureArgs += @("-A", $Platform)
}

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed."
}

& cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed."
}

$exeCandidates = @(
    (Join-Path (Join-Path $buildDir $Config) "CRT_EMULATOR.exe"),
    (Join-Path $buildDir "CRT_EMULATOR.exe")
)

$builtExe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $builtExe) {
    $builtExe = Get-ChildItem -Path $buildDir -Recurse -Filter "CRT_EMULATOR.exe" -File |
        Select-Object -ExpandProperty FullName -First 1
}

if (-not $builtExe) {
    throw "Built executable was not found under $buildDir"
}

if (Test-Path $portableDir) {
    Get-ChildItem -Path $portableDir -Force | Remove-Item -Recurse -Force
}
[System.IO.Directory]::CreateDirectory($portableDir) | Out-Null

$portableExe = Join-Path $portableDir "CRT_EMULATOR.exe"
Copy-Item $builtExe $portableExe -Force

if (-not $SkipDeploy) {
    $windeployqt = Join-Path $qtRoot "bin\\windeployqt.exe"
    if (-not (Test-Path $windeployqt)) {
        throw "windeployqt.exe was not found under $qtRoot\\bin"
    }

    $deployArgs = @("--force", "--dir", $portableDir)
    if ($Config -eq "Debug") {
        $deployArgs += "--debug"
    }
    else {
        $deployArgs += "--release"
    }
    $deployArgs += $portableExe

    & $windeployqt @deployArgs
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt deployment failed."
    }

    $launcherScript = Join-Path $projectRoot "packaging\\windows\\run_portable.cmd"
    if (Test-Path $launcherScript) {
        Copy-Item $launcherScript (Join-Path $portableDir "run_portable.cmd") -Force
    }
}

Write-Host "Build finished successfully."
Write-Host "Build directory: $buildDir"
Write-Host "Portable output: $portableDir"
