#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int minimumScratchBytes = 8192;
const juce::Identifier sceneBankId { "SceneBank" };
const juce::Identifier sceneId { "Scene" };
const juce::Identifier rowId { "Row" };
const juce::Identifier stateVersionId { "stateVersion" };
const juce::Identifier activeSceneId { "activeScene" };
const juce::Identifier switchSceneByProgramChangeId { "switchSceneByProgramChange" };
const juce::Identifier filterProgramChangeAfterSwitchId { "filterProgramChangeAfterSwitch" };
const juce::Identifier sceneIndexId { "sceneIndex" };
const juce::Identifier sceneNameId { "sceneName" };
const juce::Identifier rowIndexId { "rowIndex" };
const juce::Identifier enabledId { "enabled" };
const juce::Identifier inputId { "input" };
const juce::Identifier outputId { "output" };
}

JDCMidiMapperAudioProcessor::JDCMidiMapperAudioProcessor()
    : AudioProcessor (BusesProperties()),
      valueTreeState (*this, nullptr, "state", jdc::parameters::createParameterLayout())
{
    currentSceneParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0));
    previousSceneParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::previousScene, 0));
    nextSceneParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::nextScene, 0));
    currentSceneValue = valueTreeState.getRawParameterValue (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0));
    jassert (currentSceneParameter != nullptr);
    jassert (previousSceneParameter != nullptr);
    jassert (nextSceneParameter != nullptr);
    jassert (currentSceneValue != nullptr);

    valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0), this);
    valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::previousScene, 0), this);
    valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::nextScene, 0), this);

    for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
    {
        clearScene (scene);
        sceneNames[static_cast<size_t> (scene)] = "Scene " + juce::String (scene + 1);
    }

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto* enableParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::enable, row));
        auto* inputParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::input, row));
        auto* outputParameter = valueTreeState.getParameter (jdc::parameters::makeParameterId (jdc::parameters::Field::output, row));

        jassert (enableParameter != nullptr);
        jassert (inputParameter != nullptr);
        jassert (outputParameter != nullptr);

        enableParameters[static_cast<size_t> (row)] = enableParameter;
        inputParameters[static_cast<size_t> (row)] = inputParameter;
        outputParameters[static_cast<size_t> (row)] = outputParameter;

        enableValues[static_cast<size_t> (row)] = valueTreeState.getRawParameterValue (jdc::parameters::makeParameterId (jdc::parameters::Field::enable, row));
        inputValues[static_cast<size_t> (row)] = valueTreeState.getRawParameterValue (jdc::parameters::makeParameterId (jdc::parameters::Field::input, row));
        outputValues[static_cast<size_t> (row)] = valueTreeState.getRawParameterValue (jdc::parameters::makeParameterId (jdc::parameters::Field::output, row));

        valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::enable, row), this);
        valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::input, row), this);
        valueTreeState.addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::output, row), this);
    }

    copyParametersToScene (0);
    refreshMappingCache();
}

JDCMidiMapperAudioProcessor::~JDCMidiMapperAudioProcessor()
{
    cancelPendingUpdate();
    valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0), this);
    valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::previousScene, 0), this);
    valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::nextScene, 0), this);

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::enable, row), this);
        valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::input, row), this);
        valueTreeState.removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::output, row), this);
    }
}

const juce::String JDCMidiMapperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JDCMidiMapperAudioProcessor::acceptsMidi() const
{
    return true;
}

bool JDCMidiMapperAudioProcessor::producesMidi() const
{
    return true;
}

bool JDCMidiMapperAudioProcessor::isMidiEffect() const
{
    return true;
}

double JDCMidiMapperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JDCMidiMapperAudioProcessor::getNumPrograms()
{
    return 1;
}

int JDCMidiMapperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JDCMidiMapperAudioProcessor::setCurrentProgram (int)
{
}

const juce::String JDCMidiMapperAudioProcessor::getProgramName (int)
{
    return {};
}

void JDCMidiMapperAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void JDCMidiMapperAudioProcessor::prepareToPlay (double, int samplesPerBlock)
{
    scratchMidiBuffer.ensureSize (static_cast<size_t> (juce::jmax (minimumScratchBytes, samplesPerBlock * 64)));
    clearActiveMappedNotes();
    refreshMappingCache();
}

void JDCMidiMapperAudioProcessor::releaseResources()
{
}

bool JDCMidiMapperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts == BusesLayout {};
}

void JDCMidiMapperAudioProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer& midiMessages)
{
    const auto pendingRevision = mappingRevision.load (std::memory_order_acquire);

    if (pendingRevision != appliedMappingRevision)
        refreshMappingCache();

    scratchMidiBuffer.clear();

    if (panicRequested.exchange (false, std::memory_order_acq_rel))
        appendPanicMessages (scratchMidiBuffer);

    const auto hasIncomingMidi = ! midiMessages.isEmpty();

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            const auto armedRow = learnArmedRow.load (std::memory_order_acquire);

            if (armedRow >= 0)
            {
                const auto learnedScene = learnArmedScene.exchange (-1, std::memory_order_acq_rel);

                if (learnArmedRow.exchange (-1, std::memory_order_acq_rel) >= 0)
                {
                    pendingLearnedRow.store (armedRow, std::memory_order_release);
                    pendingLearnedScene.store (learnedScene, std::memory_order_release);
                    pendingLearnedNote.store (message.getNoteNumber(), std::memory_order_release);
                    triggerAsyncUpdate();
                }
            }
        }

        if (message.isProgramChange()
            && switchSceneByProgramChangeEnabled.load (std::memory_order_acquire))
        {
            const auto programNumber = message.getProgramChangeNumber();

            if (juce::isPositiveAndBelow (programNumber, jdc::parameters::numScenes))
            {
                setActiveSceneIndexInternal (programNumber, false);

                if (filterProgramChangeAfterSwitchEnabled.load (std::memory_order_acquire))
                    continue;
            }
        }

        if (message.isNoteOnOrOff())
        {
            const auto noteNumber = message.getNoteNumber();
            const auto channel = juce::jlimit (0, jdc::parameters::numMidiChannels - 1, message.getChannel() - 1);
            const auto shouldRemap = mappingEnabled[static_cast<size_t> (noteNumber)];
            const auto mappedNote = static_cast<int> (mappingTable[static_cast<size_t> (noteNumber)]);

            if (message.isNoteOn())
            {
                if (shouldRemap)
                {
                    message.setNoteNumber (mappedNote);
                    pushRemappedTarget (channel, noteNumber, mappedNote);

                    auto& activeCount = activeMappedNotes[static_cast<size_t> (channel)][static_cast<size_t> (mappedNote)];
                    activeCount = static_cast<uint16_t> (juce::jmin (65535, static_cast<int> (activeCount) + 1));
                }
            }
            else
            {
                int remappedTarget = 0;

                if (popRemappedTarget (channel, noteNumber, remappedTarget))
                {
                    message.setNoteNumber (remappedTarget);

                    auto& activeCount = activeMappedNotes[static_cast<size_t> (channel)][static_cast<size_t> (remappedTarget)];

                    if (activeCount > 0)
                        --activeCount;
                }
                else if (shouldRemap)
                {
                    message.setNoteNumber (mappedNote);
                }
            }
        }

        scratchMidiBuffer.addEvent (message, metadata.samplePosition);
    }

    if (hasIncomingMidi)
        midiActivityCounter.fetch_add (1, std::memory_order_release);

    midiMessages.swapWith (scratchMidiBuffer);
}

bool JDCMidiMapperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* JDCMidiMapperAudioProcessor::createEditor()
{
    return new JDCMidiMapperAudioProcessorEditor (*this);
}

void JDCMidiMapperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty (stateVersionId, 2, nullptr);
    state.setProperty (activeSceneId, getActiveSceneIndex(), nullptr);
    state.setProperty (switchSceneByProgramChangeId, switchSceneByProgramChangeEnabled.load (std::memory_order_relaxed), nullptr);
    state.setProperty (filterProgramChangeAfterSwitchId, filterProgramChangeAfterSwitchEnabled.load (std::memory_order_relaxed), nullptr);

    state.removeChild (state.getChildWithName (sceneBankId), nullptr);

    juce::ValueTree sceneBank (sceneBankId);

    for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
    {
        juce::ValueTree sceneTree (sceneId);
        sceneTree.setProperty (sceneIndexId, scene, nullptr);
        sceneTree.setProperty (sceneNameId, sceneNames[static_cast<size_t> (scene)], nullptr);

        for (int row = 0; row < jdc::parameters::numRows; ++row)
        {
            juce::ValueTree rowTree (rowId);
            const auto& sceneRow = scenes[static_cast<size_t> (scene)][static_cast<size_t> (row)];

            rowTree.setProperty (rowIndexId, row, nullptr);
            rowTree.setProperty (enabledId, static_cast<int> (sceneRow.enabled.load (std::memory_order_relaxed)), nullptr);
            rowTree.setProperty (inputId, static_cast<int> (sceneRow.inputNote.load (std::memory_order_relaxed)), nullptr);
            rowTree.setProperty (outputId, static_cast<int> (sceneRow.outputNote.load (std::memory_order_relaxed)), nullptr);
            sceneTree.addChild (rowTree, -1, nullptr);
        }

        sceneBank.addChild (sceneTree, -1, nullptr);
    }

    state.addChild (sceneBank, -1, nullptr);

    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void JDCMidiMapperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto restoredState = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
        restoredState.isValid())
    {
        valueTreeState.replaceState (restoredState);

        for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
            clearScene (scene);

        switchSceneByProgramChangeEnabled.store (static_cast<bool> (restoredState.getProperty (switchSceneByProgramChangeId, false)), std::memory_order_release);
        filterProgramChangeAfterSwitchEnabled.store (static_cast<bool> (restoredState.getProperty (filterProgramChangeAfterSwitchId, false)), std::memory_order_release);

        if (const auto sceneBank = restoredState.getChildWithName (sceneBankId); sceneBank.isValid())
        {
            for (const auto sceneTree : sceneBank)
            {
                const auto sceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, static_cast<int> (sceneTree.getProperty (sceneIndexId, 0)));
                sceneNames[static_cast<size_t> (sceneIndex)] = sceneTree.getProperty (sceneNameId,
                                                                                      "Scene " + juce::String (sceneIndex + 1)).toString();

                for (const auto rowTree : sceneTree)
                {
                    const auto rowIndex = juce::jlimit (0, jdc::parameters::numRows - 1, static_cast<int> (rowTree.getProperty (rowIndexId, 0)));
                    auto& sceneRow = scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (rowIndex)];

                    sceneRow.enabled.store (static_cast<uint8_t> (juce::jlimit (0, 1, static_cast<int> (rowTree.getProperty (enabledId, 0)))), std::memory_order_relaxed);
                    sceneRow.inputNote.store (static_cast<uint8_t> (juce::jlimit (0, 127, static_cast<int> (rowTree.getProperty (inputId, rowIndex)))), std::memory_order_relaxed);
                    sceneRow.outputNote.store (static_cast<uint8_t> (juce::jlimit (0, 127, static_cast<int> (rowTree.getProperty (outputId, rowIndex)))), std::memory_order_relaxed);
                }
            }
        }
        else
        {
            copyParametersToScene (0);
        }

        const auto restoredScene = juce::jlimit (0, jdc::parameters::numScenes - 1,
                                                 static_cast<int> (restoredState.getProperty (activeSceneId,
                                                                                              juce::roundToInt (currentSceneValue->load (std::memory_order_relaxed)))));

        activeSceneIndex.store (restoredScene, std::memory_order_release);
        pendingSceneIndex.store (restoredScene, std::memory_order_release);

        applySceneToExposedParameters (restoredScene);
        clearActiveMappedNotes();
        mappingRevision.fetch_add (1, std::memory_order_release);
        refreshMappingCache();
    }
}

juce::AudioProcessorValueTreeState& JDCMidiMapperAudioProcessor::getValueTreeState() noexcept
{
    return valueTreeState;
}

