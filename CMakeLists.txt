cmake_minimum_required(VERSION 3.24)
project(PRISMVST VERSION 0.3.1 LANGUAGES CXX)

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

set(PRISMVST_VST2_SDK_PATH "" CACHE PATH "Path to a licensed Steinberg VST2 SDK include directory")
set(PRISMVST_FORMATS VST3)

if(PRISMVST_VST2_SDK_PATH)
  juce_set_vst2_sdk_path("${PRISMVST_VST2_SDK_PATH}")
  list(APPEND PRISMVST_FORMATS VST)
  message(STATUS "PRISMVST VST2 build enabled: ${PRISMVST_VST2_SDK_PATH}")
else()
  message(STATUS "PRISMVST VST2 build disabled. Set PRISMVST_VST2_SDK_PATH to build PRISMVST.dll")
endif()

juce_add_plugin(PRISMVST
  COMPANY_NAME "SpiralEtech"
  BUNDLE_ID "com.spiraletech.prism.alpha001"
  PLUGIN_MANUFACTURER_CODE SpEt
  PLUGIN_CODE Pa01
  FORMATS ${PRISMVST_FORMATS}
  PRODUCT_NAME "PRISM Alpha 001"
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
