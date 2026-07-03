#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace jdc::parameters
{
constexpr int numScenes = 16;
constexpr int numRows = 128;
constexpr int numMidiNotes = 128;
constexpr int numMidiChannels = 16;

enum class Field
{
    currentScene,
    previousScene,
    nextScene,
    enable,
    input,
    output
};

inline juce::String makeParameterId (Field field, int row)
{
    const auto prefix = juce::String (row).paddedLeft ('0', 3);

    switch (field)
    {
        case Field::currentScene: return "currentScene";
        case Field::previousScene: return "previousScene";
        case Field::nextScene: return "nextScene";
        case Field::enable: return "row" + prefix + ".enable";
        case Field::input:  return "row" + prefix + ".input";
        case Field::output: return "row" + prefix + ".output";
    }

    jassertfalse;
    return {};
}

inline juce::String makeParameterName (Field field, int row)
{
    const auto rowLabel = juce::String (row + 1).paddedLeft ('0', 3);

    switch (field)
    {
        case Field::currentScene: return "Current Scene";
        case Field::previousScene: return "Previous Scene";
        case Field::nextScene: return "Next Scene";
        case Field::enable: return "Map " + rowLabel + " Enable";
        case Field::input:  return "Map " + rowLabel + " Input";
        case Field::output: return "Map " + rowLabel + " Output";
    }

    jassertfalse;
    return {};
}

inline juce::String noteName (int midiNote)
{
    return juce::MidiMessage::getMidiNoteName (midiNote, true, true, 3) + " (" + juce::String (midiNote) + ")";
}

inline juce::StringArray buildNoteChoices()
{
    juce::StringArray choices;
    choices.ensureStorageAllocated (numMidiNotes);

    for (int note = 0; note < numMidiNotes; ++note)
        choices.add (noteName (note));

    return choices;
}

inline juce::StringArray buildSceneChoices()
{
    juce::StringArray choices;
    choices.ensureStorageAllocated (numScenes);

    for (int scene = 0; scene < numScenes; ++scene)
        choices.add ("Scene " + juce::String (scene + 1));

    return choices;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto noteChoices = buildNoteChoices();
    const auto sceneChoices = buildSceneChoices();

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { makeParameterId (Field::currentScene, 0), 1 },
        makeParameterName (Field::currentScene, 0),
        sceneChoices,
        0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { makeParameterId (Field::previousScene, 0), 1 },
        makeParameterName (Field::previousScene, 0),
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { makeParameterId (Field::nextScene, 0), 1 },
        makeParameterName (Field::nextScene, 0),
        false));

    for (int row = 0; row < numRows; ++row)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { makeParameterId (Field::enable, row), 1 },
            makeParameterName (Field::enable, row),
            false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { makeParameterId (Field::input, row), 1 },
            makeParameterName (Field::input, row),
            noteChoices,
            row));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { makeParameterId (Field::output, row), 1 },
            makeParameterName (Field::output, row),
            noteChoices,
            row));
    }

    return layout;
}
}  // namespace jdc::parameters
