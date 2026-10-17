# PRISMVST 0.4.0

This update uses the PRISMVST source and preserves its VST3 identity and parameter IDs. It does not replace the distinct SURGERY EQ identity. This document supersedes the recovery-build UI description in README.md.

## Interface

Select a band with a tab or numbered graph node. Drag nodes horizontally for frequency and vertically for static gain. Amber is the combined static EQ response on a ±18 dB scale; cyan is the output spectrum on the 0 to −36 dBFS scale, with 4.5 dB/octave display compensation. Dynamic attenuation is not included in the amber curve.

The lower control row exposes Q, threshold, ratio, ONYX drive/bias/density, output trim and sample ceiling. ONYX wet amount now displays its actual 0–1 value. Frequency double-click defaults follow the selected band. The precision lever adjusts main-row knob sensitivity. Direct numeric entry and host automation remain available.

The initial editor resize now occurs after all child controls are constructed. Previously, the resize callback could dereference missing band buttons while opening the editor.

## Processing

Dynamics use a fixed sample cadence, independent of host buffer size. Coefficient generation uses stack arrays, with coefficient storage initialized before processing. Parameter pointers are cached for the audio callback. Frequency, Q and static gain changes have approximately 10 ms smoothing; output trim and ONYX controls ramp over 20 ms. Band enable changes crossfade over 5 ms. Disabled filters continue tracking input to avoid stale filter state. Meters reset on preparation and empty buffers are ignored. The host bypass parameter is explicitly exposed. Analyzer magnitude uses the normalized Hann window and a one-sided amplitude scale. CMake explicitly enables both C and C++ for JUCE's mixed-language modules.

## Limits and release status

PEAK measures sample peaks. WTD BLOCK and WTD AVG are approximate weighted energy readings, not standards-compliant LUFS. The ceiling is a sample hard clipper, not a true-peak limiter. ONYX has no oversampling. The analyzer has 2048-point FFT resolution. The historical `solfeggio_grid` host parameter is retained for state compatibility but has no processing effect.

This update requires listening tests and host compatibility checks before commercial release. Rapid automation, band-enable/bypass transitions, ONYX aliasing, high-Q modulation and session migration need practical host testing. Review JUCE licensing and code ownership before distribution. No price or commercial certification is implied.

## Validation

The regression executable checks dry bypass, sample ceiling, host buffer partition invariance, state restoration, malformed state, empty buffers and extreme settings at five sample rates. It renders the actual editor at 1080×820 and 1280×920 for visual inspection. See the delivered validation report for executed results.

Use the CMake build instructions in README.md. Install the entire `PRISMVST.vst3` folder under your host's VST3 directory after backing up the previous version and closing the host. Test copies of existing sessions first.
