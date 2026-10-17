# SURGERY EQ v1.0 — Spiral Ether Tech

Windows-first native JUCE C++20 VST3 **development build**, evolved from the PRISM ROOTS PROTO 001 engine.

## Current functionality
- Seven selectable frequency ROOT territories with two frequency-boundary handles each.
- Measured frequency analyzer, ROOT SOLO, ROOT gain, ROOT/Global SPACE and stereo controls.
- ROOT-only contextual menu: enabled, SOLO, EQ channel, compressor channel, SPACE presets.
- Dynamic processing controlled through the explicit **DYN ON/OFF** button; COMP THRESHOLD is enabled only when DYN is active.
- Hardware-style ON, SOLO and MONO switches.
- Four initial chassis themes via the THEME button.
- Existing host parameter IDs and VST3 bundle identity retained for backwards project compatibility.

## What is *not* implemented yet
- The planned 28 CARVE nodes, true source extraction and AI-based surgical analysis.
- Calibrated SONAR thermal history/heatmaps and user-custom RGB palettes.
- Independent full-window recoloring for every graph element.
- Production release acceptance on FL Studio.

See [SURGERY_EQ_V1_0_STATUS.md](SURGERY_EQ_V1_0_STATUS.md) for accurate status.

## Build

Requires Visual Studio 2022 C++ workloads, CMake 3.24+, and network access to fetch JUCE 8.0.4.

```powershell
cmake -S . -B build -A x64 -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The Windows Actions workflow stages the bundle as `SURGERY EQ v1.0.vst3`. Verify the workflow result before installing.

## Compatibility
Renaming the plugin's VST3 identifier would risk breaking existing DAW project recall. This development branch deliberately retains the previous identifier while changing the visible product name. Back up sessions and test reload behavior in FL Studio.
