# PRISM Analysis → PRISM ROOTS: Spectrum BAR port

Source: [spiraletech/prismanalysis](https://github.com/spiraletech/prismanalysis), latest verified artifact `PRISM_LAB_ORBMODES` including reconstructed `prism.cpp` (256 bars and `drawDbHover`). PRISM Analysis is a separate native Windows WASAPI/GDI executable. ROOTS is a JUCE VST3; do not copy Win32 window/audio-capture code.

## What is actually ported
- 256 logarithmically spaced spectrum bars, 20–20,000 Hz, preserving the narrow-bar analyzer reading style.
- Hover `frequency Hz | BAR xx.x dBFS | PEAK xx.x dBFS`; BAR is the calibrated FFT peak-amplitude reading for the relevant log band, and PEAK has a slow visual decay.
- Analyzer-owned attack/release envelope and display-only peak hold, separate from all audio effects.
- LAB data is produced by the **existing background FFT analyzer worker**, communicated to JUCE GUI via atomics and drawn on the existing, single analyzer graph.
- The original PRISM v1.2 spectrum trace and its default **0 to -96 dB** viewport are retained.

## Accuracy upgrade over the source
Original PRISM Analysis used `abs(real)+abs(imag)` for FFT magnitude and `4*mag/FFT_SIZE`; those choices yield a visually useful analyzer but are not precise complex magnitude calibration. In ROOTS we use JUCE's frequency-only FFT magnitude plus Hann-window coherent-gain normalization:
`peakAmplitude ≈ 2*fftMagnitude/sum(HannWindow)`; symmetric Hann sum equals `(FFT_SIZE-1)/2`.
`dBFS = 20*log10(max(1e-12, peakAmplitude))`.

This is an **FFT peak-amplitude estimate**, not sample-peak or LUFS. A sine not bin-centred leaks into adjacent bins, so displayed levels can deviate from input amplitude. At 48 kHz and 4096 samples the frequency resolution is ~11.72 Hz. Bins narrower than one FFT bin inherit the nearest bin. Claims of exact sub-bass harmonic separation or 0-ms FFT measurement latency would be technically false. For finer bass resolution later, use a larger FFT and account for the longer analysis window.

The cursor Hz label is the **nominal frequency at the mouse position**, not automatically a pitch estimator. Peak values come from FFT measurements; cursor movement cannot alter dB readings.

## Audio / UI separation
- Only visual analysis code was added to `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp` and their headers.
- The shipping v1.2 audio processing function and original feature parameters remain untouched.
- No WASAPI capture inside a plugin; the spectrum uses the host-provided post-processing signal.
- The new standalone analyzer is testable without JUCE at `Source/PrismLabAnalyzer.h`.

## Independent four test proofs
1. Calibrated FFT sine magnitude maps a 0.5 peak sine to -6.0206 dBFS.
2. 256 log bands cover 20–20,000 Hz in increasing order.
3. Digital silence produces floor readings and no non-finite output.
4. Meter release and peak memory behave independently.

Pending before release: actual JUCE end-to-end FFT sine test on Windows, anti-aliased bin-edge sweep, visual inspection inside FL Studio, frame-rate/CPU measurements, and hover positioning at high DPI.
