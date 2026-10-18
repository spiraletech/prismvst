# SURGERY EQ 1.0.2 — anchored ONYX rack skin

Replaces theme slot 0 (ONYX) with the user's industrial rack reference direction.
ONYX remains the first and factory-default theme. Existing projects still recall
their selected theme; the other three themes remain available in the same order.

- Embedded scratched metal chassis, silver chamfers, screw heads, hazard inserts.
- Live cyan-lit beveled root/mode/control buttons, machined silver knobs with
  calibration ticks, inset numeric fields, matching dropdowns and output trim.
- Dedicated layouts for ROOT, CARVE, ONYX and ANALYZE; CARVE C1–C4 forms a
  compact 2 x 2 bank to the right of its three visible knobs.
- Dark glass analyzer with pastel pink/lilac live FFT contour and subdued SONAR
  fill. All frequency/level data remain real; no baked-in spectrum artwork.
- Uniform content scaling keeps text, circular knobs, hit targets and graph
  coordinates aligned. Default editor is 1320 x 880; existing resize support
  and other-theme layouts are retained.

No DSP, parameter IDs, parameter order/ranges/defaults, saved-state schema,
compression behavior, ONYX algorithms or VST3 class identity changed from the
validated 1.0.2 revision a5de604. Compression still defaults OFF / 0 dBFS and
the CARVE trigger remains hidden with its parameter preserved.

The version stays 1.0.2; download names use ONYX-Rack to distinguish this visual
revision from the earlier 1.0.2 build. The previous build is retained for rollback.

Artwork was produced with the imagegen skill using the built-in image_gen tool.
See Assets/OnyxRack/README.md for the saved artwork and exact final prompt.
