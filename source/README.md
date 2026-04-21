# CRT_EMULATOR Source Package

The source itself is prepared to build with CMake on Windows, Linux, and macOS using Qt 6.

## Project Layout

- `src/` application source code
- `assets/branding/` logo and branding assets used by the app and Windows icon
- `packaging/linux/` Linux desktop metadata and install layout assets
- `packaging/macos/` macOS bundle metadata and icon generation helpers
- `packaging/windows/` optional Windows-only packaging helpers
- `scripts/` quick build automation for Windows and Unix-like systems
- `CMakePresets.json` ready-made configure/build/package presets for Windows, Linux, and macOS

## Requirements

- CMake 3.16 or newer
- Qt 6.2 or newer with the `Widgets` module
- A C++ compiler that matches the installed Qt package
- On Windows: PowerShell 5.1 or newer for the helper scripts
- On Windows: .NET Framework C# compiler for the optional one-file package step
- For preset-based builds: CMake 3.23 or newer

Typical Qt prefixes:

- Windows: `C:\Qt\6.8.3\msvc2022_64`
- Linux: `/opt/Qt/6.8.3/gcc_64`
- macOS: `$HOME/Qt/6.8.3/macos`

## Quick Build On Windows

Use the helper script and point it to your Qt installation root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_windows.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64
```

You can also set `QT6_ROOT` once and call the script without passing `-QtRoot`.

The quick build creates:

- `build/` generated CMake files and binaries
- `dist/portable/` a runnable Windows build with the Qt runtime beside it
- `dist/portable/run_portable.cmd` a convenience launcher

## CMake Presets

Set `QT6_ROOT` first, then use the preset that matches the host OS.

Windows:

```powershell
$env:QT6_ROOT="C:\Qt\6.8.3\msvc2022_64"
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release-build
cmake --build --preset windows-msvc-release-package
```

Linux:

```bash
export QT6_ROOT=/opt/Qt/6.8.3/gcc_64
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release-build
cmake --build --preset linux-ninja-release-package
```

macOS:

```bash
export QT6_ROOT="$HOME/Qt/6.8.3/macos"
cmake --preset macos-ninja-release
cmake --build --preset macos-ninja-release-build
cmake --build --preset macos-ninja-release-package
```

## Quick Build On Linux Or macOS

Use the Unix helper script:

```bash
QT6_ROOT=/opt/Qt/6.8.3/gcc_64 ./scripts/build_unix.sh
```

If Qt is discoverable globally, you can omit `QT6_ROOT`. The script builds the app and installs it into `dist/install/`.

## Manual Build On Any Platform

Windows with Visual Studio:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build --config Release
cmake --install build --config Release --prefix dist\install
```

Linux:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.3/gcc_64
cmake --build build --parallel
cmake --install build --prefix dist/install
```

macOS:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.3/macos"
cmake --build build --parallel
cmake --install build --prefix dist/install
```

Expected outputs:

- Windows: `dist/install/bin/CRT_EMULATOR.exe`
- Linux: `dist/install/bin/CRT_EMULATOR`
- macOS: `dist/install/CRT_EMULATOR.app`

## Distribution Layout

The install/package output is now prepared per platform:

- Linux installs the executable, Qt runtime, desktop file, AppStream metadata, and application icon into a standard Unix tree.
- macOS installs a `.app` bundle and, on macOS hosts with `sips` and `iconutil`, generates a native `.icns` icon for the bundle.
- Windows keeps the existing portable and one-file packaging flow, and CPack can also emit a `.zip` package.

## Windows Packaging

After the portable build is ready, create a single executable package with:

```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\windows\build_onefile.ps1
```

This writes `dist/CRT_EMULATOR_ONEFILE.exe`.