const juce::AudioProcessorValueTreeState& JDCMidiMapperAudioProcessor::getValueTreeState() const noexcept
{
    return valueTreeState;
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getEnableParameter (int row) const noexcept
{
    return *enableParameters[static_cast<size_t> (row)];
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getInputParameter (int row) const noexcept
{
    return *inputParameters[static_cast<size_t> (row)];
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getOutputParameter (int row) const noexcept
{
    return *outputParameters[static_cast<size_t> (row)];
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getCurrentSceneParameter() const noexcept
{
    return *currentSceneParameter;
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getPreviousSceneParameter() const noexcept
{
    return *previousSceneParameter;
}

juce::RangedAudioParameter& JDCMidiMapperAudioProcessor::getNextSceneParameter() const noexcept
{
    return *nextSceneParameter;
}

void JDCMidiMapperAudioProcessor::requestPanic() noexcept
{
    panicRequested.store (true, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::resetAllMappings()
{
    requestPanicAndResetTracking();

    const auto sceneIndex = getActiveSceneIndex();

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto& sceneRow = scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)];
        sceneRow.enabled.store (0, std::memory_order_relaxed);
        sceneRow.inputNote.store (static_cast<uint8_t> (row), std::memory_order_relaxed);
        sceneRow.outputNote.store (static_cast<uint8_t> (row), std::memory_order_relaxed);
    }

    applySceneToExposedParameters (sceneIndex);
    mappingRevision.fetch_add (1, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::setAllRowsEnabled (bool shouldBeEnabled)
{
    if (! shouldBeEnabled)
        requestPanicAndResetTracking();

    const auto sceneIndex = getActiveSceneIndex();
    const auto enabledValue = static_cast<uint8_t> (shouldBeEnabled ? 1 : 0);

    for (auto& row : scenes[static_cast<size_t> (sceneIndex)])
        row.enabled.store (enabledValue, std::memory_order_relaxed);

    applySceneToExposedParameters (sceneIndex);
    mappingRevision.fetch_add (1, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::setCurrentSceneIndexFromUI (int sceneIndex)
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    setActiveSceneIndexInternal (boundedSceneIndex, true);

    suppressSceneWriteback.store (true, std::memory_order_release);
    setParameterValueAndNotifyHost (getCurrentSceneParameter(),
                                    getCurrentSceneParameter().convertTo0to1 (static_cast<float> (boundedSceneIndex)));
    suppressSceneWriteback.store (false, std::memory_order_release);
}

int JDCMidiMapperAudioProcessor::getActiveSceneIndex() const noexcept
{
    return activeSceneIndex.load (std::memory_order_acquire);
}

int JDCMidiMapperAudioProcessor::getMappedRowCountForActiveScene() const noexcept
{
    int count = 0;
    const auto sceneIndex = getActiveSceneIndex();

    for (const auto& row : scenes[static_cast<size_t> (sceneIndex)])
        count += row.enabled.load (std::memory_order_relaxed) != 0 ? 1 : 0;

    return count;
}

uint32_t JDCMidiMapperAudioProcessor::getMidiActivityCounter() const noexcept
{
    return midiActivityCounter.load (std::memory_order_acquire);
}

juce::String JDCMidiMapperAudioProcessor::getSceneName (int sceneIndex) const
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    return sceneNames[static_cast<size_t> (boundedSceneIndex)];
}

void JDCMidiMapperAudioProcessor::setSceneName (int sceneIndex, juce::String newName)
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    const auto trimmed = newName.trim();
    sceneNames[static_cast<size_t> (boundedSceneIndex)] = trimmed.isNotEmpty() ? trimmed : "Scene " + juce::String (boundedSceneIndex + 1);
}

bool JDCMidiMapperAudioProcessor::getSwitchSceneByProgramChangeEnabled() const noexcept
{
    return switchSceneByProgramChangeEnabled.load (std::memory_order_acquire);
}

void JDCMidiMapperAudioProcessor::setSwitchSceneByProgramChangeEnabled (bool shouldEnable) noexcept
{
    switchSceneByProgramChangeEnabled.store (shouldEnable, std::memory_order_release);
}

bool JDCMidiMapperAudioProcessor::getFilterProgramChangeAfterSwitchEnabled() const noexcept
{
    return filterProgramChangeAfterSwitchEnabled.load (std::memory_order_acquire);
}

void JDCMidiMapperAudioProcessor::setFilterProgramChangeAfterSwitchEnabled (bool shouldEnable) noexcept
{
    filterProgramChangeAfterSwitchEnabled.store (shouldEnable, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::copyActiveScene()
{
    const auto sceneIndex = getActiveSceneIndex();

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        const auto& sceneRow = scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)];
        auto& clipboardRow = clipboardScene[static_cast<size_t> (row)];

        clipboardRow.enabled = sceneRow.enabled.load (std::memory_order_relaxed);
        clipboardRow.inputNote = sceneRow.inputNote.load (std::memory_order_relaxed);
        clipboardRow.outputNote = sceneRow.outputNote.load (std::memory_order_relaxed);
    }

    clipboardHasData = true;
}

void JDCMidiMapperAudioProcessor::pasteClipboardToActiveScene()
{
    if (! clipboardHasData)
        return;

    requestPanicAndResetTracking();

    const auto sceneIndex = getActiveSceneIndex();

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto& sceneRow = scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)];
        const auto& clipboardRow = clipboardScene[static_cast<size_t> (row)];

        sceneRow.enabled.store (clipboardRow.enabled, std::memory_order_relaxed);
        sceneRow.inputNote.store (clipboardRow.inputNote, std::memory_order_relaxed);
        sceneRow.outputNote.store (clipboardRow.outputNote, std::memory_order_relaxed);
    }

    applySceneToExposedParameters (sceneIndex);
    mappingRevision.fetch_add (1, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::duplicateActiveSceneToNext()
{
    const auto sourceScene = getActiveSceneIndex();
    const auto targetScene = (sourceScene + 1) % jdc::parameters::numScenes;

    copyScene (sourceScene, targetScene);
    setCurrentSceneIndexFromUI (targetScene);
}

void JDCMidiMapperAudioProcessor::armLearnForRow (int rowIndex) noexcept
{
    learnArmedScene.store (getActiveSceneIndex(), std::memory_order_release);
    learnArmedRow.store (juce::jlimit (0, jdc::parameters::numRows - 1, rowIndex), std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::cancelLearn() noexcept
{
    learnArmedRow.store (-1, std::memory_order_release);
    learnArmedScene.store (-1, std::memory_order_release);
}

bool JDCMidiMapperAudioProcessor::isLearnArmed() const noexcept
{
    return learnArmedRow.load (std::memory_order_acquire) >= 0;
}

int JDCMidiMapperAudioProcessor::getLearnArmedRowForActiveScene() const noexcept
{
    const auto armedRow = learnArmedRow.load (std::memory_order_acquire);
    const auto armedScene = learnArmedScene.load (std::memory_order_acquire);

    if (armedRow < 0 || armedScene != getActiveSceneIndex())
        return -1;

    return armedRow;
}

void JDCMidiMapperAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (suppressSceneWriteback.load (std::memory_order_acquire))
        return;

    if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0))
    {
        const auto sceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, juce::roundToInt (newValue));
        setActiveSceneIndexInternal (sceneIndex, true);
        return;
    }

    if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::previousScene, 0))
    {
        if (newValue >= 0.5f)
        {
            stepSceneFromTrigger (-1);
            pendingPreviousSceneReset.store (true, std::memory_order_release);
            triggerAsyncUpdate();
        }

        return;
    }

    if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::nextScene, 0))
    {
        if (newValue >= 0.5f)
        {
            stepSceneFromTrigger (1);
            pendingNextSceneReset.store (true, std::memory_order_release);
            triggerAsyncUpdate();
        }

        return;
    }

    const auto sceneIndex = getActiveSceneIndex();

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::enable, row))
        {
            scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)].enabled.store (newValue >= 0.5f ? 1 : 0, std::memory_order_relaxed);
            mappingRevision.fetch_add (1, std::memory_order_release);
            return;
        }

        if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::input, row))
        {
            scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)].inputNote.store (
                static_cast<uint8_t> (juce::jlimit (0, 127, juce::roundToInt (newValue))), std::memory_order_relaxed);
            mappingRevision.fetch_add (1, std::memory_order_release);
            return;
        }

        if (parameterID == jdc::parameters::makeParameterId (jdc::parameters::Field::output, row))
        {
            scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)].outputNote.store (
                static_cast<uint8_t> (juce::jlimit (0, 127, juce::roundToInt (newValue))), std::memory_order_relaxed);
            mappingRevision.fetch_add (1, std::memory_order_release);
            return;
        }
    }
}

