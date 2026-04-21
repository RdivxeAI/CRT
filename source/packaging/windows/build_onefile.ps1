param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..")).Path,
    [string]$PortableDir = "",
    [string]$OutputName = "CRT_EMULATOR_ONEFILE.exe"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
$buildRoot = Join-Path $projectRoot "build"
$distDir = Join-Path $projectRoot "dist"
$packagingDir = $PSScriptRoot
$logoPath = Join-Path $projectRoot "assets\\branding\\logo.png"
$tempIconDir = Join-Path $buildRoot "onefile"
$tempPayloadDir = Join-Path $buildRoot "onefile"
$iconPath = Join-Path $tempIconDir "app_icon.ico"
$payloadZip = Join-Path $tempPayloadDir "payload.zip"
$generatedLauncher = Join-Path $packagingDir "OneFileLauncher.generated.cs"
$outputExe = Join-Path $distDir $OutputName
$templateSource = Join-Path $packagingDir "OneFileLauncher.cs"
$cscPath = @(
    "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe",
    "C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $cscPath) {
    $cscCommand = Get-Command csc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cscCommand) {
        $cscPath = $cscCommand.Source
    }
}

if ([string]::IsNullOrWhiteSpace($PortableDir)) {
    $portableDir = Join-Path $projectRoot "dist\\portable"
}
else {
    $portableDir = [System.IO.Path]::GetFullPath($PortableDir)
}

if (-not (Test-Path $portableDir)) {
    throw "Portable directory was not found: $portableDir"
}

if (-not (Test-Path $templateSource)) {
    throw "Launcher template was not found: $templateSource"
}

if (-not (Test-Path $cscPath)) {
    throw "C# compiler was not found. Install the .NET Framework build tools or make csc.exe available in PATH."
}

[System.IO.Directory]::CreateDirectory($distDir) | Out-Null
[System.IO.Directory]::CreateDirectory($tempIconDir) | Out-Null

& powershell -ExecutionPolicy Bypass -File (Join-Path $packagingDir "generate_icon.ps1") -InputPng $logoPath -OutputIco $iconPath

if (Test-Path $payloadZip) {
    Remove-Item $payloadZip -Force
}

$payloadEntries = Get-ChildItem -Path $portableDir -Force
if ($payloadEntries.Count -eq 0) {
    throw "No payload files were found in $portableDir"
}

Compress-Archive -Path ($payloadEntries | ForEach-Object { $_.FullName }) -DestinationPath $payloadZip -Force

$versionToken = (Get-Date).ToString("yyyy.MM.dd.HHmmss")
$launcherSource = (Get-Content $templateSource -Raw).Replace("__PAYLOAD_VERSION__", $versionToken)
[System.IO.File]::WriteAllText($generatedLauncher, $launcherSource, [System.Text.Encoding]::UTF8)

if (Test-Path $outputExe) {
    Remove-Item $outputExe -Force
}

& $cscPath `
    /nologo `
    /target:winexe `
    /optimize+ `
    /platform:anycpu `
    "/out:$outputExe" `
    "/win32icon:$iconPath" `
    "/resource:$payloadZip,PayloadZip" `
    /r:System.Windows.Forms.dll `
    /r:System.IO.Compression.dll `
    /r:System.IO.Compression.FileSystem.dll `
    $generatedLauncher

if ($LASTEXITCODE -ne 0) {
    throw "Failed to compile the one-file launcher."
}

if (Test-Path $iconPath) {
    Remove-Item $iconPath -Force
}

Write-Host "One-file package created at: $outputExe"
