# SURGERY EQ v1.0 — Engineering acceptance status

Historical baseline notes. The 1.0.1 quality candidate and its current validation scope are described in [SURGERY_QUALITY_PASS.md](SURGERY_QUALITY_PASS.md); delivered logs record the executed results.

**In source (not yet proof of runtime correctness)**
- [x] Product name updated and Windows VST3 packaging renamed
- [x] Seven ROOTS with independent boundary handles based on ROOTS v2 baseline
- [x] 28 CARVE DSP instances, host parameters, stereo/Mid/Side routing and four C++ proof tests
- [x] Five mode tabs, CARVE slot editing controls, graph drag gestures and ON/SOLO/MONO hardware buttons
- [x] Stationary SONAR intensity/persistence engine and four new tests
- [x] Deterministic ASSIST prominence scanner, four new tests, explicit user-approved insertion
- [x] Persisted four-theme selection and seven custom ROOT RGB colors
- [x] ROOT right-click targeting and no hardware-model selection in its menu
- [x] Audio doctrine and UI thesis documentation

**Required gates**
- [ ] Windows CI successful on final commit
- [ ] FL Studio discovery, parameter persistence, saved project recall and actual GUI interaction
- [ ] Stereo width / phase and CARVE response listening tests
- [ ] CPU, aliasing, click-free automation and thread-safety profiling
- [ ] SONAR calibrated display and graph accuracy verified against reference tones
- [ ] Accessibility and high-DPI screenshot review

**Explicitly future / not in v1.0 engineering baseline**
- Actual vocal/drum/bass stem separation
- Trained source-aware AI model
- Full spectral trail history and dual pre/post spectrogram
- FIR linear-phase mode and true-peak limiting
- Fully configurable compressor modeling and DELTA audio subtraction

Main branch left untouched to preserve the previous verified release line.
