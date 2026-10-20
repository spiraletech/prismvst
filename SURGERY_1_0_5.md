# SURGERY EQ 1.0.5 — SONAR recovery and anchored chassis

Starts from verified PR #17 HEAD 7030d503c4cd81675e9d30f7393620b9846e80ae (1.0.4); no rollback or DSP redesign.

## Confirmed SONAR failure

The 1.0.4 cached heat image was drawn while the graphics context still held a ROOT territory shading alpha of 0.025. JUCE drawImage multiplies stored image alpha by the current graphics opacity. The cached field was therefore dimmed a second time to near invisibility. The adjacent cached CARVE overlay similarly inherited peak-trace opacity. Restore explicit opacity 1 before compositing either image. The analyzer worker, FIFO, FFT, published 256-band values and editor timer were not the failing stages in the reproduced case.

Reproduction on 1.0.4: real noise through processBlock, worker maximum -34.1376 dBFS and displayed peak -33.9384 dBFS, but ZERO heat pixels with at least 5/255 contrast; mean contrast 0.41283. The new regression deliberately failed. With only the opacity repair applied: 166674 visible heat pixels and mean contrast 13.3159; the same test passed. Earlier screenshots/pixel-count tests missed this because a one-level colour difference counted as success, and several captures created the editor after feeding audio. New coverage creates a native editor BEFORE audio and pumps the Windows message loop; natural native paint callbacks must occur before snapshots.

## Visual evolution

Retain the approved muted burgundy/amber, graphite/teal/violet heat and individual ROOT colours. Two subordinate rose/teal/violet glow layers follow the measured single trace. Existing measured-amplitude brightness and fast-attack/adjustable-release envelope drive their intensity; no random hue animation, independent moving objects, scrolling history or fabricated silence activity. Existing peak-hold, memory, slope, view-depth controls and ROOT territories retain their meaning. No DSP or calibrated measurements change.

## Continuous physical chassis

The exact existing PNG is reused. Fixed 72-logical-pixel square corner sections keep their screws and proportions. The top centre plaque and both warning-stripe assemblies are uniformly scaled. Plain metal rails tile between sections, so resizing does not stretch screws, grain, red dots or stripes. All four outer edges are rendered in the editor's own background bounds. The engraving is centred in the upper silver plaque; SURGERY EQ remains at the upper left of its instrument panel. Controls fit inside a reserved frame inset with uniform scaling. Unusual aspect ratios retain dark interior margins rather than stretching knobs.

The frame background cache now keys on physical render scale as well as size and tab, and invalidates on resizing. The cached CARVE overlay also tracks render scale. JUCE's resize grip is explicitly excluded from the panel transform: applying that transform previously made the grip float over the controls. It stays at the actual editor corner.

## Verification and limits

Live pipeline regressions cover 1 kHz and 4 kHz sine waves, pink-shaped noise, a recorded vocal/music sample from pinned JUCE 8.0.4 examples/Assets/singing.ogg, all four tabs, changed frequency/amplitude, silence, native paint activity, ROOT selection/FFT-buffer invariance and bit-exact processed-audio equality while display controls are automated. The test recording is not redistributed in the release ZIP. Existing parameter/state/preset, allocation and audio regressions remain required.

Chassis tests capture 100/125/150% rendering at 840x610, 1320x880, 1800x1200 and extreme 840x1200/1800x610 sizes. They verify connected metal edges, fixed corner image proportions, centred engraving, control containment, resize-grip anchoring and physical cache resolution. Rounding of fractional device pixels permits two pixels of raster-size approximation, not reuse of a stale logical-resolution cache. Screenshots and CSV evidence accompany the build.

The graph update/paint benchmark records median/p95/max, Windows thread CPU estimates and one/two-instance costs at three scales. Native paint-callback cadence is separate from compositor FPS. Cold full-editor snapshots include background regeneration and all controls, so their times are not steady SONAR frame times. Host/Windows scheduling, GPU and multiple scaled instances can prevent 60 displayed FPS. No claim of guaranteed 60 FPS, manual FL Studio listening, actual audio-device dropout certification or production FLP-project verification is made. State round trips and callback regressions do not replace testing copies of user sessions.

Release gates: clean Windows x64 Release CI, all 18 CTest suites and pluginval strictness 8; local pluginval includes GUI, CI uses its existing headless validation setting. Version is 1.0.5; VST3 filename/class IDs, parameter schema and every audio/FFT/measurement source file are unchanged from 1.0.4. FAST 4:1 and existing menus remain. Unsigned development build; PR #17 remains draft/unmerged. The actual compiled VST3 ZIP includes INSTALL.txt.
