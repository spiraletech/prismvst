# PRISMVST

PRISMVST is EtherTech's Windows-first VST3 spectral dynamics processor for FL Studio.

## v0.3.3 UI / architecture lock

The front panel intentionally stays sparse. There are seven fixed frequency territories:

\`SUB | KICK | LOW | LOWER MID | MID | HIGH | HIGHER\`

Fixed territory boundaries:

\`60 Hz | 120 Hz | 250 Hz | 500 Hz | 2.00 kHz | 6.00 kHz\`

The selected section exposes only:

\`INPUT | OUTPUT | ATTACK | RELEASE | WIDTH | ONYX\`

plus \`ON\`, \`SOLO\`, and the universal \`PRECISION\` lever. The lever changes drag sensitivity for every rotary control instead of adding separate fine/coarse versions of parameters.

## Analyzer

PRISM separates measurement from display:

- FFT engine: 16384 points
- Internal measurement floor: -144 dBFS
- Default visible viewport: 0 to -36 dBFS
- Display slope: 4.5 dB/oct, visualization only
- Persistent aura field: no scrolling spectrogram history
- Hover readout: frequency, true measured dBFS, musical note

Analyzer display compensation does not alter DSP measurements or audio processing.

## DSP

Each territory uses fixed sixth-order (36 dB/oct) Butterworth boundary filters for section analysis/processing. Neutral settings are constructed as a dry-plus-delta topology so the default state remains bit-stable dry while per-territory shaping is inactive.

- INPUT drives the section detector.
- OUTPUT applies section trim.
- ATTACK / RELEASE control detector timing.
- WIDTH changes stereo width inside the selected territory.
- ONYX controls the section's dynamic restraint amount.
- SOLO auditions the selected filtered territory.

## Build

Requires CMake 3.24+ and a C++20 compiler. JUCE 8.0.4 is fetched by CMake.

\`\`\`bash
cmake -S . -B build -A x64 -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
\`\`\`

The Windows workflow builds, tests, and uploads \`PRISMVST.vst3\` as the \`PRISMVST-Windows-VST3\` artifact.
