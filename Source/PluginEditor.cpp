#include "PluginEditor.h"

namespace
{
constexpr int rowHeight = 32;
constexpr int panelCornerSize = 18;
constexpr int outerMargin = 24;
constexpr int sectionGap = 10;
constexpr int sceneSectionGap = 16;
constexpr int headerGap = 10;
constexpr int controlRowHeight = 34;
constexpr int narrowControlHeight = 26;
constexpr int toolbarButtonHeight = 34;
constexpr int toolbarButtonGap = 10;
constexpr int tableHeaderHeight = 24;
constexpr int tableColumnInsetX = 12;
constexpr int tableColumnInsetY = 5;
constexpr int tableColumnGap = 12;
constexpr int rowColumnWidth = 60;
constexpr int enabledColumnWidth = 120;
constexpr int minimumInputNoteColumnWidth = 200;
constexpr int minimumOutputNoteColumnWidth = 250;
constexpr int learnColumnWidth = 78;
constexpr int clearButtonWidth = 86;
constexpr int minimumStatusWidth = 360;
constexpr int sceneButtonGap = 8;
constexpr int searchLabelWidth = 62;
constexpr int searchFieldGap = 12;
constexpr int statusGap = 16;
constexpr int titleWidth = 340;
constexpr int toolbarButtonWidth = 136;
constexpr int duplicateButtonWidth = 124;
constexpr int standardButtonWidth = 104;
constexpr int versionLabelInset = 18;
constexpr int versionLabelBottomInset = 10;
constexpr int versionLabelHeight = 14;

struct TableColumnLayout
{
    int rowWidth = rowColumnWidth;
    int enabledWidth = enabledColumnWidth;
    int inputWidth = minimumInputNoteColumnWidth;
    int outputWidth = minimumOutputNoteColumnWidth;
    int learnWidth = learnColumnWidth;
};

TableColumnLayout getTableColumnLayout (int totalWidth)
{
    TableColumnLayout layout;
    const auto contentWidth = juce::jmax (0, totalWidth - (tableColumnInsetX * 2));
    const auto fixedWidth = layout.rowWidth + layout.enabledWidth + layout.learnWidth + (tableColumnGap * 4);
    const auto minimumNoteAreaWidth = minimumInputNoteColumnWidth + minimumOutputNoteColumnWidth;
    const auto noteAreaWidth = juce::jmax (minimumNoteAreaWidth, contentWidth - fixedWidth);
    layout.inputWidth = juce::jmax (minimumInputNoteColumnWidth, juce::roundToInt (static_cast<float> (noteAreaWidth) * 0.40f));
    layout.outputWidth = juce::jmax (minimumOutputNoteColumnWidth, noteAreaWidth - layout.inputWidth);
    return layout;
}
}

void SceneButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = getLocalBounds().toFloat();
    auto background = findColour (getToggleState() ? juce::TextButton::buttonOnColourId
                                                   : juce::TextButton::buttonColourId);

    if (shouldDrawButtonAsDown)
        background = background.darker (0.15f);
    else if (shouldDrawButtonAsHighlighted)
        background = background.brighter (0.08f);

    g.setColour (background);
    g.fillRoundedRectangle (bounds.reduced (0.5f), 7.0f);

    g.setColour (juce::Colour (0x773d4757));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 7.0f, 1.0f);

    g.setColour (findColour (getToggleState() ? juce::TextButton::textColourOnId
                                              : juce::TextButton::textColourOffId));
    auto buttonFont = getLookAndFeel().getTextButtonFont (*this, getHeight())
                          .withHeight (juce::jmin (17.0f, static_cast<float> (getHeight()) * 0.46f));
    g.setFont (buttonFont);
    g.drawFittedText (getButtonText(),
                      getLocalBounds().reduced (8, 4),
                      juce::Justification::centred,
                      1,
                      0.68f);
}

