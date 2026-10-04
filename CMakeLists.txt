cmake_minimum_required(VERSION 3.24)
project(PRISMVST VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PRISMVST
  COMPANY_NAME "SpiralEtech"
  BUNDLE_ID "com.spiraletech.prismvst"
  PLUGIN_MANUFACTURER_CODE SpEt
  PLUGIN_CODE Prsm
  FORMATS VST3
  PRODUCT_NAME "PRISMVST"
  VST3_CATEGORIES Fx Mastering
  COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(PRISMVST)

target_sources(PRISMVST PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
)

target_compile_definitions(PRISMVST PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PRISMVST PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

option(PRISMVST_BUILD_TESTS "Build DSP smoke tests" ON)
if(PRISMVST_BUILD_TESTS)
  enable_testing()
  juce_add_console_app(prismvst_tests PRODUCT_NAME "PRISMVST Tests")
  target_sources(prismvst_tests PRIVATE
    Tests/ProcessorTests.cpp
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
  )
  juce_generate_juce_header(prismvst_tests)
  target_compile_definitions(prismvst_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(prismvst_tests PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
  )
  add_test(NAME prismvst_tests COMMAND prismvst_tests)
endif()
