# SURGERY EQ v1.0 — development baseline

Spiral Ether Tech. Branch: `surgery-eq-v1.0`.

This is the first engineering update built on the latest verified ROOTS source branch, **not** an assertion that all planned v1.0 features exist.

## Implemented in this patch
- User-facing product name and Windows VST3 artifact name changed to SURGERY EQ v1.0.
- Historic VST3 bundle ID, plugin code and existing parameter IDs intentionally retained for backwards compatibility with ROOTS PROTO 001.
- ROOT right-click ownership selects the territory under the pointer instead of unintentionally targeting a previously selected root.
- Root menu compressor model selections removed; the existing internal models remain in the DSP until replaced through tested migration.
- MONO / SOLO / ON are proper illuminated buttons instead of checkbox-styled toggles.
- Theme button cycles four chassis appearance variants (ONYX / ETHER LAB / ALIEN GRID / CINEMA VOID).
- Original third-party branding references removed from the Roots editor.
- Windows artifact packaging renamed.

## Unfinished: do not advertise as implemented
- Independent full-panel theme palette and per-ROOT RGB customization.
- Calibrated sonar thermal heatmap and AI diagnostics.
- 28 CARVE nodes (current engine includes fourteen boundary handles).
- True source separation, source-aware instrument SOLO.
- New native adaptive compressor, exact compressor timing response.
- Complete surgical parametric node DSP.
- Full proof tests for the updated UI and FL Studio.

## Verification
The GitHub Actions Windows job is the acceptance gate. No local Windows compilation is claimed. Confirm tests and FL Studio plugin discovery before declaring release stable.

## Compatibility
The host-facing display name changes, but plugin identity is deliberately retained for host-recall migration. Existing projects should be backed up and tested.
