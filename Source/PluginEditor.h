#pragma once

#include "PluginProcessor.h"

#include <array>
#include <functional>

class SceneButton final : public juce::TextButton
{
public:
    std::function<void()> onRenameRequested;

    void mouseDoubleClick (const juce::MouseEvent& event) override
    {
        juce::ignoreUnused (event);

        if (onRenameRequested != nullptr)
            onRenameRequested();
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class MappingRowComponent final : public juce::Component
{
public:
    MappingRowComponent (JDCMidiMapperAudioProcessor& processor,
                         int rowIndex,
                         const juce::StringArray& noteChoices,
                         std::function<void (int)> onSelected);

    bool matchesFilters (const juce::String& query, bool showEnabledOnly) const;
    int getPreferredHeight() const noexcept;
    bool isEnabledRow() const noexcept;
    void setSelected (bool shouldBeSelected);
    int getRowIndex() const noexcept;
    void updateLearnState();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    JDCMidiMapperAudioProcessor& ownerProcessor;
    const int rowIndex;
    std::function<void (int)> onSelected;
    bool selected = false;

    juce::Label rowLabel;
    juce::ToggleButton enableToggle;
    juce::ComboBox inputNoteBox;
    juce::ComboBox outputNoteBox;
    juce::TextButton learnButton { "Learn" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingRowComponent)
};

class MappingTableContent final : public juce::Component
{
public:
    explicit MappingTableContent (JDCMidiMapperAudioProcessor& processor,
                                  std::function<void (int)> onSelectionChanged);

    void applyFilters (juce::String query, bool showEnabledOnly);
    int getVisibleRowCount() const noexcept;
    int getTotalRowCount() const noexcept;
    void setSelectedRow (int rowIndex);
    int getSelectedRow() const noexcept;
    void updateLearnState();

    void resized() override;

private:
    JDCMidiMapperAudioProcessor& ownerProcessor;
    const juce::StringArray noteChoices;
    juce::String currentQuery;
    bool showEnabledOnly = false;
    int selectedRow = 0;
    std::function<void (int)> onSelectionChanged;
    std::array<std::unique_ptr<MappingRowComponent>, jdc::parameters::numRows> rowComponents;

    void refreshVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingTableContent)
};

class JDCMidiMapperAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer,
                                                private juce::AudioProcessorValueTreeState::Listener,
                                                private juce::AsyncUpdater
{
public:
    explicit JDCMidiMapperAudioProcessorEditor (JDCMidiMapperAudioProcessor&);
    ~JDCMidiMapperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JDCMidiMapperAudioProcessor& ownerProcessor;
    juce::Rectangle<int> tableHeaderBounds;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label searchLabel;
    juce::Label statusLabel;
    juce::Label versionLabel;
    juce::TextEditor searchEditor;
    juce::TextEditor sceneRenameEditor;

    juce::ToggleButton showEnabledOnlyToggle;
    juce::ToggleButton switchSceneByProgramChangeToggle;
    juce::ToggleButton filterProgramChangeAfterSwitchToggle;
    juce::TextButton clearSearchButton { "Clear" };
    juce::TextButton copySceneButton { "Copy Scene" };
    juce::TextButton pasteSceneButton { "Paste Scene" };
    juce::TextButton duplicateSceneButton { "Duplicate Scene" };
    juce::TextButton resetAllButton { "Reset All" };
    juce::TextButton disableAllButton { "Disable All" };
    juce::TextButton enableAllButton { "Enable All" };
    juce::TextButton panicButton { "Panic" };
    std::array<SceneButton, jdc::parameters::numScenes> sceneButtons;

    juce::Viewport tableViewport;
    MappingTableContent tableContent;

    int displayedSceneIndex = 0;
    uint32_t lastMidiActivityCounter = 0;
    int midiFlashTicks = 0;
    int renamingSceneIndex = -1;
    std::atomic<int> pendingDisplayedSceneIndex { -1 };

    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void configureButton (juce::TextButton& button, juce::Colour background, juce::Colour text);
    void refreshStatusText();
    void refreshSceneButtonStyles();
    void refreshSceneButtonLabels();
    void updateLearnState();
    void applyTableFilters();
    void renameScene (int sceneIndex);
    void finishSceneRename (bool shouldCommit);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JDCMidiMapperAudioProcessorEditor)
};
