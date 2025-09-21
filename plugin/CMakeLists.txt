juce_add_plugin(LPMorph
    COMPANY_NAME "projectfmusic"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "/Users/anthony/Library/Audio/Plug-Ins/VST3"
    AU_COPY_DIR "/Users/anthony/Library/Audio/Plug-Ins/Components"
    MICROPHONE_PERMISSION_ENABLED TRUE
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Temp
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "LP Morph")

juce_generate_juce_header(LPMorph)

file(GLOB SRC_FILES "src/*.cpp")
file(GLOB LIBS_FILES "libs/*")
file(GLOB HEADER_FILES "src/*.h")
target_sources(LPMorph PRIVATE ${SRC_FILES} ${LIBS_FILES} ${HEADER_FILES})
source_group("Source Files" FILES ${SRC_FILES})
source_group("Library Files" FILES ${LIBS_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

target_include_directories(LPMorph PUBLIC libs)

file(GLOB BINARY_FILES "resources/*.bin")
juce_add_binary_data(bindata SOURCES ${BINARY_FILES})

target_compile_definitions(LPMorph
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LPMorph
    PRIVATE
        juce::juce_audio_utils
        bindata
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

