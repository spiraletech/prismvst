cmake_minimum_required(VERSION 3.24)
project(PRISMVST VERSION 1.0.0 LANGUAGES CXX)

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
  COMPANY_NAME "Spiral Ether Tech"
  BUNDLE_ID "com.spiraletech.prism.roots.proto001"
  PLUGIN_MANUFACTURER_CODE SpEt
  PLUGIN_CODE PrR1
  FORMATS ${PRISMVST_FORMATS}
  PRODUCT_NAME "SURGERY EQ v1.0"
  VST3_CATEGORIES Fx Mastering
  COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(PRISMVST)

target_sources(PRISMVST PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  Source/RootsEditor.cpp
  Source/RootsEditor.h
  Source/RootsAudioEngine.h
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
    Source/RootsEditor.cpp
    Source/RootsEditor.h
    Source/RootsAudioEngine.h
    Source/SurgeryCarveEngine.h
    Source/SurgeryLookAndFeel.h
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
  # Independent native C++20 ROOTS core; deliberately not linked into v1.2 DSP.
  add_executable(prism_roots_tests Tests/RootsCoreTests.cpp Source/PrismRootsCore.h)
  target_compile_features(prism_roots_tests PRIVATE cxx_std_20)
  add_test(NAME prism_roots_four_proofs COMMAND prism_roots_tests)
  add_executable(prism_lab_analyzer_tests Tests/PrismLabAnalyzerTests.cpp Source/PrismLabAnalyzer.h)
  target_compile_features(prism_lab_analyzer_tests PRIVATE cxx_std_20)
  add_test(NAME prism_lab_analyzer_four_proofs COMMAND prism_lab_analyzer_tests)
  add_executable(prism_roots_realtime_tests Tests/RootsRealtimeTests.cpp Source/RootsAudioEngine.h)
  target_compile_features(prism_roots_realtime_tests PRIVATE cxx_std_20)
  add_test(NAME prism_roots_realtime_four_proofs COMMAND prism_roots_realtime_tests)
  add_executable(prism_roots_ui_spectrum_tests Tests/RootsUiSpectrumTests.cpp Source/RootsSpectrumColours.h)
  target_compile_features(prism_roots_ui_spectrum_tests PRIVATE cxx_std_20)
  add_test(NAME prism_roots_ui_spectrum_four_proofs COMMAND prism_roots_ui_spectrum_tests)
  add_executable(prism_roots_parent_controls_tests Tests/RootsParentControlsTests.cpp Source/RootsAudioEngine.h)
  target_compile_features(prism_roots_parent_controls_tests PRIVATE cxx_std_20)
  add_test(NAME prism_roots_parent_controls_four_proofs COMMAND prism_roots_parent_controls_tests)
  add_executable(surgery_carve_tests Tests/SurgeryCarveTests.cpp Source/SurgeryCarveEngine.h)
  target_compile_features(surgery_carve_tests PRIVATE cxx_std_20)
  add_test(NAME surgery_carve_four_proofs COMMAND surgery_carve_tests)
  add_executable(surgery_sonar_tests Tests/SurgerySonarTests.cpp Source/SurgerySonar.h)
  target_compile_features(surgery_sonar_tests PRIVATE cxx_std_20)
  add_test(NAME surgery_sonar_four_proofs COMMAND surgery_sonar_tests)
  add_executable(surgery_assist_tests Tests/SurgeryAssistTests.cpp Source/SurgeryAssist.h)
  target_compile_features(surgery_assist_tests PRIVATE cxx_std_20)
  add_test(NAME surgery_assist_four_proofs COMMAND surgery_assist_tests)
  add_executable(surgery_header_layout_tests Tests/SurgeryHeaderLayoutTests.cpp Source/SurgeryHeaderLayout.h)
  target_compile_features(surgery_header_layout_tests PRIVATE cxx_std_20)
  add_test(NAME surgery_header_layout_proofs COMMAND surgery_header_layout_tests)
  add_executable(surgery_node_focus_tests Tests/SurgeryNodeFocusTests.cpp Source/SurgeryNodeFocus.h)
  target_compile_features(surgery_node_focus_tests PRIVATE cxx_std_20)
  add_test(NAME surgery_node_focus_proofs COMMAND surgery_node_focus_tests)
endif()