MappingRowComponent::MappingRowComponent (JDCMidiMapperAudioProcessor& owner,
                                          int row,
                                          const juce::StringArray& noteChoices,
                                          std::function<void (int)> onSelectedCallback)
    : ownerProcessor (owner),
      rowIndex (row),
      onSelected (std::move (onSelectedCallback))
{
    rowLabel.setText (juce::String (rowIndex + 1).paddedLeft ('0', 3), juce::dontSendNotification);
    rowLabel.setJustificationType (juce::Justification::centredLeft);
    rowLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8ced8));
    addAndMakeVisible (rowLabel);

    enableToggle.setClickingTogglesState (true);
    enableToggle.setButtonText ("Active");
    enableToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffdfe7f1));
    addAndMakeVisible (enableToggle);

    auto populateNoteBox = [&noteChoices] (juce::ComboBox& box)
    {
        box.addItemList (noteChoices, 1);
        box.setJustificationType (juce::Justification::centredLeft);
        box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff232934));
        box.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3d4757));
        box.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeef2f7));
        box.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffeef2f7));
    };

    populateNoteBox (inputNoteBox);
    populateNoteBox (outputNoteBox);

    addAndMakeVisible (inputNoteBox);
    addAndMakeVisible (outputNoteBox);

    learnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff6a5521));
    learnButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8c6c1f));
    learnButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xfffff4d9));
    learnButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xfffff4d9));
    learnButton.onClick = [this]
    {
        if (ownerProcessor.getLearnArmedRowForActiveScene() == rowIndex)
            ownerProcessor.cancelLearn();
        else
            ownerProcessor.armLearnForRow (rowIndex);

        updateLearnState();
    };
    addAndMakeVisible (learnButton);

    enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        ownerProcessor.getValueTreeState(),
        jdc::parameters::makeParameterId (jdc::parameters::Field::enable, rowIndex),
        enableToggle);

    inputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        ownerProcessor.getValueTreeState(),
        jdc::parameters::makeParameterId (jdc::parameters::Field::input, rowIndex),
        inputNoteBox);

    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        ownerProcessor.getValueTreeState(),
        jdc::parameters::makeParameterId (jdc::parameters::Field::output, rowIndex),
        outputNoteBox);
}

bool MappingRowComponent::matchesFilters (const juce::String& query, bool showEnabledOnly) const
{
    if (showEnabledOnly && ! isEnabledRow())
        return false;

    const auto trimmed = query.trim();

    if (trimmed.isEmpty())
        return true;

    const auto haystack = juce::StringArray {
        rowLabel.getText(),
        inputNoteBox.getText(),
        outputNoteBox.getText(),
        enableToggle.getToggleState() ? "enabled" : "disabled"
    }.joinIntoString (" ").toLowerCase();

    return haystack.contains (trimmed.toLowerCase());
}

int MappingRowComponent::getPreferredHeight() const noexcept
{
    return rowHeight;
}

bool MappingRowComponent::isEnabledRow() const noexcept
{
    return enableToggle.getToggleState();
}

void MappingRowComponent::setSelected (bool shouldBeSelected)
{
    if (selected == shouldBeSelected)
        return;

    selected = shouldBeSelected;
    repaint();
}

int MappingRowComponent::getRowIndex() const noexcept
{
    return rowIndex;
}

void MappingRowComponent::updateLearnState()
{
    const auto isLearningThisRow = ownerProcessor.getLearnArmedRowForActiveScene() == rowIndex;
    learnButton.setButtonText (isLearningThisRow ? "..." : "Learn");
    learnButton.setColour (juce::TextButton::buttonColourId, isLearningThisRow ? juce::Colour (0xff8c6c1f) : juce::Colour (0xff6a5521));
    learnButton.repaint();
}

void MappingRowComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto background = (rowIndex % 2 == 0) ? juce::Colour (0xff151b24) : juce::Colour (0xff11161e);

    g.setColour (selected ? juce::Colour (0xff1f2b39) : background);
    g.fillRoundedRectangle (bounds.reduced (1.0f, 2.0f), 10.0f);

    g.setColour (selected ? juce::Colour (0xff5d8fbe) : juce::Colour (0x223d4757));
    g.drawRoundedRectangle (bounds.reduced (1.0f, 2.0f), 10.0f, selected ? 1.5f : 1.0f);
}

