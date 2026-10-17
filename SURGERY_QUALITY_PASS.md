# SURGERY EQ 1.0.1 — quality pass

This update starts at `6615d366e520bff3772822a6071b60a188b45216`, the exact source of the supplied SURGERY EQ ZIP. Its SHA-256 matches GitHub Actions artifact 12104645604: `f9fcf8df445e85e0a9eb9e8b6dc042cd0383d508a3233a577a9432e2e0d8b98b`. No code from the unrelated PRISM 0.4.0 redesign is used as the baseline.

## Preserved instrument

Seven ROOTS with two frequency boundaries each, 28 CARVE slots, per-ROOT ONYX, ROOT/CARVE/ONYX/ANALYZE modes, four themes, custom ROOT colours, high-Q precision and the chromatic stationary analyzer remain. Existing parameter IDs, ranges, defaults and ordering, product name, manufacturer code, plugin code and bundle ID remain intact. The version is advanced to 1.0.1; the existing bundle filename remains unchanged for discovery. Only two parameters are appended: `surgery_bypass` and `surgery_output_db`.

## Repairs and refinements

- GUI metering now reads atomic published values instead of concurrently reading mutable engine memory.
- Active ROOT/CARVE processing uses cached parameter pointers. Meter filter state is fully prepared before audio callbacks.
- ROOT enable, mono, compression activation, threshold and ONYX amounts ramp over approximately 10 ms. CARVE frequency/Q/depth smoothing runs at a fixed 32-sample cadence; CARVE activation/deactivation has an approximately 4 ms blend.
- Disabled CARVEs clear fully faded filter/detector history instead of replaying old ringing.
- ROOT effect taps stay warm and are shared by gain and audition. SOLO auditions the actual selected ROOT processing, including ONYX/compression, and fades into/out of audition over 8 ms.
- Mono reduction follows ONYX/compression; ROOT gain cannot put SIDE back into a mono audition.
- Stereo spectrum combines left/right FFT power. Out-of-phase material no longer disappears from the display. The visible analyzer now uses the actual normalized Hann sum, correcting its roughly 6 dB calibration error. This changes visualization only and adds no audio-path latency.
- Empty buffers and non-finite input samples are handled. Oversized host buffers are processed in prepared chunks, without resizing audio storage in the callback.

## Added controls within the original layout

The ROOT shelf now exposes compression profile, EQ domain and detector selection. The persistent bottom strip adds BYPASS, A/B, COPY, SAVE/LOAD preset files, resettable output sample peak hold and output trim (-24 to +12 dB).

BYPASS crossfades over 5 ms; once settled, output is exact dry input. Processing remains warm. Output trim ramps over 20 ms. A/B keeps two settings in the current plugin instance; it is an audition buffer, not two saved project states. Use SAVE to preserve either setting as a `.surgerypreset` file. Preset writes use a temporary file and replacement so a failed write does not discard an existing preset. Invalid preset XML is rejected.

The original light theme's label contrast is corrected, tooltips are enabled, the status line no longer competes with CARVE slot controls, and graph gestures close when the host closes the editor.

## Verification and release boundaries

The existing 17-test CTest suite is extended with whole-processor buffer-partition, exact-bypass, oversized/empty-buffer, invalid-input, every-parameter preset recall, legacy-shaped state, A/B, SOLO/MONO/ONYX/gain interaction, stale-CARVE, stereo analyzer calibration and C++ allocation-probe checks. Sixteen screenshots cover every original mode and theme; an additional 2x render checks vector scaling. Host validation runs via pluginval. Invalid numeric preset values are rejected, and legacy states reset the new output utilities to unity/engaged even when loaded over an edited instance. See delivered logs for actual executed results.

This is an unsigned development candidate. Automated validation cannot certify a plugin as “AAA” or set its commercial price. Actual FL Studio listening, existing-project recall, sustained automation and multi-instance tests remain release gates. No installer or DAW installation is performed automatically.

ONYX retains its existing nonlinear character without oversampling. Output trim and sample peak hold are not a true-peak limiter or loudness auto-match. Filter/domain changes and exotic host configurations still need listening and compatibility evaluation. Deprecated legacy parameters remain for recall and are not repurposed. Review JUCE licensing and code ownership before commercial distribution.
