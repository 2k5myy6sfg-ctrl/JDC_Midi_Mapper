cmake_minimum_required(VERSION 3.24)

project(JDCMidiMapper VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Apple Silicon only" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(JDCMidiMapper
    COMPANY_NAME "JDC"
    BUNDLE_ID "com.jdc.midimapper"
    PLUGIN_MANUFACTURER_CODE Jdcm
    PLUGIN_CODE Jdmm
    FORMATS AU VST3
    PRODUCT_NAME "JDC MIDI Mapper"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(JDCMidiMapper
    PRIVATE
        Source/Parameters.h
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
)

target_compile_definitions(JDCMidiMapper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(JDCMidiMapper
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    Source/Parameters.h
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
)
