# JDC MIDI Mapper

JDC MIDI Mapper is a macOS MIDI remapping plugin available in VST3 and Audio Unit formats. It performs real-time MIDI note remapping and was designed primarily for live drummers using **Gig Performer** with **Superior Drummer 3**, while remaining useful in any AU/VST3-compatible DAW or plugin host.

The core idea is simple: one drum performance setup, one Superior Drummer 3 project, and multiple instantly selectable MIDI mapping configurations called **Scenes**. This makes it possible to switch between different drums, articulations, percussion layouts, and e-drum mappings without changing kits or reloading plugins.

## Overview

JDC MIDI Mapper helps performers route incoming MIDI notes to different target notes in real time. This is especially useful when a drummer wants to:

- switch between multiple snare drums inside one Superior Drummer 3 session
- swap between acoustic and electronic kick mappings
- access multiple X-Drums and percussion instruments
- adapt to different e-drum modules or custom pad layouts
- recall different live song mappings instantly

The plugin is built for low-latency live use, fast scene switching, and a practical workflow on stage.

## Features

### MIDI Mapping

- Map any incoming MIDI note to any outgoing MIDI note.
- Enable or disable each mapping row independently.
- Edit up to 128 mappings per Scene.
- Process MIDI in real time with no audible interruption.

### Scenes

- 16 independent Scenes.
- Each Scene stores its own complete 128-row mapping table.
- Scene names are fully user editable.
- Copy Scene.
- Paste Scene.
- Duplicate Scene.
- Instant Scene switching.

### Scene Control

- Click Scene buttons to activate a Scene.
- `Current Scene` is exposed as an automatable plugin parameter.
- `Previous Scene` parameter.
- `Next Scene` parameter.
- Host automation support.
- Full synchronization between automation and the GUI.

### Learn Mode

- Per-row MIDI Learn buttons directly inside the mapping table.
- Automatic detection of the next played note for the selected row only.
- Learn writes only the `Input Note` of that row.
- Learn mode stops automatically after one received note.
- Only one row can be in Learn mode at a time.
- Fast mapping workflow for building or editing Scenes.

### Program Change

- Optional Scene switching using incoming MIDI Program Change.
- Optional filtering of the triggering Program Change message.

### Mapping Management

- Enable All.
- Disable All.
- Reset All.
- Panic button.

### Search

- Search by:
  - MIDI note number
  - Note name
  - Row number

### User Interface

- Dark modern interface.
- Compact layout.
- Optimized for live performance.
- Large Scene buttons.
- Per-row Learn buttons in the mapping table.
- Status bar.
- Responsive table layout.
- Full keyboard and mouse operation.

## How It Works

JDC MIDI Mapper receives MIDI, remaps note events according to the active Scene, and passes the result to the destination instrument or plugin. Non-note MIDI data can continue through the chain unchanged, while note mappings are controlled independently inside each Scene.

This makes it ideal for performance rigs where the drummer wants to change articulation layouts or instrument assignments instantly without touching the underlying drum plugin project.

## Typical Use Cases

### Switching Between Multiple Snare Drums

Use separate Scenes for different snare assignments and switch instantly between them while keeping the same Superior Drummer 3 kit loaded.

### Switching Between Acoustic and Electronic Kick Sounds

Route the same incoming kick trigger to different output notes depending on the Scene to move between acoustic kick articulations and electronic layered sounds.

### Playing Multiple Superior Drummer 3 X-Drums

Assign different Scenes to different X-Drum layouts, making it easy to repurpose pads for alternate percussion or custom instruments.

### Layering Percussion

Build Scenes focused on tambourine, cowbell, clap, shaker, or auxiliary percussion note layouts without rebuilding your full SD3 session.

### Alternative MIDI Layouts

Create song-specific or player-specific layouts so the same physical pad arrangement can drive different note maps depending on the performance context.

### E-Drums With Different Mappings

If different modules transmit different MIDI note numbers, JDC MIDI Mapper can normalize them into a single working layout for your software instruments.

### Live Performance

Switch Scenes instantly during rehearsal or performance without changing kits, without reloading instruments, and without interrupting the player's flow.

## Gig Performer Workflow

One common live-performance routing setup looks like this:

```text
EFNOTE module
   ↓
JDC MIDI Mapper
   ↓
Superior Drummer 3
```

In this workflow, the drummer plays a single module or pad setup, JDC MIDI Mapper translates the incoming note layout based on the active Scene, and Superior Drummer 3 receives the remapped notes.

This allows multiple mappings to exist inside one performance rig:

- one Scene for a main snare
- one Scene for a side snare
- one Scene for alternate kick routing
- one Scene for percussion
- one Scene for electronic layers

All of this can be controlled from Gig Performer widgets or buttons without changing the Superior Drummer 3 project itself.

## Per-Row Learn Workflow

JDC MIDI Mapper includes a compact **Learn** button on every mapping row.

Typical workflow:

1. Click the **Learn** button on the row you want to edit.
2. Play the source pad, key, or trigger once.
3. The received MIDI note is written into that row's **Input Note** field automatically.
4. Learn mode immediately stops after that note is captured.

Additional behavior:

- Only one mapping row can listen at a time.
- Clicking **Learn** on another row transfers Learn mode to that row.
- Clicking the same active **Learn** button again cancels Learn mode.
- Learn never changes the row's **Output Note**.

## Plugin Formats

Supported plugin formats:

- VST3
- Audio Unit

Platform:

- macOS

## Host Compatibility

JDC MIDI Mapper is intended to work in:

- Gig Performer
- Logic Pro
- MainStage
- Reaper
- Cubase
- Studio One
- Ableton Live
- Any AU/VST3-compatible host

## Performance

JDC MIDI Mapper is intended for:

- live performance
- low latency
- instant Scene switching
- no audio interruption

The plugin is designed to remain lightweight and responsive in performance rigs where reliability matters more than complexity.

## Version

Current documented project version:

- `v1.1.0`

## Build Notes

This project is built with JUCE and targets macOS plugin formats.

Files of interest:

- `CMakeLists.txt`
- `Source/PluginProcessor.*`
- `Source/PluginEditor.*`
- `Source/Parameters.h`
- `Xcode.build.md`
- `build-to-outputs.sh`

For local build instructions, see:

- [Xcode.build.md](Xcode.build.md)

For the repository's canonical exported plugin bundles, use:

- `./build-to-outputs.sh`

That script builds a Release version and refreshes the final AU/VST3 copies in `outputs/`.

## Screenshots

### Main Interface

_Screenshot coming soon._

### Scene Management

_Screenshot coming soon._

### Mapping Table

_Screenshot coming soon._

### Gig Performer Integration

_Screenshot coming soon._

## Future Ideas

Possible future improvements include:

- Import/export Scene files.
- MIDI CC mapping.
- Scene groups.
- Keyboard shortcuts.
- Favorites.
- XML/JSON Scene import/export.

## License

_License information to be added._

## Credits

Developed by **Joris de Coo**.

Designed for live drummers using **Gig Performer** and **Superior Drummer 3**.
