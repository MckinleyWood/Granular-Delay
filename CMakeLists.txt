cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0175 OLD)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(GranularDelay VERSION 0.0.1)

add_subdirectory(JUCE)
set(JUCE_PATH "${CMAKE_SOURCE_DIR}/JUCE")
include_directories(${JUCE_PATH}/modules)

juce_add_plugin(GranularDelay
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    PLUGIN_NAME "Granular Delay"
    COMPANY_NAME "Mckinley Wood"                # Specify the name of the plugin's author
    BUNDLE_ID "com.MckinleyWood.GranularDelay"
    IS_SYNTH FALSE                              # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE                      # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE                     # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                        # Is this plugin a MIDI effect?
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE            # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE                # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE MWod               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Gdel                            # A unique four-character plugin id with exactly one upper-case character
    FORMATS VST3 Standalone                     # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "GranularDelay")               # The name of the final executable, which can differ from the target name

juce_generate_juce_header(GranularDelay)

target_sources(GranularDelay
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.h
        )

target_link_libraries(GranularDelay
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)