void MappingRowComponent::resized()
{
    auto area = getLocalBounds().reduced (tableColumnInsetX, tableColumnInsetY);
    const auto columns = getTableColumnLayout (getWidth());

    rowLabel.setBounds (area.removeFromLeft (columns.rowWidth));
    area.removeFromLeft (tableColumnGap);
    enableToggle.setBounds (area.removeFromLeft (columns.enabledWidth));
    area.removeFromLeft (tableColumnGap);
    inputNoteBox.setBounds (area.removeFromLeft (columns.inputWidth));
    area.removeFromLeft (tableColumnGap);
    outputNoteBox.setBounds (area.removeFromLeft (columns.outputWidth));
    area.removeFromLeft (tableColumnGap);
    learnButton.setBounds (area.removeFromLeft (columns.learnWidth));
}

void MappingRowComponent::mouseDown (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);

    if (onSelected != nullptr)
        onSelected (rowIndex);
}

MappingTableContent::MappingTableContent (JDCMidiMapperAudioProcessor& owner,
                                          std::function<void (int)> onSelectionChangedCallback)
    : ownerProcessor (owner),
      noteChoices (jdc::parameters::buildNoteChoices()),
      onSelectionChanged (std::move (onSelectionChangedCallback))
{
    for (int row = 0; row < jdc::parameters::numRows; ++row)
    {
        auto& rowComponent = rowComponents[static_cast<size_t> (row)];
        rowComponent = std::make_unique<MappingRowComponent> (ownerProcessor,
                                                              row,
                                                              noteChoices,
                                                              [this] (int selected) { setSelectedRow (selected); });
        addAndMakeVisible (*rowComponent);
    }
    refreshVisibility();
}

void MappingTableContent::applyFilters (juce::String query, bool shouldShowEnabledOnly)
{
    currentQuery = std::move (query);
    showEnabledOnly = shouldShowEnabledOnly;
    refreshVisibility();
    resized();
}

int MappingTableContent::getVisibleRowCount() const noexcept
{
    int visibleRows = 0;

    for (const auto& row : rowComponents)
        visibleRows += row->isVisible() ? 1 : 0;

    return visibleRows;
}

int MappingTableContent::getTotalRowCount() const noexcept
{
    return static_cast<int> (rowComponents.size());
}

void MappingTableContent::setSelectedRow (int rowIndex)
{
    selectedRow = juce::isPositiveAndBelow (rowIndex, jdc::parameters::numRows) ? rowIndex : -1;

    for (auto& row : rowComponents)
        row->setSelected (row->getRowIndex() == selectedRow);

    if (onSelectionChanged != nullptr)
        onSelectionChanged (selectedRow);
}

int MappingTableContent::getSelectedRow() const noexcept
{
    return selectedRow;
}

void MappingTableContent::updateLearnState()
{
    for (auto& row : rowComponents)
        row->updateLearnState();
}

void MappingTableContent::resized()
{
    auto y = 0;

    for (auto& row : rowComponents)
    {
        if (! row->isVisible())
            continue;

        row->setBounds (0, y, getWidth(), row->getPreferredHeight());
        y += row->getPreferredHeight();
    }

    setSize (getWidth(), juce::jmax (y, 1));
}

void MappingTableContent::refreshVisibility()
{
    for (auto& row : rowComponents)
        row->setVisible (row->matchesFilters (currentQuery, showEnabledOnly));
}

