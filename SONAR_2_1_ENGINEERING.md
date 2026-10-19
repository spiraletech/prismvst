# SONAR 2.1 — Surgery EQ 1.0.6

Optional, audio-independent spectral weather. Open ANALYZE, select **STORM SYMBIOTE**. STANDARD remains the default and preserves the 1.0.5 SONAR. SONAR OFF hides both fills while retaining the accurate spectrum trace. REDUCED MOTION defaults ON: it suppresses drift and lightning, while allowing gradual energy/colour changes. The existing Freeze control holds spectrum and weather; switching SONAR OFF still takes effect while frozen.

## Ownership and compatibility

`SurgeryWeatherMeasurement.h` computes acoustic features; `SurgeryWeatherInterpretation.h` supplies bounded creative mappings; `SurgeryWeatherField.h` renders a deterministic 128 × 64 field. `SurgeryWeatherAnalysis.cpp` connects measurement to the **existing analysis worker after its existing FFT and analyzer publications**. It reads the existing combined stereo FFT magnitudes and raw stereo FFT window. It never writes samples, FFT data, the original Lab measurements or legacy spectrum values.

The audio callback and audio engine sources are unchanged. There is no added audio latency. Extra weather measurement runs only with an open editor, Storm selected and SONAR ON. An eight-slot SPSC ring (seven usable snapshots) uses release/acquire ownership, never blocks, drops new snapshots if full and drains a bounded number on the UI thread. The field/image are allocated once per editor; there are no per-frame image allocations or unbounded histories. Closing the editor disables extra analysis work. One editor consumer per processor is supported, matching JUCE's active-editor model.

Only `sonar_weather_mode` (STANDARD/STORM SYMBIOTE, default STANDARD) and `sonar_reduced_motion` (default true) are appended to the parameter list. Historical parameter IDs/order/defaults, presets, VST3 class IDs, discovery filename, chassis artwork/layout and audio behavior remain intact. Missing parameters in older project state receive these defaults explicitly. The bundle remains `SURGERY EQ v1.0.vst3`; its internal version is 1.0.6.

## Measurement equations and references

Use the existing N = 4096 real FFT and its actual Hann window, with unchanged hop = 384 samples. Let `m[k] = sqrt((|XL[k]|² + |XR[k]|²)/2)`, `Sw = sum(w[n])`, `Ew = sum(w[n]²)`. Set `g[k] = 1` at DC/Nyquist and `2` for interior bins.

