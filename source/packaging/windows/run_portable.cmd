@echo off
setlocal
cd /d "%~dp0"

if not exist "CRT_EMULATOR.exe" (
    echo CRT_EMULATOR.exe was not found in %~dp0
    exit /b 1
)

start "" "%~dp0CRT_EMULATOR.exe"
exit /b 0