JDCMidiMapperAudioProcessorEditor::JDCMidiMapperAudioProcessorEditor (JDCMidiMapperAudioProcessor& owner)
    : AudioProcessorEditor (&owner),
      ownerProcessor (owner),
      tableContent (owner, [this] (int)
      {
          updateLearnState();
          refreshStatusText();
      })
{
    setResizable (true, true);
    setResizeLimits (960, 620, 1320, 1240);
    setSize (1024, 840);

    titleLabel.setText ("JDC MIDI Mapper", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions().withHeight (28.0f)).boldened());
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff4f7fb));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Live-performance MIDI remapping for Gig Performer and drum instruments", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98a3b3));
    addAndMakeVisible (subtitleLabel);

    searchLabel.setText ("Search", juce::dontSendNotification);
    searchLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8ced8));
    addAndMakeVisible (searchLabel);

    statusLabel.setJustificationType (juce::Justification::centredRight);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98a3b3));
    addAndMakeVisible (statusLabel);

    versionLabel.setText ("v" + juce::String (JucePlugin_VersionString), juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    versionLabel.setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
    versionLabel.setColour (juce::Label::textColourId, juce::Colour (0x7faeb8c6));
    versionLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (versionLabel);

    searchEditor.setTextToShowWhenEmpty ("Filter by row, note name or note number", juce::Colour (0xff6d7888));
    searchEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff10161f));
    searchEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff384354));
    searchEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xffeef2f7));
    searchEditor.onTextChange = [this]
    {
        applyTableFilters();
        refreshStatusText();
    };
    addAndMakeVisible (searchEditor);

    sceneRenameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff10161f));
    sceneRenameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff5d8fbe));
    sceneRenameEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xffeef2f7));
    sceneRenameEditor.onReturnKey = [this] { finishSceneRename (true); };
    sceneRenameEditor.onEscapeKey = [this] { finishSceneRename (false); };
    sceneRenameEditor.onFocusLost = [this] { finishSceneRename (true); };
    sceneRenameEditor.setVisible (false);
    addAndMakeVisible (sceneRenameEditor);

    showEnabledOnlyToggle.setButtonText ("Show only enabled mappings");
    showEnabledOnlyToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffdfe7f1));
    showEnabledOnlyToggle.onClick = [this]
    {
        applyTableFilters();
        refreshStatusText();
    };
    addAndMakeVisible (showEnabledOnlyToggle);

    switchSceneByProgramChangeToggle.setButtonText ("Switch Scene by incoming Program Change");
    switchSceneByProgramChangeToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffdfe7f1));
    switchSceneByProgramChangeToggle.setToggleState (ownerProcessor.getSwitchSceneByProgramChangeEnabled(), juce::dontSendNotification);
    switchSceneByProgramChangeToggle.onClick = [this]
    {
        ownerProcessor.setSwitchSceneByProgramChangeEnabled (switchSceneByProgramChangeToggle.getToggleState());
        filterProgramChangeAfterSwitchToggle.setEnabled (switchSceneByProgramChangeToggle.getToggleState());
        refreshStatusText();
    };
    addAndMakeVisible (switchSceneByProgramChangeToggle);

    filterProgramChangeAfterSwitchToggle.setButtonText ("Filter triggering Program Change");
    filterProgramChangeAfterSwitchToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffdfe7f1));
    filterProgramChangeAfterSwitchToggle.setToggleState (ownerProcessor.getFilterProgramChangeAfterSwitchEnabled(), juce::dontSendNotification);
    filterProgramChangeAfterSwitchToggle.setEnabled (switchSceneByProgramChangeToggle.getToggleState());
    filterProgramChangeAfterSwitchToggle.onClick = [this]
    {
        ownerProcessor.setFilterProgramChangeAfterSwitchEnabled (filterProgramChangeAfterSwitchToggle.getToggleState());
        refreshStatusText();
    };
    addAndMakeVisible (filterProgramChangeAfterSwitchToggle);

    configureButton (clearSearchButton, juce::Colour (0xff202833), juce::Colour (0xffdfe7f1));
    configureButton (copySceneButton, juce::Colour (0xff283242), juce::Colour (0xffeef2f7));
    configureButton (pasteSceneButton, juce::Colour (0xff283242), juce::Colour (0xffeef2f7));
    configureButton (duplicateSceneButton, juce::Colour (0xff283242), juce::Colour (0xffeef2f7));
    configureButton (resetAllButton, juce::Colour (0xff283242), juce::Colour (0xffeef2f7));
    configureButton (disableAllButton, juce::Colour (0xff283242), juce::Colour (0xffeef2f7));
    configureButton (enableAllButton, juce::Colour (0xff2f6d5a), juce::Colour (0xfff4fff8));
    configureButton (panicButton, juce::Colour (0xff97363a), juce::Colour (0xfffff6f6));

    clearSearchButton.onClick = [this]
    {
        searchEditor.clear();
        searchEditor.grabKeyboardFocus();
    };

    copySceneButton.onClick = [this] { ownerProcessor.copyActiveScene(); };
    pasteSceneButton.onClick = [this]
    {
        ownerProcessor.pasteClipboardToActiveScene();
        applyTableFilters();
        refreshStatusText();
    };
    duplicateSceneButton.onClick = [this] { ownerProcessor.duplicateActiveSceneToNext(); };
    resetAllButton.onClick = [this]
    {
        ownerProcessor.resetAllMappings();
        applyTableFilters();
        refreshStatusText();
    };
    disableAllButton.onClick = [this]
    {
        ownerProcessor.setAllRowsEnabled (false);
        applyTableFilters();
        refreshStatusText();
    };
    enableAllButton.onClick = [this]
    {
        ownerProcessor.setAllRowsEnabled (true);
        applyTableFilters();
        refreshStatusText();
    };
    panicButton.onClick = [this] { ownerProcessor.requestPanic(); };

    for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
    {
        auto& button = sceneButtons[static_cast<size_t> (scene)];
        button.onClick = [this, scene] { ownerProcessor.setCurrentSceneIndexFromUI (scene); };
        button.onRenameRequested = [this, scene] { renameScene (scene); };
        button.setClickingTogglesState (false);
        addAndMakeVisible (button);
    }

    ownerProcessor.getValueTreeState().addParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0), this);

    tableViewport.setViewedComponent (&tableContent, false);
    tableViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (tableViewport);

    displayedSceneIndex = ownerProcessor.getActiveSceneIndex();
    lastMidiActivityCounter = ownerProcessor.getMidiActivityCounter();
    refreshSceneButtonLabels();
    refreshSceneButtonStyles();
    updateLearnState();
    applyTableFilters();
    refreshStatusText();
    startTimerHz (12);
}

