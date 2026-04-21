#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"
INSTALL_DIR="${INSTALL_DIR:-$PROJECT_ROOT/dist/install}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
GENERATOR="${GENERATOR:-Ninja}"
QT_ROOT="${QT6_ROOT:-${QT_ROOT:-${CMAKE_PREFIX_PATH:-}}}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root)
      PROJECT_ROOT="$(cd "$2" && pwd)"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --build-type)
      BUILD_TYPE="$2"
      shift 2
      ;;
    --generator)
      GENERATOR="$2"
      shift 2
      ;;
    --qt-root)
      QT_ROOT="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

configure_args=(
  -S "$PROJECT_ROOT"
  -B "$BUILD_DIR"
  -G "$GENERATOR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if [[ -n "$QT_ROOT" ]]; then
  configure_args+=("-DCMAKE_PREFIX_PATH=$QT_ROOT")
fi

cmake "${configure_args[@]}"
cmake --build "$BUILD_DIR" --parallel
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

echo "Build finished successfully."
echo "Build directory: $BUILD_DIR"
echo "Install output: $INSTALL_DIR"
