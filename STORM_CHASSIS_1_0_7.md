# Surgery EQ 1.0.7 — JUKE JUICE / Storm Chassis

Border-only replacement based on the supplied approved reference. Baseline: `21d7d34fdc9feb7792f69fa95e327254f2ed8179` (1.0.6). The analyzer, control geometry, tabs, knobs, dropdowns, button layout, internal panel rendering, audio DSP, analysis measurements, presets and parameter definitions are preserved. Only the outer frame, an isolated read-only frame subscription and version identification change.

## Artwork and geometry

`Assets/StormChassis/approved-reference.png` is an unmodified copy of the supplied 960 × 682 image. Native JUCE source rectangles sample only its plasma and four screw heads. None of the reference analyzer, controls or text is painted into the product. Software-backed optical strips are prepared once; their fixed reusable output strips receive a subtle smooth glass deformation. This preserves the reference's luminous blue/cyan, violet, magenta and warm coral optical detail rather than approximating it as thin neon lines.

`SurgeryStormChassis.h` owns the new rendering system. Its black, cool graphite twill, glass chambers, four X-head corner fasteners and micro-engraved equations stay outside the existing instrument area. Static carbon/metal/engraving joins the existing physical-pixel background cache. Only the top and two side chamber rectangles are invalidated during animation. Rounded alpha masks rebuild only on size changes; static chrome/glass remains cached, so each animated paint needs three image draws without vector path clipping. Corners and tube thickness stay fixed in logical pixels as the actual editor bounds change; DPI scales them uniformly. Size/DPI changes invalidate the existing background cache. The main content transform and every control's layout coordinates are unchanged.

The old outer artwork is no longer drawn. Its existing dark interior source rectangle is retained verbatim to avoid changing the protected instrument background. The old border's silver rails, plaque, warning stripes and frame fasteners are absent from the new UI. Only four new outer corner screws appear; pre-existing screws inside control panels remain part of the locked instrument face.

## Read-only reaction

The frame reads the existing smoothed Storm `Meaning` values; it adds no analysis, parameters, features, audio code or controls. STANDARD/OFF continue to use their existing behavior and leave the self-lit reference glass at rest. Audio-driven reaction is available with STORM SYMBIOTE enabled. The frame never enables Storm or changes analyzer parameters.

`E = clamp(.30 L + .35 B + .20 F + .15 W, 0, 1)`.

`S = E + (previousS − E) exp(−dt/tau)`, with 25 ms attack and the existing Visual Decay value (bounded .05…10 seconds) for release. `dt` is actual elapsed UI time, bounded to .25 s after a long stall.

`G = .18 + .82 S^1.35`.

`V = .08 + .55 F + .25 B`; flow advances by `dt × V × S`, so silence has no independent animation. Phase wraps at a fixed bound. Smooth transverse/longitudinal optical displacements are limited to 3.5%/1.8% of a chamber; endpoints stay pinned. Centroid makes a restrained colour-temperature adjustment. A constant self-lit glass layer preserves the reference at rest; measured G modulates the additional brightness without full-window flashes.

Freeze returns before updating energy, brightness, temperature or phase: the held border stays lit and motionless. Reduced Motion holds phase while allowing measured brightness/colour response and inherited release. Silence settles to G=.18; no autonomous particles, flash generator or random animation is added. Tiny state tails are zeroed. State updates allocate no memory.

## Bounded resources and verification

Three reusable dynamic strips: 1024 × 40, 40 × 768, 40 × 768 (102,400 pixels). Matching software source strips, four 40 × 40 screw tiles and a 64 × 64 carbon tile plus rounded alpha output and 102,400 bytes of fixed masks give 850,688 bytes of logical texture/mask payload in total. Backend alignment, native copies, the shared decoded reference ImageCache entry and existing plugin caches are additional; payload is not a total-process-memory claim. No images are recreated per animation frame or resize. Source sampling and output filling are bounded loops.

The source-preservation audit compares every existing source file except the narrowly integrated editor, the complete graph implementation, the entire layout function, interior panel paint and timer logic excluding the new frame subscription. Version strings are the only allowed graph-text change. Processor, DSP, FFT, SONAR measurement/interpretation/rendering, parameter layout and preset source contents must match 1.0.6 exactly.

`surgery_storm_chassis` checks the specified energy/glow equations, elapsed-time decay, Freeze invariance, Reduced Motion, silence and allocation-free state updates. It paints both chassis layers into transparent software images at all five supported size/aspect cases and checks that every interior pixel remains untouched. It also records dynamic border timings at 100/125/150%. The existing full plugin suite retains all four tabs, live signals, native popup/scaling, automation, recall, audio-null, callback allocation and 15 chassis size/DPI captures.

Release requires a Windows x64 Release build, all 21 CTest suites, pluginval strictness 8 and exact-head clean CI. Actual screenshots and CSV/log evidence accompany the compiled VST3. Measurements distinguish border-only rendering from full-editor/compositor performance. Physical-monitor DPI switching, manual FL Studio listening/production project recall, compositor window dragging and actual audio-device dropout certification remain unverified. The build is unsigned; PR #17 stays draft and unmerged.
