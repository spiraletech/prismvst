# SURGERY EQ 1.0.2

This revision continues the exact SURGERY 1.0 source underlying the supplied ZIP,
through the 1.0.1 quality pass. It retains the existing seven ROOTS, 28 CARVEs,
four tabs, four themes, VST3 identity, and every existing host parameter ID/order.

## Control corrections

- New instances: all seven ROOT compressor profiles OFF, COMP THRESHOLD 0.0 dBFS.
- Threshold dragging, typing, automation and double-click reset cannot select a
  compression profile. Select a profile explicitly in COMPRESSION to enable it.
  Saved sessions and host automation can still restore an explicitly saved mode.
- ROOT GAIN remains -60 to +60 dB. Threshold remains -60 to 0 dBFS.
- CARVE has FREQUENCY, CUT DEPTH, Q / TIGHTNESS, CARVE ON and DYNAMIC.
  The former TRIGGER slider, label and attachment are removed; the remaining
  three controls fill its available row. The parameter remains available to the
  host and in saved states without changes to its range/order/default.
- CARVE policy: -24 dBFS for new instances, exact saved/automated threshold for
  recalled sessions. Dynamics remain OFF by default and require the DYNAMIC
  switch. The existing detector uses 3 ms attack / 65 ms release, with the cut
  ramping from zero to requested depth over the 12 dB above threshold. Depth
  remains bounded by CUT DEPTH. No hidden automatic threshold adaptation.

## ONYX implementation

All amounts remain 0–100%, double-click to zero. Existing parameter IDs and
stored values are retained. Amounts receive the existing 10 ms smoothing before
the new taper `a * (0.45 + 0.55*a)`: 20% maps to 11.2% internal depth.

- SPARKLE: transient-weighted high-frequency nonlinear residue; two low-pass
  poles on the exciter input, first-order antiderivative antialiasing (ADAA),
  ROOT-relative detail filters, and reduced depth below 48 kHz. Unlike SHINE, it adds actual harmonics rather
  than a predominantly linear high-frequency boost.
- SHINE: bounded linear first-order presence shelf, maximum +1.94 dB at the
  asymptotic shelf endpoint. No saturator. Actual response depends on sample rate.
- SMOOTHENING: level- and transient-dependent high-frequency subtraction, at
  most 35% of the high-pass component. It replaces the static low-pass blend;
  dry attacks remain present. It can generate low-level modulation harmonics.
- VINTAGE TAPE: low-drive odd-harmonic tanh residue, prefiltered and processed
  with ADAA, small fixed HF rounding, and continuously running residual DC
  removal. Unity small-signal saturation slope replaces the former large
  amplitude-dependent boost. It is a restrained colour model, not a physical
  tape-machine or hysteresis simulation.

Colour stages derive their residues from the original ROOT tap rather than
serially driving each other's saturators. M/S states are separate for every
ROOT; MONO and selected EQ domain retain the original routing. At all-zero
amounts the returned signal nulls exactly, even with previously warm state.
No buffers, allocations, locks, UI calls or extra reported host latency are added.

ADAA applies only to the small nonlinear residue; the dry signal is not
delayed. The residue has frequency-dependent phase/filtering, including the
ADAA half-sample averaging characteristic. This is a tradeoff, not zero-phase
or alias-free processing. Generated harmonics can extend beyond a ROOT's
original input band; ROOT localisation means the input to each colour stage
is its own ROOT tap, not a guarantee of brick-wall output confinement.

## Research and reproducibility

The baseline implementation is retained in `Tests/OnyxStudy.cpp` and Git commit
0175d5608bedcbd694670113cfc1eff00b4438e8. The harness compares settled isolated
controls at 20/100%, 0.1/0.7 input peak, 1/8 kHz, 44.1/48/96 kHz (192 renders).
It checks exact zero null, finite bounded output and silence/DC settling.
The standalone spectral study uses the default 4.2 kHz detail crossover;
ROOT-relative filtering and out-of-band isolation are checked in the full
engine tests. These isolated transfer measurements do not describe every
ROOT boundary configuration or every combination of the four controls.
`Tests/analyse_onyx.py` produces harmonic spectra, tape transfer/shelf/transient
plots, THD+N, DC/peak/level and out-of-harmonic energy, plus 12 RMS-matched WAVs
from a deterministic synthetic bass/pluck/percussion source.

Run `surgery_onyx_study <existing-directory>` and
`python Tests/analyse_onyx.py <directory>` with numpy, scipy and matplotlib.
THD+N here means discrete spectral residual relative to the fundamental,
including intentional harmonics/modulation; it is not an analog noise-floor
measurement. Out-of-harmonic energy is an alias proxy, not a complete alias
measurement: folded harmonics can coincide with genuine harmonic bins,
particularly with the 8 kHz tone at 48/96 kHz. Use the 44.1 kHz case to inspect
non-coincident folds. Linear SHINE should remain near the float render floor.

Algorithm background: Parker, Zavalishin and Le Bivic, DAFx 2016,
[Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution](https://www.dafx.de/paper-archive/details/vem_XXF5qBbfiWOH2RVVAA).
Implementation here is independently written from the analytic tanh primitive.

CPU measurements include the isolated colour harness and the full processor's
stressed callback allocation/timing probe. They describe this machine/test only.
Audio examples are RMS matched, not LUFS matched; they are synthetic and mono.
No blind listening trial on vocals, hats, kicks or commercial full mixes has
been performed, so no subjective winner or commercial certification is claimed.

Saved projects using nonzero ONYX values will sound different by design.
Saved compressor modes/thresholds are preserved, including old FET defaults
that were saved into a project. Keep 1.0.1 for exact sonic rollback.
