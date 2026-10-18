# SURGERY EQ 1.0.3 — matte ONYX and Synesthesia V1

## Chassis and branding

The first/default ONYX rack theme retains its layout and live controls, with a
new embedded matte brushed/tarnished chassis. The cracked chrome finish and
bright top-center reflection are removed. Crimson paint speckles are more
prominent on both yellow side inserts. The small screw/rivet rows along both
vertical rails are removed and replaced by crimson paint speckles; the large
corner fasteners remain. The top SPIRAL ETHER TECH lettering is about 50% larger,
fitted to the plaque, with no animated light sweep. Control
bevel highlights are quieter. Other themes remain available in the same order.

## SONAR correction

The 1.0.2 rack skin skipped the full colour-fill renderer and substituted a
very faint cyan fill. That made SONAR ON look almost like OFF. The bypass is
removed. Every theme now renders the same visible frequency-addressed heatmap
when SONAR is ON. New instances still default ON; saved OFF settings remain OFF
until explicitly toggled. No existing state is silently overwritten.

A GUI regression checks the actual SONAR button attachment and verifies that
ON changes thousands of analyzer pixels, OFF removes the fill, and ON restores
the parameter. Actual ON/OFF screenshots accompany the build.

## Synesthesia V1 prototype

The prototype is a designed visual correspondence: **log frequency -> hue,
measured post-DSP dBFS -> brightness, nearest 12-TET note -> readout label**.
It is not a biological synesthesia detector, AI source separator, or a claim
that audible frequencies physically correspond to these visible wavelengths.

| Hz | Colour anchor |
|---:|---|
| 20 | red |
| 60 | orange |
| 160 | amber |
| 350 | yellow |
| 750 | green |
| 1,600 | mint |
| 3,500 | cyan |
| 7,000 | blue |
| 12,000 | violet |
| 20,000 | magenta |

Colours interpolate continuously in log frequency. Each displayed band uses
its own measured level; the field is clipped to the one real FFT envelope.
Quiet or missing bands are not fabricated to display every colour. The existing
display envelope controls decay. FREEZE holds the measurement and its colours.
The moving foil/glint renderer is removed: hues do not change with elapsed time.
Brightness is a relative display cue, not equal perceived luminance across hues.

Hover readout and ANALYZE peak-bin readout now include an approximate note label
(A4=440 Hz). This is a label for an FFT display-band frequency, not a precise
pitch tracker; bass-bin resolution, multiple notes and inharmonic sounds limit
its musical interpretation. Numeric Hz/dBFS and the centroid remain available.

## Compatibility and validation

No audio DSP, processor implementation, parameter IDs/order/ranges/defaults or
state schema changed from the validated 1.0.2 rack build c1835cb. ROOT compression
remains OFF / 0 dBFS by default, and CARVE's trigger parameter stays hidden and
compatible. Plugin version/editor are 1.0.3. The bundle filename and class IDs
remain unchanged for project continuity. Keep the previous bundle for rollback.

Run the existing 18 CTest suites and pluginval strictness 8. Colour tests cover
the full hue range, finite bounds, silence gating, deterministic mapping and
known A4/A5/C4 note labels. Actual editor renders cover SONAR ON/OFF, every
mode/theme, minimum rack sizes, factory defaults and 2x scaling. Build-specific
results and limits are included with the compiled download.

The chassis was edited with the built-in image_gen tool via the imagegen skill.
The old asset remains in source for provenance/rollback. See
Assets/OnyxRack/README-1.0.3.md for the saved asset and exact final prompt.