* Coherent-bin sinusoidal peak amplitude: `A[k] = g[k] m[k] / Sw`; `20 log10(max(A, 1e-8))` dBFS. Off-bin tones retain the window's leakage/scalloping behavior.
* Integrated bin power: `P[k] = g[k] m[k]² / (N Ew)`. This is stereo-average mean square, not tone peak amplitude squared. Summing all bins equals `sum(w² (L²+R²)/2)/Ew` by Parseval. White/pink-noise references use that independent time-domain expression. Power density would be `P[k]/df`, where `df = sampleRate/N`; integration here uses bin powers directly.
* Full power spans DC–Nyquist. Weather analysis spans 20 Hz–`min(20 kHz, Nyquist)`. Each bin has a clipped half-bin frequency cell. Integration fractions divide intersected bandwidth by that cell's bandwidth. DC/Nyquist endpoint cells have half width. This is a finite-resolution rectangular-bin approximation, not sub-bin spectral reconstruction.
* Each current ROOT interval uses its actual two frequency parameters. Bin cells are subdivided at all ROOT edges; overlapping territories share each covered segment equally. Gaps remain unassigned. Thus `sum(ROOT power) + unassigned = analysis power`, including reversed/overlapping intervals. Boundary moves rebuild a cached sparse weighting map.
* `Lb = 10 log10(max(Eb, 1e-16))` remains dBFS mean square. Only visual intensity uses `ub = clamp((Lb + 72)/66, 0, 1)`. These −72/−6 dB references are explicit design bounds, not automatic gain or audio normalization. ROOT PWR in the footer remains measured dBFS, never percent.
* Bass pressure `B = .45 u0 + .35 u1 + .20 u2` uses the first three actual ROOT territories. These weights and all following visual mappings are creative design choices, not universal perceptual constants.
* Power centroid `C = sum(fk Pk analysisWeightk)/sum(Pk analysisWeightk)`. Normalize `Cn = clamp(log(max(20,C)/20)/log(fmax/20),0,1)`. Silence reports zero centroid; it does not invent high-frequency content above Nyquist.
* Aggregate the same power into 256 logarithmic feature bands with fractional bin weights. `Mi = sqrt(Pi/max(.25, totalPower))`. Positive spectral flux is `sum(max(0, Mi−Mi_previous)²) × (.008/dtAudio)`, referenced to an 8 ms hop. A 64-value winsorized history supplies mean μ and population σ. Onset threshold is `max(.0005, μ+4σ)`; history inserts `min(flux,max(.001,μ+3σ))`. `Fn = clamp(flux/max(.001,2 threshold),0,1)`. Require analysis power > 1e−9 and a 0.75 s audio-time refractory interval. Persist event identity, time, strength and spectral position between snapshots, so UI queue draining does not lose every event.
* Spectral density D is the fraction of 256 bands exceeding `max(1e−12,totalPower × .002)`. Transient rate R counts recent accepted events within four UI seconds, normalized against `4/.75` and clamped. R is a descriptive visualization feature, not a tempo estimator.
* Real unwindowed `M=(L+R)/2`, `S=(L−R)/2`: `W=RMS(S)/(RMS(M)+RMS(S)+1e−12)`. Independently, `rho=sum(LR)/(sqrt(sum(L²)sum(R²))+1e−18)`, clamped −1…1. Negative correlation below −.2 displays MONO CAUTION; it never changes audio. W is relative side energy, not a complete perceptual-width measurement.

Measurement magnitudes are finite-checked and bounded. Power below 1e−24 is zeroed, far below the −160 dB reporting floor; tiny history/smoothing tails are zeroed to avoid subnormal state. Nonfinite elapsed times become zero/default. Storage and event counts remain bounded.

## Interpretation and visual physics

All weather smoothing uses actual elapsed UI time, clamped 0…60 s for pathological stalls. Snapshots older than 250 ms release toward silence. Let `q = clamp(log(clamp(memory,.05,10)/.05)/log(200),0,1)`. Feature attack is 25 ms; release is `.15 + .65q` seconds. Use `target + (previous−target) exp(−dt/tau)`. Field density attack is 35 ms, atmosphere release `.5 + 1.5q` seconds. Settled silence stops field simulation, image upload and weather repaint requests.

`Tstorm = clamp(.35B + .30Fn + .20R + .15D,0,1)`. The highest of these bounded scores names the sonic texture (level < .03 always CALM):

| Descriptor | Score |
| --- | --- |
| BROODING | `.65B + .20(1−Fn) + .15D` |
| TURBULENT | `.45Fn + .35R + .20D` |
| FLOATING | `.50W + .25(1−Fn) + .25(1−D)` |
| ELECTRIC | `.50Cn + .35Fn + .15R` |
| CALM | `.50(1−level) + .30(1−Fn) + .20(1−R)` |

The footer exposes B, C, F, W, R, D as unitless values, selected ROOT power in dBFS and separate correlation. These are explainable **creative sonic descriptors**, not detected emotions, medical findings or physical atmospheric measurements.

