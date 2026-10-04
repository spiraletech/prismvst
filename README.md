# PRISM Alpha 001

PRISM is EtherTech's Windows-first VST3 for precision spectral dynamics, sound
study, and stationary frequency-state visualization in FL Studio.

## AAA interaction laws

PRISM treats interaction quality as part of the audio engine:

- Six persistent nodes: **1 / 2 / 3 / 4 / 5 / 6**.
- Node identity never changes when another node is selected.
- Analyzer background clicks never silently select or move a node.
- Node dragging uses explicit hit targets and cannot cross a neighboring node.
- A one-semitone minimum spacing prevents stacked / ambiguous node handles.
- Controls use acceleration-free, long-travel movement:
  - NORMAL: ~3000 px full sweep
  - FINE: ~9000 px full sweep
  - MICRO: ~24000 px full sweep
- Shift temporarily invokes fine movement; Ctrl/Cmd invokes micro movement.
- Double-click reset and direct numeric entry remain available.

## Per-node engine

Every node owns independent state for:

- Center frequency
- Static trim
- Q / width
- Maximum dynamic depth (0 to 24 dB)
- Dynamics slope / ratio
- Attack
- Transfer-curve shape
- Release A
- Release B
- Release blend
- Sustain / RMS integration time
- Peak-to-RMS detector weighting

Threshold and slope are also exposed through a Maximus-derived input-to-output
transfer map. The curve display uses the same depth and curve law as the DSP.

## Heat Aura analyzer

**ANALYZER VIEW != DSP.** Display behavior never feeds back into processing.

- 16384-point FFT.
- Overlapping FFT snapshots via a circular FIFO.
- Internal analysis floor to -144 dBFS.
- MIX viewport: 0 to -36 dBFS.
- DEEP viewport: 0 to -72 dBFS.
- FORENSIC viewport: 0 to -120 dBFS.
- Display compensation slope: 0 to 6 dB/oct, default 4.5 dB/oct.
- Stationary Heat Aura: x is always frequency. There is no time-scrolling
  waterfall or row-history buffer.
- Aura memory controls in-place persistence / afterglow.
- Solfeggio Aura colors use EtherTech's canonical reference palette and octave
  families across 20 Hz to 20 kHz.
- Hover inspection reports Hz, dBFS, derived acoustic wavelength, nearest Aura
  family, and affinity.

The Solfeggio palette is a creative/perceptual visualization language, not a
medical, healing, or physical-science claim.

## Metering

The Alpha 001 UI now exposes input peak, current maximum node gain reduction,
output peak, LUFS short-term, and LUFS integrated telemetry.

## Current DSP architecture

The current Alpha uses six dynamic spectral peak nodes plus the master stage.
Each node has a band-limited detector and independent peak/RMS timing law.
The master stage includes ONYX nonlinear shaping, output trim, and ceiling.

The next architectural milestone is complementary crossover reconstruction and
full per-band PRE/POST, stereo linking, per-band saturation, lookahead, and
parallel multiband mix. Those controls are intentionally not displayed until
their DSP paths are real.

## Build

Requires CMake 3.24+ and a C++20 compiler. JUCE 8.0.4 is fetched by CMake.

```bash
cmake -S . -B build -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The Windows workflow builds and uploads the distinct **PRISM Alpha 001.vst3**
artifact so FL Studio cannot confuse it with earlier PRISMVST prototypes.
