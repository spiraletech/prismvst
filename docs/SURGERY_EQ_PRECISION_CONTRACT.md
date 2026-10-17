# SURGERY EQ — Precision and Evidence Contract (Prototype)

## CARVE centres: ROOT-bound by construction
Each of ROOT 1–7 owns four independent CARVE centre frequencies. A CARVE centre cannot cross that ROOT's current left/right handle positions using graph drag, Ctrl-drag, knob edits, or audio engine parameter configuration. Existing out-of-range automation is bounded at processing time. Moving a ROOT boundary constrains its child centres at gesture end.

A centre-frequency boundary does not mean an infinitely steep band-limited EQ. The actual peaking filter has skirts outside its centre, and the audio engine preserves smooth tails. Zero influence beyond ROOT would require a different DSP architecture.

## Precision control
- Ctrl+drag active CARVE: slow logarithmic frequency scrub only; depth unchanged
- Normal drag: centre frequency + cut depth, restricted to ROOT
- Mouse wheel on active CARVE: Q factor; Shift+wheel gives smaller steps
- Ctrl+wheel: fine frequency adjustment
- Q host parameter resolution: 0.001 (0.300 to 24.000); edit exact numeric value in its field
- Higher Q = narrower modeled notch

## SONAR display
- SPECTRUM: calibrated FFT log-frequency envelope, optional persistence fill
- HEATMAP: separate frequency-versus-level persistence field, never a second histogram underneath the spectrum
- New host parameter surgery_heat_view persists view choice (factory spectrum)
- CARVE overlay is a modeled maximum static response, not measured live dynamic gain reduction
- Existing aura_color switches SONAR shading on/off; no visual option changes audio

## Conservative ASSIST
- Measured narrow-peak prominence limited to selected ROOT
- At least 60 consecutive stable GUI frames, ~2 seconds at 30 Hz, and >15 dB prominence
- Proposed cut never exceeds 3 dB; a resonance is not inherently defective
- Explicit APPLY SUGGESTION required; no overwrite when all four CARVEs are active
- This is deterministic diagnostics, not a trained audio ML model

## GOLF-GUFF / XENON assessment
GOLF-GUFF provides verified model routing, identities, scorecard, governance and model bridging, not a turnkey audio diagnostic neural network. XENON provides music-data contracts, not an equalizer ML inference runtime. Their verification and permission principles can inform a future non-real-time architecture but neither repository was integrated into this real-time VST callback.

## Release gates
Windows build and CTest are necessary but not sufficient. FL Studio interaction, visual accuracy, Ctrl-drag feel, Q wheel adjustment, automation recall, listening A/B, click-free automation and CPU profiling still require in-DAW tests.
