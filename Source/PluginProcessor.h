#pragma once

#include "Parameters.h"

#include <array>
#include <atomic>

class JDCMidiMapperAudioProcessor final : public juce::AudioProcessor,
                                          private juce::AudioProcessorValueTreeState::Listener,
                                          private juce::AsyncUpdater
{
public:
    JDCMidiMapperAudioProcessor();
    ~JDCMidiMapperAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept;
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept;

    juce::RangedAudioParameter& getEnableParameter (int row) const noexcept;
    juce::RangedAudioParameter& getInputParameter (int row) const noexcept;
    juce::RangedAudioParameter& getOutputParameter (int row) const noexcept;
    juce::RangedAudioParameter& getCurrentSceneParameter() const noexcept;
    juce::RangedAudioParameter& getPreviousSceneParameter() const noexcept;
    juce::RangedAudioParameter& getNextSceneParameter() const noexcept;

    void requestPanic() noexcept;
    void resetAllMappings();
    void setAllRowsEnabled (bool shouldBeEnabled);
    void setCurrentSceneIndexFromUI (int sceneIndex);
    int getActiveSceneIndex() const noexcept;
    int getMappedRowCountForActiveScene() const noexcept;
    uint32_t getMidiActivityCounter() const noexcept;
    juce::String getSceneName (int sceneIndex) const;
    void setSceneName (int sceneIndex, juce::String newName);
    bool getSwitchSceneByProgramChangeEnabled() const noexcept;
    void setSwitchSceneByProgramChangeEnabled (bool shouldEnable) noexcept;
    bool getFilterProgramChangeAfterSwitchEnabled() const noexcept;
    void setFilterProgramChangeAfterSwitchEnabled (bool shouldEnable) noexcept;
    void copyActiveScene();
    void pasteClipboardToActiveScene();
    void duplicateActiveSceneToNext();
    void armLearnForRow (int rowIndex) noexcept;
    void cancelLearn() noexcept;
    bool isLearnArmed() const noexcept;
    int getLearnArmedRowForActiveScene() const noexcept;

private:
    static constexpr int trackedNoteStackDepth = 8;
    struct SceneRow
    {
        std::atomic<uint8_t> enabled { 0 };
        std::atomic<uint8_t> inputNote { 0 };
        std::atomic<uint8_t> outputNote { 0 };
    };

    using SceneRows = std::array<SceneRow, jdc::parameters::numRows>;
    struct SceneClipboardRow
    {
        uint8_t enabled = 0;
        uint8_t inputNote = 0;
        uint8_t outputNote = 0;
    };
    using SceneClipboardRows = std::array<SceneClipboardRow, jdc::parameters::numRows>;
    using ActiveNoteMatrix = std::array<std::array<uint16_t, jdc::parameters::numMidiNotes>, jdc::parameters::numMidiChannels>;
    using PerSourceNoteDepthMatrix = std::array<std::array<uint8_t, jdc::parameters::numMidiNotes>, jdc::parameters::numMidiChannels>;
    using PerSourceNoteTargetStack = std::array<std::array<std::array<uint8_t, trackedNoteStackDepth>, jdc::parameters::numMidiNotes>, jdc::parameters::numMidiChannels>;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void refreshMappingCache() noexcept;
    void clearActiveMappedNotes() noexcept;
    void requestPanicAndResetTracking() noexcept;
    void appendPanicMessages (juce::MidiBuffer& midiBuffer);
    void pushRemappedTarget (int channel, int sourceNote, int mappedNote) noexcept;
    bool popRemappedTarget (int channel, int sourceNote, int& mappedNote) noexcept;
    void setParameterValueAndNotifyHost (juce::RangedAudioParameter& parameter, float normalisedValue);
    void copyParametersToScene (int sceneIndex) noexcept;
    void applySceneToExposedParameters (int sceneIndex);
    void clearScene (int sceneIndex) noexcept;
    void copyScene (int sourceSceneIndex, int targetSceneIndex) noexcept;
    void setSceneRowInputFromLearn (int sceneIndex, int rowIndex, int midiNote);
    void setActiveSceneIndexInternal (int sceneIndex, bool syncParameterAsync);
    void stepSceneFromTrigger (int offset);

    juce::AudioProcessorValueTreeState valueTreeState;

    juce::RangedAudioParameter* currentSceneParameter = nullptr;
    juce::RangedAudioParameter* previousSceneParameter = nullptr;
    juce::RangedAudioParameter* nextSceneParameter = nullptr;
    std::array<juce::RangedAudioParameter*, jdc::parameters::numRows> enableParameters {};
    std::array<juce::RangedAudioParameter*, jdc::parameters::numRows> inputParameters {};
    std::array<juce::RangedAudioParameter*, jdc::parameters::numRows> outputParameters {};

    std::atomic<float>* currentSceneValue = nullptr;
    std::array<std::atomic<float>*, jdc::parameters::numRows> enableValues {};
    std::array<std::atomic<float>*, jdc::parameters::numRows> inputValues {};
    std::array<std::atomic<float>*, jdc::parameters::numRows> outputValues {};

    std::array<SceneRows, jdc::parameters::numScenes> scenes {};
    std::array<juce::String, jdc::parameters::numScenes> sceneNames {};
    SceneClipboardRows clipboardScene {};
    bool clipboardHasData = false;

    std::array<uint8_t, jdc::parameters::numMidiNotes> mappingTable {};
    std::array<bool, jdc::parameters::numMidiNotes> mappingEnabled {};
    ActiveNoteMatrix activeMappedNotes {};
    PerSourceNoteDepthMatrix activeRemappedSourceDepths {};
    PerSourceNoteTargetStack activeRemappedSourceTargets {};

    std::atomic<int> activeSceneIndex { 0 };
    std::atomic<int> pendingSceneIndex { 0 };
    std::atomic<bool> switchSceneByProgramChangeEnabled { false };
    std::atomic<bool> filterProgramChangeAfterSwitchEnabled { false };
    std::atomic<uint32_t> mappingRevision { 1 };
    std::atomic<bool> panicRequested { false };
    std::atomic<uint32_t> midiActivityCounter { 0 };
    std::atomic<int> learnArmedRow { -1 };
    std::atomic<int> learnArmedScene { -1 };
    std::atomic<int> pendingLearnedRow { -1 };
    std::atomic<int> pendingLearnedScene { -1 };
    std::atomic<int> pendingLearnedNote { -1 };
    std::atomic<bool> pendingPreviousSceneReset { false };
    std::atomic<bool> pendingNextSceneReset { false };
    std::atomic<bool> pendingSceneParameterSync { false };
    std::atomic<bool> suppressSceneWriteback { false };
    uint32_t appliedMappingRevision = 0;

    juce::MidiBuffer scratchMidiBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JDCMidiMapperAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
