# PRISM ROOTS v2 — engineering contract (experimental)

**Protected branch from `alpha-1.2.0-resonant-wax`. Not a replacement for the working PRISM v1.2.0 VST3.** The first PR only adds a standalone native C++20 DSP module with four executable tests.

## Single-graph roots
Seven color-coded territories: SUB, KICK, LOW, LOWER MID, MID, HIGH, HIGHER. Each contains two independent overlapping peaking-EQ nodes (14 total). Territory boundaries are visual/semantic, not strict frequency walls. Each node supports adjustable frequency, Q, and gain clamped **-60 to +60 dB**, and domain assignment **STEREO / MID / SIDE**. The first DSP core already processes each domain separately, with no audio-path allocations.

SPACE 0–100 proposed semantics: **0 mono**, **50 untouched original stereo**, **100 up to 150% side**, limited by live mid/side correlation. A separate mono flag serves the future right-click action. No automatic destructive polarity flip: existing anti-phase material remains audible and receives accurate negative-correlation reporting. Shared Mid/Side EQ coefficients reduce accidental left/right mismatch, but **minimum-phase parametric EQ is not linear-phase or perfectly phase-neutral**. Future per-node alignment needs verified algorithms and separate tests.

## UI target
One premium Sony DAT-era-inspired graphite-plastic interface: a cubic frequency grid, responsive analyzer, seven distinct palette colors, 14 draggable nodes, audible SOLO / UNSOLO, accurately labelled dBFS and Hz hover readouts, and a separate MONO/STEREO domain selection within the same graph. Remove input/output and attack/release knobs *in the future UI only*. Per-node gain replaces them; proposed SPACE knob 0–100. No new graph and no clutter.

## Comp profiles: separate DSP increment
Per-band context menu offers OFF, LA-2A-inspired optical, CLB1 3:1 tube concept, and 1176LN-inspired FET (4:1 default); stereo link, dual-mono and mid/side signal-domain modes. Do not imply exact emulations, endorsements, patents or official licensed brand algorithms. Real LA-2A ratios are program- and frequency-dependent; use approximations identified honestly. There will be no exposed attack/release knobs, but compressors still require timing models internally.

## Output and source separation
HQ oversampling 1× / 2× / 4× / 8× will require a separate anti-alias filter design, host latency reporting, bypass/phase matching, CPU measurements and explicit output peak protection. A **zero-delay DSP mode** and an FFT visualization's unavoidable analysis window can coexist, but no credible guarantee of truly zero latency at all oversampling modes. +/-60 dB boosts demand a configurable safety ceiling before production.

A normal EQ cannot isolate vocals, kick, snares, hi-hats and overlapping synths from a finished stereo master by instrument identity. Deliver that as an **optional offline neural source-separation workflow** with explicit algorithm limits, latency and artifacts; it is not part of the current DSP prototype.

## Four executable proofs in Tests/RootsCoreTests.cpp
1. Neutral bypass produces identical stereo samples.
2. Two overlapping node gains combine near the expected +12 dB and gains clamp to +/-60 dB.
3. MID-only EQ leaves pure SIDE signal unchanged; explicit MONO yields matching L and R samples.
4. Measured correlation guard limits *additional* widening while leaving existing anti-phase material unchanged.

## Critical real-time integration gate
`Engine::updateNode` recalculates coefficients and must **not** race with `Engine::process`; perform it on a stopped setup thread during these tests. Before JUCE integration, add staged immutable parameter snapshots, coefficient smoothing, filter reset/automation tests, cross-sample-rate numerical testing and DAW preset migration. No code on this branch redirects the shipping 1.2 audio output to ROOTS yet.

## Historical inspiration
Sony's early PCM/DAT decks and the turn-of-the-millennium cinema surround aesthetic are inspiration for precision and presentation, **not Sony, Dolby or THX compatibility, certification or endorsement**. Stereo processing is two-channel; it is not genuine encoded Dolby Atmos or surround.