void JDCMidiMapperAudioProcessor::handleAsyncUpdate()
{
    if (pendingPreviousSceneReset.exchange (false, std::memory_order_acq_rel))
    {
        suppressSceneWriteback.store (true, std::memory_order_release);
        setParameterValueAndNotifyHost (getPreviousSceneParameter(), 0.0f);
        suppressSceneWriteback.store (false, std::memory_order_release);
    }

    if (pendingNextSceneReset.exchange (false, std::memory_order_acq_rel))
    {
        suppressSceneWriteback.store (true, std::memory_order_release);
        setParameterValueAndNotifyHost (getNextSceneParameter(), 0.0f);
        suppressSceneWriteback.store (false, std::memory_order_release);
    }

    const auto shouldSyncSceneParameters = pendingSceneParameterSync.exchange (false, std::memory_order_acq_rel);

    const auto learnedNote = pendingLearnedNote.exchange (-1, std::memory_order_acq_rel);

    if (learnedNote >= 0)
    {
        const auto learnedScene = pendingLearnedScene.exchange (-1, std::memory_order_acq_rel);
        const auto learnedRow = pendingLearnedRow.exchange (-1, std::memory_order_acq_rel);

        if (learnedScene >= 0 && learnedRow >= 0)
            setSceneRowInputFromLearn (learnedScene, learnedRow, learnedNote);
    }

    if (shouldSyncSceneParameters)
        applySceneToExposedParameters (pendingSceneIndex.load (std::memory_order_acquire));
}

