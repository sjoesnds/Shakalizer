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
