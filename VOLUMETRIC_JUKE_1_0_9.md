# Surgery EQ 1.0.9 — volumetric JUKE liquid

Border-only correction from 1.0.8 (`ccd1973ef093db1cd39fbe481fad92db0a3876bc`). The existing carbon shell, glass tube geometry, X-head screws and complete inner instrument are retained. No analyzer, measurement, audio, parameter, preset, control or layout code changes; the editor version text changes to 1.0.9.

## Layered liquid inside hollow glass

Each chamber contains one main core current plus three secondary strands, a broader translucent body, and low-opacity diffusion/haze. Localized packets move through the volume. Four separately coloured paths braid through the body; every chamber can show cyan/blue, violet/lavender and pink/coral together. Small left/right hue biases retain the cool-left and warm-right character without assigning a fixed tint. Glass highlights are composited above the current. The dark empty part of each chamber and its polished casing remain visible; there is no luminous static photograph or decorative filled gradient.

The normal regression scene (normalized level .65, bass .45, flux .20, width .50, centroid .50) must show 25–45% of pixels above 10% output opacity in EACH chamber. This is a reproducible optical occupancy measurement, not a guarantee for every song or decay setting. The energetic scene must increase body/bloom while staying below 70% occupancy. Emission drive is capped before applying the spatial body profile to retain empty volume at high energy. Strong decay can deliberately leave overlapping trails.

Existing read-only Storm values drive the border:

| Value | Response |
| --- | --- |
| Level | Brightness, body density and activity gate |
| Bass | Wider/heavier cool body and longer current packets |
| Flux | Turbulence, faster surges and warm accents |
| Centroid | Cool-to-warm balance across body and four currents |
| Stereo width | Greater lateral spread/secondary complexity |
| Visual Decay | Density, RGB and bright-core trail persistence |
| Existing bounded weather flash | Local white-hot/coral core emphasis |

Centroid/flux supply a visual proxy for higher spectral activity; no new mid-band measurement or analyzer calculations are introduced. The colours are a designed synesthetic visualization, not a diagnostic or scientific colour measurement.

Density and premultiplied RGB/core histories share a 35 ms optical attack and the existing decay release. A fixed 30,720-cell history gives the plasma volume and coloured trails. Continuous periodic phase functions remain coherent across phase wrap. Freeze holds feature state, phase, density, colour and core history exactly. Reduced Motion retains body/colour/light response while using 3.5% drift and much less agitation. SONAR OFF/Standard hides all interior energy immediately, even when frozen; held history remains intact. Silence releases toward zero, stops autonomous travel, and leaves faint cached glass reflections only. Existing SONAR defaults and controls are unchanged.

## Resource bounds and verification

Three reusable ARGB outputs (512×24, two 24×384), 30,720 density floats, four floats of colour/core history per cell, rounded masks, carbon and four screw tiles: 799,488 bytes logical payload. Native image alignment/copies, reference image cache and existing background caches are additional. No per-frame particles or dynamic containers. State updates allocate no heap memory. The same existing dimension/DPI-sensitive static cache and editor-bound geometry are retained. Both border layers explicitly exclude the protected instrument face.

Required checks: 21 CTest suites, Windows x64 Release build, pluginval strictness 8, source preservation audit, actual UI captures at 100/125/150%, all four tabs, native dropdowns, automation/recall, four-rate audio-null checks, Freeze including optical history, OFF transparency, silence settling, multicolour presence in all chambers and normal/energetic occupancy. Border timings are measured separately; they do not certify full-editor/compositor FPS. Manual FL Studio production-project listening, physical DPI switching, live window-drag performance and device dropout certification remain unverified. This is an unsigned development build; PR #17 remains unmerged.