Each field cell stores density, illumination, velocity, displacement and a deterministic spatial seed. No independent clock animates noise or particles. Measured energy drives forces and gradients. Analytic damping with κ=3 integrates constant per-frame force: `v1=v0 exp(−3dt)+(a/3)(1−exp(−3dt))`; `p1=p0+(v0−a/3)(1−exp(−3dt))/3+(a/3)dt`, displacement bounded ±.08. Reduced motion zeros velocity/displacement. Bass cloud mass is `.30+.70 B^1.5`; its centre/spread and actual ROOT energy gradients produce heavier bass clouds. Width controls a centered lateral Gaussian span `.15+.65W`. Sustained power supplies fog, positive flux local ripples, and `.035 Fn Cn level × grain^4` fine rain texture. Texture is deterministic bilinear seed sampling modulated by measured energy.

Centroid selects peach → rose → butter → mint → aqua → blue → lavender, with a subtle ±.06 spatial tint. The supplied palette is decoded from sRGB into linear light, interpolated with smoothstep `s=t²(3−2t)`, and composited on smoked glass. Field colour alpha ≤ .28; localized ivory illumination ≤ .10. A 4097-entry encoding LUT returns to sRGB. Lightning decays with 110 ms time constant, appears only for recent real onsets, and has an independent 0.75 s UI-time refractory limit preserved through mode automation. Reduced motion suppresses all lightning. It is localized illumination, not a full-window flash. The original measured spectrum trace/grid/ROOT markers remain separate foreground layers.

## Verification and resource interpretation

`surgery_weather_calibration`: coherent sine/DC/Nyquist amplitude; independent Parseval sine/white/pink references; silence; 44.1/48/96/192 kHz; mono, antiphase, one-channel and independent stereo; fractional ROOT conservation including overlap; sustained/transient flux; bounded concurrent SPSC transfer; tiny/nonfinite input containment. Calibration tolerances are stated in test source, not inferred from screenshots.

`surgery_weather_field`: linear-light round trips; deterministic output; bounded field and flash rate under mode automation; reduced motion; 30/60/120 Hz density equivalence; long stalls; silence/idle; zero per-update heap allocations.

The full plugin suite verifies bit-exact processed samples and unchanged existing Lab values for OFF/Standard/Storm at all four rates, zero callback allocations, unchanged latency, live sine/noise/stereo/recorded singing, actual native UI snapshots, four tabs at 100/125/150%, Freeze state invariance, OFF while frozen, old/new state recall, menus and automation. Existing 15 minimum/default/maximum/extreme-aspect chassis captures and cache checks continue to run. Freeze state must be bit-identical; only up to eight raster pixels with ≤2 channel levels of native image-interpolation rounding are tolerated.

Fixed C++ storage on Windows x64: field 253,956 bytes, interpretation 400, measurement 85,448, snapshot queue 1,792. Add a 24,576-byte RGB upload image (plus backend alignment/copies), two small snapshot instances, flags/counters and a shared 4,097-byte encoding LUT. Approximately 360 KiB additional fixed per-instance state/texture; existing FFT, chassis caches and graphics-driver copies are excluded. Histories are 64 flux samples, eight event timestamps and eight queue slots. CSV files record actual sizeof values, timed measurement cost and one/two-instance GUI results.

Performance evidence separates worker measurement time (existing FFT excluded), audio allocation/null checks, graph simulation/upload/draw timings and native paint-callback cadence. Thread CPU counters are coarse; offscreen graph timings do not measure compositor FPS, full-window dragging or physical GPU load. 60 Hz is the scheduling target, not a guaranteed displayed frame rate. High DPI and multiple instances can exceed 16.7 ms; inspect the release CSVs for measured medians/p95/max. Manual FL Studio listening/production-project recall, physical-monitor DPI switching, compositor window movement and real audio-device dropout certification remain unverified. Automated parameter/state and offscreen scaling tests do not establish those claims.

Run a Windows x64 Release build, all 20 CTest suites, and pluginval strictness 8 before packaging. CI performs a clean build and pluginval without GUI; local validation includes GUI. Deliver the complete compiled bundle, source archive, equations, logs/CSV/screenshots and exact source/CI identity. PR #17 remains draft and unmerged. This is an unsigned development build.
