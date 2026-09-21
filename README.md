# Shakalizer v1.0

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
