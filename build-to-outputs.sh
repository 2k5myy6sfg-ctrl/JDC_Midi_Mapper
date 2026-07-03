#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$project_root/build-make"
output_dir="$project_root/outputs"
artifact_dir="$build_dir/JDCMidiMapper_artefacts/Release"
au_name="JDC MIDI Mapper.component"
vst3_name="JDC MIDI Mapper.vst3"

jobs="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

cmake -S "$project_root" -B "$build_dir" -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-O3 -DNDEBUG" \
    -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"
cmake --build "$build_dir" -j"$jobs"

mkdir -p "$output_dir"
rm -rf "$output_dir/$au_name" "$output_dir/$vst3_name"
ditto "$artifact_dir/AU/$au_name" "$output_dir/$au_name"
ditto "$artifact_dir/VST3/$vst3_name" "$output_dir/$vst3_name"
touch "$output_dir/$au_name" "$output_dir/$vst3_name"

# Keep outputs/ as the single delivery location for compiled plugin bundles.
find "$project_root/build" "$project_root/build-make" -type d \
    \( -name "*.component" -o -name "*.vst3" \) -prune -exec rm -rf {} +

printf 'Exported bundles:\n'
printf '  %s\n' "$output_dir/$au_name" "$output_dir/$vst3_name"
