# Surgery EQ 1.0.8 — hollow JUKE correction

Border-only correction from 1.0.7 (`20d332a825dffdc8ef25df173e918e4af7b377f7`). The filled reference-image plasma is completely removed. Reference artwork is sampled only for the existing four X-head screws. The same black carbon shell, editor-bound tube rectangles, corner positions, inner instrument geometry and lower-panel structure remain.

## Empty chambers, generated interior energy

The cached casing is dark semi-transparent glass over the carbon shell, with desaturated polished seams, cylindrical edge shading and narrow low-opacity reflections. There is no bright static gradient, luminous photograph or full-panel colour wash inside it. At rest the empty volume stays dark; only faint casing reflections/tinted seams remain.

Three reusable ARGB strips (512×24 and two 24×384) draw **sparse localized moving currents**. A bounded 30,720-cell density field supplies lingering trails. In each chamber, three measured-energy-driven packets follow transverse sinusoidal paths. The third path's contribution and lateral spread increase with stereo width. Each packet has a compact longitudinal envelope and narrow cross section, using `1/(1+q²)^3` profiles. Actual elapsed-time phase advances the packet positions, rather than deforming a static neon picture. Colour is generated only at current/trail pixels; the rest of the volume is transparent. Local emission uses a bounded square-root opacity curve so quiet measured currents remain visible without illuminating the empty volume. The test measures actual rendered occupancy as well as density: most pixels must remain below 10% opacity.

Existing Storm values drive the result:

| Existing value | Border response |
| --- | --- |
| Normalized broadband level | Gates activity, brightness and current density; zero level cannot sustain motion through width alone |
| Bass | Thicker current cross-section, longer/heavier packets and increased current mass |
| Positive spectral flux | Faster travel, stronger localized surges and agitation |
| Centroid | Cool/warm colour bias of the moving currents |
| Stereo width | Greater lateral spread and contribution from the third path |
| Visual Decay | Release of the feature envelope and spatial trail field |

`E = clamp(.30L+.35B+.20F+.15W,0,1) × clamp(L/.08,0,1)`.

E and contributing features use 25 ms attack and the existing Visual Decay release (.05…10 s). `G=.012+.988 E^1.35` controls local current emission, not a filled chamber background. Phase advances by `dt × (.25+.60F+.20B) × E`. Sparse density targets also depend on level, so sustained width or a residual centroid cannot light silent vessels. Density attacks in 35 ms and releases with Visual Decay. Tiny feature/density tails are zeroed. All state is bounded; no independent particles, random animation or autonomous silent motion are introduced.

Left currents are blue/cyan dominant, right currents pink/magenta/coral dominant. The top carries separated multicolour packets. Existing bounded weather flash strength may add a small localized surge within a current; it never illuminates the entire window. These are designed acoustic-to-colour mappings, not changes to the audio analyzer or a physical-fluid measurement.

## Controls and safety of the locked face

* Freeze holds phase, feature state and every trail cell. Resizing may rebuild rounded masks, but does not advance held simulation.
* Reduced Motion uses 3.5% drift speed and substantially reduced agitation. Level/colour response remains active; it does not turn the vessels into static filled bars.
* SONAR OFF or Standard visualization hides interior energy immediately, including while frozen. The held trail state is preserved during Freeze. STORM SYMBIOTE + SONAR ON use the existing weather measurements; no extra analyzer calculations are enabled for Standard mode.
* Silence releases energy and trails toward zero. Empty glass, faint reflections and the carbon shell remain.

There are no new controls, parameters, tabs, presets or audio functions. The complete graph implementation, interior panel paint, main control layout and historical processor/DSP/FFT/weather sources are preserved. Only border rendering, its existing read-only timer subscription/gate, release identification and relevant verification change. Both border layers continue to exclude the protected instrument area explicitly.

## Resources and verification

Fixed payload: 122,880 bytes ARGB output, 122,880 bytes density history, 30,720 bytes rounded masks, 19,200 bytes screw tiles and 12,288 bytes carbon tile = **307,968 bytes logical texture/field payload**. Native alignment/copies, shared decoded reference ImageCache entry and existing caches are additional. The previous 307,200-byte optical-source strips are gone. Storage is allocated once; masks rebuild on bounds changes; state updates allocate zero heap memory. Static glass/chrome remains in the existing size/DPI-sensitive cache.

The border suite checks energy/glow equations, actual nonzero sparse currents with most cells dark, spatial trail Freeze invariance, SONAR OFF transparency while frozen, preserved held cells, silence settling and no silent independent motion. It also retains allocation checks, five editor sizes with every interior pixel untouched and 100/125/150% timings. The frame-integrity test samples glass **walls**, not the intentionally empty volume. Existing audio-null, calibration, automation, recall, four tabs, native dropdowns and 15 size/DPI captures remain required. Recorded-music integration saves actual interface frames for an animation preview; the preview is not a concept rendering.

Release verification: clean Windows x64 Release CI, all 21 CTest suites and pluginval strictness 8; local pluginval includes GUI, CI skips GUI. Actual CSV/logs, stills and animation accompany the compiled bundle. Timings are measured and do not certify compositor FPS or hardware scheduling. Physical-monitor DPI switching, manual FL Studio production-project/listening checks, window-drag compositor performance and audio-device dropout certification remain unverified. Unsigned development build; PR #17 remains unmerged.