JDCMidiMapperAudioProcessorEditor::~JDCMidiMapperAudioProcessorEditor()
{
    cancelPendingUpdate();
    ownerProcessor.getValueTreeState().removeParameterListener (jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0), this);
}

void JDCMidiMapperAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (juce::Colour (0xff10151d), 0.0f, 0.0f,
                                     juce::Colour (0xff1a2431), static_cast<float> (getWidth()), static_cast<float> (getHeight()), false);
    background.addColour (0.6, juce::Colour (0xff101820));

    g.setGradientFill (background);
    g.fillAll();

    const auto panelBounds = bounds.reduced (18.0f);
    g.setColour (juce::Colour (0xcc0c1118));
    g.fillRoundedRectangle (panelBounds, static_cast<float> (panelCornerSize));

    g.setColour (juce::Colour (0x443f4b5d));
    g.drawRoundedRectangle (panelBounds, static_cast<float> (panelCornerSize), 1.0f);

    if (! tableHeaderBounds.isEmpty())
    {
        const auto alignedHeaderBounds = tableHeaderBounds.withWidth (tableContent.getWidth());
        auto headerLine = alignedHeaderBounds.withHeight (1).toFloat().translated (0.0f, -6.0f);

        g.setColour (juce::Colour (0x553d4757));
        g.fillRect (headerLine);

        auto headerArea = alignedHeaderBounds.reduced (tableColumnInsetX, 0);
        const auto columns = getTableColumnLayout (alignedHeaderBounds.getWidth());
        g.setColour (juce::Colour (0xff93a0b2));
        g.setFont (juce::Font (juce::FontOptions().withHeight (13.0f)).boldened());
        g.drawText ("Row", headerArea.removeFromLeft (columns.rowWidth), juce::Justification::centred);
        headerArea.removeFromLeft (tableColumnGap);
        g.drawText ("Enabled", headerArea.removeFromLeft (columns.enabledWidth), juce::Justification::centred);
        headerArea.removeFromLeft (tableColumnGap);
        g.drawText ("Input Note", headerArea.removeFromLeft (columns.inputWidth), juce::Justification::centred);
        headerArea.removeFromLeft (tableColumnGap);
        g.drawText ("Output Note", headerArea.removeFromLeft (columns.outputWidth), juce::Justification::centred);
        headerArea.removeFromLeft (tableColumnGap);
        g.drawText ("Learn", headerArea.removeFromLeft (columns.learnWidth), juce::Justification::centred);
    }
}

void JDCMidiMapperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (outerMargin, 28);
    const auto panelBounds = getLocalBounds().reduced (18);

    auto topRow = area.removeFromTop (44);
    titleLabel.setBounds (topRow.removeFromLeft (juce::jmin (titleWidth, topRow.getWidth() / 2)));

    const auto toolbarWidth = (toolbarButtonWidth * 4) + (toolbarButtonGap * 3);
    auto emergencyArea = topRow.removeFromRight (juce::jmin (toolbarWidth, topRow.getWidth()));
    const auto buttonY = topRow.getY() + (topRow.getHeight() - toolbarButtonHeight) / 2;
    auto panicBounds = emergencyArea.removeFromRight (toolbarButtonWidth);
    panicBounds.setY (buttonY);
    panicBounds.setHeight (toolbarButtonHeight);
    panicButton.setBounds (panicBounds);
    emergencyArea.removeFromRight (toolbarButtonGap);
    auto enableBounds = emergencyArea.removeFromRight (toolbarButtonWidth);
    enableBounds.setY (buttonY);
    enableBounds.setHeight (toolbarButtonHeight);
    enableAllButton.setBounds (enableBounds);
    emergencyArea.removeFromRight (toolbarButtonGap);
    auto disableBounds = emergencyArea.removeFromRight (toolbarButtonWidth);
    disableBounds.setY (buttonY);
    disableBounds.setHeight (toolbarButtonHeight);
    disableAllButton.setBounds (disableBounds);
    emergencyArea.removeFromRight (toolbarButtonGap);
    auto resetBounds = emergencyArea.removeFromRight (toolbarButtonWidth);
    resetBounds.setY (buttonY);
    resetBounds.setHeight (toolbarButtonHeight);
    resetAllButton.setBounds (resetBounds);

    subtitleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (sectionGap);

    auto utilityRow = area.removeFromTop (controlRowHeight);
    copySceneButton.setBounds (utilityRow.removeFromLeft (standardButtonWidth));
    utilityRow.removeFromLeft (toolbarButtonGap);
    pasteSceneButton.setBounds (utilityRow.removeFromLeft (standardButtonWidth));
    utilityRow.removeFromLeft (toolbarButtonGap);
    duplicateSceneButton.setBounds (utilityRow.removeFromLeft (duplicateButtonWidth));
    utilityRow.removeFromLeft (18);
    showEnabledOnlyToggle.setBounds (utilityRow.removeFromLeft (250));

    area.removeFromTop (sectionGap);

    auto programChangeRow = area.removeFromTop (narrowControlHeight);
    switchSceneByProgramChangeToggle.setBounds (programChangeRow.removeFromLeft (300));
    programChangeRow.removeFromLeft (18);
    filterProgramChangeAfterSwitchToggle.setBounds (programChangeRow.removeFromLeft (260));

    area.removeFromTop (sectionGap);

    auto scenesArea = area.removeFromTop (68);
    const auto sceneButtonWidth = (scenesArea.getWidth() - (sceneButtonGap * 7)) / 8;

    for (int row = 0; row < 2; ++row)
    {
        auto sceneRowArea = scenesArea.removeFromTop (30);

        for (int scene = 0; scene < 8; ++scene)
        {
            auto& button = sceneButtons[static_cast<size_t> (row * 8 + scene)];
            button.setBounds (sceneRowArea.removeFromLeft (sceneButtonWidth));
            sceneRowArea.removeFromLeft (sceneButtonGap);
        }

        scenesArea.removeFromTop (sceneButtonGap);
    }

    area.removeFromTop (sceneSectionGap);

    auto searchRow = area.removeFromTop (controlRowHeight);
    const auto statusWidth = juce::jmax (minimumStatusWidth,
                                         juce::GlyphArrangement::getStringWidthInt (getLookAndFeel().getLabelFont (statusLabel),
                                                                                    statusLabel.getText()) + 24);
    const auto reservedRightWidth = statusWidth + statusGap + clearButtonWidth;
    auto rightSide = searchRow.removeFromRight (juce::jmin (reservedRightWidth, searchRow.getWidth()));
    clearSearchButton.setBounds (rightSide.removeFromRight (clearButtonWidth));
    rightSide.removeFromRight (statusGap);
    statusLabel.setBounds (rightSide);
    searchRow.removeFromRight (statusGap);

    auto leftSide = searchRow;
    searchLabel.setBounds (leftSide.removeFromLeft (searchLabelWidth));
    leftSide.removeFromLeft (searchFieldGap);
    searchEditor.setBounds (leftSide);

    area.removeFromTop (headerGap);
    tableHeaderBounds = area.removeFromTop (tableHeaderHeight);
    area.removeFromTop (8);
    tableViewport.setBounds (area);

    if (renamingSceneIndex >= 0)
        sceneRenameEditor.setBounds (sceneButtons[static_cast<size_t> (renamingSceneIndex)].getBounds());

    tableHeaderBounds.setWidth (tableViewport.getViewWidth());

    const auto minimumTableWidth = rowColumnWidth + enabledColumnWidth + minimumInputNoteColumnWidth + minimumOutputNoteColumnWidth
                                   + learnColumnWidth + (tableColumnInsetX * 2) + (tableColumnGap * 4);
    const auto tableWidth = juce::jmax (tableViewport.getViewWidth(), minimumTableWidth);
    tableContent.setSize (tableWidth, juce::jmax (tableContent.getVisibleRowCount() * rowHeight, tableViewport.getHeight()));
    tableContent.resized();

    const auto versionWidth = juce::GlyphArrangement::getStringWidthInt (versionLabel.getFont(), versionLabel.getText()) + 4;
    versionLabel.setBounds (panelBounds.getRight() - versionLabelInset - versionWidth,
                            panelBounds.getBottom() - versionLabelBottomInset - versionLabelHeight,
                            versionWidth,
                            versionLabelHeight);
}