void JDCMidiMapperAudioProcessor::refreshMappingCache() noexcept
{
    for (int note = 0; note < jdc::parameters::numMidiNotes; ++note)
    {
        mappingTable[static_cast<size_t> (note)] = static_cast<uint8_t> (note);
        mappingEnabled[static_cast<size_t> (note)] = false;
    }

    const auto sceneIndex = activeSceneIndex.load (std::memory_order_acquire);

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        const auto& sceneRow = scenes[static_cast<size_t> (sceneIndex)][static_cast<size_t> (row)];
        const auto enabled = sceneRow.enabled.load (std::memory_order_relaxed) != 0;

        if (! enabled)
            continue;

        const auto inputNote = juce::jlimit (0, 127, static_cast<int> (sceneRow.inputNote.load (std::memory_order_relaxed)));
        const auto outputNote = juce::jlimit (0, 127, static_cast<int> (sceneRow.outputNote.load (std::memory_order_relaxed)));

        mappingTable[static_cast<size_t> (inputNote)] = static_cast<uint8_t> (outputNote);
        mappingEnabled[static_cast<size_t> (inputNote)] = true;
    }

    appliedMappingRevision = mappingRevision.load (std::memory_order_acquire);
}

void JDCMidiMapperAudioProcessor::clearActiveMappedNotes() noexcept
{
    for (auto& channel : activeMappedNotes)
        channel.fill (0);

    for (auto& channelDepths : activeRemappedSourceDepths)
        channelDepths.fill (0);

    for (auto& channelTargets : activeRemappedSourceTargets)
    {
        for (auto& noteTargets : channelTargets)
            noteTargets.fill (0);
    }
}

void JDCMidiMapperAudioProcessor::requestPanicAndResetTracking() noexcept
{
    clearActiveMappedNotes();
    requestPanic();
}

