# PRISMVST

PRISMVST is EtherTech / SpiralEtech's Windows-first VST3 frequency-specialized dynamic EQ and spectral shaping processor for FL Studio.

## v0.3.4 recovery build

This build restores the last known-good processor/parameter architecture from commit \`40527b0\` and keeps the VST3 identity stable. The hardware-shelf redesign is isolated to the editor layer.

### Stable DSP

Six dynamic EQ bands preserve the original parameter IDs:

- \`bandN_enabled\`
- \`bandN_freq\`
- \`bandN_gain\`
- \`bandN_q\`
- \`bandN_dyn_range\`
- \`bandN_threshold\`
- \`bandN_ratio\`
- \`bandN_attack\`
- \`bandN_release\`

The original global parameters are also preserved:

- \`onyx\`, \`onyx_drive\`, \`onyx_bias\`, \`onyx_density\`
- \`solfeggio_grid\`
- \`master_trim\`, \`ceiling\`, \`master_bypass\`

No new audio-processing topology is introduced by the UI overhaul.

### Front panel

The visible hardware shelf intentionally stays compact:

\`FREQ | GAIN | DYN | ATTACK | RELEASE | ONYX | PRECISION\`

The selected band also exposes \`ON\`. Global \`BYPASS\` remains available. Q, threshold, ratio and the deeper ONYX/master parameters remain valid host parameters and are not deleted from presets or automation.

### Analyzer

The editor uses a stationary aura display rather than scrolling spectrogram history:

- visible working range: 0 to -36 dBFS
- 4.5 dB/oct display compensation
- display compensation affects visualization only
- frequency / measured dBFS / musical-note hover readout
- persistent radial aura field
- no conveyor-belt history

## Build

Requires CMake 3.24+ and a C++20 compiler. JUCE 8.0.4 is fetched by CMake.

\`\`\`bash
cmake -S . -B build -A x64 -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
\`\`\`

The Windows workflow verifies the VST3 bundle structure before uploading the build artifact.
