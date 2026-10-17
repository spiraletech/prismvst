# SURGERY EQ — Precision and Display Contract

## ROOT-constrained CARVE
Each of ROOT 1–7 has four independent, initially OFF CARVEs. CARVE centre frequencies are restricted to the selected ROOT boundary interval. EQ filter skirts outside the centre interval remain smooth; the graph must not falsely suggest brick-wall filtering.

## Precision
- Ctrl+drag an enabled CARVE node: logarithmic fine-frequency scrub without depth changes
- Normal drag: frequency and cut depth (centre clamped within ROOT)
- Mouse wheel: quality factor, Shift+wheel: finer Q adjustment
- Ctrl+wheel: fine-frequency adjustment
- True Q parameter range is 0.300–96.000 with 0.001 host parameter steps
- Higher Q means narrower filter response, but does not improve FFT frequency resolution

## Chromatic SONAR
- One measured post-DSP frequency / dBFS FFT spectrum, filled beneath the real spectrum envelope
- Hue varies by frequency; fill intensity and holographic glints are scaled by actual measured FFT energy
- Optional visual-only moving pearlescent sweep suggests a holographic foil finish; it is **not** additional audio information
- Display animation pauses when the spectrum is frozen or silent
- Background Hann-windowed FFT: size 4096, audio-to-analyzer FIFO, and worker-thread transform
- No FFT or graphics on the audio callback; zero extra audio latency is reported to the host
- No source separation, sidechain masking detection, or automatic correction engine in this version

## Manual analysis
ANALYZE provides view-range, display-slope, visual decay, SONAR toggle and FREEZE plus post-FFT readouts. The user decides whether and where to CARVE. No candidate-generation or apply-suggestion path exists.

## Quality gates
Windows CMake/CTest success does not prove FL Studio visual appearance, CPU efficiency, automation compatibility or perceptual EQ quality. Those need in-DAW verification.
