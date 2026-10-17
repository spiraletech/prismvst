# SURGERY EQ v1.0 — AUDIO DOCTRINE / UI THESIS

**Manufacturer:** Spiral Ether Tech  
**Identity:** Original surgical spectral processor. No third-party company affiliation.  
**Prime law:** Precision serves emotion. Understand sound before alteration; verify changes.

## Architecture
- Seven independently selected ROOT frequency territories: SUB, KICK, LOW, LOWER MID, MID, HIGH, HIGHER.
- Two frequency boundary handles per ROOT. Their position controls parallel band audition, gain effects and CARVE eligibility. Soft filter tails extend through neighboring territories.
- Twenty-eight CARVE slots: four for each ROOT, controlled by persistent DAW parameters.
- CARVE processing: peaking EQ cuts with frequency (20–20k Hz), depth (0–36 dB), Q (0.3–24), channel (stereo, mid, side), dynamic enable and trigger threshold. These are EQ bands, not trained source separation.
- Existing ROOT gain is ±60 dB; levels may exceed digital full scale. No claim of global true-peak limiting.
- ROOT SPACE and stereo-domain compression remain implemented by existing ROOTS engine, including correlation-aware widening restriction. The audio will not be universally phase-corrected.
- No manual attack/release knobs; internal envelope timing is retained.
- Existing 14 ROOT handles do not count as CARVE nodes.

## SURGERY SONAR
- Measurement is based on 256 logarithmically distributed analysis bars from existing worker-thread FFT telemetry.
- Separate stationary heat/persistence and positive-onset indicators live underneath the measured spectrum.
- Heat brightness is an engineered display mapping of actual dBFS readings; the raw values remain separate. Visual changes never alter the DSP.
- Independent visual frame smoothing has a configurable memory time.
- This version does **not** provide a time-scrolling spectrogram or instrument-specific source heat. Do not call it machine-learning separation.

## SURGERY ASSIST
- Implements deterministic candidate detection: frequency bins are compared against surrounding bands, evaluated over successive frames and restricted by minimum measured level.
- No learned model. It cannot name a vocal, kick or snare with confidence.
- User must explicitly choose APPLY SUGGESTION and the selected ROOT must contain the detected frequency.
- A proposal produces a real CARVE filter with a bounded 1–6 dB depth and visible Q/frequency parameters.
- A suggestion is not proof a resonance is unwanted. A/B, DELTA and listening validation remain essential.

## UI philosophy: ECG precision × Alien SONAR × hardware instrument
- One permanent graph; modes ROOT, CARVE, SPACE, DYN and ANALYZE change control emphasis.
- Seven direct ROOT selection buttons across the top.
- Distinct context menu belongs to the ROOT under right click, not a boundary node.
- Hardware-style ON, SOLO, MONO and DYN buttons; no exposed hardware brand compressor profiles in right-click.
- Editable CARVE markers on graph: horizontal frequency, vertical cut depth. Contextual bottom shelf with C1–C4 controls.
- Full-theme selection plus per-ROOT RGB values persisted as host state; ROOT context menu offers preset swatches and custom hexadecimal entry.
- ONYX / ETHER LAB / ALIEN GRID / CINEMA VOID palette names are original Spiral Ether Tech variants.
- Crosshair labels represent existing analyzer's calibrated spectral data, not a synthetic AI score.

## AUDIO / DISPLAY separation
1. Audio DSP never waits for GPU, UI, analysis or diagnostics.
2. Parameter IDs are stable and host-automatable; neutral/default CARVE settings produce no audio changes.
3. Display slope, persistence and theme choices are not DSP operations.
4. Disabled visual modes cannot alter sound.
5. Input/output analysis, processing-response curves and true-peak measurements are distinct future features and must not be mislabeled.

## Testing and release acceptance
Four mandatory proof categories:
1. CARVE transfer, neutrality, envelope and numerical stability (new standalone C++20 tests).
2. SONAR heat calibration, decay/reset, candidate gating (new standalone C++20 tests).
3. ROOTS stereo, SOLO and boundary consistency (existing suite).
4. Windows VST3 CMake, JUCE tests, artifact packaging and FL Studio project recall.

A successful compiler run is not a substitute for DAW testing, listening comparison, CPU profiling and gain-matched A/B. Release blockers remain until actual measurements are recorded.

## Not yet implemented
- Learned, trained AI diagnosis; true stems/source separation.
- Full spectral history TRAIL, scrolling waterfall, spectral resynthesis.
- Oversampled full-band processing with validated latency accounting.
- Full linear-phase surgical engine, multichannel surround/Atmos, standard-compliant integrated LUFS.
- Dedicated input/output independently measured spectrums, true transfer overlay and rigorous dynamic GR overlays.
- User-facing undo transaction for multi-parameter ASSIST insertion.
