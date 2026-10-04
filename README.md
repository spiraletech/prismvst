# PRISMVST

PRISMVST is EtherTech's Windows-first VST3 for frequency-specialized dynamics,
sound study, and precision spectral editing in FL Studio.

## EtherTech interaction model

PRISM separates **measurement**, **visualization**, and **processing**.

- Analyzer engine: 16384-point FFT with an internal display floor down to -144 dBFS.
- Default analyzer viewport: MIX, 0 to -36 dBFS.
- Deeper viewports: DEEP (0 to -72 dBFS) and FORENSIC (0 to -120 dBFS).
- Spectrum slope: visualization-only compensation, default 4.5 dB/oct.
- Stationary **Heat Aura**: frequency positions stay fixed; energy blooms and decays
  in place instead of scrolling through a waterfall spectrogram.
- Optional Solfeggio Aura palette: 174 / 285 / 396 / 417 / 528 / 639 / 741 /
  852 / 963 Hz use a fixed EtherTech color canon with smooth interpolation.
  This is a perceptual visualization language, not a medical or healing claim.
- Hover inspection reports frequency, dBFS, derived wavelength, nearest aura
  reference, and affinity.
- SUB / KICK / LOW / MID / HIGH are semantic spectral territories separated by
  subtle visual bumpers rather than EQ-style graph nodes.
- A second **Dynamics Transfer Map** exposes threshold / ratio behavior as
  input-to-output geometry, Maximus-style, instead of duplicating those values
  as front-panel knobs.
- UAD-like deliberate control feel: no velocity acceleration, long virtual drag
  travel, NORMAL / FINE / MICRO precision modes, modifier precision, numeric
  readouts, and double-click reset.

## Current DSP core

The current processor branch uses five dynamic spectral peak domains plus the
master stage. Each domain exposes center frequency, trim, width, maximum dynamic
depth, attack, and release. The transfer map controls the existing threshold and
ratio parameters.

The next DSP milestone is a complementary crossover reconstruction engine with
full Maximus-derived per-domain PRE / POST, stereo-link, dual-release,
sustain/RMS detector behavior, saturation ceiling/shape, lookahead, and
multiband mix. Those controls should only be exposed when their DSP paths are
real; PRISM does not add decorative controls that do nothing.

MASTER currently provides ONYX shaping, drive, output trim, ceiling, bypass,
peak monitoring, and short/integrated loudness telemetry.

## Build

Requires CMake 3.24+ and a C++20 compiler. JUCE 8.0.4 is fetched by CMake.

```bash
cmake -S . -B build -DPRISMVST_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, the VST3 bundle is produced under the JUCE artefacts directory in
`build`.

## Design law

**ANALYZER VIEW != DSP.** Display range, slope, persistence, aura color, and
other visual transforms never alter the audio or dynamics measurements.
