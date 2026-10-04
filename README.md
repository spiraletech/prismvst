# PRISMVST

PRISMVST is a Windows-first VST3 frequency-specialized dynamics and shaping processor for FL Studio.

## Signal model

Stereo input -> SUB -> KICK -> LOW -> MID -> HIGH -> MASTER

The five zones are complementary crossover bands that reconstruct the input at unity when all zone processing is neutral. KICK is a frequency zone, not a drum detector or source separator.

Each zone exposes direct scraper-style controls rather than EQ curves or shelves:
- Scraper threshold
- Depth
- Tone
- Density
- Output trim
- Solo / bypass

MASTER provides output trim, ceiling, bypass, peak monitoring and integrated/short-term loudness telemetry.

## Build

Requires CMake 3.24+ and a C++20 compiler. JUCE is fetched by CMake.

```bash
cmake -S . -B build -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, the VST3 bundle is produced under the JUCE artefacts directory in `build`.

## Status

The repository contains the reconstructed production branch after the interrupted Astra session. GitHub Actions is the canonical Windows build/test gate.
