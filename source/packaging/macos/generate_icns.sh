#!/usr/bin/env sh
set -eu

if [ "$#" -lt 4 ]; then
  echo "Usage: generate_icns.sh <input-png> <output-icns> <sips> <iconutil>" >&2
  exit 1
fi

input_png="$1"
output_icns="$2"
sips_bin="$3"
iconutil_bin="$4"

work_dir="$(mktemp -d "${TMPDIR:-/tmp}/crt_iconset.XXXXXX")"
iconset_dir="$work_dir/CRT_EMULATOR.iconset"
mkdir -p "$iconset_dir"

cleanup() {
  rm -rf "$work_dir"
}
trap cleanup EXIT INT TERM

generate_png() {
  size="$1"
  scale="$2"
  out_name="$3"
  pixel_size=$((size * scale))
  "$sips_bin" -z "$pixel_size" "$pixel_size" "$input_png" --out "$iconset_dir/$out_name" >/dev/null
}

generate_png 16 1 icon_16x16.png
generate_png 16 2 icon_16x16@2x.png
generate_png 32 1 icon_32x32.png
generate_png 32 2 icon_32x32@2x.png
generate_png 128 1 icon_128x128.png
generate_png 128 2 icon_128x128@2x.png
generate_png 256 1 icon_256x256.png
generate_png 256 2 icon_256x256@2x.png
generate_png 512 1 icon_512x512.png
generate_png 512 2 icon_512x512@2x.png

"$iconutil_bin" -c icns "$iconset_dir" -o "$output_icns"
