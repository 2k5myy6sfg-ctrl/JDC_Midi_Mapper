# Xcode Build Instructions

This document reflects the current `v1.1.0` build of **JDC MIDI Mapper**, including the per-row Learn buttons in the mapping table.

## Requirements

- macOS 15 or newer
- Apple Silicon Mac
- Xcode 16 or newer with command-line tools installed
- A JUCE 8 checkout or JUCE 8 CMake package
- A VST3 SDK only if your local JUCE setup requires one separately

## Generate the Project

From the project root:

```bash
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/JUCE
```

If you keep JUCE in a local `JUCE/` folder beside the project, omit `CMAKE_PREFIX_PATH`.

## Open in Xcode

Open:

```text
build/JDCMidiMapper.xcodeproj
```

Recommended Xcode settings to confirm:

- Architecture: `arm64`
- Deployment Target: `macOS 15.0`
- Configuration: `Release`

These values are already enforced by `CMakeLists.txt`.

## Build Targets

Build either:

- `JDCMidiMapper_AU`
- `JDCMidiMapper_VST3`
- or the aggregate `JDCMidiMapper` target

## Validation

After building:

1. Open Gig Performer or another AU/VST3 host.
2. Insert `JDC MIDI Mapper` on a MIDI routing slot.
3. Confirm the editor opens and resizing is smooth.
4. Enable one mapping, for example `38 -> 36`.
5. Send note `38` and verify the destination instrument receives `36`.
6. Confirm CC, pitch bend, transport, and clock messages still pass through unchanged.
7. Press `Panic` and verify any held mapped notes are released.
8. While holding a mapped note, change or disable that row and confirm the eventual release is still correct.
9. Click a row-level `Learn` button and send one MIDI note.
10. Confirm only that row's `Input Note` updates.
11. Confirm Learn turns off immediately after capturing the note.
12. Confirm the row's `Output Note` is unchanged.

## Notes

- The plugin is MIDI-only and intentionally performs no audio processing.
- Host preset save/restore is handled through JUCE state serialization.
- When multiple enabled rows share the same input note, the last enabled row wins.
