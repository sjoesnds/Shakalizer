# Shakalizer v3.0

Shakalizer is a VST3 digital-destruction effect built around controlled digital damage instead of generic harsh distortion.

## Engine

### Core
- SHAKAL macro
- DESTROY / CRUSH / DECIMATE
- DRIVE / CLIP
- GLITCH / JITTER / UNSTABLE
- TRANSIENT / BODY processing
- low-end protection
- stereo widening
- post filtering
- Auto Match
- 1x / 2x / 4x nonlinear quality

### Shatter
- six-way spectral processing concept
- LOW / MID / HIGH / AIR control
- WAVE FOLD
- SHIFT
- RESONATOR
- ENVELOPE FOLLOW
- CHARACTER
- SMOOTH safety control

### Routing
- Standard
- Damage > Shatter
- Shatter > Damage
- Parallel

### Modulation Matrix
Four modulation slots.

Sources: LFO, Envelope, Random, Step, Beat.

Destinations: Shakal, Destroy, Crush, Decimate, Shatter, Fold, Shift, Glitch, Filter.

### Performance
- host BPM sync
- glitch grid
- M/S processing modes
- Live Scenes
- Morph
- Smart adaptive processing

## Randomization

RANDOM ALL randomizes the complete processor parameter list, including all continuous controls, modes, routing, M/S, Live Scene, modulation sources/destinations/amounts, quality, sync, glitch grid, Auto Match and Smart.

Scoped randomizers are also available for Core, Shatter and Glitch.

## Presets

Factory starting points: INIT / SAFE, VOCAL DIGITAL, SHAKAL LEAD, BROKEN 808, PIXEL DRUM, GLITCH GRID, ALIEN, MELT, HARD SHATTER.

A/B snapshots capture the complete parameter state.

## Safety / character

The engine intentionally protects transients and the low end and applies a final softened safety stage. The goal is destructive character with a controllable top end.

## Build

    cmake -S . -B build-shakalizer
    cmake --build build-shakalizer --config Release --parallel

GitHub Actions packages a Windows VST3 artifact as Shakalizer-VST3-Windows.


## v2.0 — Destruction Engine

The v2.0 engine adds a dedicated glitch event system with Stutter, Repeat, Tape Stop, Gate, Reverse and Beat Chop modes, tempo-aware micro-loop lengths, and grid-triggered events.

The spectral section now has six band-spectral behaviors: Smooth, Shatter, Blur, Freeze, Bits and Ring. Four-band processing stays lightweight for real-time use.

The editor now includes a live output scope, `.shakal` preset save/load, expanded glitch and spectral selectors, and additional factory presets.


### 2.1 Lab expansion

- **Glitch Lab:** density, probability, fade, variation, multiple event modes, tempo-aware lengths and grid triggering.
- **Spectral Lab:** dedicated spectral mix, smear, freeze, bit reduction and ring controls layered over the four-band shatter engine.
- **Modulation Lab:** independent modulation rate, depth, smoothing, waveform and tempo-sync controls, plus a dedicated MOD randomizer.
- **Editor:** all new controls are exposed in the UI and included in host state/preset serialization.


### 2.2 — Smart / Visual / Performance Lab

- **Smart Destruction:** adaptive intensity with independent bass, transient and high-frequency protection controls.
- **Visual Lab:** live waveform scope plus LOW / MID / HIGH / AIR peak telemetry.
- **Performance Lab:** realtime CPU-load telemetry and final finite-sample output protection against NaN/Inf propagation.
- **Preset Lab:** user `.shakal` save/load, factory FAV markers, A/B state swapping and scoped randomization.


## v3.0 — Destruction Workstation

The v3 engine adds a realtime 128-sample FFT destruction/resynthesis stage, deterministic 8-step glitch timeline, 8-slot modulation matrix, character fingerprints, granular micro-stutter processing, bounded feedback destruction, pitch-chaos control, spectrum telemetry, and expanded preset/randomization coverage.

### v3 modules

- **FFT Spectral Lab:** mix, shatter, freeze, spectral bit reduction and frequency shift.
- **Glitch Timeline:** eight intensity steps synchronized to the host timing phase.
- **Modulation Workstation:** eight modulation slots with LFO / envelope / random / step / beat sources.
- **Character Engine:** Digital, VHS, Console, Radio, Metallic, Broken, Alien, Cheap DAC and Corrupt fingerprints.
- **Granular / Micro-Stutter:** grain size, pitch, jitter and blend.
- **Feedback Engine:** feedback amount, tone and feedback drive with bounded nonlinear feedback.
- **Telemetry:** live FFT spectrum plus glitch/modulation activity and CPU reporting.
- **Stability:** fixed-size DSP buffers remain allocation-free during audio processing and final finite-sample protection stays enabled.

The v3 FFT stage intentionally uses short non-overlapping windows to keep the effect responsive inside a realtime VST3 host.


## v4.0 — Destruction Workstation 2.0

The v4 pass expands the workstation with adaptive reactive destruction, delayed nonlinear feedback, pitch-damage modes, deeper granular controls, FFT shaping controls, and routing topology behavior.

### v4 modules

- **FFT 2.0 controls:** window selection, spread, threshold and warp.
- **Granular 2.0:** density, position, spray, reverse behavior and pan shaping.
- **Feedback 2.0:** timed feedback, diffusion, freeze, stereo spread and pitch coloration.
- **Pitch Damage:** micro, semitone, octave and corrupt modes with range/drift controls.
- **Audio Reactive:** transient, spectral, bass and high-frequency response shaping.
- **Routing Topology:** serial, parallel, split, crossfade, feedback-loop and wide behaviors.
- **Macro / Scene:** macro curve and scene morph timing controls.
- **Editor:** expanded 101-slider bank plus dedicated v4 mode selectors.

v4 keeps fixed-size realtime buffers and avoids dynamic allocation in the audio-processing path.


## v4.1 — Compact / Stability Pass

- Default editor size reduced to 1180x760.
- Minimum editor size reduced to 980x700.
- 101 controls are split across compact CORE / GLITCH / SPECTRAL / MOD / GRANULAR / FEEDBACK / REACTIVE pages.
- Control spacing is reduced to a tight 2px grid with compact rotary controls.
- Utility buttons remain available in the compact header.
- Feedback recursion is hard-limited and invalid feedback states are reset.
- Auto Match gain is finite-checked and bounded.
- Final output keeps finite-sample and safety limiting protection.