void JDCMidiMapperAudioProcessorEditor::timerCallback()
{
    const auto currentScene = ownerProcessor.getActiveSceneIndex();

    if (currentScene != displayedSceneIndex)
    {
        displayedSceneIndex = currentScene;
        refreshSceneButtonLabels();
        refreshSceneButtonStyles();
        applyTableFilters();
    }

    const auto activityCounter = ownerProcessor.getMidiActivityCounter();

    if (activityCounter != lastMidiActivityCounter)
    {
        lastMidiActivityCounter = activityCounter;
        midiFlashTicks = 3;
    }
    else if (midiFlashTicks > 0)
    {
        --midiFlashTicks;
    }

    updateLearnState();
    refreshStatusText();
}

void JDCMidiMapperAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID != jdc::parameters::makeParameterId (jdc::parameters::Field::currentScene, 0))
        return;

    pendingDisplayedSceneIndex.store (juce::jlimit (0, jdc::parameters::numScenes - 1, juce::roundToInt (newValue)),
                                      std::memory_order_release);
    triggerAsyncUpdate();
}

void JDCMidiMapperAudioProcessorEditor::handleAsyncUpdate()
{
    const auto pendingScene = pendingDisplayedSceneIndex.exchange (-1, std::memory_order_acq_rel);

    if (pendingScene < 0)
        return;

    displayedSceneIndex = pendingScene;
    refreshSceneButtonStyles();
    tableContent.updateLearnState();
    refreshStatusText();
}