void JDCMidiMapperAudioProcessor::appendPanicMessages (juce::MidiBuffer& midiBuffer)
{
    for (int channel = 0; channel < jdc::parameters::numMidiChannels; ++channel)
    {
        for (int note = 0; note < jdc::parameters::numMidiNotes; ++note)
        {
            auto& activeCount = activeMappedNotes[static_cast<size_t> (channel)][static_cast<size_t> (note)];

            while (activeCount > 0)
            {
                midiBuffer.addEvent (juce::MidiMessage::noteOff (channel + 1, note), 0);
                --activeCount;
            }
        }

        midiBuffer.addEvent (juce::MidiMessage::allNotesOff (channel + 1), 0);
        midiBuffer.addEvent (juce::MidiMessage::allSoundOff (channel + 1), 0);
    }

    for (auto& channelDepths : activeRemappedSourceDepths)
        channelDepths.fill (0);
}

void JDCMidiMapperAudioProcessor::pushRemappedTarget (int channel, int sourceNote, int mappedNote) noexcept
{
    auto& depth = activeRemappedSourceDepths[static_cast<size_t> (channel)][static_cast<size_t> (sourceNote)];
    auto& targetStack = activeRemappedSourceTargets[static_cast<size_t> (channel)][static_cast<size_t> (sourceNote)];

    if (depth < trackedNoteStackDepth)
    {
        targetStack[depth] = static_cast<uint8_t> (mappedNote);
        ++depth;
        return;
    }

    for (int index = 1; index < trackedNoteStackDepth; ++index)
        targetStack[static_cast<size_t> (index - 1)] = targetStack[static_cast<size_t> (index)];

    targetStack[static_cast<size_t> (trackedNoteStackDepth - 1)] = static_cast<uint8_t> (mappedNote);
}

bool JDCMidiMapperAudioProcessor::popRemappedTarget (int channel, int sourceNote, int& mappedNote) noexcept
{
    auto& depth = activeRemappedSourceDepths[static_cast<size_t> (channel)][static_cast<size_t> (sourceNote)];

    if (depth == 0)
        return false;

    --depth;
    mappedNote = activeRemappedSourceTargets[static_cast<size_t> (channel)][static_cast<size_t> (sourceNote)][depth];
    return true;
}

void JDCMidiMapperAudioProcessor::setParameterValueAndNotifyHost (juce::RangedAudioParameter& parameter, float normalisedValue)
{
    if (juce::approximatelyEqual (parameter.getValue(), normalisedValue))
        return;

    parameter.beginChangeGesture();
    parameter.setValueNotifyingHost (normalisedValue);
    parameter.endChangeGesture();
}

void JDCMidiMapperAudioProcessor::copyParametersToScene (int sceneIndex) noexcept
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto& sceneRow = scenes[static_cast<size_t> (boundedSceneIndex)][static_cast<size_t> (row)];
        sceneRow.enabled.store (enableValues[static_cast<size_t> (row)]->load (std::memory_order_relaxed) >= 0.5f ? 1 : 0, std::memory_order_relaxed);
        sceneRow.inputNote.store (static_cast<uint8_t> (juce::jlimit (0, 127, juce::roundToInt (inputValues[static_cast<size_t> (row)]->load (std::memory_order_relaxed)))), std::memory_order_relaxed);
        sceneRow.outputNote.store (static_cast<uint8_t> (juce::jlimit (0, 127, juce::roundToInt (outputValues[static_cast<size_t> (row)]->load (std::memory_order_relaxed)))), std::memory_order_relaxed);
    }
}

void JDCMidiMapperAudioProcessor::applySceneToExposedParameters (int sceneIndex)
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    suppressSceneWriteback.store (true, std::memory_order_release);

    setParameterValueAndNotifyHost (getCurrentSceneParameter(),
                                    getCurrentSceneParameter().convertTo0to1 (static_cast<float> (boundedSceneIndex)));

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        const auto& sceneRow = scenes[static_cast<size_t> (boundedSceneIndex)][static_cast<size_t> (row)];
        setParameterValueAndNotifyHost (getEnableParameter (row), sceneRow.enabled.load (std::memory_order_relaxed) != 0 ? 1.0f : 0.0f);
        setParameterValueAndNotifyHost (getInputParameter (row), getInputParameter (row).convertTo0to1 (static_cast<float> (sceneRow.inputNote.load (std::memory_order_relaxed))));
        setParameterValueAndNotifyHost (getOutputParameter (row), getOutputParameter (row).convertTo0to1 (static_cast<float> (sceneRow.outputNote.load (std::memory_order_relaxed))));
    }

    suppressSceneWriteback.store (false, std::memory_order_release);
}

