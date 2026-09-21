# Shakalizer v0.4

Shakalizer is a VST3 sound-destruction effect for FL Studio. v0.4 is focused on controlled destruction, musical randomisation and a softer high-frequency response.

## Core engine

- SHAKAL macro with multi-stage macro mapping
- DESTROY / CRUSH / DECIMATE
- oversampled DRIVE / CLIP
- sparse GLITCH with grid timing
- JITTER / UNSTABLE movement
- TRANSIENT / BODY envelope-aware processing
- frequency SPLIT with low-end protection
- restrained STEREO widening
- post filter: LP / BP / HP
- Auto Match and full-state A/B

## Shatter engine

- SPECTRAL SHATTER
- separate LOW / MID / HIGH / AIR amounts
- WAVE FOLD
- SHIFT
- RESONATOR / comb feedback
- ENVELOPE FOLLOW
- CHARACTER
- final SMOOTH / safety stage

## Quality

1x, 2x and 4x nonlinear oversampling.

v0.4 also uses a separate dry buffer so MIX is a real dry/wet control and Auto Match compares against the untouched input.

## Modes

Clean, Crunch, Shakal, Destroy, Fried, Pixel, Alien, Melt and Shatter.

## Movement / sync

- Sine / Triangle / Sample+Hold / Stepped
- Free / 1/4 / 1/8 / 1/16 / 1/32 movement sync
- Free / 1/8 / 1/16 / 1/32 glitch grid

## Presets

Built-in starting points:

- INIT / SAFE
- VOCAL DIGITAL
- SHAKAL LEAD
- BROKEN 808
- PIXEL DRUM
- GLITCH GRID
- ALIEN
- MELT
- HARD SHATTER

## Randomisation

RANDOM ALL is global: it iterates over the processor's complete parameter list and randomises every continuous parameter, every choice selector and Auto Match.

Scoped randomisers:

- CORE
- SHATTER
- GLITCH

After manual randomisation the preset selector shows CUSTOM.

## Safety

The main nonlinear stage was softened in v0.4. High-frequency destruction is reduced by default, transients are protected dynamically and a final low-pass/soft-ceiling stage prevents extreme edge harshness.

## Build

Windows:

    cmake -S . -B build-shakalizer
    cmake --build build-shakalizer --config Release --parallel

GitHub Actions builds and uploads a Windows VST3 artifact for source/CMake/workflow changes.