void JDCMidiMapperAudioProcessorEditor::configureButton (juce::TextButton& button,
                                                          juce::Colour background,
                                                          juce::Colour text)
{
    button.setColour (juce::TextButton::buttonColourId, background);
    button.setColour (juce::TextButton::buttonOnColourId, background.brighter (0.1f));
    button.setColour (juce::TextButton::textColourOffId, text);
    button.setColour (juce::TextButton::textColourOnId, text);
    addAndMakeVisible (button);
}

void JDCMidiMapperAudioProcessorEditor::refreshStatusText()
{
    const auto visibleRows = tableContent.getVisibleRowCount();
    const auto totalRows = tableContent.getTotalRowCount();
    const auto mappedRows = ownerProcessor.getMappedRowCountForActiveScene();
    const auto midiState = midiFlashTicks > 0 ? "Live" : "Idle";
    const auto learnState = ownerProcessor.isLearnArmed() ? " | Learn: Armed" : "";
    const auto pcState = ownerProcessor.getSwitchSceneByProgramChangeEnabled()
                             ? (ownerProcessor.getFilterProgramChangeAfterSwitchEnabled() ? " | PC: Switch+Filter" : " | PC: Switch")
                             : " | PC: Off";

    statusLabel.setText (ownerProcessor.getSceneName (ownerProcessor.getActiveSceneIndex())
                             + " | Mapped: " + juce::String (mappedRows)
                             + " | MIDI: " + midiState
                             + " | Showing: " + juce::String (visibleRows) + "/" + juce::String (totalRows)
                             + pcState
                             + learnState,
                         juce::dontSendNotification);
}

void JDCMidiMapperAudioProcessorEditor::refreshSceneButtonStyles()
{
    for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
    {
        auto& button = sceneButtons[static_cast<size_t> (scene)];
        const auto isActive = scene == displayedSceneIndex;
        button.setToggleState (isActive, juce::dontSendNotification);
        button.setColour (juce::TextButton::buttonColourId, isActive ? juce::Colour (0xff355d8b) : juce::Colour (0xff212a36));
        button.setColour (juce::TextButton::buttonOnColourId, isActive ? juce::Colour (0xff4674aa) : juce::Colour (0xff2a3442));
        button.setColour (juce::TextButton::textColourOffId, isActive ? juce::Colour (0xfff5f9ff) : juce::Colour (0xffdfe7f1));
        button.setColour (juce::TextButton::textColourOnId, juce::Colour (0xfff5f9ff));
        button.repaint();
    }
}

void JDCMidiMapperAudioProcessorEditor::refreshSceneButtonLabels()
{
    for (int scene = 0; scene < jdc::parameters::numScenes; ++scene)
        sceneButtons[static_cast<size_t> (scene)].setButtonText (ownerProcessor.getSceneName (scene));
}

void JDCMidiMapperAudioProcessorEditor::updateLearnState()
{
    tableContent.updateLearnState();
}

void JDCMidiMapperAudioProcessorEditor::applyTableFilters()
{
    tableContent.applyFilters (searchEditor.getText(), showEnabledOnlyToggle.getToggleState());
    resized();
}

void JDCMidiMapperAudioProcessorEditor::renameScene (int sceneIndex)
{
    renamingSceneIndex = juce::jlimit (0, jdc::parameters::numScenes - 1, sceneIndex);
    sceneRenameEditor.setText (ownerProcessor.getSceneName (renamingSceneIndex), juce::dontSendNotification);
    sceneRenameEditor.setBounds (sceneButtons[static_cast<size_t> (renamingSceneIndex)].getBounds());
    sceneRenameEditor.setVisible (true);
    sceneRenameEditor.toFront (false);
    sceneRenameEditor.grabKeyboardFocus();
    sceneRenameEditor.selectAll();
}

void JDCMidiMapperAudioProcessorEditor::finishSceneRename (bool shouldCommit)
{
    if (renamingSceneIndex < 0)
        return;

    if (shouldCommit)
        ownerProcessor.setSceneName (renamingSceneIndex, sceneRenameEditor.getText());

    renamingSceneIndex = -1;
    sceneRenameEditor.setVisible (false);
    refreshSceneButtonLabels();
    refreshStatusText();
}
