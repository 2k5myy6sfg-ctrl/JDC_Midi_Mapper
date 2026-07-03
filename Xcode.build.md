# Xcode Build Instructions

This document explains how to build **JDC MIDI Mapper** locally on macOS using **Xcode**, **CMake**, and **JUCE 8**.

The project targets **macOS on Apple Silicon** and builds the plugin in:

- `VST3`
- `Audio Unit`

This document reflects the current `v1.1.0` project, including the per-row Learn buttons in the mapping table.

## Requirements

You should have the following installed:

- macOS
- Xcode
- CMake
- JUCE 8
- Apple Silicon Mac

Recommended:

- Xcode command line tools installed
- A local JUCE 8 checkout inside the project folder, or a JUCE CMake package available on your machine

## Expected Plugin Outputs

A successful build should produce:

- `JDC MIDI Mapper.vst3`
- `JDC MIDI Mapper.component`

These are the VST3 and Audio Unit versions of the plugin.

For this repository, the canonical exported copies should live in:

```text
outputs/
```

Use the provided helper script to build a Release version and copy the final bundles there:

```bash
./build-to-outputs.sh
```

## Generate Xcode Project

Open Terminal and change to the project folder:

```bash
cd /path/to/JDC-MIDI-Mapper
```

If JUCE is included locally inside the project as a `JUCE/` folder:

```bash
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug
```

If JUCE is installed elsewhere and needs to be provided through `CMAKE_PREFIX_PATH`:

```bash
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/JUCE
```

This will generate an Xcode project in the `build/` folder.

## Build Debug

To build a Debug version:

```bash
cmake --build build --config Debug
```

You can also open the generated Xcode project and build the `Debug` configuration directly in Xcode.

## Build Release

To build a Release version:

```bash
cmake --build build --config Release
```

You can also switch Xcode to the `Release` configuration and build there.

If you want the repository's final delivery copies updated automatically, run:

```bash
./build-to-outputs.sh
```

## Install Locally

After building, copy the plugin files into your local plugin folders.

### User-level plugin locations

VST3:

```text
~/Library/Audio/Plug-Ins/VST3
```

Audio Unit:

```text
~/Library/Audio/Plug-Ins/Components
```

### System-wide plugin locations

VST3:

```text
/Library/Audio/Plug-Ins/VST3
```

Audio Unit:

```text
/Library/Audio/Plug-Ins/Components
```

Example copy commands for user-level installation from the canonical `outputs/` folder:

```bash
cp -R "outputs/JDC MIDI Mapper.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "outputs/JDC MIDI Mapper.component" ~/Library/Audio/Plug-Ins/Components/
```

If your generator or JUCE setup produces a slightly different output path, copy the resulting `.vst3` and `.component` bundles from the build folder to the directories above.

## Rescan Plugins

After copying the plugin:

- restart or rescan plugins in **Gig Performer**
- restart or rescan plugins in **Logic Pro**
- restart or rescan plugins in **MainStage**
- restart or rescan plugins in **Reaper**
- restart or rescan plugins in any other host you use

Some hosts cache plugin scans aggressively, so a full restart is often the fastest way to confirm installation.

## Quick Functional Check

After installing the plugin, a practical sanity check is:

1. Open the plugin editor.
2. Confirm the version label shows `v1.1.0`.
3. Click a row-level `Learn` button in the mapping table.
4. Send one MIDI note.
5. Confirm only that row's `Input Note` changes.
6. Confirm Learn mode switches off immediately after the note is captured.
7. Confirm `Output Note` does not change.
8. Confirm Scene switching, Search, `Show only enabled mappings`, `Copy Scene`, `Paste Scene`, `Duplicate Scene`, `Reset All`, `Disable All`, `Enable All`, and `Panic` still behave normally.

## Troubleshooting

### Plugin Does Not Appear in Gig Performer

- Make sure the `.vst3` or `.component` file was copied to the correct folder.
- Restart Gig Performer completely.
- Force a plugin rescan inside Gig Performer if available.
- Confirm that you built for Apple Silicon.
- Confirm that macOS did not quarantine the plugin bundle.

### AU Does Not Appear in Logic/MainStage

- Make sure the `.component` bundle is in:

```text
~/Library/Audio/Plug-Ins/Components
```

or:

```text
/Library/Audio/Plug-Ins/Components
```

- Restart Logic Pro or MainStage.
- Run Audio Unit validation tools if needed.
- Check whether the AU was blocked by macOS security.

### macOS Security / Quarantine Issues

If macOS blocks the plugin after copying, you may need to remove quarantine attributes:

```bash
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/JDC MIDI Mapper.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/JDC MIDI Mapper.component"
```

If you installed system-wide, use the equivalent `/Library/...` paths.

### CMake Cannot Find JUCE

If CMake fails to find JUCE:

- place a JUCE 8 checkout in a local `JUCE/` folder beside `CMakeLists.txt`
- or pass the JUCE package path explicitly:

```bash
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/JUCE
```

Check that JUCE contains a valid `CMakeLists.txt` and/or JUCE package config files.

### Xcode Command Line Tools Missing

If Terminal tools are missing, install them:

```bash
xcode-select --install
```

To verify Xcode is active:

```bash
xcodebuild -version
```

If needed, point the system at the full Xcode install:

```bash
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
```

### Apple Silicon Architecture Issues

This project is intended for **Apple Silicon**.

If you see architecture-related issues:

- confirm you are building on an Apple Silicon Mac
- confirm Xcode is targeting `arm64`
- delete the build folder and regenerate the project
- avoid forcing Intel or Rosetta-based builds unless you explicitly know why

### Cleaning the Build Folder

If the build becomes inconsistent or stale, remove the build folder and regenerate:

```bash
rm -rf build
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug
```

If JUCE is not local:

```bash
rm -rf build
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/JUCE
```

## Clean Rebuild

To force a fully clean rebuild:

```bash
rm -rf build
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Or with an explicit JUCE path:

```bash
rm -rf build
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/JUCE
cmake --build build --config Release
```

## Notes

- This project is intended for **macOS**.
- This project is intended for **Apple Silicon**.
- The main expected deliverables are:
  - `JDC MIDI Mapper.vst3`
  - `JDC MIDI Mapper.component`
- If plugin output paths differ slightly depending on your JUCE/CMake version, copy the resulting `.vst3` and `.component` bundles from the build artefacts folder into the appropriate macOS plugin directories.