void JDCMidiMapperAudioProcessor::clearScene (int sceneIndex) noexcept
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto& sceneRow = scenes[static_cast<size_t> (boundedSceneIndex)][static_cast<size_t> (row)];
        sceneRow.enabled.store (0, std::memory_order_relaxed);
        sceneRow.inputNote.store (static_cast<uint8_t> (row), std::memory_order_relaxed);
        sceneRow.outputNote.store (static_cast<uint8_t> (row), std::memory_order_relaxed);
    }
}

void JDCMidiMapperAudioProcessor::copyScene (int sourceSceneIndex, int targetSceneIndex) noexcept
{
    const auto boundedSourceScene = juce::jlimit (0, jdc::parameters::numScenes - 1, sourceSceneIndex);
    const auto boundedTargetScene = juce::jlimit (0, jdc::parameters::numScenes - 1, targetSceneIndex);

    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        const auto& sourceRow = scenes[static_cast<size_t> (boundedSourceScene)][static_cast<size_t> (row)];
        auto& targetRow = scenes[static_cast<size_t> (boundedTargetScene)][static_cast<size_t> (row)];

        targetRow.enabled.store (sourceRow.enabled.load (std::memory_order_relaxed), std::memory_order_relaxed);
        targetRow.inputNote.store (sourceRow.inputNote.load (std::memory_order_relaxed), std::memory_order_relaxed);
        targetRow.outputNote.store (sourceRow.outputNote.load (std::memory_order_relaxed), std::memory_order_relaxed);
    }

    if (boundedTargetScene == getActiveSceneIndex())
    {
        applySceneToExposedParameters (boundedTargetScene);
        mappingRevision.fetch_add (1, std::memory_order_release);
    }
}

void JDCMidiMapperAudioProcessor::setSceneRowInputFromLearn (int sceneIndex, int rowIndex, int midiNote)
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    const auto boundedRowIndex = juce::jlimit (0, jdc::parameters::numRows - 1, rowIndex);
    const auto boundedNote = juce::jlimit (0, 127, midiNote);

    scenes[static_cast<size_t> (boundedSceneIndex)][static_cast<size_t> (boundedRowIndex)].inputNote.store (static_cast<uint8_t> (boundedNote), std::memory_order_relaxed);

    if (boundedSceneIndex == getActiveSceneIndex())
    {
        suppressSceneWriteback.store (true, std::memory_order_release);
        setParameterValueAndNotifyHost (getInputParameter (boundedRowIndex),
                                        getInputParameter (boundedRowIndex).convertTo0to1 (static_cast<float> (boundedNote)));
        suppressSceneWriteback.store (false, std::memory_order_release);
        mappingRevision.fetch_add (1, std::memory_order_release);
    }
}

void JDCMidiMapperAudioProcessor::setActiveSceneIndexInternal (int sceneIndex, bool syncParameterAsync)
{
    const auto boundedSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    const auto previousSceneIndex = activeSceneIndex.exchange (boundedSceneIndex, std::memory_order_acq_rel);
    pendingSceneIndex.store (boundedSceneIndex, std::memory_order_release);
    
    if (boundedSceneIndex != previousSceneIndex)
        mappingRevision.fetch_add (1, std::memory_order_release);

    juce::ignoreUnused (syncParameterAsync);
    pendingSceneParameterSync.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

void JDCMidiMapperAudioProcessor::stepSceneFromTrigger (int offset)
{
    const auto currentScene = getActiveSceneIndex();
    const auto wrappedScene = (currentScene + offset + jdc::parameters::numScenes) % jdc::parameters::numScenes;
    setCurrentSceneIndexFromUI (wrappedScene);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JDCMidiMapperAudioProcessor();
}
