# SURGERY EQ v1.0 — Spiral Ether Tech

Original Windows-first surgical equalization VST3 developed in C++20/JUCE.

**Development branch:** `surgery-eq-v1.0`. This is an engineering build and is not declared production-ready until Windows CI and actual FL Studio verification pass.

## Built features
- Seven ROOTS, two boundary handles each; fixed stereo/root EQ and compression engine
- Four CARVE slots per ROOT (28 total), static and threshold-driven dynamic cuts with stereo/Mid/Side targeting
- Direct graph CARVE frequency/depth dragging and four-slot parameter shelf
- ROOT / CARVE / SPACE / DYN / ANALYZE UI modes
- Stationary SONAR heat/persistence field from calibrated spectrum frames
- Evidence-based, deterministic ASSIST candidate scan with explicit user approval to apply a correction
- ON, SOLO, MONO and DYN buttons; root-level context menu without compressor model listings
- Four chassis themes plus per-ROOT persisted RGB customization (palette and custom hex)
- Windows VST3 packaging as `SURGERY EQ v1.0.vst3`

This version **does not** contain an instrument-separating AI model, pretrained intelligence or completed EXTRACT engine.

See [Audio Doctrine / UI Thesis](docs/SURGERY_EQ_V1_0_AUDIO_BIBLE.md) and [implementation status](SURGERY_EQ_V1_0_STATUS.md).

## Build
Windows x64, Visual Studio C++ workload, CMake 3.24+, JUCE 8.0.4 fetched by configure:

```powershell
cmake -S . -B build -A x64 -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

## Compatibility
VST3 bundle identity and historic host parameter IDs remain unchanged from ROOTS PROTO 001. Visible product name and artifact filename changed; FL Studio discovery and old-project recall require in-host confirmation. Back up prior projects before using this branch.
