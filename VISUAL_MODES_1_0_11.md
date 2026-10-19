# Surgery EQ 1.0.11 — JUKE Juice and analyzer modes

The main rack, analyzer bounds/axes, knobs, lower controls, tabs and all existing feature positions are retained. The only added UI component is a JUKE MODE selector with its label in previously unused Analyze-page space above VIEW RANGE. The existing SONAR selector now presents eight styles.

## Independent selectors and recall

SONAR: Standard / Storm Symbiote / Synesthesia / Brooding / Pulse / Aura / Focus / Standard+.
JUKE: Real Jukebox / Storm Tube / Symbiote Flow / Pulse Rail / Ambient Idle / Off.

Two IDs are appended after every historical parameter: `sonar_visual_style` and `juke_mode`. The original two-choice `sonar_weather_mode` retains its ID, choices, order and normalized host-automation meaning. Older automation selects Standard/Storm whenever the new style is Standard/default. An explicit GUI style gesture clears that legacy gate; host/state notifications never clear or rewrite it. Existing normalized automation is not remapped onto an eight-choice range. Fresh instances use Standard, Real Jukebox and Reduced Motion ON. Missing new IDs in old sessions recall the old SONAR gate and Symbiote Flow only if Storm+SONAR were enabled, otherwise JUKE Off. Use the new style lane for new automation; avoid mixing old/new SONAR lanes deliberately.

SONAR OFF deactivates analyzer heat/weather overlays, preserving its accurate spectrum trace. JUKE Off independently hides all plasma while leaving glass/carbon/screws. Either system can run with the other disabled. Freeze holds both simulations and their density/colour/core histories; Off overrides visibility immediately without destroying held history. Reduced Motion reduces drift/turbulence and flashes while retaining light/body response. Silence releases energy and stops autonomous motion, leaving faint casing reflections.

## One measured music stream

The existing analysis worker publishes one snapshot consumed once by the editor, then shared by graph and chassis. No extra FFT or audio-callback analysis is added. Read-only fixed bands (20–160, 160–500, 500–2500 and 2500–20,000 Hz, Nyquist-clipped) are integrated from the same completed FFT power bins. RMS and crest factor use the same worker-side stereo window; existing centroid, positive spectral flux, width, event strength and decay remain available. Existing calibrated Parseval/ROOT/amplitude calculations, FFT boundaries and actual audio processing remain intact. New band integration does not alter ROOT territories. Bass visual interpretation uses fixed low-band energy, with the legacy value as a fallback for old synthetic snapshots.

A shared colour reader maps low energy to cyan/electric blue, middle energy to violet/lavender, highs to magenta/pink and bounded transient accents to coral/warm highlights. Only the six approved cyan/blue/violet/lavender/pink/coral stops are used by the new mapping. This is a designed acoustic visualization, not detection of human synesthesia. Standard retains the existing restrained diagnostic presentation. Every mode keeps the measured spectrum path and frequency/dB geometry intact.

## SONAR personalities

- Standard: existing measured trace and restrained spectral fill.
- Storm Symbiote: the existing bounded weather field, now coloured by the shared reader.
- Synesthesia: frequency/energy colour wash and shared-palette measured-spectrum fill.
- Brooding: low-energy weighted deep violet cloud.
- Pulse: transient/crest-driven pressure wave behind the measured trace.
- Aura: broad centroid/width-driven diffused light field.
- Focus: narrow centroid-localized emphasis with minimal surrounding fill.
- Standard+: restrained shared-palette fill and a slightly stronger precision trace.

Creative overlays are decorations driven by measured features; none replaces or fabricates the diagnostic spectrum. Images/fields are reused at fixed resolution. Shared smoothing advances once per UI tick, not once per consumer.

## JUKE chambers

All modes retain the manufactured hollow glass/acrylic, black carbon fibre, silver X-head hardware, inner reflections, contained diffusion and empty space around a continuous plasma river. There is no winding body/head/tail. Internal currents remain submerged within nearly steady banks. Broad translucent mass, brighter internal filaments, multiple streams, soft interior halo and bass-driven pressure swells provide depth and motion without a static filled-panel photograph.

Real Jukebox has a fuller pressurized body and polished bright currents; Storm Tube has stronger internal agitation and surges; Symbiote Flow retains the continuous river; Pulse Rail emphasizes advancing pressure fronts; Ambient Idle slows/dims the measured flow. Width changes the count/spread of two to four currents (Symbiote Flow retains four for visual continuity). Level controls emission, low energy body thickness/pressure, flux agitation, centroid and measured band balance hue, and Visual Decay persistence. There is no autonomous active plasma in silence. Personality changes defer while frozen; Off gates visibility immediately.

## Verification and practical limits

Required: clean Windows x64 Release build, all 21 CTest suites and pluginval strictness 8. Regression additions cover calibrated RMS/crest/bands, eight distinct analyzer scenes, five distinct active JUKE scenes plus Off, legacy normalized parameter identity/order, independent selector recall/activation, both Off gates while frozen, all six native dropdowns at 100/125/150% and screen edges, all existing tab/size captures, and four-rate bit-exact audio tests with all eight SONAR styles and six JUKE choices cycled. Source checks protect core processing and every existing layout bound. Recorded music supplies the actual native previews; no mockup stands in for a compiled result.

Rendering timings are reported separately and do not certify full-editor/compositor 60 FPS. Manual FL Studio production-project listening, physical monitor DPI changes, compositor window dragging and real-device dropout certification remain unverified. Unsigned development build; PR #17 remains unmerged. The visual-field/queue payload is fixed and reused; the shared snapshot grows only by its additional fixed measurement fields.
